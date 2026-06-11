#include <ctype.h>
#include <errno.h>
#include <libconfig.h>
#include <mosquitto.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sysexits.h>
#include <unistd.h>

#include "../config.h"
#include "bme280.h"
#include "bme280_mqtt.h"

extern int initGatherData(struct config *config);
extern int gatherData (struct config *config, struct data *data);
extern int deinitGatherData();

int publishToMqtt (struct mosquitto *mosq, struct data *data, const char *pTopic, const char *pNode);
static void printHelp(void);

volatile int keepRunning=1;


void intHandler(int sig)
{
  if (sig==SIGINT)				// only quit on CTRL-C
  {
    keepRunning = 0;
  }
}

int main(int argc, char **argv)
{
  int rc;

  // handling Crl-C
  struct sigaction act;

  // config file handling
  char configFilePath[MAXPATHLEN];
  config_t cfg;
  struct config config;
  int c;						// for getopt

  // mqtt
  struct mosquitto *mosq;

  // data gathering
  struct data data;

  act.sa_handler = intHandler;
  sigaction(SIGINT, &act, NULL);	// catch Ctrl-C

  if (argc==1)
  {
    printHelp();
    return 1;
  }

  opterr = 0;
  while ((c=getopt(argc, argv, "c:")) != -1)
  {
    switch (c)
    {
      case 'c':
        strcpy(configFilePath, optarg);
        break;
      case '?':
        if (optopt=='d' || optopt=='h')
          fprintf(stderr, "Option -%c requires an argument.\n", optopt);
        else if (isprint (optopt))
          fprintf(stderr, "Unknown option '-%c'.\n", optopt);
        else
          fprintf(stderr, "Unknown option character '\\x%x'.\n", optopt);
        return 1;
      default:
        abort();
    }
  }


  // read config file
  config_init(&cfg);
  /* Read the file. If there is an error, report it and exit. */
  if (!config_read_file(&cfg, configFilePath))
  {
    fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg), config_error_line(&cfg), config_error_text(&cfg));
    config_destroy(&cfg);
    return EXIT_FAILURE;
  }
  // node, host, apikey, wlaninterface
  if (!(config_lookup_string(&cfg, "node", &(config.pNodeName))))
  {
    fprintf(stderr, "No 'node' setting in configuration file.\n");
    config_destroy(&cfg);
    return EXIT_FAILURE;
  }
  if (!(config_lookup_string(&cfg, "broker", &(config.pBrokerName))))
  {
    fprintf(stderr, "No 'broker' setting in configuration file.\n");
    config_destroy(&cfg);
    return EXIT_FAILURE;
  }
  if (!(config_lookup_string(&cfg, "username", &(config.pUserName))))
  {
    fprintf(stderr, "No 'username' setting in configuration file.\n");
    config_destroy(&cfg);
    return EXIT_FAILURE;
  }
  if (!(config_lookup_string(&cfg, "password", &(config.pPassword))))
  {
    fprintf(stderr, "No 'password' setting in configuration file.\n");
    config_destroy(&cfg);
    return EXIT_FAILURE;
  }
  if (!(config_lookup_string(&cfg, "topic", &(config.pTopic))))
  {
    fprintf(stderr, "No 'topic' setting in configuration file.\n");
    config_destroy(&cfg);
    return EXIT_FAILURE;
  }
  if (!(config_lookup_string(&cfg, "i2cbus", &(config.pi2cBus))))
  {
    fprintf(stderr, "No 'i2cbus' setting in configuration file.\n");
    config_destroy(&cfg);
    return EXIT_FAILURE;
  }

printf ("conf file: %s\n", configFilePath);
printf ("node: %s\n", config.pNodeName);
printf ("broker: %s\n", config.pBrokerName);
printf ("username: %s\n", config.pUserName);
printf ("password: [hidden]\n");
printf ("topic: %s\n", config.pTopic);
printf ("i2c bus: %s\n", config.pi2cBus);

  initGatherData(&config);

  mosquitto_lib_init();

  rc = 0;
  mosq = mosquitto_new(config.pNodeName, true, NULL);
  if (!mosq)
  {
    printf("mosquitto handler could not be allocated, error %d (%s)\n", errno, strerror(errno));
    rc = errno;
  }

  if (!rc)
  {
    rc = mosquitto_username_pw_set(mosq, config.pUserName, config.pPassword);
    if (rc)
    {
      printf("Could not set authentication info, error %d (%s)\n", rc, mosquitto_strerror(rc));
    }
  }

  if (!rc)
  {
    rc = mosquitto_connect(mosq, config.pBrokerName, 1883, 60);
    if (rc)
    {
      printf("Client could not connect to broker '%s', error %d (%s)\n", config.pBrokerName, rc, mosquitto_strerror(rc));
    }
  }

  while (keepRunning && !rc)
  {
    if (!(gatherData(&config, &data)))
    {
      fprintf(stderr, "could not gather data. Sleeping...\n");
    }
    else
    {
      if (publishToMqtt (mosq, &data, config.pTopic, config.pNodeName))
      {
        fprintf(stderr, "publishing failed. Sleeping...\n");
      }
    }

    sleep(60);
  }

  printf ("Closing down.\n");
  deinitGatherData();
  if (mosq) mosquitto_disconnect(mosq);
  if (mosq) mosquitto_destroy(mosq);
  mosquitto_lib_cleanup();
  if (&cfg) config_destroy(&cfg);
  if (!rc)
    return EXIT_SUCCESS;
  else
    return EXIT_FAILURE;
}

int publishToMqtt (struct mosquitto *mosq, struct data *data, const char *pTopic, const char *pNode)
{
  int rc;
  char topic[128];
  char payload[256];

  rc = mosquitto_reconnect(mosq);
  if (rc)
  {
    printf ("Could not reconnect: error %d (%s)\n", rc, mosquitto_strerror(rc));
  }
  else
  {
    snprintf (topic, sizeof topic, "%s/%s/environment_temperature/config", pTopic, pNode);
    snprintf (payload, sizeof payload, "{\"name\": \"%s_environment_temperature\", \"device_class\": \"temperature\", \"state_topic\": \"%s/%s/environment/state\", \"value_template\": \"{{value_json.temperature}}\"}", pNode, pTopic, pNode);
    printf ("topic '%s' payload '%s'\n", topic, payload);
    rc = mosquitto_publish(mosq, NULL, topic, strlen(payload), payload, 0, false);
    if (rc)
    {
      printf ("Could not publish config msg, error %d (%s)\n", rc, mosquitto_strerror(rc));
    }
    snprintf (topic, sizeof topic, "%s/%s/environment_humidity/config", pTopic, pNode);
    snprintf (payload, sizeof payload, "{\"name\": \"%s_environment_humidity\", \"unit_of_measurement\": \"%\", \"state_topic\": \"%s/%s/environment/state\", \"value_template\": \"{{value_json.humidity}}\"}", pNode, pTopic, pNode);
    printf ("topic '%s' payload '%s'\n", topic, payload);
    rc = mosquitto_publish(mosq, NULL, topic, strlen(payload), payload, 0, false);
    if (rc)
    {
      printf ("Could not publish config msg, error %d (%s)\n", rc, mosquitto_strerror(rc));
    }
    snprintf (topic, sizeof topic, "%s/%s/environment_pressure/config", pTopic, pNode);
    snprintf (payload, sizeof payload, "{\"name\": \"%s_environment_pressure\", \"unit_of_measurement\": \"mbar\", \"state_topic\": \"%s/%s/environment/state\", \"value_template\": \"{{value_json.pressure}}\"}", pNode, pTopic, pNode);
    printf ("topic '%s' payload '%s'\n", topic, payload);
    rc = mosquitto_publish(mosq, NULL, topic, strlen(payload), payload, 0, false);
    if (rc)
    {
      printf ("Could not publish config msg, error %d (%s)\n", rc, mosquitto_strerror(rc));
    }
    snprintf (topic, sizeof topic, "%s/%s/environment_pressure_reduced/config", pTopic, pNode);
    snprintf (payload, sizeof payload, "{\"name\": \"%s_environment_pressure_reduced\", \"unit_of_measurement\": \"mbar\", \"state_topic\": \"%s/%s/environment/state\", \"value_template\": \"{{value_json.pressure_reduced}}\"}", pNode, pTopic, pNode);
    printf ("topic '%s' payload '%s'\n", topic, payload);
    rc = mosquitto_publish(mosq, NULL, topic, strlen(payload), payload, 0, false);
    if (rc)
    {
      printf ("Could not publish config msg, error %d (%s)\n", rc, mosquitto_strerror(rc));
    }
    snprintf (topic, sizeof topic, "%s/%s/environment/state", pTopic, pNode);
    snprintf (payload, sizeof payload, "{\"temperature\": %0.2f, \"humidity\": %0.2f, \"pressure\": %0.2f, \"pressure_reduced\": %0.2f}", data->Temperature, data->Humidity, data->Pressure/100.0, data->PressureReduced/100.0);
    printf ("topic '%s' payload '%s'\n", topic, payload);
    rc = mosquitto_publish(mosq, NULL, topic, strlen(payload), payload, 0, false);
    if (rc)
    {
      printf ("Could not publish state msg, error %d (%s)\n", rc, mosquitto_strerror(rc));
    }
  }

  return rc;
}


void printHelp(void)
{
  printf ("\n");
  printf ("%s %s %s\n", TOOLNAME, BME280_VERSION, COPYRIGHT);
  printf ("\n");
  printf ("options:\n");
  printf ("  -c config : specify config file\n");
  printf ("\n");
}

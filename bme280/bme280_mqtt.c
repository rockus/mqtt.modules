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

extern int initGatherData();
extern int gatherData (struct config *config, struct data *data);

int publishToMqtt (struct mosquitto *mosq, struct data *data);
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
  if (!(config_lookup_string(&cfg, "i2cbus", &(config.pi2cBus))))
  {
    fprintf(stderr, "No 'i2cbus' setting in configuration file.\n");
    config_destroy(&cfg);
    return EXIT_FAILURE;
  }

printf ("conf file: %s\n", configFilePath);
printf ("node: %s\n", config.pNodeName);
printf ("broker: %s\n", config.pBrokerName);
printf ("i2c bus: %s\n", config.pi2cBus);

  initGatherData();

  mosquitto_lib_init();
  mosq = mosquitto_new(config.pNodeName, true, NULL);
  if (!mosq)
  {
    printf("mosquitto handler could not be allocated! error %d (%s)\n", errno, strerror(errno));
    rc = errno;
  }
  else
  {
    rc = mosquitto_connect(mosq, config.pBrokerName, 1883, 60);
    if (rc)
    {
      printf("Client could not connect to broker '%s'! Error Code: %d (%s)\n", config.pBrokerName, rc, mosquitto_strerror(rc));
    }
  }

  while (keepRunning && !rc)
  {
    if (!(gatherData(&config, &data)))
    {
      fprintf(stderr, "could not gather data. Sleeping...\n");
    }

    if (publishToMqtt (mosq, &data))
    {
      fprintf(stderr, "publishing failed. Sleeping...\n");
    }

    sleep(60);
  }

  printf ("Closing down.\n");
  if (mosq) mosquitto_disconnect(mosq);
  if (mosq) mosquitto_destroy(mosq);
  mosquitto_lib_cleanup();
  if (&cfg) config_destroy(&cfg);
  if (!rc)
    return EXIT_SUCCESS;
  else
    return EXIT_FAILURE;
}

//int publishToMqtt (struct config *config, struct data *data)
int publishToMqtt (struct mosquitto *mosq, struct data *data)
{
  int rc;
  char payload[128];

  rc = mosquitto_reconnect(mosq);
  if (rc)
  {
    printf ("Could not reconnect: error %d (%s)\n", rc, mosquitto_strerror(rc));
  }
  else
  {
    snprintf (payload, sizeof payload, "{\"temperature\": %0.2f, \"humidity\": %0.2f, \"pressure\": %0.2f, \"pressure_reduced\": %0.2f}", data->Temperature, data->Humidity, data->Pressure/100.0, data->PressureReduced/100.0);
    printf ("topic '%s' payload '%s'\n", "Werkstatt/environment", payload);
    rc = mosquitto_publish(mosq, NULL, "Werkstatt/environment", strlen(payload), payload, 0, false);
    if (rc)
    {
      printf ("Could not publish: error %d (%s)\n", rc, mosquitto_strerror(rc));
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

/*
 * raspi_pulsecount_mqtt
 * author: Oliver Gerler 2022,2024
 *
 * based on wiringpi example isr.c
 */

#include <ctype.h>
#include <errno.h>
#include <libconfig.h>
#include <limits.h>
#include <mosquitto.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/time.h>
#include <unistd.h>
#include <wiringPi.h>

#include "../config.h"
#include "raspi_pulsecount_mqtt.h"

struct mosquitto *mosq;
int publishToMqtt(uint8_t);
void printHelp(void);

struct config config;

uint32_t counter[8];
struct timeval tv[8];
const char topictext[8][7] = {{""}, {"Server"}, {""}, {""}, {"Herd"}, {""}, {"Fridge"}, {"Boiler"}};

// wiringPi pin numbers
//  function   wiringPi BCM Header
//  Boiler S0     7      4    7
//  Server S0     1     18   12
//  Herd S0       4     23   16
//                5     24   18
//  fridge S0     6     25   22
#define PIN_BOILER 7
#define PIN_SERVER 1
#define PIN_HERD   4
#define PIN_FRIDGE 6

// max. power consumption (in Watts; to ignore spurious pulses)
#define MAX_POWER_BOILER 3000
#define MAX_POWER_SERVER  500
#define MAX_POWER_HERD  12000
#define MAX_POWER_FRIDGE 1000

uint8_t running=1;

// sig handler
void sigHandler(int sig)
{
  if (sig==SIGINT)				// only quit on CTRL-C
  {
    running = 0;
  }
}


void irqBoilerHandler(void)
{
  publishToMqtt(PIN_BOILER);
}
void irqServerHandler(void)
{
  publishToMqtt(PIN_SERVER);
}
void irqHerdHandler(void)
{
  publishToMqtt(PIN_HERD);
}
void irqFridgeHandler(void)
{
  publishToMqtt(PIN_FRIDGE);
}


int main(int argc, char **argv)
{
  // handling Crl-C
  struct sigaction act;

  // config file handling
  char configFilePath[MAXPATHLEN];
  config_t cfg;
  int c;						// for getopt

  int rc;

  if (argc==1)
  {
    printHelp();
    return 1;
  }

  act.sa_handler = sigHandler;
  sigaction(SIGINT, &act, NULL);	// catch Ctrl-C

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

  mosquitto_lib_init();
  mosq = mosquitto_new(config.pNodeName, true, NULL);
  if (!mosq)
  {
    printf("mosquitto handler could not be allocated! error %d (%s)\n", errno, strerror(errno));
  }
  else
  {
    rc = mosquitto_connect(mosq, config.pBrokerName, 1883, 60);
    if (rc)
    {
      printf("Client could not connect to broker '%s'! Error Code: %d (%s)\n", config.pBrokerName, rc, mosquitto_strerror(rc));
    }
  }

  uint8_t pin;
  char payload[512], topic[256];
  pin = PIN_BOILER;
  snprintf(topic, (sizeof topic)-1, "homeassistant/sensor/%s/%s_energy/config", config.pNodeName, topictext[pin]);
  snprintf(payload, (sizeof payload)-1, "{\"name\":\"%s\", \"object_id\":\"%s_%s_energy\", \"device_class\":\"energy\", \"state_class\":\"total_increasing\", \"unit_of_measurement\":\"kWh\", \"state_topic\":\"homeassistant/sensor/%s/%s/state\", \"value_template\":\"{{value_json.energy}}\"}", topictext[pin], config.pNodeName, topictext[pin], config.pNodeName, topictext[pin], config.pNodeName, topictext[pin]);	// 1000imp/kWh
//printf ("topic payload: %s %s\n", topic, payload);
  rc = mosquitto_publish(mosq, NULL, topic, strlen(payload), payload, 0, TRUE);
  pin = PIN_SERVER;
  snprintf(topic, (sizeof topic)-1, "homeassistant/sensor/%s/%s_energy/config", config.pNodeName, topictext[pin]);
  snprintf(payload, (sizeof payload)-1, "{\"name\":\"%s\", \"object_id\":\"%s_%s_energy\", \"device_class\":\"energy\", \"state_class\":\"total_increasing\", \"unit_of_measurement\":\"kWh\", \"state_topic\":\"homeassistant/sensor/%s/%s/state\", \"value_template\":\"{{value_json.energy}}\"}", topictext[pin], config.pNodeName, topictext[pin], config.pNodeName, topictext[pin], config.pNodeName, topictext[pin]);	// 1000imp/kWh
//printf ("topic payload: %s %s\n", topic, payload);
  rc = mosquitto_publish(mosq, NULL, topic, strlen(payload), payload, 0, TRUE);
  pin = PIN_HERD;
  snprintf(topic, (sizeof topic)-1, "homeassistant/sensor/%s/%s_energy/config", config.pNodeName, topictext[pin]);
  snprintf(payload, (sizeof payload)-1, "{\"name\":\"%s\", \"object_id\":\"%s_%s_energy\", \"device_class\":\"energy\", \"state_class\":\"total_increasing\", \"unit_of_measurement\":\"kWh\", \"state_topic\":\"homeassistant/sensor/%s/%s/state\", \"value_template\":\"{{value_json.energy}}\"}", topictext[pin], config.pNodeName, topictext[pin], config.pNodeName, topictext[pin], config.pNodeName, topictext[pin]);	// 1000imp/kWh
//printf ("topic payload: %s %s\n", topic, payload);
  rc = mosquitto_publish(mosq, NULL, topic, strlen(payload), payload, 0, TRUE);
  pin = PIN_FRIDGE;
  snprintf(topic, (sizeof topic)-1, "homeassistant/sensor/%s/%s_energy/config", config.pNodeName, topictext[pin]);
  snprintf(payload, (sizeof payload)-1, "{\"name\":\"%s\", \"object_id\":\"%s_%s_energy\", \"device_class\":\"energy\", \"state_class\":\"total_increasing\", \"unit_of_measurement\":\"kWh\", \"state_topic\":\"homeassistant/sensor/%s/%s/state\", \"value_template\":\"{{value_json.energy}}\"}", topictext[pin], config.pNodeName, topictext[pin], config.pNodeName, topictext[pin], config.pNodeName, topictext[pin]);	// 1000imp/kWh
//printf ("topic payload: %s %s\n", topic, payload);
  rc = mosquitto_publish(mosq, NULL, topic, strlen(payload), payload, 0, TRUE);

  wiringPiSetup();
  pinMode(PIN_BOILER, INPUT);
  pullUpDnControl(PIN_BOILER, PUD_UP);
  pinMode(PIN_SERVER, INPUT);
  pullUpDnControl(PIN_SERVER, PUD_UP);
  pinMode(PIN_HERD, INPUT);
  pullUpDnControl(PIN_HERD, PUD_UP);
  pinMode(PIN_FRIDGE, INPUT);
  pullUpDnControl(PIN_FRIDGE, PUD_UP);

  wiringPiISR(PIN_BOILER, INT_EDGE_FALLING, &irqBoilerHandler);
  wiringPiISR(PIN_SERVER, INT_EDGE_FALLING, &irqServerHandler);
  wiringPiISR(PIN_HERD, INT_EDGE_FALLING, &irqHerdHandler);
  wiringPiISR(PIN_FRIDGE, INT_EDGE_FALLING, &irqFridgeHandler);

  while (running)
    sleep (1);

  printf ("Closing down mqtt...");
  if (mosq) mosquitto_disconnect(mosq);
  if (mosq) mosquitto_destroy(mosq);
  mosquitto_lib_cleanup();
  printf ("done\n");

  return 0 ;
}


int publishToMqtt(uint8_t pin)
{
  int rc;
  char payload[512], topic[256];
  struct timeval tv_current;
  uint64_t timediff, timediff_min;

  // it does not matter if not all messages get transmitted, since the payload is a quasi-absolute counter
  // as long as this tool does not restart, that quasi-absolute counter will increment

  gettimeofday(&tv_current, NULL);
  timediff = (tv_current.tv_sec - tv[pin].tv_sec)*1e6 + tv_current.tv_usec - tv[pin].tv_usec;
  tv[pin].tv_sec = tv_current.tv_sec;
  tv[pin].tv_usec = tv_current.tv_usec;

  if ( ((pin == PIN_BOILER) && ((3600*1e6 / MAX_POWER_BOILER) >= timediff_min)) ||	// to avoid spurious pulses
       ((pin == PIN_SERVER) && ((3600*1e6 / MAX_POWER_SERVER) >= timediff_min)) ||
       ((pin == PIN_HERD)   && ((3600*1e6 / MAX_POWER_HERD  ) >= timediff_min)) ||
       ((pin == PIN_FRIDGE) && ((3600*1e6 / MAX_POWER_FRIDGE) >= timediff_min)) )
  {
    counter[pin]++;
  }

  rc = mosquitto_reconnect(mosq);
  // rc==MOSQ_ERR_NO_CONN - client not currently connected
  // rc==MOSQ_ERR_SUCCESS - success

  if (rc == MOSQ_ERR_SUCCESS)
  {
    snprintf(topic, (sizeof topic)-1, "homeassistant/sensor/%s/%s_energy/config", config.pNodeName, topictext[pin]);
    snprintf(payload, (sizeof payload)-1, "{\"name\":\"%s\", \"object_id\":\"%s_%s_energy\", \"device_class\":\"energy\", \"state_class\":\"total_increasing\", \"unit_of_measurement\":\"kWh\", \"state_topic\":\"homeassistant/sensor/%s/%s/state\", \"value_template\":\"{{value_json.energy}}\"}", topictext[pin], config.pNodeName, topictext[pin], config.pNodeName, topictext[pin], config.pNodeName, topictext[pin]);	// 1000imp/kWh
//printf ("topic payload: %s %s\n", topic, payload);
    rc = mosquitto_publish(mosq, NULL, topic, strlen(payload), payload, 0, TRUE);
  }

  if (rc == MOSQ_ERR_SUCCESS)
  {
    snprintf(topic, (sizeof topic)-1, "homeassistant/sensor/%s/%s/state", config.pNodeName, topictext[pin]);
    snprintf(payload, (sizeof payload)-1, "{\"energy\":%.3f}", (float)counter[pin]/1000);	// convert from Wh to kWh
//printf ("topic payload: %s %s\n", topic, payload);
    rc = mosquitto_publish(mosq, NULL, topic, strlen(payload), payload, 0, false);
  }

  if (rc)
  {
    printf ("Could not publish: error %d (%s)\n", rc, mosquitto_strerror(rc));
  }

/*
  snprintf (topic, (sizeof topic)-1, "Energie/%s", topictext[pin]);

  printf ("pin %d timediff %7.3fs topic %s power %.3fW\n", pin, (float)timediff/1e6, topic, (float)3600*1e6/timediff);

  if (pin == PIN_BOILER)
  {
    timediff_min = 3600*1e6 / MAX_POWER_BOILER;
    if (timediff >= timediff_min)			// to avoid suprious pulses
    {
      counter[pin]++;
      snprintf (payload, (sizeof payload)-1, "%d", counter[pin]);
      rc = mosquitto_publish(mosq, NULL, topic, strlen(payload), payload, 0, false);
      if (rc==MOSQ_ERR_NO_CONN)
      {
        printf ("The client is currently not connected, trying to reconnect... ");
        rc = mosquitto_reconnect(mosq);
        if (rc==MOSQ_ERR_SUCCESS)
        {
          printf ("ok\n");
          rc = mosquitto_publish(mosq, NULL, topic, strlen(payload), payload, 0, false);
        }
        else
        {
          printf ("fail\n");
        }
      }
    }
    else		// we had a spurious pulse, ignore it
      rc=0;
  }
  if (pin == PIN_SERVER)
  {
    timediff_min = 3600*1e6 / MAX_POWER_SERVER;
    if (timediff >= timediff_min)			// to avoid suprious pulses
    {
      counter[pin]++;
      snprintf (payload, (sizeof payload)-1, "%d", counter[pin]);
      rc = mosquitto_publish(mosq, NULL, topic, strlen(payload), payload, 0, false);
      if (rc==MOSQ_ERR_NO_CONN)
      {
        printf ("The client is currently not connected, trying to reconnect... ");
        rc = mosquitto_reconnect(mosq);
        if (rc==MOSQ_ERR_SUCCESS)
        {
          printf ("ok\n");
          rc = mosquitto_publish(mosq, NULL, topic, strlen(payload), payload, 0, false);
        }
        else
        {
          printf ("fail\n");
        }
      }
    }
    else		// we had a spurious pulse, ignore it
      rc=0;
  }
  if (pin == PIN_HERD)
  {
    timediff_min = 3600*1e6 / MAX_POWER_HERD;
    if (timediff >= timediff_min)			// to avoid suprious pulses
    {
      counter[pin]++;
      snprintf (payload, (sizeof payload)-1, "%d", counter[pin]);
      rc = mosquitto_publish(mosq, NULL, topic, strlen(payload), payload, 0, false);
      if (rc==MOSQ_ERR_NO_CONN)
      {
        printf ("The client is currently not connected, trying to reconnect... ");
        rc = mosquitto_reconnect(mosq);
        if (rc==MOSQ_ERR_SUCCESS)
        {
          printf ("ok\n");
          rc = mosquitto_publish(mosq, NULL, topic, strlen(payload), payload, 0, false);
        }
        else
        {
          printf ("fail\n");
        }
      }
    }
    else		// we had a spurious pulse, ignore it
      rc=0;
  }
  if (pin == PIN_FRIDGE)
  {
    timediff_min = 3600*1e6 / MAX_POWER_FRIDGE;
    if (timediff >= timediff_min)			// to avoid suprious pulses
    {
      counter[pin]++;
      snprintf (payload, (sizeof payload)-1, "%d", counter[pin]);
      rc = mosquitto_publish(mosq, NULL, topic, strlen(payload), payload, 0, false);
      if (rc==MOSQ_ERR_NO_CONN)
      {
        printf ("The client is currently not connected, trying to reconnect... ");
        rc = mosquitto_reconnect(mosq);
        if (rc==MOSQ_ERR_SUCCESS)
        {
          printf ("ok\n");
          rc = mosquitto_publish(mosq, NULL, topic, strlen(payload), payload, 0, false);
        }
        else
        {
          printf ("fail\n");
        }
      }
    }
    else		// we had a spurious pulse, ignore it
      rc=0;
  }
*/
  return rc;
}

void printHelp(void)
{
  printf ("\n");
  printf ("%s %s %s\n", TOOLNAME, PULSECOUNT_VERSION, COPYRIGHT);
  printf ("\n");
  printf ("options:\n");
  printf ("  -c config : specify config file\n");
  printf ("\n");
}

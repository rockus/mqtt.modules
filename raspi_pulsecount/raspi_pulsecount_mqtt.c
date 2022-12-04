/*
 * raspi_pulsecount_mqtt
 * author: Oliver Gerler 2022
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
#include <unistd.h>
#include <wiringPi.h>

#include "../config.h"
#include "raspi_pulsecount_mqtt.h"

struct mosquitto *mosq;
int publishToMqtt(uint8_t);
void printHelp(void);

uint32_t counter[8];

// wiringPi pin numbers
//  function   wiringPi BCM Header
//  Boiler S0     7      4    7
//  Server S0     1     18   12
//  Herd S0       4     23   16
#define PIN_BOILER 7
#define PIN_SERVER 1
#define PIN_HERD   4

// sig handler
void sigHandler(int sig)
{
  if (sig==SIGINT)				// only quit on CTRL-C
  {
    printf ("Closing down.\n");
    if (mosq) mosquitto_disconnect(mosq);
    if (mosq) mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
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


int main(int argc, char **argv)
{
  // handling Crl-C
  struct sigaction act;

  // config file handling
  char configFilePath[MAXPATHLEN];
  config_t cfg;
  struct config config;
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

  wiringPiSetup();
  pinMode(PIN_BOILER, INPUT);
  pullUpDnControl(PIN_BOILER, PUD_UP);
  pinMode(PIN_SERVER, INPUT);
  pullUpDnControl(PIN_SERVER, PUD_UP);
  pinMode(PIN_HERD, INPUT);
  pullUpDnControl(PIN_HERD, PUD_UP);

  wiringPiISR(PIN_BOILER, INT_EDGE_FALLING, &irqBoilerHandler);
  wiringPiISR(PIN_SERVER, INT_EDGE_FALLING, &irqServerHandler);
  wiringPiISR(PIN_HERD, INT_EDGE_FALLING, &irqHerdHandler);

  for (;;)
    sleep (UINT_MAX);

  printf ("end\n");

  return 0 ;
}


int publishToMqtt(uint8_t pin)
{
  int rc;
  char payload[128];

  rc = mosquitto_reconnect(mosq);
  if (!rc)		// it does not matter if not all messages get transmitted, since the payload is a quasi-absolute counter
			// as long as this tool does not restart, that quasi-absolute counter will increment
  {
    counter[pin]++;		// to make each msg different from the next, otherwise homeassistant doesn't accept them
			// in homeassistant: state_class: total_increasing
    snprintf (payload, sizeof payload, "%d", counter[pin]);
    if (pin == PIN_BOILER)
      rc = mosquitto_publish(mosq, NULL, "Energie/Boiler", strlen(payload), payload, 0, false);
    if (pin == PIN_SERVER)
      rc = mosquitto_publish(mosq, NULL, "Energie/Server", strlen(payload), payload, 0, false);
    if (pin == PIN_HERD)
      rc = mosquitto_publish(mosq, NULL, "Energie/Herd", strlen(payload), payload, 0, false);
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
  printf ("%s %s %s\n", TOOLNAME, PULSECOUNT_VERSION, COPYRIGHT);
  printf ("\n");
  printf ("options:\n");
  printf ("  -c config : specify config file\n");
  printf ("\n");
}

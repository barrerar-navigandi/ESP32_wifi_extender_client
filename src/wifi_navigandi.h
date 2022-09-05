#pragma once

#include "config.h"

void configure_wifi(void);

//WiFi Mode
#define WIFI_MODE_DEFAULT WIFI_MODE_APSTA

//Wifi Power
#define WIFI_POWER_DEFAULT WIFI_POWER_19_5dBm

//Wifi Protocol
#define WIFI_PROTOCOL_DEFAULT WIFI_PROTOCOL_11B

#define WIFI_SSID_DEFAULT "NAVIGANDI_ORBIS_00030004"
#define WIFI_PASSWORD_DEFAULT "Navigandi"
#define WIFI_CHANNEL_DEFAULT 1
#define WIFI_SSID_HIDDEN_DEFAULT 0
#define WIFI_MAX_CONN_DEFAULT 4
#define WIFI_FTM_DEFAULT 0

//Wifi port
#define WIFI_SERVER_PORT_DEFAULT 5

//Wifi Client configs
#define TCP_KEEP_ALIVE_DEFAULT 3000
#define TCP_KEEPINTVL_DEFAULT 2
#define TCP_KEEPCNT_DEFAULT 2
#define TCP_NODELAY_DEFAULT 0

extern WiFiServer wifi_server_obj;
extern WiFiClient wifi_server_clients_obj[WIFI_MAX_CONN_DEFAULT];
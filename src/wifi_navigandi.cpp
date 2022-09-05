#include "wifi_navigandi.h"

WiFiServer wifi_server_obj(WIFI_SERVER_PORT_DEFAULT, WIFI_MAX_CONN_DEFAULT);
WiFiClient wifi_server_clients_obj[WIFI_MAX_CONN_DEFAULT];
WiFiClient wifi_orbis_client_obj;

void configure_wifi(void){
    bool success = true;
    
    IPAddress local_IP(192,168,4,1);
    IPAddress gateway(192,168,4,1);
    IPAddress subnet(255,255,255,0);

    uint8_t current_protocol;

     //store wifi config in flash area
     WiFi.persistent(true);
    
    //Config Wifi Mode
   if(WiFi.getMode() != WIFI_MODE_DEFAULT){
        if(WiFi.mode(WIFI_MODE_DEFAULT) != true){
           Serial.println("Could not update Wifi mode");
           success = false;
        };
    }
    //Serial.print("Wifi mode "); Serial.print(WiFi.getMode()); Serial.print("\r\n");

     //Config Wifi Power
   if(WiFi.getTxPower()!= WIFI_POWER_DEFAULT ){
        if(WiFi.setTxPower(WIFI_POWER_DEFAULT)!= true){
           Serial.println("Could not set Wifi TX power");
           success = false;
        };
    } 
    //Serial.print("Wifi power "); Serial.print(WiFi.getTxPower()); Serial.print("\r\n");

    //Get Current Protocol
    esp_wifi_get_protocol(WIFI_IF_AP, &current_protocol);
    if(current_protocol != WIFI_PROTOCOL_DEFAULT){
        if(esp_wifi_set_protocol(WIFI_IF_AP, WIFI_PROTOCOL_DEFAULT) != ESP_OK){
            Serial.println("Could not update Wifi protocol");
            success = false;
        };
    }
    //esp_wifi_get_protocol(WIFI_IF_AP, &current_protocol); Serial.print("Wifi AP Protocol "); Serial.print(current_protocol); Serial.print("\r\n");

    esp_wifi_get_protocol(WIFI_IF_STA, &current_protocol);
    if(current_protocol != WIFI_PROTOCOL_DEFAULT){
        if(esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_DEFAULT) != ESP_OK){
            Serial.println("Could not update Wifi protocol");
            success = false;
        };
    }
    //esp_wifi_get_protocol(WIFI_IF_STA, &current_protocol); Serial.print("Wifi STA Protocol "); Serial.print(current_protocol); Serial.print("\r\n");
    
    //Enable AP
   // WiFi.enableAP(true);
    /*if(WiFi.enableAP(true) != true){
          Serial.println("Could not enable Wifi");
          success = false;
    };*/

    //Configure AP
    if(WiFi.softAP(WIFI_SSID_DEFAULT, WIFI_PASSWORD_DEFAULT, WIFI_CHANNEL_DEFAULT, WIFI_SSID_HIDDEN_DEFAULT, WIFI_MAX_CONN_DEFAULT, WIFI_FTM_DEFAULT) != true){
        Serial.println("Could not configure Wifi SSID, Password, Channel, etc");
        success = false;
    }

   if(WiFi.softAPConfig(local_IP, gateway, subnet) != true){
        Serial.println("Could not configure Soft AP Config");
        success = false;
    }

    //Initiate trials to connect STATION to WIFI
    WiFi.begin(WIFI_SSID_DEFAULT, WIFI_PASSWORD_DEFAULT);

    //Begin AP Server
    wifi_server_obj.begin();
    wifi_server_obj.setNoDelay(false);

    if(success){
        Serial.println("Wifi successfully initialized!");
    }else{
        Serial.println("Could not initialize Wifi, resetting ESP32!");
        while(1){};    
    }
    
}
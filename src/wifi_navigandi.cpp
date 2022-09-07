#include "wifi_navigandi.h"

WiFiServer wifi_server_obj(WIFI_SERVER_PORT_DEFAULT, WIFI_MAX_CONN_DEFAULT);
WiFiClient wifi_server_clients_obj[WIFI_MAX_CONN_DEFAULT];
WiFiClient wifi_orbis_client_obj;

static void orbis_client_in_server_out_task(void);
static void server_in_orbis_client_out(void);
static void set_server_out_states(server_out_states_t state_in);

//FreeRTOS
TaskHandle_t Server_In_TaskHandle;
TaskHandle_t Server_Out_TaskHandle;
static portMUX_TYPE server_out_state_flag_mutex = portMUX_INITIALIZER_UNLOCKED;


//State Machine Flags
server_out_states_t server_out_state_flag = CONNECTING_STATE;
//volatile uint8_t server_in_state_flag;
    
IPAddress local_IP_station(192,168,4,1);//station will connect to ORBIS
IPAddress local_IP_AP(10,0,0,1);
IPAddress gateway_AP(10,0,0,1);
IPAddress subnet_AP(255,255,255,0);



void configure_wifi(void){
    bool success = true;
    
    uint8_t current_protocol;

   //ip_portmap_add(6,  ipaddr_addr("192.168.4.1") ,5, ipaddr_addr("10.0.0.1") ,5);
    

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

   if(WiFi.softAPConfig(local_IP_AP, gateway_AP, subnet_AP) != true){
        Serial.println("Could not configure Soft AP Config");
        success = false;
    }

    //Initiate trials to connect STATION to WIFI
    WiFi.begin(WIFI_SSID_DEFAULT, WIFI_PASSWORD_DEFAULT);
    
    //Begin AP Server
    //wifi_server_obj.begin();
    //wifi_server_obj.setNoDelay(false);

    if(success){
        Serial.println("Wifi successfully initialized!");
    }else{
        Serial.println("Could not initialize Wifi, resetting ESP32!");
        while(1){};    
    }
    
}


void wifi_init_freertos(void){

    xTaskCreate((TaskFunction_t) orbis_client_in_server_out_task,(const char *)"SERVER_OUT_TASK", SERVER_OUT_TASK_STACK_SIZE, NULL, SERVER_OUT_TASK_PRIORITY,&Server_Out_TaskHandle);
	xTaskCreate((TaskFunction_t) server_in_orbis_client_out,(const char *)"SERVER_IN_TASK", SERVER_IN_TASK_STACK_SIZE, NULL, SERVER_IN_TASK_PRIORITY,&Server_In_TaskHandle);
}

static void orbis_client_in_server_out_task(void){
    Serial.print("Initialized server out task\r\n");

    wl_status_t station_status;
    uint8_t max_retries_counter_wifi = 0;
    uint8_t max_retries_counter_client = 0;
    int value;
    uint8_t byte = 0;

    while(1){
        station_status = WiFi.status();
        switch(server_out_state_flag){
            case CONNECTING_STATE:
                if(station_status == WL_CONNECTED){//try to stablish tcp/ip connection
                    Serial.print("WIFI EXTENDER CONNECTED TO ORBIS WIFI\r\n");
                    if(wifi_orbis_client_obj.connect(local_IP_station, WIFI_SERVER_PORT_DEFAULT) == 1){
                        Serial.print("WIFI EXTENDER CLIENT CONNECTED TO ORBIS SERVER \r\n");
                        
                        //Set Client configurations
                        wifi_orbis_client_obj.getOption(TCP_KEEPALIVE ,&value);
                        if(value != TCP_KEEP_ALIVE_DEFAULT){
                            value = TCP_KEEP_ALIVE_DEFAULT;
                            wifi_orbis_client_obj.setOption(TCP_KEEPALIVE , &value);
                            wifi_orbis_client_obj.getOption(TCP_KEEPALIVE ,&value);
                            Serial.print("TCP KEEP ALIVE ");Serial.print(value); Serial.print("\r\n");
                        }
                        //wifi_clients_obj[client_index].getOption(TCP_KEEPIDLE  ,&value);
                        //Serial.print("TCP KEEP IDLE ");Serial.print(value); Serial.print("\r\n");

                        wifi_orbis_client_obj.getOption(TCP_KEEPINTVL ,&value);
                        if(value != TCP_KEEPINTVL_DEFAULT){
                            value = TCP_KEEPINTVL_DEFAULT;
                            wifi_orbis_client_obj.setOption(TCP_KEEPINTVL , &value);
                            wifi_orbis_client_obj.getOption(TCP_KEEPINTVL   ,&value);
                            Serial.print("TCP KEEP INTVL ");Serial.print(value); Serial.print("\r\n");
                        }
                        
                        wifi_orbis_client_obj.getOption(TCP_KEEPCNT, &value);
                        if(value != TCP_KEEPCNT_DEFAULT){
                            value = TCP_KEEPCNT_DEFAULT;
                            wifi_orbis_client_obj.setOption(TCP_KEEPCNT, &value);
                            wifi_orbis_client_obj.getOption(TCP_KEEPCNT, &value);
                            Serial.print("TCP KEEP CNT ");Serial.print(value); Serial.print("\r\n");
                        }

                        wifi_orbis_client_obj.getOption(TCP_NODELAY, &value);
                        if(value != TCP_NODELAY_DEFAULT){
                            value = TCP_NODELAY_DEFAULT;
                            wifi_orbis_client_obj.setOption(TCP_NODELAY, &value);
                            wifi_orbis_client_obj.getOption(TCP_NODELAY, &value);
                            Serial.print("TCP NO DELAY ");Serial.print(value); Serial.print("\r\n");
                        }
                
                        set_server_out_states(RUNNING_STATE);
                        max_retries_counter_client = 0;
                    }else{
                        max_retries_counter_client++;
                        if(max_retries_counter_client > SERVER_OUT_CLIENT_CONNECTING_STATE_MAX_RETRIES){//reset ESP32 if client cant connect for 20 seconds
                            Serial.print("UNABLE TO STABLISH A CLIENT CONNECTED TO ORBIS WIFI, RESETTING ESP32 \r\n");
                            vTaskDelay(1000/portTICK_PERIOD_MS);
                            ESP.restart();
                        }
                    }
                    max_retries_counter_wifi = 0;
                }else{
                    max_retries_counter_wifi++;
                    if(max_retries_counter_wifi > SERVER_OUT_WIFI_CONNECTING_STATE_MAX_RETRIES){//reset ESP32
                        Serial.print("UNABLE TO CONNECT TO ORBIS WIFI, RESETTING ESP32 \r\n");
                        vTaskDelay(1000/portTICK_PERIOD_MS);
                        ESP.restart();
                    }
                }
                /*else if(station_status == WL_IDLE_STATUS){
                    Serial.print(" WL_IDLE_STATUS \r\n");
                }else if(station_status == WL_NO_SSID_AVAIL){
                    Serial.print(" WL_NO_SSID_AVAIL \r\n");
                }else if(station_status == WL_SCAN_COMPLETED){
                    Serial.print(" WL_SCAN_COMPLETED \r\n");
                }else if(station_status == WL_CONNECT_FAILED){
                    Serial.print(" WL_CONNECT_FAILED \r\n");
                }else if(station_status == WL_CONNECTION_LOST){
                    Serial.print(" WL_CONNECTION_LOST \r\n");
                }else if(station_status == WL_DISCONNECTED){
                    Serial.print(" WL_DISCONNECTED \r\n");
                }*/
                 Serial.print("CONNECTING STATE \r\n");
                vTaskDelay(SERVER_OUT_CONNECTING_STATE_DEFAULT_DELAY_MS/portTICK_PERIOD_MS);
                break;

            case RUNNING_STATE:
                if(wifi_orbis_client_obj.connected() != true || station_status != WL_CONNECTED){//come back to previous state if connection is lost
                    wifi_orbis_client_obj.stop();
                    set_server_out_states(CONNECTING_STATE);

                }else if (wifi_orbis_client_obj.available()){//read data from orbis and send to clients connected on extension server
                    while(wifi_orbis_client_obj.available()){
                        byte = wifi_orbis_client_obj.read();
                        Serial.print((char) byte);
                        //wifi_buffer_over_the_air(c, j);
                    }
                }else{//if no data received, give a task delay to let other tasks to run
                    vTaskDelay(50/portTICK_PERIOD_MS);
                }
                //Serial.print("RUNNING STATE \r\n");
                //vTaskDelay(1000/portTICK_PERIOD_MS);
                break;

        };
        
        
    }
}

static void server_in_orbis_client_out(void){
     Serial.print("Initialized server in task\r\n");
    while(1){
        /* switch(server_in_state_flag){
            case :
                break;

            case :
                break;

        };*/

        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
}

static void set_server_out_states(server_out_states_t state_in){
    taskENTER_CRITICAL(&server_out_state_flag_mutex);
        server_out_state_flag = state_in;
    taskEXIT_CRITICAL(&server_out_state_flag_mutex);
}




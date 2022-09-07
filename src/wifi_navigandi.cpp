#include "wifi_navigandi.h"

WiFiClient wifi_orbis_client_obj;


static void client_rx_task(void);
static void client_tx_task(void);
static void set_client_rx_states(client_rx_states_t state_in);

//FreeRTOS
TaskHandle_t Client_Tx_TaskHandle;
TaskHandle_t Client_Rx_TaskHandle;
static portMUX_TYPE client_rx_state_flag_mutex = portMUX_INITIALIZER_UNLOCKED;


//State Machine Flags
client_rx_states_t client_rx_state_flag = CONNECTING_STATE;
//volatile uint8_t server_in_state_flag;
    
IPAddress local_IP_station(192,168,4,1);//station will connect to ORBIS

void configure_wifi(void){
    bool success = true;
    
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


    esp_wifi_get_protocol(WIFI_IF_STA, &current_protocol);
    if(current_protocol != WIFI_PROTOCOL_DEFAULT){
        if(esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_DEFAULT) != ESP_OK){
            Serial.println("Could not update Wifi protocol");
            success = false;
        };
    }
    //esp_wifi_get_protocol(WIFI_IF_STA, &current_protocol); Serial.print("Wifi STA Protocol "); Serial.print(current_protocol); Serial.print("\r\n");
    

    //Initiate trials to connect STATION to WIFI
    WiFi.begin(WIFI_SSID_DEFAULT, WIFI_PASSWORD_DEFAULT);

    if(success){
        Serial.println("Wifi successfully initialized!");
    }else{
        Serial.println("Could not initialize Wifi, resetting ESP32!");
        while(1){};    
    }
    
}


void wifi_init_freertos(void){

    xTaskCreate((TaskFunction_t) client_rx_task,(const char *)"CLIENT_RX_TASK", CLIENT_RX_TASK_STACK_SIZE, NULL, CLIENT_RX_TASK_PRIORITY,&Client_Rx_TaskHandle);
	xTaskCreate((TaskFunction_t) client_tx_task,(const char *)"CLIENT_TX_TASK", CLIENT_TX_TASK_STACK_SIZE, NULL, CLIENT_TX_TASK_PRIORITY,&Client_Tx_TaskHandle);
}

static void client_rx_task(void){
    Serial.print("Initialized server out task\r\n");

    wl_status_t station_status;
    uint8_t max_retries_counter_wifi = 0;
    uint8_t max_retries_counter_client = 0;
    int value;
    uint8_t byte = 0;

    while(1){
        station_status = WiFi.status();
        switch(client_rx_state_flag){
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
                
                        set_client_rx_states(RUNNING_STATE);
                        max_retries_counter_client = 0;
                    }else{
                        max_retries_counter_client++;
                        if(max_retries_counter_client > CLIENT_CONNECTING_STATE_MAX_RETRIES){//reset ESP32 if client cant connect for 20 seconds
                            Serial.print("UNABLE TO STABLISH A CLIENT CONNECTED TO ORBIS WIFI, RESETTING ESP32 \r\n");
                            vTaskDelay(1000/portTICK_PERIOD_MS);
                            ESP.restart();
                        }
                    }
                    max_retries_counter_wifi = 0;
                }else{
                    max_retries_counter_wifi++;
                    if(max_retries_counter_wifi > WIFI_CONNECTING_STATE_MAX_RETRIES){//reset ESP32
                        Serial.print("UNABLE TO CONNECT TO ORBIS WIFI, RESETTING ESP32 \r\n");
                        vTaskDelay(1000/portTICK_PERIOD_MS);
                        ESP.restart();
                    }
                }

                Serial.print("CONNECTING STATE \r\n");
                vTaskDelay(CONNECTING_STATE_DEFAULT_DELAY_MS/portTICK_PERIOD_MS);
                break;

            case RUNNING_STATE:
                if(wifi_orbis_client_obj.connected() != true || station_status != WL_CONNECTED){//come back to previous state if connection is lost
                    wifi_orbis_client_obj.stop();
                    set_client_rx_states(CONNECTING_STATE);

                }else if (wifi_orbis_client_obj.available()){//read data from orbis and send to clients connected on extension server
                    while(wifi_orbis_client_obj.available()){
                        byte = wifi_orbis_client_obj.read();
                        xQueueSend(Uart2_Tx_Queue_Handle, &byte, ( TickType_t ) 1);
                        //Serial.print((char) byte);
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

static void client_tx_task(void){
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

static void set_client_rx_states(client_rx_states_t state_in){
    taskENTER_CRITICAL(&client_rx_state_flag_mutex);
        client_rx_state_flag = state_in;
    taskEXIT_CRITICAL(&client_rx_state_flag_mutex);
}




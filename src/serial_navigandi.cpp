#include "serial_navigandi.h"



static void uart2_tx_task(void);
static void uart2_rx_task(void);
static void uart2_buffer_and_send(uint8_t byte);
static void buffering_uart2_rx(uint8_t byte_received);
static void redirect_uart2_rx_data(uint8_t *buffer, uint16_t buff_size, uint8_t id);

//FREERTOS
TaskHandle_t Uart2_Rx_TaskHandle;
TaskHandle_t Uart2_Tx_TaskHandle;
QueueHandle_t Uart2_Tx_Queue_Handle;

static uint16_t counter_uart2_tx = 0;
static uint8_t buffer_uart2_tx[255] = {0,};

//RX vars
static uint8_t buffer_rec_data[256] = {0,};
static uint16_t buffer_rec_counter = 0;
static uint16_t len_bytes_unstuffed = 0;
static uint16_t index_to_check = 0, stuff_pt_value = 0;
static uint32_t checksum_to_compare = 0; static uint8_t checksum_to_compare_arr[4] = {0,};
static uint8_t stuff_msg_valid = 0; static uint8_t checksum_msg_valid = 0;


void init_serial(void){

    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_REF_TICK,
    };

    uart_driver_install(UART_NUM_2, UART2_RX_QUEUE_BUFFER_SIZE, UART2_TX_QUEUE_BUFFER_SIZE, 0, NULL, 0);
    uart_param_config(UART_NUM_2, &uart_config);
    uart_set_pin(UART_NUM_2, TXD2, RXD2, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    Uart2_Tx_Queue_Handle = xQueueCreate(UART2_TX_QUEUE_BUFFER_SIZE, sizeof( uint8_t ) );
    xTaskCreate((TaskFunction_t) uart2_rx_task,(const char *)"UART2_RX_TASK", UART2_RX_TASK_STACK_SIZE, NULL, UART2_RX_TASK_PRIORITY,&Uart2_Rx_TaskHandle);
	xTaskCreate((TaskFunction_t) uart2_tx_task,(const char *)"UART2_TX_TASK", UART2_TX_TASK_STACK_SIZE, NULL, UART2_TX_TASK_PRIORITY,&Uart2_Tx_TaskHandle);
}

static void uart2_tx_task(void){
    Serial.print("Entered UART2 Tx Task \r\n");
    uint8_t byte;

    while(1){
        if( xQueueReceive(Uart2_Tx_Queue_Handle, &byte, ( TickType_t ) (1000/portTICK_PERIOD_MS)) ){
              //Serial.print((char) byte);
              uart2_buffer_and_send(byte);
        }

    }

}

static void uart2_rx_task(void){//received data from serial and send to connected devices
    Serial.println("Initialized UART2 Rx Task");
    uint8_t data; 

    //add_self_task_wdt();

    while (1) {
        const int rxBytes = uart_read_bytes(UART_NUM_2, &data, sizeof(data), 1000 / portTICK_PERIOD_MS);
        if (rxBytes > 0) {
            //Serial.print((char) data);
            buffering_uart2_rx(data);  
        }

        //esp_task_wdt_reset();
    }
}

static void buffering_uart2_rx(uint8_t byte_received){
    //Serial.print((char)byte_received);
    
    //Do not let buffer overflow
	if(buffer_rec_counter >= sizeof(buffer_rec_data)){
		buffer_rec_counter = 0;
	}
	
	//Add data to buffer
	buffer_rec_data[buffer_rec_counter] = byte_received;
	
	//Check if message is valid
	if(byte_received == 0x00){
		while(index_to_check < buffer_rec_counter){
			stuff_pt_value = buffer_rec_data[index_to_check];
			index_to_check += stuff_pt_value;
			//If index to check > bytes_index, it is pointing to a zero outside the message, thus msg invalid.
			if(index_to_check > buffer_rec_counter){
				stuff_msg_valid = 0; //Trace_UartWriteString("COMM RX STUFFED DATA INVALID\r");
			}else if(buffer_rec_data[index_to_check] == 0x00){
				stuff_msg_valid = 1; //Trace_UartWriteString(usart_DBG, "\rSTUFFED DATA VALID");
				len_bytes_unstuffed = System_UnStuffData(buffer_rec_data);
				checksum_to_compare = System_GenerateChecksumAdler32(&buffer_rec_data[1], (len_bytes_unstuffed-4));
				//Checksum comparision
				memcpy(checksum_to_compare_arr, (uint8_t *)&checksum_to_compare, sizeof(checksum_to_compare));
				if(memcmp(checksum_to_compare_arr,&buffer_rec_data[len_bytes_unstuffed - 4 + 1], 4) == 0){
					checksum_msg_valid = 1;//Trace_UartWriteString(usart_DBG, "\rCHECKSUM VALID");
				}else{
					checksum_msg_valid = 0;//Trace_UartWriteString("COMM RX CHECKSUM INVALID\r");
				}
			}
			
		}
		
		//Used for debug only
		if(checksum_msg_valid == 1 && stuff_msg_valid == 1){
            redirect_uart2_rx_data(&buffer_rec_data[5], (len_bytes_unstuffed-8), buffer_rec_data[4]);
            //Serial.print("COMM RX VALID\r\n");
		}else{
			Serial.print("COMM RX INVALID ESP CLIENT\r\n");
		}
		buffer_rec_counter = 0;index_to_check = 0;
		stuff_pt_value = 0;checksum_to_compare = 0;
        checksum_msg_valid = 0; stuff_msg_valid = 0;
		
	}else{
		buffer_rec_counter+=1;
	}
}

static void redirect_uart2_rx_data(uint8_t *buffer, uint16_t buff_size, uint8_t id){
    switch(id){
        case NMEA_TO_ESP32_ID:
            for(uint8_t i = 0; i < buff_size; i++){
                Serial.print( (char) buffer[i]);
            }
            wifi_send_data(buffer, buff_size);
            break;

        /*case SSID_CONFIG_TO_ESP32_ID:
            write_flash_network_name((char *) buffer);
            Serial.println("Network name updated, resetting ESP32");
            vTaskDelay(2000/portTICK_PERIOD_MS);
            ESP.restart();
            break;*/
    }
}


static void uart2_buffer_and_send(uint8_t byte){
    uint32_t send_checksum = 0;
    
    if(counter_uart2_tx >= sizeof(buffer_uart2_tx)){
		counter_uart2_tx = 0;
	}
    
    buffer_uart2_tx[counter_uart2_tx] = byte;

    if(byte == '\n'){//received new nmea msg
        /*for(uint8_t k = 5; k < counter_over_the_air[index][0]; k++){
            Serial.print((char)buffer_rec_data_over_the_air[index][k]);
        }*/
        buffer_uart2_tx[1] = N_HEADER; buffer_uart2_tx[2] = V_HEADER;
        buffer_uart2_tx[3] = ESP32_CL; buffer_uart2_tx[4] = NMEA_TO_ESP32_ID;
        send_checksum = System_GenerateChecksumAdler32(&buffer_uart2_tx[1], counter_uart2_tx);
        memcpy(&buffer_uart2_tx[counter_uart2_tx + 1] , (uint8_t *)&send_checksum, sizeof(send_checksum));
        counter_uart2_tx += sizeof(send_checksum);
        System_StuffData(&buffer_uart2_tx[0], counter_uart2_tx );
        counter_uart2_tx += 2;
        /*for(uint8_t k = 0; k < counter_uart2_tx; k++){
            Serial.print(buffer_uart2_tx[k], HEX);
        }
        Serial.print("\r\n\r\n");*/

        uart_write_bytes(UART_NUM_2, &buffer_uart2_tx[0], counter_uart2_tx);
        counter_uart2_tx = 5;
    }else{
        counter_uart2_tx += 1;
    }
    //Serial.print((char) byte);
}
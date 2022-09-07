#include "serial_navigandi.h"



static void uart2_tx_task(void);
static void uart2_buffer_and_send(uint8_t byte);

//FREERTOS
TaskHandle_t Uart2_Tx_TaskHandle;
QueueHandle_t Uart2_Tx_Queue_Handle;

static uint16_t counter_uart2_tx = 0;
static uint8_t buffer_uart2_tx[255] = {0,};


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
    //xTaskCreate((TaskFunction_t) uart2_rx_task,(const char *)"UART2_RX_TASK", UART2_RX_TASK_STACK_SIZE, NULL, UART2_RX_TASK_PRIORITY,&Uart2_Rx_TaskHandle);
	xTaskCreate((TaskFunction_t) uart2_tx_task,(const char *)"UART2_TX_TASK", UART2_TX_TASK_STACK_SIZE, NULL, UART2_TX_TASK_PRIORITY,&Uart2_Tx_TaskHandle);
}

static void uart2_tx_task(void){
    Serial.print("Entered UART2 Tx Task \r\n");
    uint8_t byte;

    while(1){
        if( xQueueReceive(Uart2_Tx_Queue_Handle, &byte, ( TickType_t ) 1000) ){
              Serial.print((char) byte);
              uart2_buffer_and_send(byte);
        }

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
        send_checksum = System_GenerateChecksumAdler32(&buffer_uart2_tx[1], (counter_uart2_tx-1));
        memcpy(&buffer_uart2_tx[counter_uart2_tx] , (uint8_t *)&send_checksum, sizeof(send_checksum));
        counter_uart2_tx += sizeof(send_checksum);
        System_StuffData(&buffer_uart2_tx[0], (counter_uart2_tx-1));
        counter_uart2_tx += 1;
        /*for(uint8_t k = 0; k < counter_uart2_tx; k++){
            Serial.print((char)buffer_uart2_tx[k]);
        }*/
        uart_write_bytes(UART_NUM_2, &buffer_uart2_tx[0], counter_uart2_tx);
        counter_uart2_tx = 5;
    }else{
        counter_uart2_tx += 1;
    }
    //Serial.print((char) byte);
}
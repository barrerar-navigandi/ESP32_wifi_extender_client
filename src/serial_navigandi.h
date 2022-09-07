#include "config.h"


#define RXD2 GPIO_NUM_16 
#define TXD2 GPIO_NUM_17 

#define UART2_RX_QUEUE_BUFFER_SIZE 2048
#define UART2_TX_TASK_PRIORITY 3
#define UART2_TX_TASK_STACK_SIZE 2048
#define UART2_TX_QUEUE_BUFFER_SIZE 2048

void init_serial(void);


extern QueueHandle_t Uart2_Tx_Queue_Handle;
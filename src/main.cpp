#include "config.h"

void setup() {
    Serial.begin(115200); //UART0 - programming/trace
    configure_wifi();
    wifi_init_freertos();
}

void loop() {

}
#include "watchdog.h"

void init_watchdog(void){
   if(esp_task_wdt_init(TWDT_TIMEOUT_S, true) != ESP_OK){
        Serial.println("Could not initialize task watchdog!");
        delay(2000);
        ESP.restart();
   }else{
        Serial.println("Sucessfully initialized task watchdog!");
   }
}

void add_self_task_wdt(void){
    if(esp_task_wdt_add(NULL) != ESP_OK){
        Serial.println("Could not add current task to wdt");
    }
}
#pragma once
#include "config.h"


#define TWDT_TIMEOUT_S  10

void init_watchdog(void);
void add_self_task_wdt(void);
void add_self_task_wdt(void);
#pragma once
#include "config.h"

//NV_HEADER_IDS
#define	N_HEADER 0x4E
#define	V_HEADER 0x56

//NV_CLASS_IDs
#define ESP32_CL 0x45

enum ESP32_CL_IDS{
	NMEA_TO_ESP32_ID				= 0x00,
    SSID_CONFIG_TO_ESP32_ID         = 0x01,
    NMEA_FROM_ESP32_ID				= 0x02,
};


uint8_t System_UnStuffData(uint8_t *Buffer);
void System_StuffData(uint8_t *Buffer, uint8_t Length);
uint32_t System_GenerateChecksumAdler32(uint8_t * buffer, uint16_t size_of_buffer);
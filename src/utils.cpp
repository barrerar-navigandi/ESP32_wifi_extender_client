#include "utils.h"

//Function: System_UnStuffData
//----------------------------------------------------------------------------//
uint8_t System_UnStuffData(uint8_t *Buffer) {

	//Consistent Overhead Byte Stuffing
	//This routine considers that the data starts on Buffer[1], not Buffer[0],
	//and that Buffer[0] and Buffer[Length+1] can are used to store the "header"
	//and the "overhead".

	uint8_t Code = *Buffer++;
	uint8_t Counter = 0;
	bool Run = true;

	while(Run==true) {
		if(*Buffer==0) { //Byte is a Real Zero:
			Run = false;
		} else if(Code<=1) { //Stuffed Zero:
			Code = *Buffer;
			*Buffer++ = 0;
		} else { //Normal Byte:
			Code--;
			Buffer++;
		}
		Counter++;
		if(Counter==0xFF) {
			Run = false;
		}
	}
	
	return (Counter - 1);
}

//----------------------------------------------------------------------------//
//Function: System_StuffData
//----------------------------------------------------------------------------//
void System_StuffData(uint8_t *Buffer, uint8_t Length) {

	//Consistent Overhead Byte Stuffing
	//This routine considers that the data starts on Buffer[1], not Buffer[0],
	//and that Buffer[0] and Buffer[Length+1] can be used to add the "header"
	//and the "overhead".

	uint8_t ZeroIndex = 0;
	uint8_t Count = 1;
	uint8_t i;

	//Final Zero:
	Buffer[Length+1] = 0;

	//Stuff:
	for(i=1; i<(Length+1); i++) {
		if(Buffer[i]==0) {
			Buffer[ZeroIndex] = Count;
			ZeroIndex = i;
			Count = 1;
		} else {
			Count++;
		}
	}

	//Final Indicator:
	Buffer[ZeroIndex] = Count;

}

uint32_t System_GenerateChecksumAdler32(uint8_t * buffer, uint16_t size_of_buffer){
	const uint32_t MOD_ADLER = 65521;
	uint32_t a = 1, b = 0;
	uint16_t counter = 0;
	
	for(counter = 0; counter < size_of_buffer; counter++){
		a = (a + buffer[counter]) % MOD_ADLER;
		b = (b + a) % MOD_ADLER;
	}
	
	return (b << 16) | a;
}//----------------------------------------------------------------------------//

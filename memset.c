#include <stddef.h>
#include <stdint.h>

void* memset(void* start,int c,size_t len){
	uint8_t* ptr=(uint8_t*)start;
	uint8_t* end=(uint8_t*)start+len;
	while(ptr<end) *ptr++=c;
	return ptr;
}

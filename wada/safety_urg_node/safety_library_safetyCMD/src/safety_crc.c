#include "safety_crc.h"
#include <limits.h>

#define POLYNOM		(0x8408u)  // INVERSION */
#define INIT_VAL	0x0000
#define USE_XOR		(0)

static unsigned short g_crc_table[UCHAR_MAX + 1];

void safety_init_crc()
{
    unsigned short result;
	int u = 0;
	int v = 0;

    for ( u = 0; u <= UCHAR_MAX; u++ ) {
        result = u;
        for ( v = 0; v < CHAR_BIT; v++ ) {
            if ( result & 1 ) {
                result = (result >> 1) ^ POLYNOM;
            } else {
                result >>= 1;
            }
        }
        g_crc_table[u] = result;
    }
}

unsigned short safety_calc_crc(const char *src, int length)
{
	unsigned short crc = INIT_VAL;
    int temp = 0;
	int i = 0;

    for (i = 0; i < length; i++){
        temp = crc ^ (0x00ff & (unsigned short)src[i]);
        crc = (unsigned short)(crc >> CHAR_BIT) ^ g_crc_table[temp & 0xff];
    }
	if(USE_XOR){
		crc = ~crc;
	}

	return crc;
}
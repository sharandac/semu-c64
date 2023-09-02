#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <c64.h>

#include "reu.h"

volatile uint32_t reu_addr = 0xf0000000;
volatile uint32_t reu_page[ REU_PAGE_SIZE / 4 ];

/**
 * @brief load a word from reu
 * 
 * @param addr          address to load from
 * @return uint32_t     word from reu
 *
 * @note: on first run ( addr = 0x00000000) we have a cache miss
 * and load the first page from reu
 */
uint32_t loadword_reu(uint32_t addr) {
    /*
     * check for cache miss
     */
    if( ( addr & 0xffffff00 ) != reu_addr ) {
        /*
         * read new page from reu
         */
        reu_addr = addr & 0xffffff00;
        REU.c64_address = (uint16_t)&reu_page;
        REU.reu_address_lo = reu_addr & 0xffff;
        REU.reu_address_hi = reu_addr >> 16;
        REU.transfer_length = REU_PAGE_SIZE;
        REU.command = ( REU_CMD_EXEC | REU_CMD_DIS_DECODE | REU_CMD_REU_TO_C64 );
    }
    // printf( "%08lX, %08lX\n", addr, reu_page[ ( addr & 0xff ) >> 2 ] );
    // VIC.bordercolor = ( addr & 0xff ) >> 2;
    return( reu_page[ ( addr & 0xff ) >> 2 ] );
}

/**
 * @brief store a word to reu
 * 
 * @param addr      address to store to
 * @param value     value to store
 */
void saveword_reu( uint32_t addr, volatile uint32_t value ) {
    /*
     * check for cache hit
     */
    if( ( addr & 0xffffff00 ) == reu_addr )
        reu_page[ ( addr & 0xff ) >> 2 ] = value;
    /*
     * always write to reu
     */
    REU.c64_address = (uint16_t)(&value);
    REU.reu_address_lo = addr & 0xffff;
    REU.reu_address_hi = addr >> 16;
    REU.transfer_length = 4;
    REU.command = ( REU_CMD_EXEC | REU_CMD_DIS_DECODE | REU_CMD_C64_TO_REU );
}

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <c64.h>

#include "reu.h"

volatile uint8_t* REU_STATUS=(void*)0xdf00;
volatile uint8_t* REU_CMD=(void*)0xdf01;
volatile uint16_t* REU_C64ADR=(void*)0xdf02;
volatile uint16_t* REU_ADDRLO=(void*)0xdf04;
volatile uint8_t* REU_ADDRHI=(void*)0xdf06;
volatile uint16_t* REU_XFERLEN=(void*)0xdf07;

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
        *REU_C64ADR = (uint16_t)&reu_page;
        *REU_ADDRLO = reu_addr & 0xffff;
        *REU_ADDRHI = reu_addr >> 16;
        *REU_XFERLEN = REU_PAGE_SIZE;
        *REU_CMD = 0b10010001;
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
    *REU_C64ADR = (uint16_t)(&value);
    *REU_ADDRLO = addr & 0xffff;
    *REU_ADDRHI = addr >> 16;
    *REU_XFERLEN = 4;
    *REU_CMD = 0b10010000;
}

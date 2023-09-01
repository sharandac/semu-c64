/**
 * @file display.c
 * @author Dirk Broßwick (dirk.brosswick@googlemail.com)
 * @brief 
 * @version 0.1
 * @date 2023-09-01
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <c64.h>
#include <6502.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdarg.h>

#include "display.h"
#include "keyboard.h"
#include "font.h"

volatile uint8_t* BITMAP = (void*)0xe000;
volatile uint8_t* COLORMAP = (void*)0xc400;
volatile uint8_t XPOS = 0;
volatile uint8_t YPOS = 0;
volatile uint8_t cursor = 0;

static void display_scroll();
static void display_char( uint8_t x, uint8_t y, char c );
/**
 * @brief init C64 display and setup hires mode for 80x25 characters
 * display memory is located at $E000 and colormap at $C400
 * 
 * @note: memory above $C7ff is used for llvm software stack
 */
void display_init() {
    /*
     * switch VIC-II to bank 3 $C000-$FFFF
     */
    CIA2.ddra = 0x03;
    CIA2.pra = 0x00;
    /*
     * switch to hires mode
     */
    VIC.ctrl1 |= 0x20;
    /*
     * set bitmap location to $E000 and color ram to $C400
     */
    VIC.addr &= 0x00;
    VIC.addr |= 0x18;
    /*
     * disable interrupts and knock out kernal and basic rom
     */
    SEI();
    volatile uint8_t* CPU_PORT = (void*)0x0001;
    *CPU_PORT = 0x35;
    /*
     * clear display and set color
     */
    display_clear();
    /*
     * print some info
     */
    display_printf("basic and kernal ROM are disabled, useable RAM from 0x0801-0x%04X\n", COLORMAP - 1 );
    display_printf("display memory is located at 0x%04X and colormap at 0x%04X\n\n", BITMAP, COLORMAP );
/*
    while( 1 ) {
        display_update_cursor();
        keyboard_scan();
    }
*/
}

/**
 * @brief update cursor for blinking    
 */
void display_update_cursor( void ) {
    if( cursor ) {
        display_char( XPOS, YPOS, 0xdb );
        cursor = 0;
    }
    else {
        display_char( XPOS, YPOS, ' ' );
        cursor = 1;
    }
}

/**
 * @brief scroll display one line up
 */
static void display_scroll() {
    memcpy( (void*)BITMAP, (void*)( BITMAP + 320 ), 8000 - 320 );
    memset( (void*)( BITMAP + 8000 - 320 ), 0x00, 320 );
    memcpy( (void*)COLORMAP, (void*)( COLORMAP + 40 ), 1000 - 40 );
    memset( (void*)COLORMAP + 1000 - 40, COLOR_BLACK | ( COLOR_WHITE << 4 ), 40 );
}

/**
 * @brief clear display and set color
 * 
 */
void display_clear() {
    memset( (void*)COLORMAP, COLOR_BLACK | ( COLOR_WHITE << 4 ), 1000 );
    memset( (void*)BITMAP, 0x00, 8000 );
    VIC.bordercolor = COLOR_BLACK;
    XPOS = 0;
    YPOS = 0;
}

/**
 * @brief printf to display
 * 
 * @param format        printf format string
 * @param ...           printf arguments
 * @return int          number of characters printed
 */
int display_printf(const char *format, ...) {
    char buffer[512];

    va_list args;
    va_start(args, format);
    int size = vsnprintf( buffer, sizeof( buffer ), format, args );
    va_end(args);

    if( size > 0 ) {
        display_puts( buffer );
    }
    
    return( size );
}

/**
 * @brief print string to display
 * 
 * @param s             string to print
 */
void display_puts( char *s ) {
    while( *s ) {
        display_putchar( *s++ );
    }
}

/**
 * @brief print character to display
 * 
 * @param c             character to print
 */
void display_putchar( char c ) {
    display_char( XPOS, YPOS, ' ' );
    switch( c ) {
        case '\n':  {
                        XPOS = 0;
                        YPOS++;
                        if( YPOS == DISPLAY_Y_CHAR ) {
                            display_scroll();
                            YPOS = 24;
                        }
                        break;
                    }
        case '\r':  {
                        break;
                    }
        case 0x05:  {
                        display_puts( "really?" );
                        break;
                    }
        case 0x0c:  {
                        display_clear();
                        break;
                    }
        case 0x18:  {
                        if( XPOS > 0 ) {
                            display_char( XPOS, YPOS, ' ' );
                            XPOS--;
                        }
                        break;
                    }
        default:    {
                        display_char( XPOS, YPOS, c );
                        XPOS++;
                        if( XPOS == DISPLAY_X_CHAR ) {
                            XPOS = 0;
                            YPOS++;
                            if( YPOS == DISPLAY_Y_CHAR ) {
                                display_scroll();
                                YPOS = 24;
                            }
                        }
                        break;
                    }
        }
    display_char( XPOS, YPOS, 0xdb );
}

/**
 * @brief print character to display
 * 
 * @param x             x position
 * @param y             y position
 * @param c             character to print
 */
static void display_char( uint8_t x, uint8_t y, char c ) {
    uint8_t *p = (void*)BITMAP + ( y * 8 * 40 ) + ( ( x >> 1 ) * 8 );
    uint8_t *f = (void*)font + ( c * 8 );
    for( uint8_t i = 0; i < 8; i++ ) {
        if( x & 0x01 ) {
            *p &= 0xf0;
            *p++ |= *f++ & 0x0f;    
        }
        else {
            *p &= 0x0f;
            *p++ |= *f++ & 0xf0;    
        }
    }
}

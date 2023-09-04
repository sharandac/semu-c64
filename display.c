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
#include <cbm.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdarg.h>

#include "display.h"
#include "keyboard.h"
#include "font.h"

volatile uint8_t* COLORMAP = (void*)0xd800;
volatile uint8_t* BITMAP = (void*)0xe000;
volatile uint8_t x_pos = 0;
volatile uint8_t y_pos = 0;
volatile uint8_t cursor = 0;

static void display_scroll();
static void display_char( uint8_t x, uint8_t y, char c );

void display_init() {
    /*
     * switch VIC-II to bank 3 $C000-$FFFF
     */
    CIA2.ddra |= 0x03;
    CIA2.pra &= 0xfc;
    /*
     * switch to hires mode
     */
    VIC.ctrl1 |= 0x20;
    /*
     * set bitmap location to $E000 and color ram to $C000
     */
    VIC.addr &= 0x00;
    VIC.addr |= 0x68;
    VIC.bordercolor = COLOR_BLACK;
    /*
     * a hack to move colormap to $D800
     */
    SEI();
    *(uint8_t*)0x0001 = 0x31;
    memset( (void*)COLORMAP, COLOR_BLACK | ( COLOR_WHITE << 4 ), 1000 );
    SEI();
    *(uint8_t*)0x0001 = 0x35;
    /*
     * clear display and set color
     */
    display_clear();

    cbm_k_open();

    return;
}

void display_update_cursor( void ) {
    if( cursor ) {
        display_char( x_pos, y_pos, 0xdb );
        cursor = 0;
    }
    else {
        display_char( x_pos, y_pos, ' ' );
        cursor = 1;
    }
}

void display_set_cursor( uint8_t x, uint8_t y ) {
    display_char( x_pos, y_pos, ' ' );
    cursor = 1;
    x_pos = x;
    y_pos = y;
}

void display_get_cursor( uint8_t *x, uint8_t *y ) {
    *x = x_pos;
    *y = y_pos;
}

static void display_scroll() {
    memcpy( (void*)BITMAP, (void*)( BITMAP + 320 ), 8000 - 320 );
    memset( (void*)( BITMAP + 8000 - 320 ), 0x00, 320 );
}

void display_clear() {
    memset( (void*)BITMAP, 0x00, 8000 );
    x_pos = 0;
    y_pos = 0;
}

uint8_t *display_get_bitmap() {
    return( (void*)BITMAP );
}

uint8_t *display_get_colormap() {
    return( (void*)COLORMAP );
}

int display_printf(const char *format, ...) {
    char buffer[ 81 ];

    va_list args;
    va_start(args, format);
    int size = vsnprintf( buffer, sizeof( buffer ), format, args );
    va_end(args);

    if( size > 0 ) {
        display_puts( buffer );
    }
    
    return( size );
}

void display_puts( char *s ) {
    while( *s ) {
        display_putchar( *s++ );
    }
}

void display_putchar( char c ) {
    volatile static uint8_t esc = 0;

    display_char( x_pos, y_pos, ' ' );
    switch( c ) {
        case '\n':  {
                        x_pos = 0;
                        y_pos++;
                        if( y_pos == DISPLAY_Y_CHAR ) {
                            display_scroll();
                            y_pos = DISPLAY_Y_CHAR - 1;
                        }
                        break;
                    }
        case '\r':  {
                        break;
                    }
        case 0x0c:  {
                        display_clear();
                        break;
                    }
        case 0x18:  {
                        if( x_pos > 0 ) {
                            display_char( x_pos, y_pos, ' ' );
                            x_pos--;
                        }
                        break;
                    }
        case 0x1b:  {
                        esc = 1;
                        break;
                    }
        case '[':   {
                        if( esc == 1) {
                            esc = 2;
                            break;
                        }
                        else {
                            esc = 0;
                        }
                    }   
        default:    {
                        if( esc  && c == 'm' ) {
                            esc = 0;
                            break;
                        }
                        display_char( x_pos, y_pos, c );
                        x_pos++;
                        if( x_pos == DISPLAY_X_CHAR ) {
                            x_pos = 0;
                            y_pos++;
                            if( y_pos == DISPLAY_Y_CHAR ) {
                                display_scroll();
                                y_pos = DISPLAY_Y_CHAR - 1;
                            }
                        }
                        break;
                    }
        }
    display_char( x_pos, y_pos, 0xdb );
}

static void display_char( uint8_t x, uint8_t y, char c ) {
    if( x >= DISPLAY_X_CHAR || y >= DISPLAY_Y_CHAR ) {
        return;
    }
    
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

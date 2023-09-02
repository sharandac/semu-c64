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

volatile uint8_t* CHARMAP = (void*)0xc000;
volatile uint8_t* COLORMAP = (void*)0xc800;
volatile uint8_t* BITMAP = (void*)0xe000;
volatile uint8_t x_pos = 0;
volatile uint8_t y_pos = 0;
volatile uint8_t x_pos_start = 0;
volatile uint8_t y_pos_start = 0;
volatile uint8_t x_pos_size = DISPLAY_X_CHAR;
volatile uint8_t y_pos_size = DISPLAY_Y_CHAR;
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
    VIC.addr |= 0x28;
    /*
     * disable interrupts and knock out kernal and basic rom
     */
    SEI();
    volatile uint8_t* CPU_PORT = (void*)0x0001;
    *CPU_PORT = 0x35;
    /*
     * clear display and set color
     */
    memset( (void*)COLORMAP, COLOR_BLACK | ( COLOR_WHITE << 4 ), 1000 );
    memset( (void*)BITMAP, 0x00, 8000 );
    memset( (void*)CHARMAP, 0x00, 2000 );
    display_clear();
    /*
     * only for debuging
     */
    return;
    while( 1 ) {
        uint8_t key = keyboard_scan( 0 );
        static struct region *region = NULL;
        if( key != 0 ) {
            display_putchar( key );
        }

        if( keyboard_c_check() ) {
            if( region != NULL ) {
                display_restore_region( region );
                region = NULL;
            }
            else {
                region = display_save_region( 5, 5, 30, 10 );
            }
        }
        display_update_cursor();
    }
}

void display_draw_frame( uint8_t x, uint8_t y, uint8_t x_size, uint8_t y_size ) {
    for( uint8_t i = 1; i < x_size - 1; i++ ) {
        display_char( x + i, y, 0xc4 );
        display_char( x + i, y + y_size - 1, 0xc4 );
    }
    for( uint8_t i = 1; i < y_size - 1; i++ ) {
        display_char( x, y + i, 0xb3 );
        display_char( x + x_size - 1, y + i, 0xb3 );
    }
    display_char( x, y, 0xda );
    display_char( x + x_size - 1, y, 0xbf );
    display_char( x, y + y_size - 1, 0xc0 );
    display_char( x + x_size - 1, y + y_size - 1, 0xd9 );
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

struct region *display_save_region( uint8_t x, uint8_t y, uint8_t x_size, uint8_t y_size ) {
    struct region *region = malloc( sizeof( struct region ) );
    region->x = x;
    region->y = y;
    region->x_size = x_size;
    region->y_size = y_size;
    region->x_pos = x_pos;
    region->y_pos = y_pos;
    region->x_pos_start = x_pos_start;
    region->y_pos_start = y_pos_start;
    region->x_pos_size = x_pos_size;
    region->y_pos_size = y_pos_size;
    region->charmap = malloc( x_size * y_size );
    for( size_t i = 0; i < x_size; i++ ) {
        for( size_t a = 0; a < y_size; a++) {
            *(region->charmap + a * x_size + i) = *(CHARMAP + ( y + a ) * 80 + ( x + i ) );
        }
    }
    display_clear_area( x, y, x_size, y_size );
    display_draw_frame( x, y, x_size, y_size );
    x_pos_start = x + 1;
    y_pos_start = y + 1;
    x_pos_size = x_size - 2;
    y_pos_size = y_size - 2;
    x_pos = x + 1;
    y_pos = y + 1;
    display_set_cursor( x_pos, y_pos );

    return( region );
}

void display_restore_region( struct region *region ) {
    for( size_t i = 0; i < region->x_size; i++ ) {
        for( size_t a = 0; a < region->y_size; a++) {
            *(CHARMAP + ( region->y + a ) * 80 + ( region->x + i ) ) = *(region->charmap + a * region->x_size + i);
        }
    }
    display_redraw_area( x_pos_start - 1, y_pos_start - 1, x_pos_size + 2, y_pos_size + 2 );
    x_pos = region->x_pos;
    y_pos = region->y_pos;
    x_pos_start = region->x_pos_start;
    y_pos_start = region->y_pos_start;
    x_pos_size = region->x_pos_size;
    y_pos_size = region->y_pos_size;
    display_set_cursor( x_pos, y_pos );

    free( region->charmap );
    free( region );
}

static void display_scroll() {
    if( x_pos_start == 0 && y_pos_start == 0 && x_pos_size == DISPLAY_X_CHAR && y_pos_size == DISPLAY_Y_CHAR ) {
        memcpy( (void*)BITMAP, (void*)( BITMAP + 320 ), 8000 - 320 );
        memset( (void*)( BITMAP + 8000 - 320 ), 0x00, 320 );
        memcpy( (void*)COLORMAP, (void*)( COLORMAP + 40 ), 1000 - 40 );
        memset( (void*)COLORMAP + 1000 - 40, COLOR_BLACK | ( COLOR_WHITE << 4 ), 40 );
        memcpy( (void*)CHARMAP, (void*)( CHARMAP + DISPLAY_X_CHAR ), 1000 - DISPLAY_X_CHAR );
        memset( (void*)CHARMAP + 2000 - DISPLAY_X_CHAR, COLOR_BLACK | ( COLOR_WHITE << 4 ), DISPLAY_X_CHAR );
        return;
    }

    for( size_t y = y_pos_start; y < y_pos_start + y_pos_size - 1; y++ ) {
        for( size_t x = x_pos_start; x < x_pos_start + x_pos_size; x++ ) {
            *(CHARMAP + y * 80 + x) = *(CHARMAP + ( y + 1 ) * 80 + x );
        }
    }
    for( size_t x = x_pos_start; x < x_pos_start + x_pos_size; x++ ) {
        *(CHARMAP + ( y_pos_start + y_pos_size - 1 ) * 80 + x ) = ' ';
    }

    display_redraw_area( x_pos_start, y_pos_start, x_pos_size, y_pos_size );
}

void display_clear() {
    for( size_t y = y_pos_start; y < y_pos_start + y_pos_size - 1; y++ ) {
        for( size_t x = x_pos_start; x < x_pos_start + x_pos_size; x++ ) {
            *(CHARMAP + y * 80 + x) = ' ';
        }
    }
    memset( (void*)COLORMAP, COLOR_BLACK | ( COLOR_WHITE << 4 ), 1000 );
    display_redraw_area( x_pos_start, y_pos_start, x_pos_size, y_pos_size );
    VIC.bordercolor = COLOR_BLACK;
    x_pos = x_pos_start;
    y_pos = y_pos_start;
}

void display_clear_area( uint8_t x, uint8_t y, uint8_t x_size, uint8_t y_size ) {
    for( uint8_t i = 0; i < y_size; i++ ) {
        for( uint8_t a = 0; a < x_size; a++ ) {
            display_char( x + a, y + i, ' ' );
        }
    }
}

uint8_t *display_get_bitmap() {
    return( (void*)BITMAP );
}

uint8_t *display_get_colormap() {
    return( (void*)COLORMAP );
}

uint8_t *display_get_charmap() {
    return( (void*)CHARMAP );
}

void display_redraw() {
    for( uint8_t y = 0; y < DISPLAY_Y_CHAR; y++ ) {
        for( uint8_t x = 0; x < DISPLAY_X_CHAR; x++ ) {
            display_char( x, y, *(CHARMAP + y * 80 + x) );
        }
    }
}

void display_redraw_area( uint8_t x, uint8_t y, uint8_t x_size, uint8_t y_size) {
    for( uint8_t a = 0; a < y_size; a++ ) {
        for( uint8_t i = 0; i < x_size; i++ ) {
            display_char( x + i, y + a, *(CHARMAP + ( y + a ) * 80 + ( x + i ) ) );
        }
    }
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
    display_char( x_pos, y_pos, ' ' );
    switch( c ) {
        case '\n':  {
                        x_pos = x_pos_start;
                        y_pos++;
                        if( y_pos == y_pos_size + y_pos_start) {
                            display_scroll();
                            y_pos = y_pos_start + y_pos_size - 1;
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
                        if( x_pos > x_pos_start ) {
                            display_char( x_pos, y_pos, ' ' );
                            x_pos--;
                        }
                        break;
                    }
        default:    {
                        display_char( x_pos, y_pos, c );
                        x_pos++;
                        if( x_pos == x_pos_size + x_pos_start ) {
                            x_pos = x_pos_start;
                            y_pos++;
                            if( y_pos == y_pos_size + y_pos_start ) {
                                display_scroll();
                                y_pos = y_pos_start + y_pos_size - 1;
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

    *(CHARMAP + y * 80 + x) = c;
}

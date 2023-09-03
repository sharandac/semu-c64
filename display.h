/**
 * @file display.h
 * @author Dirk Broßwick (dirk.brosswick@googlemail.com)
 * @brief 
 * @version 0.1
 * @date 2023-09-01
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#pragma once

#define DISPLAY_X_CHAR      80
#define DISPLAY_Y_CHAR      25
/**
 * @brief init C64 display and setup hires mode for 80x25 characters
 * display memory is located at $E000 and colormap at $C400
 * 
 * @note: memory above $C7ff is used for llvm software stack
 */
void display_init();
/**
 * @brief clear display and set color
 * 
 */
void display_clear();
/**
 * @brief update cursor for blinking    
 */
void display_update_cursor( void );
/**
 * @brief set cursor position
 * 
 * @param x         new x cursor position
 * @param y         new y cursor position
 */
void display_set_cursor( uint8_t x, uint8_t y );
/**
 * @brief get cursor position
 * 
 * @param x         pointer to x cursor position
 * @param y         pointer to y cursor position
 */
void display_get_cursor( uint8_t *x, uint8_t *y );
/**
 * @brief print character to display
 * 
 * @param c             character to print
 */
void display_putchar( char c );
/**
 * @brief print string to display
 * 
 * @param s             string to print
 */
void display_puts( char *s );
/**
 * @brief get display bitmap address
 * 
 * @return uint8_t* 
 */
uint8_t *display_get_bitmap();
/**
 * @brief get display colormap address
 * 
 * @return uint8_t* 
 */
uint8_t *display_get_colormap();
/**
 * @brief printf to display
 * 
 * @param format        printf format string
 * @param ...           printf arguments
 * @return int          number of characters printed
 */
int display_printf( const char *format, ...);

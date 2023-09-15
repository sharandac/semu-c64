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
 * @brief 
 */
struct region {
    uint8_t       x;            /** x position */     
    uint8_t       y;            /** y position */
    uint8_t       x_size;       /** x size */
    uint8_t       y_size;       /** y size */
    uint8_t       x_pos;        /** x cursor position */
    uint8_t       y_pos;        /** y cursor position */
    uint8_t       x_pos_start;  /** x cursor start position */
    uint8_t       y_pos_start;  /** y cursor start position */
    uint8_t       x_pos_size;   /** x cursor size */
    uint8_t       y_pos_size;   /** y cursor size */
    uint8_t       cursor_active;/** cursor */
    uint8_t*      charmap;      /** charmap */
};
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
 * @brief set cursor
 * 
 * @param active 
 */
void display_set_cursor_active( uint8_t active );
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
 * @brief get display charmap address
 * 
 * @return uint8_t* 
 */
uint8_t *display_get_charmap();
/**
 * @brief redraw display from charmap
 */
void display_redraw();
/**
 * @brief printf to display
 * 
 * @param format        printf format string
 * @param ...           printf arguments
 * @return int          number of characters printed
 */
int display_printf( const char *format, ...);
/**
 * @brief save region of display
 * 
 * @param x             x position
 * @param y             y position
 * @param x_size        x size
 * @param y_size        y size
 * @return struct region*
 */
struct region *display_save_region( uint8_t x, uint8_t y, uint8_t x_size, uint8_t y_size );
/**
 * @brief restore region of display
 * 
 * @param region        region to restore
 */
void display_restore_region( struct region *region );
/**
 * @brief draw frame
 * 
 * @param x             x position
 * @param y             y position
 * @param x_size        x size
 * @param y_size        y size
 */
void display_draw_frame( uint8_t x, uint8_t y, uint8_t x_size, uint8_t y_size );
/**
 * @brief redraw a area of the display
 * 
 * @param x             x position
 * @param y             y position
 * @param x_size        x size
 * @param y_size        y size
 */
void display_redraw_area( uint8_t x, uint8_t y, uint8_t x_size, uint8_t y_size);
/**
 * @brief clear a area of the display
 * 
 * @param x             x position
 * @param y             y position
 * @param x_size        x size
 * @param y_size        y size
 */
void display_clear_area( uint8_t x, uint8_t y, uint8_t x_size, uint8_t y_size );
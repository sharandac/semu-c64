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

void display_init();
void display_clear();
void display_update_cursor( void );
void display_putchar( char c );
void display_puts( char *s );
int display_printf( const char *format, ...);
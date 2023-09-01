/**
 * @file keyboard.c
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

#include "keyboard.h"
#include "display.h"

volatile uint8_t lastkey = 0;
volatile uint8_t key = 0;   

uint8_t unshifted_keys[] = {
    0x18,'\n',0x1D,0x88,0x85,0x86,0x87,0x11,
    '3' ,'w' ,'a' ,'4' ,'z' ,'s' ,'e' ,0x01,
    '5' ,'r' ,'d' ,'6' ,'c' ,'f' ,'t' ,'x' ,
    '7' ,'y' ,'g' ,'8' ,'b' ,'h' ,'u' ,'v' ,
    '9' ,'i' ,'j' ,'0' ,'m' ,'k' ,'o' ,'n' ,
    '+' ,'p' ,'l' ,'-' ,'.' ,':' ,'@' ,',' ,
    '_','*' ,';' ,0x13,0x01,'=' ,0x5E,'/' ,
    '1' ,0x5F,0x04,'2' ,' ' ,0x02,'q' ,0x03
};

uint8_t shifted_keys[] = {
    0x18,'\n',0x9D,0x8C,0x89,0x8A,0x8B,0x91,
    '#' ,'W' ,'A' ,'$' ,'Z' ,'S' ,'E' ,0x01,
    '%' ,'R' ,'D' ,'&' ,'C' ,'F' ,'T' ,'X' ,
    '/' ,'Y' ,'G' ,'(' ,'B' ,'H' ,'U' ,'V' ,
    ')' ,'I' ,'J' ,'0' ,'M' ,'K' ,'O' ,'N' ,
    '+' ,'P' ,'L' ,'-' ,'>' ,'[' ,'@' ,'<' ,
    '-' ,'*' ,']' ,0x93,0x01,'=' ,0xDE,'?' ,
    '!' ,0x5F,0x04,'"' ,' ' ,0x02,'Q' ,0x83
};

uint8_t ctrl_keys[] = {
    0x0c,'\n',0x9D,0x8C,0x89,0x8A,0x8B,0x91,
    '#' ,'W' ,'A' ,'$' ,'Z' ,'S' ,'E' ,0x01,
    '%' ,'R' ,'D' ,'&' ,0x05,'F' ,'T' ,'X' ,
    '/' ,'Y' ,'G' ,'(' ,'B' ,'H' ,'U' ,'V' ,
    ')' ,'I' ,'J' ,'0' ,'M' ,'K' ,'O' ,'N' ,
    '+' ,'P' ,'L' ,'-' ,'>' ,'[' ,'@' ,'<' ,
    '-' ,'*' ,']' ,0x93,0x01,'=' ,0xDE,'?' ,
    '!' ,0x5F,0x04,'"' ,' ' ,0x02,'Q' ,0x83
};

int16_t keyboard_scan( void ) {
    uint8_t mask = 0;
    uint8_t shift = 0;
    uint8_t ctrl = 0;
    uint8_t row = 0xff;
    uint8_t col = 0xff;
    /*
     * check for left shift
     */
    mask = 0x02;
    CIA1.pra = ~mask;
    if( !( CIA1.prb & 0x80 ) )
        shift = 1;
    /*
     * check for right shift
     */
    mask = 0x40;
    CIA1.pra = ~mask;
    if( !( CIA1.prb & 0x10 ) )
        shift = 1;
    /*
     * check for right shift
     */
    mask = 0x80;
    CIA1.pra = ~mask;
    if( !( CIA1.prb & 0x04 ) )
        ctrl = 1;
    /*
     * crawl all rows
     */
    for( size_t keyboard_row = 0; keyboard_row < 8; keyboard_row++ ) {
        /*
         * select row
         */
        CIA1.pra = ~( 1 << keyboard_row );
        /*
         * crawl all columns
         */
        for( size_t keyboard_col = 0; keyboard_col < 8 ; keyboard_col++ ) {
            /*
             * read column
             */
            uint8_t temp_col = CIA1.prb;
            /*
             * mask out shift keys
             */
            if( keyboard_row == 1 )
                temp_col |= 0x80;
            if( keyboard_row == 6 )
                temp_col |= 0x10;
            if( keyboard_row == 7 )
                temp_col |= 0x04;
            /*
             * check for keypress
             */
            if( !( temp_col & ( 1 << keyboard_col ) ) ) {
                row = keyboard_row;
                col = keyboard_col;
                break;
            }
        }
    }
    /*
     * check for keypress
     */
    if( row != 0xff && col != 0xff ) {
        uint8_t* matrix = NULL;
        /*
         * select matrix
         */
        if( shift )
            matrix = shifted_keys;
        else if( ctrl )
            matrix = ctrl_keys;
        else
            matrix = unshifted_keys;
        /*
         * get key
         */
        key = matrix[ row * 8 + col ];
        /*
         * key bounce
         */        
        if( key != lastkey )
            lastkey = key;
        else
            key = 0;
    }
    else {
        lastkey = 0;
        key = 0;
    }
    /*
     *
     */
    if( key == 0x05 )
        display_putchar( key );
    /*
     * return key
     */
    return( key );
}


/**
 * @file debug.c
 * @author Dirk Broßwick (dirk.brosswick@googlemail.com)
 * @brief 
 * @version 0.1
 * @date 2023-09-05
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

#include "debug.h"
#include "keyboard.h"
#include "display.h"
#include "reu.h"

uint8_t  debug_menu( vm_t *vm ) {
    static struct region *region = NULL;
    /*
     * check if debug window is active
     */
    if( region ) {
        /*
         * update screen
         */
        display_set_cursor( 0, 0 );
        display_printf("  INSN: %08lX:%08lX\n", vm->insn_count_hi, vm->insn_count );
        display_printf("    PC: 0x%08lX  SIE: %08lX\n\n", vm->current_pc, vm->sie );
        for( size_t i = 0 ; i < 8; i++ )
            display_printf("  %08lX %08lX %08lX %08lX\n", loadword_reu( vm->x_regs[ i * 4 ] ), loadword_reu( vm->x_regs[ i * 4 + 1 ] ), loadword_reu( vm->x_regs[ i * 4 + 2 ] ), loadword_reu( vm->x_regs[ i * 4 + 3 ] ) );
        display_printf("\n  s = single step, C= to continue");
    }
    else {
        /*
         * create debug window when C= is pressed
         */
        if( keyboard_c_check() ) {
            region = display_save_region( 20, 5, 41, 15 );
            display_set_cursor_active( 0 );
            return( 0 );
        }
    }
    /*
     * if window is active, wait for keypress
     */
    while( region ) {
        if( keyboard_scan( 0 ) == 's' ) {
            return( 0 );
        }
        if( keyboard_c_check() ) {
            display_restore_region( region );
            region = NULL;
        }
    }

    return( 255 );
}
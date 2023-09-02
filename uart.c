#include <errno.h>
#include <cbm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "display.h"
#include "keyboard.h"
#include "device.h"
#include "riscv.h"
#include "riscv_private.h"

extern vm_t vm;
char *login_stop_test="buildroot login:";
#define U8250_INT_THRE 1

/* Emulate 8250 (plain, without loopback mode support) */

void u8250_update_interrupts(u8250_state_t *uart)
{
    /* Some interrupts are level-generated. */
    /* TODO: does it also generate an LSR change interrupt? */
    if (uart->in_ready)
        uart->pending_ints |= 1;
    else
        uart->pending_ints &= ~1;

    /* Prevent generating any disabled interrupts in the first place */
    uart->pending_ints &= uart->ier;

    /* Update current interrupt (higher bits -> more priority) */
    if (uart->pending_ints)
        uart->current_int = ilog2(uart->pending_ints);
}

void u8250_check_ready(u8250_state_t *uart)
{
    if (uart->in_ready)
        return;

    uart->in_char = keyboard_scan( 0 );

    if (uart->in_char != 0)
        uart->in_ready = true;
}

static void u8250_handle_out(u8250_state_t *uart, uint8_t value)
{
    (void)(uart);
    display_putchar(value);
}

static uint8_t u8250_handle_in(u8250_state_t *uart)
{
    uint8_t value = 0;
    u8250_check_ready(uart);
    if ( uart->in_ready ) {
        uart->in_ready=false;
        value = uart->in_char;
    }
    return value;
}

static void u8250_reg_read(u8250_state_t *uart, uint32_t addr, uint8_t *value)
{
    switch (addr) {
        case 0:
            if (uart->lcr & (1 << 7)) { /* DLAB */
                *value = uart->dll;
                break;
            }
            *value = u8250_handle_in(uart);
            break;
        case 1:
            if (uart->lcr & (1 << 7)) { /* DLAB */
                *value = uart->dlh;
                break;
            }
            *value = uart->ier;
            break;
        case 2:
            *value = (uart->current_int << 1) | (uart->pending_ints ? 0 : 1);
            if (uart->current_int == U8250_INT_THRE)
                uart->pending_ints &= ~(1 << uart->current_int);
            break;
        case 3:
            *value = uart->lcr;
            break;
        case 4:
            *value = uart->mcr;
            break;
        case 5:
            /* LSR = no error, TX done & ready */
            *value = 0x60 | (uint8_t) uart->in_ready;
            break;
        case 6:
            /* MSR = carrier detect, no ring, data ready, clear to send. */
            *value = 0xb0;
            break;
            /* no scratch register, so we should be detected as a plain 8250. */
        default:
            *value = 0;
    }
}

static void u8250_reg_write(u8250_state_t *uart, uint32_t addr, uint8_t value)
{
    switch (addr) {
        case 0:
            if (uart->lcr & (1 << 7)) { /* DLAB */
                uart->dll = value;
                break;
            }
            u8250_handle_out(uart, value);
            uart->pending_ints |= 1 << U8250_INT_THRE;
            break;
        case 1:
            if (uart->lcr & (1 << 7)) { /* DLAB */
                uart->dlh = value;
                break;
            }
            uart->ier = value;
            break;
        case 3:
            uart->lcr = value;
            break;
        case 4:
            uart->mcr = value;
            break;
    }
}

void u8250_read(vm_t *vm,
                u8250_state_t *uart,
                uint32_t addr,
                uint8_t width,
                uint32_t *value)
{
    uint8_t u8value;
    switch (width) {
        case RV_MEM_LBU:
            u8250_reg_read(uart, addr, &u8value);
            *value = (uint32_t) u8value;
            break;
        case RV_MEM_LB:
            u8250_reg_read(uart, addr, &u8value);
            *value = (uint32_t) (int8_t) u8value;
            break;
        case RV_MEM_LW:
        case RV_MEM_LHU:
        case RV_MEM_LH:
            vm_set_exception(vm, RV_EXC_LOAD_MISALIGN, vm->exc_val);
            return;
        default:
            vm_set_exception(vm, RV_EXC_ILLEGAL_INSTR, 0);
            return;
    }
}

void u8250_write(vm_t *vm,
                 u8250_state_t *uart,
                 uint32_t addr,
                 uint8_t width,
                 uint32_t value)
{
    switch (width) {
        case RV_MEM_SB:
            u8250_reg_write(uart, addr, value);
            break;
        case RV_MEM_SW:
        case RV_MEM_SH:
            vm_set_exception(vm, RV_EXC_STORE_MISALIGN, vm->exc_val);
            return;
        default:
            vm_set_exception(vm, RV_EXC_ILLEGAL_INSTR, 0);
            return;
    }
}

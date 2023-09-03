
#include <c64.h>
#include <6502.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "device.h"
#include "riscv.h"
#include "riscv_private.h"

#include "reu.h"
#include "display.h"

/* Define fetch separately since it is simpler (fixed width, already checked
 * alignment, only main RAM is executable).
 */
static void mem_fetch(vm_t *vm, uint32_t addr, uint32_t *value)
{
    if (unlikely(addr >= RAM_SIZE)) {
        /* TODO: check for other regions */
        vm_set_exception(vm, RV_EXC_FETCH_FAULT, vm->exc_val);
        return;
    }
    *value = loadword_reu(addr& 0xfffffffc);
}

static void emu_update_uart_interrupts(vm_t *vm)
{
    emu_state_t *data = (emu_state_t *) vm->priv;
    u8250_update_interrupts(&data->uart);
    if (data->uart.pending_ints)
        data->plic.active |= IRQ_UART_BIT;
    else
        data->plic.active &= ~IRQ_UART_BIT;
    plic_update_interrupts(vm, &data->plic);
}

#if SEMU_HAS(VIRTIONET)
static void emu_update_vnet_interrupts(vm_t *vm)
{
    emu_state_t *data = (emu_state_t *) vm->priv;
    if (data->vnet.InterruptStatus)
        data->plic.active |= IRQ_VNET_BIT;
    else
        data->plic.active &= ~IRQ_VNET_BIT;
    plic_update_interrupts(vm, &data->plic);
}
#endif

#if SEMU_HAS(VIRTIOBLK)
static void emu_update_vblk_interrupts(vm_t *vm)
{
    emu_state_t *data = (emu_state_t *) vm->priv;
    if (data->vblk.InterruptStatus)
        data->plic.active |= IRQ_VBLK_BIT;
    else
        data->plic.active &= ~IRQ_VBLK_BIT;
    plic_update_interrupts(vm, &data->plic);
}
#endif

static void mem_load(vm_t *vm, uint32_t addr, uint8_t width, uint32_t *value)
{
    emu_state_t *data = (emu_state_t *) vm->priv;

    /* RAM at 0x00000000 + RAM_SIZE */
    if (addr < RAM_SIZE) {
        ram_read(vm, data->ram, addr, width, value);
        return;
    }

    if ((addr >> 28) == 0xF) { /* MMIO at 0xF_______ */
        /* 256 regions of 1MiB */
        switch ((addr >> 20) & MASK(8)) {
        case 0x0:
        case 0x2: /* PLIC (0 - 0x3F) */
            plic_read(vm, &data->plic, addr & 0x3FFFFFF, width, value);
            plic_update_interrupts(vm, &data->plic);
            return;
        case 0x40: /* UART */
            u8250_read(vm, &data->uart, addr & 0xFFFFF, width, value);
            emu_update_uart_interrupts(vm);
            return;
#if SEMU_HAS(VIRTIONET)
        case 0x41: /* virtio-net */
            virtio_net_read(vm, &data->vnet, addr & 0xFFFFF, width, value);
            emu_update_vnet_interrupts(vm);
            return;
#endif
#if SEMU_HAS(VIRTIOBLK)
        case 0x42: /* virtio-blk */
            virtio_blk_read(vm, &data->vblk, addr & 0xFFFFF, width, value);
            emu_update_vblk_interrupts(vm);
            return;
#endif
        }
    }
    vm_set_exception(vm, RV_EXC_LOAD_FAULT, vm->exc_val);
}

static void mem_store(vm_t *vm, uint32_t addr, uint8_t width, uint32_t value)
{
    emu_state_t *data = (emu_state_t *) vm->priv;

    /* RAM at 0x00000000 + RAM_SIZE */
    if (addr < RAM_SIZE) {
        ram_write(vm, data->ram, addr, width, value);
        return;
    }

    if ((addr >> 28) == 0xF) { /* MMIO at 0xF_______ */
        /* 256 regions of 1MiB */
        switch ((addr >> 20) & MASK(8)) {
        case 0x0:
        case 0x2: /* PLIC (0 - 0x3F) */
            plic_write(vm, &data->plic, addr & 0x3FFFFFF, width, value);
            plic_update_interrupts(vm, &data->plic);
            return;
        case 0x40: /* UART */
            u8250_write(vm, &data->uart, addr & 0xFFFFF, width, value);
            emu_update_uart_interrupts(vm);
            return;
#if SEMU_HAS(VIRTIONET)
        case 0x41: /* virtio-net */
            virtio_net_write(vm, &data->vnet, addr & 0xFFFFF, width, value);
            emu_update_vnet_interrupts(vm);
            return;
#endif
#if SEMU_HAS(VIRTIOBLK)
        case 0x42: /* virtio-blk */
            virtio_blk_write(vm, &data->vblk, addr & 0xFFFFF, width, value);
            emu_update_vblk_interrupts(vm);
            return;
#endif
        }
    }
    vm_set_exception(vm, RV_EXC_STORE_FAULT, vm->exc_val);
}

/* SBI */
#define SBI_IMPL_ID 0x999
#define SBI_IMPL_VERSION 1

typedef struct {
    int32_t error;
    int32_t value;
} sbi_ret_t;

static inline sbi_ret_t handle_sbi_ecall_TIMER(vm_t *vm, int32_t fid)
{
    emu_state_t *data = (emu_state_t *) vm->priv;
    switch (fid) {
    case SBI_TIMER__SET_TIMER:
        data->timer_lo = vm->x_regs[RV_R_A0];
        data->timer_hi = vm->x_regs[RV_R_A1];
        return (sbi_ret_t){SBI_SUCCESS, 0};
    default:
        return (sbi_ret_t){SBI_ERR_NOT_SUPPORTED, 0};
    }
}

static inline sbi_ret_t handle_sbi_ecall_RST(vm_t *vm, int32_t fid)
{
    emu_state_t *data = (emu_state_t *) vm->priv;
    switch (fid) {
    case SBI_RST__SYSTEM_RESET:
        data->stopped = true;
        return (sbi_ret_t){SBI_SUCCESS, 0};
    default:
        return (sbi_ret_t){SBI_ERR_NOT_SUPPORTED, 0};
    }
}

#define RV_MVENDORID 0x12345678
#define RV_MARCHID ((1UL << 31) | 1)
#define RV_MIMPID 1

static inline sbi_ret_t handle_sbi_ecall_BASE(vm_t *vm, int32_t fid)
{
    switch (fid) {
    case SBI_BASE__GET_SBI_IMPL_ID:
        return (sbi_ret_t){SBI_SUCCESS, SBI_IMPL_ID};
    case SBI_BASE__GET_SBI_IMPL_VERSION:
        return (sbi_ret_t){SBI_SUCCESS, SBI_IMPL_VERSION};
    case SBI_BASE__GET_MVENDORID:
        return (sbi_ret_t){SBI_SUCCESS, RV_MVENDORID};
    case SBI_BASE__GET_MARCHID:
        return (sbi_ret_t){SBI_SUCCESS, RV_MARCHID};
    case SBI_BASE__GET_MIMPID:
        return (sbi_ret_t){SBI_SUCCESS, RV_MIMPID};
    case SBI_BASE__GET_SBI_SPEC_VERSION:
        return (sbi_ret_t){SBI_SUCCESS, (0UL << 24) | 3}; /* version 0.3 */
    case SBI_BASE__PROBE_EXTENSION: {
        int32_t eid = (int32_t) vm->x_regs[RV_R_A0];
        bool available =
            eid == SBI_EID_BASE || eid == SBI_EID_TIMER || eid == SBI_EID_RST;
        return (sbi_ret_t){SBI_SUCCESS, available};
    }
    default:
        return (sbi_ret_t){SBI_ERR_NOT_SUPPORTED, 0};
    }
}

#define SBI_HANDLE(TYPE) ret = handle_sbi_ecall_##TYPE(vm, vm->x_regs[RV_R_A6])

static void handle_sbi_ecall(vm_t *vm)
{
    sbi_ret_t ret;
    switch (vm->x_regs[RV_R_A7]) {
    case SBI_EID_BASE:
        SBI_HANDLE(BASE);
        break;
    case SBI_EID_TIMER:
        SBI_HANDLE(TIMER);
        break;
    case SBI_EID_RST:
        SBI_HANDLE(RST);
        break;
    default:
        ret = (sbi_ret_t){SBI_ERR_NOT_SUPPORTED, 0};
    }
    vm->x_regs[RV_R_A0] = (uint32_t) ret.error;
    vm->x_regs[RV_R_A1] = (uint32_t) ret.value;

    /* Clear error to allow execution to continue */
    vm->error = ERR_NONE;
}

emu_state_t emu;
vm_t vm = {
        .priv = &emu,
        .mem_fetch = mem_fetch,
        .mem_load = mem_load,
        .mem_store = mem_store
};

__attribute__((nonreentrant))
static int semu_start(int argc, char **argv)
{
    (void) argc;
    (void) argv;
    uint8_t peripheral_update_ctr = 0;      /** @brief peripheral update counter */
    uint32_t dtb_addr = RAM_SIZE - INITRD_SIZE - DTB_SIZE;
    /*
     * Initialize the emulator
     */
    memset(&emu, 0, sizeof(emu));
    /*
     * Set up RISC-V hart
     */
    emu.timer_hi = emu.timer_lo = 0xFFFFFFFF;
    vm.page_table_addr = 0;
    vm.s_mode = true;
    vm.x_regs[RV_R_A0] = 0; /* hart ID. i.e., cpuid */
    vm.x_regs[RV_R_A1] = dtb_addr;
    /* Set up peripherals */
    emu.uart.in_fd = 0, emu.uart.out_fd = 1;
    /*
     * disable interrupts and knock out kernal, basic
     *
     * memory map:  0x0801-0xCFFF    free to use RAM
     *              0xD000-0xD3FF    I/O
     *              0xD800-0xDCFF    color ram
     *              0xDD00-0xDFFF    I/O
     *              0xE000-0xFFFF    bitmap
     */
    SEI();
    *(uint8_t*)0x0001 = 0x35;
    /*
     * init display
     */
    display_init();
    /*
     * print some info
     */
    display_printf("basic and kernal ROM are disabled, useable RAM from 0x0801-0xCFFF\n" );
    display_printf("bitmap: 0x%04X, colormap: 0x%04X\n\n", display_get_bitmap(), display_get_colormap() );
    display_printf("C-64 semu risc-v emulator\n");
    display_printf("Git commit: $Id: 7fd94cf6e0e62f69375dd3ee60ebf7bd275884d0 $\n");
    display_printf("emu state begin: 0x%p, size: 0x%04x\n", &emu, sizeof(emu));
    display_printf("vm state begin: 0x%p, size: 0x%04x\n\n", &vm, sizeof(vm));
    /*
     * run the emulator
     */
    while (!emu.stopped) {
        if (peripheral_update_ctr-- == 0) {
            display_update_cursor();
            // debug_menu( &vm );
            u8250_check_ready(&emu.uart);
            if (emu.uart.in_ready)
                emu_update_uart_interrupts(&vm);

#if SEMU_HAS(VIRTIONET)
            virtio_net_refresh_queue(&emu.vnet);
            if (emu.vnet.InterruptStatus)
                emu_update_vnet_interrupts(&vm);
#endif

#if SEMU_HAS(VIRTIOBLK)
            if (emu.vblk.InterruptStatus)
                emu_update_vblk_interrupts(&vm);
#endif
            if (vm.insn_count_hi > emu.timer_hi ||
                (vm.insn_count_hi == emu.timer_hi && vm.insn_count > emu.timer_lo))
                vm.sip |= RV_INT_STI_BIT;
            else
                vm.sip &= ~RV_INT_STI_BIT;

            /* Stop after fixed amount of instructions for performance testing or
               to cross-check instruction traces etc. */
            //if (vm.insn_count > 200000000) exit(0);
        }

        vm_step(&vm);
        if (likely(!vm.error))
            continue;

        if (vm.error == ERR_EXCEPTION && vm.exc_cause == RV_EXC_ECALL_S) {
            handle_sbi_ecall(&vm);
            continue;
        }

        if (vm.error == ERR_EXCEPTION) {
            vm_trap(&vm);
            continue;
        }
        return 2;
    }

    /* unreachable */
    return 0;
}

__attribute__((nonreentrant))
int main(int argc, char **argv)
{
    return semu_start(argc, argv);
}

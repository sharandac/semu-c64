#pragma once
#include <stdint.h>

/**
 * @brief REU registers
 */
struct __REU {
    uint8_t       status;                                       /** status register, read only */
    uint8_t       command;                                      /** command register */
    uint16_t      c64_address;                                  /** c64 address register */
    uint16_t      reu_address_lo;                               /** reu address register low */
    uint8_t       reu_address_hi;                               /** reu address register high */
    uint16_t      transfer_length;                              /** transfer length register */
    uint8_t       int_mask;                                     /** interrupt mask register */
    uint8_t       addr_ctrl;                                    /** address control register */
};
/**
 * @brief REU defines   
 */
#define REU                 (*(volatile struct __REU*)0xDF00)   /** REU I/O base address */
#define REU_PAGE_SIZE       0x100                               /** REU cache size */
#define REU_CMD_EXEC        0x80                                /** execute command after command write */
#define REU_CMD_DIS_DECODE  0x10                                /** disable address decoding */
#define REU_CMD_C64_TO_REU  0x00                                /** transfer from c64 to reu */         
#define REU_CMD_REU_TO_C64  0x01                                /** transfer from reu to c64 */
/**
 * @brief load a word from reu
 * 
 * @param addr          address to load from
 * @return uint32_t     word from reu
 *
 * @note: on first run ( addr = 0x00000000) we have a cache miss
 * and load the first page from reu
 */
uint32_t loadword_reu(uint32_t addr);
/**
 * @brief store a word to reu
 * 
 * @param addr      address to store to
 * @param value     value to store
 */
void saveword_reu(uint32_t addr, uint32_t value);

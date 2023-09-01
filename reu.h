#pragma once
#include <stdint.h>

#define REU_PAGE_SIZE       0x100
#define REU_CACHE_SIZE      1

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

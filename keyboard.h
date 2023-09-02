/**
 * @file keyboard.h
 * @author Dirk Broßwick (dirk.brosswick@googlemail.com)
 * @brief 
 * @version 0.1
 * @date 2023-09-01
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#pragma once
    /**
     * @brief scan keyboard
     * 
     * @return 0 if no key pressed, else key
     */
    uint16_t keyboard_scan( uint8_t state_only );
    /**
     * @brief check for commodore key
     * 
     * @return 0x02 if commodore key pressed, else 0
     */
    uint16_t keyboard_c_check( void );
    

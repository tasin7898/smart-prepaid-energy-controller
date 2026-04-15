/*
 * keypad.h
 *
 *  Created on: Apr 24, 2025
 *      Author: 23142251
 */

#ifndef INC_KEYPAD_H_
#define INC_KEYPAD_H_

#include "stm32g0xx_hal.h"  // Update for your STM32 family

#define ROWS 4
#define COLS 3
#define DEBOUNCE_DELAY_MS 20

char scan_keypad_press_release(void);

#endif




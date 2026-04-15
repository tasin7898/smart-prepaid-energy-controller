#include "keypad.h"
#include "stm32g0xx_hal.h"
#include "main.h"
#include <string.h>  // For snprintf

extern UART_HandleTypeDef huart2;

// Row and column pin definitions
GPIO_TypeDef* RowPorts[4] = { Row_1_GPIO_Port, Row_2_GPIO_Port, Row_3_GPIO_Port, Row_4_GPIO_Port };
uint16_t RowPins[4] = { Row_1_Pin, Row_2_Pin, Row_3_Pin, Row_4_Pin };
GPIO_TypeDef* ColPorts[3] = { Col_1_GPIO_Port, Col_2_GPIO_Port, Col_3_GPIO_Port };
uint16_t ColPins[3] = { Col_1_Pin, Col_2_Pin, Col_3_Pin };

// Key mapping (row x col)
char keyMap[4][3] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}
};

static uint32_t lastDebounceTime = 0;
static const uint32_t debounceDelay = 100;  // 200ms debounce delay
//static char lastKey = '\0'; // Track the last key pressed

// Helper function to reset the keypad scanning state
void Keypad_Reset(void) {
    // Set all rows to low initially to avoid false key press detection
    for (int i = 0; i < 4; i++) {
        HAL_GPIO_WritePin(RowPorts[i], RowPins[i], GPIO_PIN_RESET);
    }
}

// Function to scan the keypad
// Function to scan the keypad with delay after setting row 1 high
char Keypad_Scan(void) {
    static char lastKey = '\0';  // Key that's currently being held
    char key = '\0';             // Key detected during this scan

    // Set all rows to low initially to avoid false key press detection
    Keypad_Reset();

    // Scan each row
    for (int row = 0; row < 4; row++) {
        // Set the current row high and the others low
        for (int i = 0; i < 4; i++) {
            HAL_GPIO_WritePin(RowPorts[i], RowPins[i], (i == row) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        }

        // Add delay for row 1 stabilization
        if (row == 0) HAL_Delay(10);

        // Check each column
        for (int col = 0; col < 3; col++) {
            if (HAL_GPIO_ReadPin(ColPorts[col], ColPins[col]) == GPIO_PIN_SET) {
                key = keyMap[row][col];

                // If it's the same key still being held, ignore it
                if (key == lastKey) {
                    return '\0';
                }

                uint32_t currentTime = HAL_GetTick();
                if ((currentTime - lastDebounceTime) > debounceDelay) {
                    lastDebounceTime = currentTime;
                    lastKey = key;  // Remember what key is being held
                    return key;     // Return only once when key is first pressed
                }

                return '\0';  // Still within debounce time
            }
        }
    }

    // No key pressed: clear the held key state
    lastKey = '\0';
    return '\0';
}

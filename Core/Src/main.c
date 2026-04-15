/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include <math.h>
#include <string.h>
#include <stdio.h>
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include "ssd1306_tests.h"
#include "keypad.h"
#include <stdlib.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

I2C_HandleTypeDef hi2c1;

RTC_HandleTypeDef hrtc;

TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM2_Init(void);
static void MX_RTC_Init(void);
static void MX_I2C1_Init(void);
/* USER CODE BEGIN PFP */
void Menu_lvl_1(char key);
void Menu_Load(char key);
void Menu_lvl_2(char key);
void Menu_Add_Units(char key);
void Menu_Count(char key);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
//#define ADC_BUFFER_SIZE 8000  // Adjust size as needed

//uint16_t adc_buffer[ADC_BUFFER_SIZE];  // One shared buffer for both channels
uint8_t studentnumber[11]="*23142251#\n";
uint8_t rxBuffer[7];
#define NUM_SAMPLES 1000   // Define the number of samples for voltage and current
#define MAX_SAMPLES 1000   // Adjust as needed
#define SAMPLE_RATE_HZ 10000  // 10kHz sampling rate
#define DEBOUNCE_DELAY 150  // ms
#define ZERO_LOW 1800
#define ZERO_HIGH 1850
#define ZERO_THRESHOLD 1900
volatile uint32_t lastButtonPressTime = 0;
char current_key = '0';              // Default starting key/menu
uint32_t last_update_time = 0;       // For refresh timing
const uint32_t update_interval = 500; // ms
uint16_t adc_values[2];  // Array to store ADC values: [0] = voltage, [1] = current
uint16_t voltage_samples[MAX_SAMPLES];
uint16_t current_samples[MAX_SAMPLES];
uint16_t sample_count = 0;
uint8_t adc_sampling_complete = 0;
float voltage_rms = 0.0;
float current_rms = 0.0;
float phase = 0.0;          // Phase in radians (x.xxx)
float apparent_power = 0.0; // Apparent power in VA (xxxx)
float real_power = 0.0;     // Real power in W (xxxx)
float reactive_power = 0.0; // Reactive power in VAR (xxxx)
float power_factor = 0.0;   // Power factor (x.xxx)
float energy_per_day = 0.0; // Energy per day in Wh (xxxxx)
float max_power = 0.0;      // Maximum power in W (xxxx)
float units_left = 1.0;     // Units left in kWh (xxx.x)
float min_units = 0.0;
uint8_t currentMenuLevel = 0;

//void get_timestamp(char *timestamp) {
//    RTC_TimeTypeDef sTime = {0};
//    RTC_DateTypeDef sDate = {0};
//
//    // Get current time and date
//    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
//    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
//
//    // Format the timestamp
//    snprintf(timestamp, 21, "20%02d/%02d/%02d %02d:%02d:%02d\n",
//             sDate.Year, sDate.Month, sDate.Date,
//             sTime.Hours, sTime.Minutes, sTime.Seconds);
//}

void get_timestamp(char *timestamp) {
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    // Get current time and date
    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

    // Format: HH:MM:SS  YY/MM/DD (2-digit year)
    snprintf(timestamp, 21, "%02d:%02d:%02d  %02d/%02d/%02d",
             sTime.Hours, sTime.Minutes, sTime.Seconds,
             sDate.Year, sDate.Month, sDate.Date);
}
void get_timestamp1(char *timestamp1) {
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    // Get current time and date
    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

    // Format the timestamp
    snprintf(timestamp1, 21, "20%02d/%02d/%02d %02d:%02d:%02d\n",
             sDate.Year, sDate.Month, sDate.Date,
             sTime.Hours, sTime.Minutes, sTime.Seconds);
}
// ADC Conversion Complete Callback
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {
    if (hadc->Instance == ADC1) {
        voltage_samples[sample_count] = adc_values[0];
        current_samples[sample_count] = adc_values[1];
        sample_count++;
        if (sample_count >= NUM_SAMPLES) {
            sample_count = 0; //reset counter
            //HAL_TIM_Base_Stop_IT(&htim2);
        }
    }
}

// Timer Period Elapsed Callback (Trigger ADC Conversion)
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM2) {
        HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_values, 2);
    }
}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART2) {
        if (strncmp((char *)rxBuffer, "*Stat#\n", 7) == 0) {
            handle_status_command();

        }
        else if (strncmp((char *)rxBuffer, "*Load#\n", 7) == 0) {
            handle_load_command();
        }
        // Add to your UART command list
//        else if (strncmp((char *)rxBuffer, "*xADC#\n", 7) == 0) {
//            handle_adc_debug_command();  // New function to show raw ADC data
//        }
        HAL_UART_Receive_IT(&huart2, rxBuffer, 7);
    }
}
// Function to calculate RMS value from samples
void calculate_rms(void) {
    uint16_t v_max = voltage_samples[0];
    uint16_t v_min = voltage_samples[0];  // ADC max value
    uint16_t i_max = current_samples[0];
    uint16_t i_min = current_samples[0];

    // Loop through buffer to find peak and min values
    for (uint16_t i = 0; i < NUM_SAMPLES; i++) {
        if (voltage_samples[i] > v_max) v_max = voltage_samples[i];
        if (voltage_samples[i] < v_min) v_min = voltage_samples[i];

        if (current_samples[i] > i_max) i_max = current_samples[i];
        if (current_samples[i] < i_min) i_min = current_samples[i];
    }

    // Get peak-to-peak values
    float v_pp = (v_max - v_min) * (3.3f / 4095.0f);  // Convert ADC to voltage
    float i_pp = (i_max - i_min) * (3.3f / 4095.0f);

    // Convert peak-to-peak voltage to RMS (2Vpp = 230V)
    voltage_rms = (v_pp / 2.0f / sqrt(2.0f)) * 230.0f*1.118;
    current_rms = (i_pp / 2.0f / sqrt(2.0f)) * 15.0f*1.118;
    apparent_power= voltage_rms*current_rms;
}

int find_zero_crossing_index(uint16_t *samples) {
    for (int i = 1; i < NUM_SAMPLES; i++) {
        if (samples[i-1] < ZERO_THRESHOLD && samples[i] >= ZERO_THRESHOLD) {
            return i;
        }
    }
    return -1; // Not found
}


void calculate_phase_and_power(void) {
    int v_idx = find_zero_crossing_index(voltage_samples);
    int i_idx = find_zero_crossing_index(current_samples);

    if (v_idx == -1 || i_idx == -1) {
        // If zero-crossing failed, just return
        return;
    }

    int delta = i_idx - v_idx;
    float sample_rate = 10000.0f; // 10kHz
    float freq = 50.0f;

    float time_diff = (float)delta / sample_rate;
    float raw_phase = 2.0f * M_PI * freq * time_diff;

    // Calibration offset
    float baseline_offset = -1.4f-3.0f;
    raw_phase -= baseline_offset;

    static float last_phase = 0.0f;

    if (raw_phase > 3.1f || raw_phase < -3.1f) {
        // Glitch detected — keep previous value
        phase = last_phase;
    } else {
        // Valid value — update
        phase = raw_phase;
        last_phase = phase;
    }

    power_factor = cosf(phase);
    power_factor = fabsf(power_factor);
    real_power = apparent_power * power_factor;
    real_power = fabsf(real_power);
    reactive_power = apparent_power * sinf(phase);
}

// UART Command Handler


void handle_status_command(void) {
	HAL_TIM_Base_Start_IT(&htim2);
    calculate_rms(); // Calculate RMS voltage and current
    calculate_phase_and_power();
    char timestamp1[21]; // Buffer for timestamp (20 characters + null terminator)
    get_timestamp1(timestamp1); // Get the current timestamp

    char message[256]; // Buffer for the entire status message
    int len = 0;

    // Add timestamp (20 characters, including newline)
    len += snprintf(message + len, sizeof(message) - len, "%.20s", timestamp1);

    // Add formatted data with proper alignment
    len += snprintf(message + len, sizeof(message) - len, "%-9s%9.1fV\n", "Voltage:", voltage_rms);
    len += snprintf(message + len, sizeof(message) - len, "%-9s%8.0fmA\n", "Current:", current_rms * 1000);
    len += snprintf(message + len, sizeof(message) - len, "%-10s%6.3frad\n", "Phase:", phase);
    len += snprintf(message + len, sizeof(message) - len, "%-10s%7.0fVA\n", "Apparent:", apparent_power);
    len += snprintf(message + len, sizeof(message) - len, "%-10s%8.0fW\n", "Real:", real_power);
    len += snprintf(message + len, sizeof(message) - len, "%-10s%6.0fVAR\n", "Reactive:", reactive_power);
    len += snprintf(message + len, sizeof(message) - len, "%-10s%9.3f\n", "PowerFact:", power_factor);
    len += snprintf(message + len, sizeof(message) - len, "%-10s%7.0fWh\n", "Energy/d:", energy_per_day);
    len += snprintf(message + len, sizeof(message) - len, "%-10s%8.0fW\n", "MaxPower:", max_power);
    len += snprintf(message + len, sizeof(message) - len, "%-10s%6.1fkWh\n", "UnitsLeft:", units_left);

    // Transmit the message
    HAL_UART_Transmit(&huart2, (uint8_t *)message, len, HAL_MAX_DELAY);
}
//void handle_adc_debug_command(void) {
//    // Capture snapshot of current ADC buffer (unchanged)
//    uint16_t v_min = voltage_samples[0], v_max = voltage_samples[0];
//    uint16_t i_min = current_samples[0], i_max = current_samples[0];
//    uint32_t v_sum = 0, i_sum = 0;
//
//    for (int i = 0; i < NUM_SAMPLES; i++) {
//        v_sum += voltage_samples[i];
//        i_sum += current_samples[i];
//        if (voltage_samples[i] < v_min) v_min = voltage_samples[i];
//        if (voltage_samples[i] > v_max) v_max = voltage_samples[i];
//        if (current_samples[i] < i_min) i_min = current_samples[i];
//        if (current_samples[i] > i_max) i_max = current_samples[i];
//    }
//
//    // Calculations (now with clear variable names)
//    const float opamp_gain = 1.3f;
//    float raw_vpp = (v_max - v_min) * 3.3f / 4095.0f;    // Higher value (op-amp output)
//    float input_vpp = raw_vpp / opamp_gain;              // Lower value (original input)
//    float raw_ipp = (i_max - i_min) * 3.3f / 4095.0f;
//    float input_ipp = raw_ipp / opamp_gain;
//
//    // Format output with swapped values
//    char message[250];
//    int len = snprintf(message, sizeof(message),
//        "ADC Accuracy Report:\n"
//        "Voltage: Min=%.3fV, Max=%.3fV, Avg=%.3fV\n"
//        "Current: Min=%.3fV, Max=%.3fV, Avg=%.3fV\n"
//        "Samples=%d\n"
//        "Vpp=%.3fV (input pre-gain vpp=%.3fV)\n"      // Raw ADC value first
//        "Ipp=%.3fV (input pre-gain ipp=%.3fV)\n",     // Then divided value
//        v_min * 3.3f / 4095.0f,
//        v_max * 3.3f / 4095.0f,
//        (v_sum * 3.3f) / (NUM_SAMPLES * 4095.0f),
//        i_min * 3.3f / 4095.0f,
//        i_max * 3.3f / 4095.0f,
//        (i_sum * 3.3f) / (NUM_SAMPLES * 4095.0f),
//        NUM_SAMPLES,
//        raw_vpp,     // Higher value (op-amp output)
//        input_vpp,   // Lower value (original input)
//        raw_ipp,
//        input_ipp);
//
//    HAL_UART_Transmit(&huart2, (uint8_t *)message, len, 100);
//}

void handle_load_command(void)
{
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_9);
}
//void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
//{
//    if (GPIO_Pin == Middle_button_Pin)
//    {
//        uint32_t now = HAL_GetTick();
//        if ((now - lastButtonPressTime) > DEBOUNCE_DELAY)
//        {
//            lastButtonPressTime = now;
//            handle_load_command();
//        }
//    }
//}

void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == Middle_button_Pin)
    {
        uint32_t now = HAL_GetTick();

        // Debounce: only allow if 200ms passed since last press
        if ((now - lastButtonPressTime) > DEBOUNCE_DELAY)
        {
            // Confirm the button is still pressed (still LOW)
            if (HAL_GPIO_ReadPin(Middle_button_GPIO_Port, Middle_button_Pin) == GPIO_PIN_RESET)
            {
                lastButtonPressTime = now;
                HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_9);
            }
        }
    }
}


void Menu_lvl_0(char key) {
    char line[32];

    HAL_TIM_Base_Start_IT(&htim2);
    calculate_rms();
    calculate_phase_and_power();
    char timestamp[21];
    get_timestamp(timestamp);

    ssd1306_Fill(Black);

    switch (key) {
        case '0':
            snprintf(line, sizeof(line), "%-9s%9.0fVA", "Apparent:", apparent_power);
            ssd1306_SetCursor(0, 0);
            ssd1306_WriteString(line, Font_6x8, White);

            snprintf(line, sizeof(line), "%-9s%9.0fWh", "Energy/d:", energy_per_day);
            ssd1306_SetCursor(0, 10);
            ssd1306_WriteString(line, Font_6x8, White);

            snprintf(line, sizeof(line), "%-9s%9.4fkWh", "Units:", units_left);
            ssd1306_SetCursor(0, 20);
            ssd1306_WriteString(line, Font_6x8, White);
            break;

        case '1':
            snprintf(line, sizeof(line), "%-9s%9.1fV", "Voltage:", voltage_rms);
            ssd1306_SetCursor(0, 0);
            ssd1306_WriteString(line, Font_6x8, White);

            snprintf(line, sizeof(line), "%-9s%9.3fA", "Current:", current_rms);
            ssd1306_SetCursor(0, 10);
            ssd1306_WriteString(line, Font_6x8, White);

            snprintf(line, sizeof(line), "%-9s%9.3fr", "Phase:", phase);
            ssd1306_SetCursor(0, 20);
            ssd1306_WriteString(line, Font_6x8, White);
            break;

        case '2':
            snprintf(line, sizeof(line), "%-9s%9.0fW", "Real:", real_power);
            ssd1306_SetCursor(0, 0);
            ssd1306_WriteString(line, Font_6x8, White);

            snprintf(line, sizeof(line), "%-9s%9.0fW", "Reactive:", reactive_power);
            ssd1306_SetCursor(0, 10);
            ssd1306_WriteString(line, Font_6x8, White);

            snprintf(line, sizeof(line), "%-9s%9.3f", "PowerFact:", power_factor);
            ssd1306_SetCursor(0, 20);
            ssd1306_WriteString(line, Font_6x8, White);
            break;

        case '3':
            get_timestamp(timestamp);  // Make sure it's updated before printing

            ssd1306_SetCursor(0, 0);
            ssd1306_WriteString(timestamp, Font_6x8, White);

            snprintf(line, sizeof(line), "%-9s%9.0fW", "Max Power:", max_power);
            ssd1306_SetCursor(0, 10);
            ssd1306_WriteString(line, Font_6x8, White);

            snprintf(line, sizeof(line), "%-9s%9.1fkWh", "Min Units:", min_units);
            ssd1306_SetCursor(0, 20);
            ssd1306_WriteString(line, Font_6x8, White);
            break;


        case '*':
            currentMenuLevel = 1;
            Menu_lvl_1('\0');  // Initial display of Menu Level 1
            return;            // Important: avoid further processing

         default:
             ssd1306_Fill(Black);
             ssd1306_SetCursor(0, 0);
             ssd1306_WriteString("Invalid key (Lvl 0)", Font_6x8, White);
             break;
    }

    ssd1306_UpdateScreen();
}

void Menu_lvl_1(char key) {
    char line[32];
    ssd1306_Fill(Black);

    switch (key) {
        case '\0':
            ssd1306_SetCursor(0, 0);
            ssd1306_WriteString("Menu(Lvl 1):", Font_6x8, White);

            ssd1306_SetCursor(0, 10);
            snprintf(line, sizeof(line), "1: Load On/Off");
            ssd1306_WriteString(line, Font_6x8, White);

            ssd1306_SetCursor(0, 20);
            snprintf(line, sizeof(line), "2: Manage Units");
            ssd1306_WriteString(line, Font_6x8, White);
            break;

        case '1':
        			currentMenuLevel = 2;
                    Menu_Load('\0');  // Entry into Load submenu
                    return;

        case '2':
        	currentMenuLevel = 3;
        	Menu_lvl_2('\0');
        	return;


        case '#':
            currentMenuLevel = 0;       // Go back to level 0
            current_key = '0';          // Reset to a valid default key for refresh
            Menu_lvl_0(current_key);    // Show default screen
            return;
                      // Done

        default:
            ssd1306_SetCursor(0, 0);
            ssd1306_WriteString("Invalid key (Lvl 1)", Font_6x8, White);
            break;
    }

    ssd1306_UpdateScreen();
}

void Menu_Load(char key) {
    static char loadSwitchSelection = '\0';  // persist selection during navigation
    currentMenuLevel = 2;

    switch (key) {
        case '\0':

            ssd1306_SetCursor(0, 0);
            ssd1306_WriteString("Load On/Off:", Font_6x8, White);
            ssd1306_SetCursor(0, 10);
            ssd1306_WriteString("1 = ON, 2 = OFF", Font_6x8, White);
            ssd1306_SetCursor(0, 20);
            ssd1306_WriteString("* to confirm", Font_6x8, White);
            ssd1306_UpdateScreen();
            loadSwitchSelection = '\0';  // Reset previous selection
            break;

        case '1':
        case '2':
            loadSwitchSelection = key;  // Store selection until confirm
            break;

        case '*':  // Confirm selection
            if (loadSwitchSelection == '1') {
                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET);  // LED ON
            } else if (loadSwitchSelection == '2') {
                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);  // LED OFF
            }
            // Return to main menu after confirm
            currentMenuLevel = 0;
            Menu_lvl_0('0');
            break;

        case '#':  // Cancel/back
            currentMenuLevel = 1;
            Menu_lvl_1('\0');
            break;
    }
}

void Menu_lvl_2(char key) {
    currentMenuLevel = 3;

    if (key == '\0') {

        ssd1306_SetCursor(0, 0);
        ssd1306_WriteString("Menu(Lvl 2):", Font_6x8, White);
        ssd1306_SetCursor(0, 10);
        ssd1306_WriteString("1: Count On/Off", Font_6x8, White);
        ssd1306_SetCursor(0, 20);
        ssd1306_WriteString("2: Add Units", Font_6x8, White);
        ssd1306_UpdateScreen();
        return;
    }

    switch (key) {
        case '1':
            currentMenuLevel = 4;
            Menu_Count('\0');  // Load Unit Count toggle menu
            break;

        case '2':
            currentMenuLevel = 5;
            Menu_Add_Units('\0');  // Load Add Units menu
            break;

        case '#':
            currentMenuLevel = 1;
            Menu_lvl_1('\0');  // Go back to Main Menu
            break;
    }
}

void Menu_Count(char key) {
    static char countSwitchSelection = '\0';  // Persist selection during navigation
    currentMenuLevel = 4;

    switch (key) {
        case '\0':
            ssd1306_Clear();
            ssd1306_SetCursor(0, 0);
            ssd1306_WriteString("Unit Count Toggle:", Font_6x8, White);
            ssd1306_SetCursor(0, 10);
            ssd1306_WriteString("1 = ON, 2 = OFF", Font_6x8, White);
            ssd1306_SetCursor(0, 20);
            ssd1306_WriteString("* to confirm", Font_6x8, White);
            ssd1306_UpdateScreen();
            countSwitchSelection = '\0';  // Reset previous selection
            break;

        case '1':
        case '2':
            countSwitchSelection = key;  // Store selection
            break;

        case '*':  // Confirm selection
            ssd1306_Clear();
            if (countSwitchSelection == '1') {
                // Handle ON logic
                ssd1306_SetCursor(0, 0);
                ssd1306_WriteString("Unit Count: ON", Font_6x8, White);
                // You can add logic here to enable unit counting
            } else if (countSwitchSelection == '2') {
                // Handle OFF logic
                ssd1306_SetCursor(0, 0);
                ssd1306_WriteString("Unit Count: OFF", Font_6x8, White);
                // You can add logic here to disable unit counting
            } else {
                ssd1306_SetCursor(0, 0);
                ssd1306_WriteString("Invalid Option", Font_6x8, White);
            }
            ssd1306_UpdateScreen();
            HAL_Delay(2000);
            currentMenuLevel = 1;  // Return to Menu Level 1
            Menu_lvl_1('\0');
            break;

        case '#':  // Cancel/back
        	ssd1306_Clear();
            currentMenuLevel = 1;
            Menu_lvl_1('\0');
            break;
    }
}

void Menu_Add_Units(char key) {
    static char unitInputBuffer[10] = "";  // Buffer to hold input digits
    static uint8_t inputIndex = 0;
    currentMenuLevel = 5;

    switch (key) {
        case '\0':
            ssd1306_Clear();
            ssd1306_SetCursor(0, 0);
            ssd1306_WriteString("Units to Add:", Font_6x8, White);
            ssd1306_SetCursor(0, 10);
            ssd1306_WriteString(unitInputBuffer, Font_6x8, White);
            ssd1306_SetCursor(0, 20);
            ssd1306_WriteString("Press * to confirm", Font_6x8, White);
            ssd1306_UpdateScreen();
            break;

        case '0'...'9':
            if (inputIndex < sizeof(unitInputBuffer) - 1) {
                unitInputBuffer[inputIndex++] = key;
                unitInputBuffer[inputIndex] = '\0';  // Null-terminate
            }
            Menu_Add_Units('\0');  // Refresh display with updated input
            break;

        case '*':  // Confirm
            if (inputIndex > 0) {
                int unitsToAdd = atoi(unitInputBuffer);  // Convert input to integer

                // Do your logic here — add units, update display, etc.
                ssd1306_Clear();
                ssd1306_SetCursor(0, 0);
                ssd1306_WriteString("Added Units:", Font_6x8, White);
                char confirmationMsg[20];
                sprintf(confirmationMsg, "%d units", unitsToAdd);
                ssd1306_SetCursor(0, 10);
                ssd1306_WriteString(confirmationMsg, Font_6x8, White);
                ssd1306_UpdateScreen();

                HAL_Delay(2000);
            }

            // Reset and go back
            memset(unitInputBuffer, 0, sizeof(unitInputBuffer));
            inputIndex = 0;
            currentMenuLevel = 1;
            Menu_lvl_1('\0');
            break;

        case '#':  // Cancel/back
            memset(unitInputBuffer, 0, sizeof(unitInputBuffer));
            ssd1306_Clear();
            inputIndex = 0;
            currentMenuLevel = 1;
            Menu_lvl_2('\0');
            break;
    }
}



/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART2_UART_Init();
  MX_ADC1_Init();
  MX_TIM2_Init();
  MX_RTC_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */

  HAL_TIM_Base_Start_IT(&htim2);
 // HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_values, 2);  // 2 channels: voltage and current

  HAL_Delay(120);
  HAL_UART_Transmit(&huart2, studentnumber, 11, 150);
  //HAL_UART_Receive_IT(&huart2, rxBuffer, sizeof(rxBuffer));
  HAL_UART_Receive_IT(&huart2, rxBuffer, 7);
  //HAL_UART_Receive_IT(&huart2, &rxByte, 1);


  //HAL_UART_Transmit(&huart2, (uint8_t *)&letter, 1, 120);
  //HAL_UART_Transmit(&huart2, studentnumber, sizeof(studentnumber) - 1, 120);
  ssd1306_Init();
  //ssd1306_SetCursor(0, 0);
  //ssd1306_WriteString("Hello!", Font_7x10, Black);
  //ssd1306_UpdateScreen();


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  char key = Keypad_Scan();

	      if (key != '\0') {
//	          char msg[16];
//	          snprintf(msg, sizeof(msg), "Key: %c\n", key);
//	          HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
//	          HAL_Delay(100);  // Debounce delay

	          switch (currentMenuLevel) {
	              case 0:
	                  if (key == '*' || key != current_key) {
	                      current_key = key;
	                      Menu_lvl_0(current_key); // Update immediately on keypress
	                  }
	                  break;
	              case 1:
	                  Menu_lvl_1(key);
	                  break;
	              case 2:
	                  Menu_Load(key);
	                  break;
	              case 3:
	                  Menu_lvl_2(key);
	                  break;
	              case 4:
	                  Menu_Count(key);
	                  break;
	              case 5:
	                  Menu_Add_Units(key);
	                  break;
	              // Extend with more levels if needed
	          }
	      }

	      // ⏱️ Periodic update for Menu Level 0
	      if (currentMenuLevel == 0 && HAL_GetTick() - last_update_time >= update_interval) {
	          last_update_time = HAL_GetTick();
	          Menu_lvl_0(current_key);  // Update display every 500 ms with same menu key
	      }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
  RCC_OscInitStruct.PLL.PLLN = 8;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SEQ_CONV;
  hadc1.Init.LowPowerAutoWait = DISABLE;
  hadc1.Init.LowPowerAutoPowerOff = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.NbrOfConversion = 2;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc1.Init.SamplingTimeCommon1 = ADC_SAMPLETIME_7CYCLES_5;
  hadc1.Init.SamplingTimeCommon2 = ADC_SAMPLETIME_7CYCLES_5;
  hadc1.Init.OversamplingMode = DISABLE;
  hadc1.Init.TriggerFrequencyMode = ADC_TRIGGER_FREQ_HIGH;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLINGTIME_COMMON_1;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = ADC_REGULAR_RANK_2;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x10B17DB5;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  hrtc.Init.OutPutPullUp = RTC_OUTPUT_PULLUP_NONE;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN Check_RTC_BKUP */

  /* USER CODE END Check_RTC_BKUP */

  /** Initialize RTC and set the Time and Date
  */
  sTime.Hours = 0x0;
  sTime.Minutes = 0x0;
  sTime.Seconds = 0x0;
  sTime.SubSeconds = 0x0;
  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sTime.StoreOperation = RTC_STOREOPERATION_RESET;
  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  sDate.WeekDay = RTC_WEEKDAY_MONDAY;
  sDate.Month = RTC_MONTH_JANUARY;
  sDate.Date = 0x1;
  sDate.Year = 0x0;

  if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 63;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 99;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_OC_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_TIMING;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_OC_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 57600;
  huart2.Init.WordLength = UART_WORDLENGTH_9B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_EVEN;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart2, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart2, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LED_GREEN_Pin|D2_LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, Row_1_Pin|Row_2_Pin|Row_3_Pin|Row_4_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LED_GREEN_Pin */
  GPIO_InitStruct.Pin = LED_GREEN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(LED_GREEN_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : Col_1_Pin Col_2_Pin */
  GPIO_InitStruct.Pin = Col_1_Pin|Col_2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : Row_1_Pin Row_2_Pin Row_3_Pin Row_4_Pin */
  GPIO_InitStruct.Pin = Row_1_Pin|Row_2_Pin|Row_3_Pin|Row_4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : Middle_button_Pin */
  GPIO_InitStruct.Pin = Middle_button_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(Middle_button_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : D2_LED_Pin */
  GPIO_InitStruct.Pin = D2_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(D2_LED_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : Col_3_Pin */
  GPIO_InitStruct.Pin = Col_3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(Col_3_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI4_15_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

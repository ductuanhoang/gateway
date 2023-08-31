/*
 * template.c
 *
 *  Created on:
 *      Author:
 */

/***********************************************************************************************************************
 * Pragma directive
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * Includes <System Includes>
 ***********************************************************************************************************************/
#include "user_adc.h"

#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "driver/gpio.h"
#include "esp_log.h"
// #include "esp_adc/adc_continuous.h"

/***********************************************************************************************************************
 * Macro definitions
 ***********************************************************************************************************************/
#define ADC_TAG "ADC"
/***********************************************************************************************************************
 * Typedef definitions
 ***********************************************************************************************************************/
#define DEFAULT_VREF 3300 // Use adc2_vref_to_gpio() to obtain a better estimate
#define NO_OF_SAMPLES 64  // Multisampling

static esp_adc_cal_characteristics_t *adc_chars;

/***********************************************************************************************************************
 * Private global variables and functions
 ***********************************************************************************************************************/
static void print_char_val_type(esp_adc_cal_value_t val_type);
static void check_efuse(void);


static const adc_channel_t channel = ADC_CHANNEL_3; // GPIO39 if ADC1, GPIO14 if ADC2
static const adc_bits_width_t width = ADC_WIDTH_BIT_10;
static const adc_atten_t atten = ADC_ATTEN_DB_11;
static const adc_unit_t unit = ADC_UNIT_1;
/***********************************************************************************************************************
 * Exported global variables and functions (to be accessed by other files)
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * Imported global variables and functions (from other files)
 ***********************************************************************************************************************/
void adc_init(void)
{
    // Configure ADC
    if (unit == ADC_UNIT_1)
    {
        adc1_config_width(width);
        adc1_config_channel_atten(channel, atten);
    }
    else
    {
        adc2_config_channel_atten((adc2_channel_t)channel, atten);
    }

    // Characterize ADC
    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(unit, atten, width, DEFAULT_VREF, adc_chars);
    print_char_val_type(val_type);
}

uint32_t adc_get_voltage_value(void)
{
    uint32_t adc_reading = 0;
    // Multisampling
    // for (int i = 0; i < NO_OF_SAMPLES; i++)
    // {
    //     if (unit == ADC_UNIT_1)
    //     {
    //         adc_reading += adc1_get_raw((adc1_channel_t)channel);
    //     }
    // }
    // adc_reading /= NO_OF_SAMPLES;

    adc_reading = adc1_get_raw((adc1_channel_t)channel);

    // Convert adc_reading to voltage in mV
    uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
    // ESP_LOGI(ADC_TAG, "Raw: %d\tVoltage: %dmV\n", adc_reading, voltage);

    return voltage;
}
/***********************************************************************************************************************
 * static functions
 ***********************************************************************************************************************/
static void check_efuse(void)
{
#if CONFIG_IDF_TARGET_ESP32
    // Check if TP is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK)
    {
        ESP_LOGI(ADC_TAG, "eFuse Two Point: Supported\n");
    }
    else
    {
        ESP_LOGI(ADC_TAG, "eFuse Two Point: NOT supported\n");
    }
    // Check Vref is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK)
    {
        ESP_LOGI(ADC_TAG, "eFuse Vref: Supported\n");
    }
    else
    {
        ESP_LOGI(ADC_TAG, "eFuse Vref: NOT supported\n");
    }
#elif CONFIG_IDF_TARGET_ESP32S2
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK)
    {
        ESP_LOGI(ADC_TAG, "eFuse Two Point: Supported\n");
    }
    else
    {
        ESP_LOGI(ADC_TAG, "Cannot retrieve eFuse Two Point calibration values. Default calibration values will be used.\n");
    }
#else
#error "This example is configured for ESP32/ESP32S2."
#endif
}

static void print_char_val_type(esp_adc_cal_value_t val_type)
{
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP)
    {
        ESP_LOGI(ADC_TAG, "Characterized using Two Point Value\n");
    }
    else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF)
    {
        ESP_LOGI(ADC_TAG, "Characterized using eFuse Vref\n");
    }
    else
    {
        ESP_LOGI(ADC_TAG, "Characterized using Default Vref\n");
    }
}
/***********************************************************************************************************************
 * End of file
 ***********************************************************************************************************************/
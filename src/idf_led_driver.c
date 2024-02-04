/*
 * SPDX-FileCopyrightText: 2024 Rossen Dobrinov
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Copyright 2024 Rossen Dobrinov
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "idf_led_driver.h"

/**
 * @brief Internal driver state type
*/
typedef struct lm_led {
    TaskHandle_t led_task_handle;   /*!< Execute task handle (not used)*/
    ledc_mode_t speed_mode;         /*<! LEDC speed mode */
    ledc_channel_t channel;         /*<! LEDC channel */
    SemaphoreHandle_t xSemaphore;   /*<! Semaphore for led program access */
    TickType_t cycle_start;         /*<! Program element start */
    TickType_t cycle_interval;      /*<! Program element interval */
    uint8_t uIndex;                 /*<! Current program element index */
    uint8_t maxPgmIdx;              /*<! Max program element index */
    lm_led_state_t *led_pgm;        /*<! Array with program elements */
} lm_led_t;

static lm_led_t *lm = NULL; /*<! Led driver task state */

/**
 * @brief Program execute task function
 * 
 * @param[in] pvParameters A NULL value that is passed as the paramater to the created task
 * 
 * @return
*/
static void vLedTask(void *pvParameters);

esp_err_t lm_init(lm_ledc_config_t *ledc_config) {
    lm = (lm_led_t *)calloc(1, sizeof(lm_led_t));
    lm->led_pgm = NULL;
    lm_ledc_config_t *defcfg = (lm_ledc_config_t *)calloc(1, sizeof(lm_ledc_config_t));
    if(ledc_config) {
        defcfg = ledc_config;
    } else {
        defcfg->ledc_timer = (ledc_timer_config_t) {
            #if defined(CONFIG_IDF_TARGET_ESP32)
            .speed_mode       = LEDC_HIGH_SPEED_MODE,
            #else
            .speed_mode       = LEDC_LOW_SPEED_MODE,
            #endif
            .timer_num        = LEDC_TIMER_0,
            .duty_resolution  = LEDC_TIMER_12_BIT,
            .freq_hz          = 16000,              // Set output frequency at 16kHz
            .clk_cfg          = LEDC_AUTO_CLK    
        };
        defcfg->ledc_channel = (ledc_channel_config_t) {
            #if defined(CONFIG_IDF_TARGET_ESP32)
            .speed_mode       = LEDC_HIGH_SPEED_MODE,
            #else
            .speed_mode       = LEDC_LOW_SPEED_MODE,
            #endif
            .channel        = LEDC_CHANNEL_0,
            .timer_sel      = LEDC_TIMER_0,         
            .intr_type      = LEDC_INTR_DISABLE,    // No interupts
            .gpio_num       = GPIO_NUM_19,          // TTGO T7 v1.4 green led pin
            .duty           = 0,                    // Set duty to 0% - Led off
            .hpoint         = 0                     // High point to 0 mean latch high at counter overflow
        };
    }
    lm->speed_mode = defcfg->ledc_channel.speed_mode;
    lm->channel = defcfg->ledc_channel.channel;
    bool ret = (ESP_OK == ledc_timer_config(&defcfg->ledc_timer));
    ret &= (ESP_OK == ledc_channel_config(&defcfg->ledc_channel));
    free(defcfg);
    lm->xSemaphore = xSemaphoreCreateBinary();
    lm->uIndex = 0;
    lm->maxPgmIdx = 0;
    xSemaphoreGive(lm->xSemaphore);
    ret &= (ESP_OK == ledc_fade_func_install(0));
    if(!ret) {
        vSemaphoreDelete(lm->xSemaphore);
        free(lm);
        return ESP_FAIL;
    }
    xTaskCreate(vLedTask, "ledctrl", 2048, NULL, 12, &lm->led_task_handle);
    return ESP_OK;
}

void lm_apply_pgm(lm_led_state_t *led_state, size_t led_state_size) {
    if(!lm) {
        if( ESP_OK != lm_init(NULL)) return;
    }
    if(xSemaphoreTake(lm->xSemaphore, (TickType_t) 2) == pdTRUE) {
        free(lm->led_pgm);
        if(led_state) {
            size_t mem_size = led_state_size * sizeof(lm_led_state_t);
            lm->led_pgm = (lm_led_state_t *)malloc(mem_size);
            lm->uIndex = 0;
            lm->maxPgmIdx = led_state_size;
            memcpy(lm->led_pgm, led_state, mem_size);
        } else lm->led_pgm = NULL;
        xSemaphoreGive(lm->xSemaphore);
    }
    return;
}

static void vLedTask(void *pvParameters) {
    while(true)
    {
        if(xSemaphoreTake(lm->xSemaphore, (TickType_t) 1) == pdTRUE) {
            if(lm->led_pgm) {
                if((xTaskGetTickCount() - lm->cycle_start) > lm->cycle_interval) {
                    lm->cycle_interval = lm->led_pgm[lm->uIndex].time + lm->led_pgm[lm->uIndex].fade_time;
                    if(lm->led_pgm[lm->uIndex].fade_time == 0) {
                        ledc_set_duty_and_update(lm->speed_mode, lm->channel, lm->led_pgm[lm->uIndex].intensity, 0);
                    } else {
                        ledc_set_fade_time_and_start(lm->speed_mode, lm->channel, lm->led_pgm[lm->uIndex].intensity, lm->led_pgm[lm->uIndex].fade_time, LEDC_FADE_NO_WAIT);
                    }
                    lm->cycle_start = xTaskGetTickCount();
                    lm->cycle_interval /= portTICK_PERIOD_MS;
                    lm->uIndex++;
                    if(lm->uIndex == lm->maxPgmIdx) lm->uIndex = 0;
                }
            }
            xSemaphoreGive(lm->xSemaphore);
            vTaskDelay(1);
        }
    }
}
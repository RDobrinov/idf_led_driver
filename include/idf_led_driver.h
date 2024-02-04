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

#ifndef _IDF_LED_DRIVER_H_
#define _IDF_LED_DRIVER_H_

#include "esp_system.h"
#include "driver/ledc.h"

/**
 * @brief LEDC timer and channel configuration type
*/
typedef struct lm_ledc_config {
    ledc_timer_config_t ledc_timer;     /*<! LEDC component timer config */
    ledc_channel_config_t ledc_channel; /*<! LEDC component channel config */
} lm_ledc_config_t;

/**
 * @verbatim
 *   |
 *   |
 * I |                                       .--------------------+
 * n |                                      . .                   |
 * t |                                     .  .                   |
 * e |    fade_time = 0                   .   .                   |
 * n |          |                        .    .                   |
 * s |          v                       .     .                   |
 * i |----------+                      .      .                   |
 * t |          |                     .       .                   |
 * y |          |                    .        .                   |
 *   |          |                   .         .                   |
 *   |          |                  .          .                   +------>
 *   |          |                 .           .                   ^
 *   |          |                .            .                   |
 *   |          |               .             .             fade_time = 0
 *   +----------+--------------+----------------------------------------->
 *              .              .              .                   .
 *   ---------->.<------------>.<------------>.<----------------->.
 *                lm_led_state . lm_led_state      lm_led_state   .
 *                    time     .  fade_time            time       .
 *                             .                                  .
 *   ------------------------->.<-------------------------------->.
 *    program element interval .    program element interval      .
 * @endverbatim
 * 
 * @note Program element interval is the sum of [time] and [fade_time].
 * [fade_time] is transition time to new intensity, so zero means none 
 * transition at all. [time] is new intensity hold time before apply
 * next program element
*/

/**
 * @brief Led driver single program element type
 * 
*/
typedef struct lm_led_state {
    uint16_t intensity; /*<! Led intensity (duty). Max value is calculated by (LEDC_TIMER_XX_BIT^^2) - 1 */
    uint16_t fade_time; /*<! Fade time in ms from last state to new state */
    uint16_t time;      /*<! Hold time in ms */
} lm_led_state_t;

/**
 * @brief Initialize driver and start control task
 * 
 * @param[in] ledc_config LEDC timer and channel configuration of NULL for driver default
 *
 * @return 
 * - ESP_OK Success
 * - ESP_FAIL Initialization failed
*/
esp_err_t lm_init(lm_ledc_config_t *ledc_config);

/**
 * @brief Apply new program to control driver
 * 
 * @param[in] led_state Pointer to an array witn program elements
 * @param[in] led_pgm_size Number of program elements ( Number of led_state[] elements )
 *
 * @return 
*/
void lm_apply_pgm(lm_led_state_t *led_state, size_t led_pgm_size);

#endif


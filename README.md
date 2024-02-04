# IDF Led control driver

ESP IDF component for monochrome LED control. It has been tested with the ESP32, ESP32-S3, and ESP32-C6 boards.

![](https://img.shields.io/badge/dynamic/yaml?url=https://raw.githubusercontent.com/RDobrinov/idf_led_driver/main/idf_component.yml&query=$.version&style=plastic&color=%230f900f&label)
![](https://img.shields.io/badge/dynamic/yaml?url=https://raw.githubusercontent.com/RDobrinov/idf_led_driver/main/idf_component.yml&query=$.dependencies.idf&style=plastic&logo=espressif&label=IDF%20Ver.)
![](https://img.shields.io/badge/-ESP32-rgb(37,194,160)?style=plastic&logo=espressif)
![](https://img.shields.io/badge/-ESP32--S3-rgb(37,194,160)?style=plastic&logo=espressif)
![](https://img.shields.io/badge/-ESP32--C6-rgb(37,194,160)?style=plastic&logo=espressif)
---

## Features

* Thread safe standalone contol task
* Fade or switch to new intensity state
* Up to 1 minute hold period per intensity
* Up to 2 minutes per LED program state

## Installation

1. Create *idf_component.yml*
```shell
idf.py create-manifest
```
2. Edit ***idf_component.yml*** to add dependency
```yaml
dependencies:
  ...
  idf_led_driver:
    version: "main"
    git: git@github.com:RDobrinov/idf_led_driver.git
  ...
```
3. Reconfigure project

or 

4. Download and unzip component in project ***components*** folder

### Example
```c
#include <stdio.h>
#include "idf_wifi_manager.h"

void app_main(void)
{
    /* Nice heartbeat blink with fade efect */
    lm_apply_pgm((lm_led_state_t[]) {{2048, 0, 150}, {256, 0, 150}, {2048, 0, 150}, {0, 750, 1500}}, 4);
}
```

### Program element explained

The led program is a sequence of program elements. Each element represents one final LED state. The final state is characterized by 3 parameters - intensity, fade time (or glow in case of increased intensity) and hold time. The fade/glow time defines the time it takes the intensity to reach its new level in milliseconds, and the hold time is the time before next program element is applied.

For example. This program has 4 final states
```c
    /* Nice heartbeat blink with fade efect */
    lm_apply_pgm((lm_led_state_t[]) {{2048, 0, 150}, {256, 0, 150}, {2048, 0, 150}, {0, 750, 1500}}, 4);
```
Final state
```c
    ...{2048, 0, 150}...
```
represents intensity of 2048, fade time 0 and hold time 150ms. The last element in example 
```c
    ...{0, 750, 1500}...
```
represents intensity 0 ( means off ), fade time 750ms (led will fade from 2048 to 0 for 750ms) and will hold that off state (intensity 0) for 1.5s

### A note
Max intensity value depends from LEDC timer duty resolution. Duty resolution can be controlled in __lm_init__. Default dirver initialization is
```c
(ledc_timer_config_t) {
    .speed_mode       = LEDC_HIGH_SPEED_MODE,
    .timer_num        = LEDC_TIMER_0,
    .duty_resolution  = LEDC_TIMER_12_BIT,
    .freq_hz          = 16000,
    .clk_cfg          = LEDC_AUTO_CLK    
};
defcfg->ledc_channel = (ledc_channel_config_t) {
    .speed_mode     = LEDC_HIGH_SPEED_MODE,
    .channel        = LEDC_CHANNEL_0,
    .timer_sel      = LEDC_TIMER_0,         
    .intr_type      = LEDC_INTR_DISABLE,
    .gpio_num       = GPIO_NUM_19,
    .duty           = 0,                    // Set duty to 0% - Led off
    .hpoint         = 0                     // High point to 0 mean latch high at counter overflow
}
```
There are no LEDC_HIGH_SPEED_MODE for ESP32-S3. Also ESP32 have maximum PWM duty resolution of 20 bits, but ESP32-S3 have only 14 bits. 
And one more warning. Default configuraton set led gpio to GPIO_NUM_19 - it's a green led pin in my favorite TTGO T7 v1.4 board. You can init driver with default params and before apply any program or after delete active one, you can change gpio directly with LEDC function call.
/* ***************************************************************************
 *
 * Copyright 2019 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/

//#define CONFIG_TARGET_WITTY_CLOUD
#ifndef ST_SWITCH_DEVICE_CONTROL_H
#define ST_SWITCH_DEVICE_CONTROL_H

#if defined(CONFIG_TARGET_WITTY_CLOUD)

#define GPIO_OUTPUT_NOTIFICATION_LED 2
#define GPIO_INPUT_BUTTON 4

#define GPIO_OUTPUT_MAINLED 15
#define GPIO_OUTPUT_MAINLED_0 16 /* use as ground */

#define GPIO_OUTPUT_NOUSE1 12
#define GPIO_OUTPUT_NOUSE2 13

enum notification_led_gpio_state {
	NOTIFICATION_LED_GPIO_ON = 0,
	NOTIFICATION_LED_GPIO_OFF = 1,
};
#else //default

#define GPIO_OUTPUT_NOTIFICATION_LED 2
#define GPIO_INPUT_BUTTON 5

#define GPIO_OUTPUT_MAINLED 12
#define GPIO_OUTPUT_MAINLED_0 26 /* use as ground */

#define GPIO_OUTPUT_NOUSE1 14
#define GPIO_OUTPUT_NOUSE2 27

enum notification_led_gpio_state {
	NOTIFICATION_LED_GPIO_ON = 1,
	NOTIFICATION_LED_GPIO_OFF = 0,
};


#endif



enum main_led_gpio_state {
	MAINLED_GPIO_ON = 1,
	MAINLED_GPIO_OFF = 0,
};

enum led_animation_mode_list {
	LED_ANIMATION_MODE_IDLE = 0,
	LED_ANIMATION_MODE_FAST,
	LED_ANIMATION_MODE_SLOW,
};

enum button_gpio_state {
	BUTTON_GPIO_RELEASED = 1,
	BUTTON_GPIO_PRESSED = 0,
};

#define BUTTON_DEBOUNCE_TIME_MS 20
#define BUTTON_LONG_THRESHOLD_MS 5000
#define BUTTON_DELAY_MS 300

enum button_event_type {
	BUTTON_LONG_PRESS = 0,
	BUTTON_SHORT_PRESS = 1,
};

void button_isr_handler(void *arg);
int get_button_event(int* button_event_type, int* button_event_count);
void led_blink(int gpio, int delay, int count);
void change_led_state(int noti_led_mode);
void gpio_init(void);

#endif // ST_SWITCH_DEVICE_CONTROL_H
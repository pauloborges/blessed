/**
 *  The MIT License (MIT)
 *
 *  Copyright (c) 2013 Paulo B. de Oliveira Filho <pauloborgesfilho@gmail.com>
 *  Copyright (c) 2013 Claudio Takahasi <claudio.takahasi@gmail.com>
 *  Copyright (c) 2013 João Paulo Rechi Vita <jprvita@gmail.com>
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 */

#include <stdint.h>
#include <string.h>

/* Platform specific headers */
#include <nrf51.h>
#include <boards.h>
#include <nrf_gpio.h>
#include <nrf_delay.h>

#include <app_common/app_gpiote.h>

/* Project headers */
#include "log.h"

#define GPIOTE_MAX_USERS			1
#define MOTION_PIN				0

static __inline void gpiote_init(void)
{
	GPIO_LED_CONFIG(LED0);
	GPIO_LED_CONFIG(LED1);

	/*
	 * Allocate the buffer needed by the GPIOTE module
	 * (including aligning the buffer correctly).
	 */
	APP_GPIOTE_INIT(GPIOTE_MAX_USERS);
}

static __inline void motion_evt_handler(uint32_t low, uint32_t high)
{
	if (high) {
		DBG("Motion: HIGH");
		nrf_gpio_pin_set(LED0);
	} else if (low) {
		DBG("Motion: LOW");
		nrf_gpio_pin_clear(LED0);
	}
}

static __inline void pir_init(void)
{
	uint32_t err_code;
	uint32_t pin_mask = 1 << MOTION_PIN;
	app_gpiote_user_id_t id;

	GPIO_BUTTON_CONFIG(MOTION_PIN);

	err_code = app_gpiote_user_register(&id, pin_mask, pin_mask,
							motion_evt_handler);
	APP_ERROR_CHECK(err_code);

	err_code = app_gpiote_user_enable(id);
	APP_ERROR_CHECK(err_code);
}

int main(void)
{
	log_init();

	DBG("PIR: setup");

	gpiote_init();
	pir_init();

	DBG("PIR: waiting events ...");

	while (1)
		__WFI();

	return 0;
}

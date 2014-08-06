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
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <app_uart.h>
#include <nrf_delay.h>
#include <boards.h>

#include <blessed/errcodes.h>
#include <blessed/log.h>

#include "nrf51822.h"

#if CONFIG_LOG_ENABLE

#ifndef CONFIG_LOG_BUFFER_LEN
#define CONFIG_LOG_BUFFER_LEN		128
#endif

#ifndef CONFIG_LOG_BAUD_RATE
#define CONFIG_LOG_BAUD_RATE		1M
#endif

#define BAUD_RATE			_BAUD_RATE(CONFIG_LOG_BAUD_RATE)
#define _BAUD_RATE(baud)		__BAUD_RATE(baud)
#define __BAUD_RATE(baud)		UART_BAUDRATE_BAUDRATE_Baud##baud

#define UNINITIALIZED			0
#define READY				1
#define BUSY				2

static volatile uint16_t pos;
static volatile uint16_t len;
static volatile uint8_t buffer[CONFIG_LOG_BUFFER_LEN] __attribute__ ((aligned));
static volatile uint8_t state = UNINITIALIZED;

static __inline void tx_next_byte(void)
{
	if (pos == len) {
		state = READY;
		return;
	}

	app_uart_put(buffer[pos++]);
}

static void uart_evt_handler(app_uart_evt_t *p_app_uart_evt)
{
	if (p_app_uart_evt->evt_type != APP_UART_TX_EMPTY)
		return;

	tx_next_byte();
}


int16_t log_print(const char *format, ...)
{
	va_list args;

	if (state == UNINITIALIZED)
		return -ENOREADY;

	while (state == BUSY);

	va_start(args, format);
	len = vsnprintf((char *) buffer, CONFIG_LOG_BUFFER_LEN, format, args);
	va_end(args);

	pos = 0;
	state = BUSY;

	tx_next_byte();

	return 0;
}

int16_t log_init(void)
{
	uint32_t err_code;

	UNUSED(err_code);

	app_uart_comm_params_t params = {
		RX_PIN_NUMBER,
		TX_PIN_NUMBER,
		RTS_PIN_NUMBER,
		CTS_PIN_NUMBER,
		APP_UART_FLOW_CONTROL_ENABLED,
		false,
		BAUD_RATE
	};

	if (state != UNINITIALIZED)
		return -EALREADY;

	APP_UART_INIT(&params, uart_evt_handler, IRQ_PRIORITY_HIGHEST,
								err_code);

	state = READY;

	/* Necessary to fully initialize the UART */
	nrf_delay_ms(1);
	log_print("\r\n");

	return 0;
}

#endif

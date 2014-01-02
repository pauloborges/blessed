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
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <app_uart.h>
#include <nrf_delay.h>
#include <boards.h>

#include "nrf51822.h"
#include "errcodes.h"
#include "log.h"

#define BUFFER_LEN			128

#define UNINITIALIZED			0
#define READY				1
#define BUSY				2

static uint16_t pos;
static uint16_t len;
static uint8_t buffer[BUFFER_LEN];
static uint8_t state = UNINITIALIZED;

static __inline void tx_next_byte(void)
{
	if (pos == len) {
		state = READY;
		return;
	}

	app_uart_put(buffer[pos++]);
}

int16_t log_print(const char *format, ...)
{
	va_list args;

	if (state == UNINITIALIZED)
		return -ENOREADY;

	while (state == BUSY);

	va_start(args, format);
	vsnprintf((char *) buffer, BUFFER_LEN, format, args);
	va_end(args);

	pos = 0;
	len = strlen((char *) buffer);
	state = BUSY;

	tx_next_byte();

	return 0;
}

static void uart_evt_handler(app_uart_evt_t *p_app_uart_evt)
{
	switch (p_app_uart_evt->evt_type) {
	case APP_UART_TX_EMPTY:
		tx_next_byte();
		break;

	default:
		break;
	}
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
		UART_BAUDRATE_BAUDRATE_Baud115200
	};

	if (state != UNINITIALIZED)
		return -EALREADY;

	APP_UART_INIT(&params, 0, 0, uart_evt_handler, IRQ_PRIORITY_HIGHEST,
								err_code);

	state = READY;

	/* Necessary to fully initialize the UART */
	nrf_delay_ms(1);
	log_print("\r\n");

	return 0;
}
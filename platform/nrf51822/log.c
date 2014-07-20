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
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <boards.h>
#include <nrf51.h>
#include <nrf51_bitfields.h>

#include <blessed/errcodes.h>
#include <blessed/log.h>

#include "nrf51822.h"
#include "delay.h"
#include "uart.h"

#ifndef CONFIG_LOG_BUFFER_LEN
#define CONFIG_LOG_BUFFER_LEN		128
#endif

#define UART_INIT_DELAY			1000 /* 1 ms */

#define UNINITIALIZED			0
#define READY				1
#define BUSY				2

#if CONFIG_LOG_ENABLE
static volatile uint16_t pos;
static volatile uint16_t len;
static volatile uint8_t buffer[CONFIG_LOG_BUFFER_LEN] __attribute__ ((aligned));
static volatile uint8_t state = UNINITIALIZED;


static void send_next_octet(void)
{
	if (pos == len) {
		state = READY;
		return;
	}

	uart_send(buffer[pos++]);
}
#endif

int16_t log_print(const char *format, ...)
{
#if CONFIG_LOG_ENABLE
	va_list args;

	if (state == UNINITIALIZED)
		return -ENOREADY;

	while (state == BUSY);

	va_start(args, format);
	len = vsnprintf((char *) buffer, CONFIG_LOG_BUFFER_LEN, format, args);
	va_end(args);

	pos = 0;
	state = BUSY;

	send_next_octet();
#endif

	return 0;
}

int16_t log_init(void)
{
#if CONFIG_LOG_ENABLE
	struct uart_config config = {
		.baud = UART_BAUD_115200,
		.rx_pin = RX_PIN_NUMBER,
		.tx_pin = TX_PIN_NUMBER,
		.parity_bit = false
	};

	if (state != UNINITIALIZED)
		return -EALREADY;

	uart_init(config, send_next_octet);
	state = READY;

	/* Necessary to fully initialize the UART */
	delay(UART_INIT_DELAY);
	log_print("\r\n");
#endif

	return 0;
}

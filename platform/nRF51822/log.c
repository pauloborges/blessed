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
#include <boards.h>
#include <nrf51_bitfields.h>

#include <blessed/errcodes.h>
#include <blessed/log.h>

#include "nrf51822.h"
#include "delay.h"

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

#define BUFFER_LEN			CONFIG_LOG_BUFFER_LEN
#define BUFFER_MASK			(BUFFER_LEN - 1)
#define BUFFER_EMPTY_SPACE()		(BUFFER_LEN + rp - wp)
#define BUFFER_USED_SPACE()		(wp - rp)
#define WP				(wp & BUFFER_MASK)
#define RP				(rp & BUFFER_MASK)

#define UART_INIT_DELAY			1000 /* 1 ms */

#define UNINITIALIZED			0
#define READY				1
#define BUSY				2

/* Check if BUFFER_LEN is power of 2 */
STATIC_ASSERT(BUFFER_LEN && !(BUFFER_LEN & (BUFFER_LEN - 1)));

static volatile uint32_t wp = 0;
static volatile uint32_t rp = 0;
static uint8_t buffer[BUFFER_LEN] __attribute__ ((aligned));

static volatile uint8_t state = UNINITIALIZED;

static char itosbuf[12];
static char *itoc = "9876543210123456789";

static __inline void tx_next_byte(void)
{
	if (BUFFER_USED_SPACE() == 0) {
		state = READY;
		return;
	}

	app_uart_put(buffer[RP]);
	rp++;
}

static void uart_evt_handler(app_uart_evt_t *p_app_uart_evt)
{
	if (p_app_uart_evt->evt_type != APP_UART_TX_EMPTY)
		return;

	tx_next_byte();
}

static inline const char *itos(int32_t n)
{
	char *p1 = itosbuf;
	char *p2 = itosbuf;
	int32_t prev;
	char c;

	do {
		prev = n;
		n /= 10;
		*p1++ = itoc[9 + prev - n * 10];
	} while (n);

	if (prev < 0)
		*p1++ = '-';

	*p1-- = '\0';

	while(p2 < p1) {
		c = *p1;
		*p1--= *p2;
		*p2++ = c;
	}

	return itosbuf;
}

static inline const char *utos(uint32_t n)
{
	char *p1 = itosbuf;
	char *p2 = itosbuf;
	uint32_t prev;
	char c;

	do {
		prev = n;
		n /= 10U;
		*p1++ = itoc[9 + prev - n * 10];
	} while (n);

	*p1-- = '\0';

	while(p2 < p1) {
		c = *p1;
		*p1--= *p2;
		*p2++ = c;
	}

	return itosbuf;
}

int16_t log_int(int32_t n)
{
	if (state == UNINITIALIZED)
		return -ENOREADY;

	log_string(itos(n));

	return 0;
}

int16_t log_uint(uint32_t n)
{
	if (state == UNINITIALIZED)
		return -ENOREADY;

	log_string(utos(n));

	return 0;
}

int16_t log_char(char c)
{
	if (state == UNINITIALIZED)
		return -ENOREADY;

	if (BUFFER_EMPTY_SPACE() == 0)
		return -ENOMEM;

	buffer[WP] = (uint8_t) c;

	wp++;

	return 0;
}

int16_t log_string(const char *str)
{
	int16_t len;

	if (state == UNINITIALIZED)
		return -ENOREADY;

	len = strlen(str);

	if (BUFFER_EMPTY_SPACE() < len)
		return -ENOMEM;

	if (WP + len < BUFFER_LEN)
		memcpy(buffer + WP, str, len);
	else {
		uint16_t slice1 = BUFFER_LEN - WP;
		uint16_t slice2 = len - slice1;

		memcpy(buffer + WP, str, slice1);
		memcpy(buffer, str + slice1, slice2);
	}

	wp += len;

	if (state == READY) {
		state = BUSY;
		tx_next_byte();
	}

	return 0;
}

int16_t log_newline(void)
{
	if (state == UNINITIALIZED)
		return -ENOREADY;

	if (BUFFER_EMPTY_SPACE() < 2)
		return -ENOMEM;

	buffer[WP] = (uint8_t) '\r';
	wp++;
	buffer[WP] = (uint8_t) '\n';
	wp++;

	return 0;
}

int16_t log_printf(const char *format, ...)
{
	uint32_t len = BUFFER_EMPTY_SPACE();
	char tmp[len];
	va_list args;

	va_start(args, format);
	vsnprintf(tmp, len, format, args);
	va_end(args);

	return log_string(tmp);
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

	APP_UART_INIT(&params, uart_evt_handler, IRQ_PRIORITY_MEDIUM,
								err_code);

	state = READY;

	/* Necessary to fully initialize the UART */
	delay(UART_INIT_DELAY);
	log_newline();

	return 0;
}

#endif

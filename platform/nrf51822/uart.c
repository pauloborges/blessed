/**
 *  The MIT License (MIT)
 *
 *  Copyright (c) 2014 Paulo B. de Oliveira Filho <pauloborgesfilho@gmail.com>
 *  Copyright (c) 2014 Claudio Takahasi <claudio.takahasi@gmail.com>
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

#include <nrf51.h>
#include <nrf51_bitfields.h>

#include <blessed/errcodes.h>

#include "nrf51822.h"
#include "uart.h"

#define STATE_UNINITIALIZED		0
#define STATE_IDLE			1
#define STATE_BUSY			2

#define DISCONNECTED_PIN		0xFFFFFFFF

#if CONFIG_UART_ENABLE

/* nRF51 Series Reference Manual v2.1, section 28.9.10, page 166 */
static const uint32_t baudrates[] = {
	0x0004F000UL, 0x0009D000UL, 0x0013B000UL, 0x00275000UL, 0x003B0000UL,
	0x004EA000UL, 0x0075F000UL, 0x009D5000UL, 0x00EBF000UL, 0x013A9000UL,
	0x01D7E000UL, 0x03AFB000UL, 0x04000000UL, 0x075F7000UL, 0x0EBEDFA4UL,
	0x10000000UL
};

static uint8_t state = STATE_UNINITIALIZED;
static uart_sent_cb_t sent_cb = 0;

void UART0_IRQHandler(void)
{
	if (NRF_UART0->EVENTS_TXDRDY) {
		NRF_UART0->EVENTS_TXDRDY = 0UL;
		NRF_UART0->TASKS_STOPTX = 1UL;

		state = STATE_IDLE;

		if (sent_cb)
			sent_cb();
	} else {
		NRF_UART0->EVENTS_ERROR = 0UL;
		NRF_UART0->TASKS_STOPTX = 1UL;

		state = STATE_IDLE;
	}
}

int16_t uart_send(uint8_t octet)
{
	if (state != STATE_IDLE)
		return -ENOREADY;

	NRF_UART0->TASKS_STARTTX = 1UL;
	NRF_UART0->TXD = octet;

	state = STATE_BUSY;

	return 0;
}

int16_t uart_init(struct uart_config config, uart_sent_cb_t cb)
{
	if (state != STATE_UNINITIALIZED)
		return -EALREADY;

	sent_cb = cb;

	NRF_UART0->PSELRTS = DISCONNECTED_PIN;
	NRF_UART0->PSELCTS = DISCONNECTED_PIN;

	NRF_UART0->PSELTXD = config.tx_pin;
	NRF_UART0->PSELRXD = config.rx_pin;

	NRF_UART0->BAUDRATE = baudrates[config.baud];

	NRF_UART0->CONFIG = 0UL;

	if (config.parity_bit)
		NRF_UART0->CONFIG |= UART_CONFIG_PARITY_Included
						<< UART_CONFIG_PARITY_Pos;
	else
		NRF_UART0->CONFIG |= UART_CONFIG_PARITY_Excluded
						<< UART_CONFIG_PARITY_Pos;

	NRF_UART0->INTENSET = UART_INTENSET_TXDRDY_Enabled
						<< UART_INTENSET_TXDRDY_Pos;

	NVIC_ClearPendingIRQ(UART0_IRQn);
	NVIC_SetPriority(UART0_IRQn, IRQ_PRIORITY_HIGHEST);
	NVIC_EnableIRQ(UART0_IRQn);

	NRF_UART0->ENABLE = UART_ENABLE_ENABLE_Enabled
						<< UART_ENABLE_ENABLE_Pos;

	state = STATE_IDLE;

	return 0;
}

#endif

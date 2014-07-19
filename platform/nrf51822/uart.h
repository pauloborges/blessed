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

#ifndef CONFIG_UART_ENABLE
#define CONFIG_UART_ENABLE		1
#endif

typedef enum {
	UART_BAUD_1200,
	UART_BAUD_2400,
	UART_BAUD_4800,
	UART_BAUD_9600,
	UART_BAUD_14400,
	UART_BAUD_19200,
	UART_BAUD_28800,
	UART_BAUD_38400,
	UART_BAUD_57600,
	UART_BAUD_76800,
	UART_BAUD_115200,
	UART_BAUD_230400,
	UART_BAUD_250000,
	UART_BAUD_460800,
	UART_BAUD_921600,
	UART_BAUD_1000000
} uart_baud_t;

struct uart_config {
	uart_baud_t baud;
	uint8_t rx_pin;
	uint8_t tx_pin;
	bool parity_bit;
};

typedef void (*uart_sent_cb_t) (void);

#if CONFIG_UART_ENABLE

int16_t uart_init(struct uart_config config, uart_sent_cb_t cb);
int16_t uart_send(uint8_t octet);

#else

inline int16_t uart_init(struct uart_config config, uart_sent_cb_t cb)
{
	return 0;
}

inline int16_t uart_send(uint8_t octet)
{
	return 0;
}

#endif

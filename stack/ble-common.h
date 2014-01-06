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

/* TODO: rename to common.h */

#define BDADDR_LEN			6

/* Link Layer specification Section 1.4, Core 4.1 page 2501 */
typedef struct bdaddr {
	uint8_t addr[BDADDR_LEN];
	uint8_t type;
} bdaddr_t;

/* HCI Funcional Specification Section 7.8.5, Core 4.1 page 1247 */
typedef enum adv_type {
	ADV_CONN_UNDIR,		/* connectable undirected */
	ADV_CONN_DIR_HIGH,	/* connectable directed (high duty cycle) */
	ADV_SCAN_UNDIR,		/* scannable undirected */
	ADV_NONCONN_UNDIR,	/* non-connectable undirected */
	ADV_CONN_DIR_LOW,	/* connectable directed (low duty cycle) */
} adv_type_t;

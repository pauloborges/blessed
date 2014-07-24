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

/* Link Layer specification Section 2.1, Core 4.1 page 2503 */
#define RADIO_MAX_PDU			39
#define RADIO_MIN_PDU			2

#define RADIO_FLAGS_RX_NEXT		1
#define RADIO_FLAGS_TX_NEXT		2

typedef enum radio_power {
	RADIO_POWER_4_DBM,
	RADIO_POWER_0_DBM,
	RADIO_POWER_N4_DBM,
	RADIO_POWER_N8_DBM,
	RADIO_POWER_N12_DBM,
	RADIO_POWER_N16_DBM,
	RADIO_POWER_N20_DBM,
	RADIO_POWER_N30_DBM
} radio_power_t;

/* The active parameter informs if the radio is currently active (e.g. because
 * of a TX/RX_NEXT flag). So, if the callback implementation wants to operate
 * the radio, it will need to first stop the radio.
 */
typedef void (*radio_recv_cb_t) (const uint8_t *pdu, bool crc, bool active);
typedef void (*radio_send_cb_t) (bool active);

int16_t radio_init(void);

int16_t radio_set_callbacks(radio_recv_cb_t recv_cb, radio_send_cb_t send_cb);
int16_t radio_prepare(uint8_t ch, uint32_t aa, uint32_t crcinit);
int16_t radio_recv(uint32_t flags);
int16_t radio_send(const uint8_t *data, uint32_t flags);
int16_t radio_stop(void);

int16_t radio_set_tx_power(radio_power_t power);
void radio_set_out_buffer(uint8_t *buf);

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

#include <string.h>

#include <nrf51.h>
#include <nrf51_bitfields.h>

#include "nrf51822.h"
#include "radio.h"

/* nRF51 Series Reference Manual v1.1, page 71:
 * S0, LENGTH and S1 occupies 3 bytes in memory, not 2
 */
#define MAX_BUF_LEN			(RADIO_MAX_PDU + 1)

#define STATUS_INITIALIZED		1
#define STATUS_RX			2
#define STATUS_BUSY			STATUS_RX

static radio_handler handler;
static uint8_t buf[MAX_BUF_LEN];
static uint8_t status;

#define COMMON_INITIALIZATION(ch, aa, crcinit)				\
	do {								\
		int8_t freq;						\
		if (!(status & STATUS_INITIALIZED))			\
			return -1;					\
		if (status & STATUS_BUSY)				\
			return -1;					\
		freq = ch2freq(ch);					\
		if (freq < 0)						\
			return -1;					\
		NRF_RADIO->DATAWHITEIV = ch & 0x3F;			\
		NRF_RADIO->FREQUENCY = freq;				\
		NRF_RADIO->BASE0 = (aa << 8) & 0xFFFFFF00;		\
		NRF_RADIO->PREFIX0 = (aa >> 24) & 0x000000FF;		\
		NRF_RADIO->CRCINIT = crcinit;				\
	} while (0)

static __inline int8_t ch2freq(uint8_t ch)
{
	switch (ch) {
	case 37:
		return 2;
	case 38:
		return 26;
	case 39:
		return 80;
	default:
		if (ch > 39)
			return -1;
		else if (ch < 11)
			return 4 + (2 * ch);
		else
			return 6 + (2 * ch);
	}
}

void RADIO_IRQHandler(void)
{
	struct radio_packet packet;
	uint8_t old_status;

	NRF_RADIO->EVENTS_END = 0UL;

	old_status = status;
	status = STATUS_INITIALIZED;

	if ((old_status & STATUS_RX) && handler) {
		packet.len = buf[1] + 2;
		packet.crcstatus = NRF_RADIO->CRCSTATUS;

		/* Mount ADV PDU header */
		packet.pdu[0] = buf[0];
		packet.pdu[1] = ((buf[2] & 0x3) << 6) | (buf[1] & 0x3F);

		/* Copy ADV PDU payload */
		memcpy(packet.pdu + 2, buf + 3, buf[1]);

		handler(RADIO_EVT_RX_COMPLETED, &packet);
	}
}

int16_t radio_recv(uint8_t ch, uint32_t aa, uint32_t crcinit)
{
	COMMON_INITIALIZATION(ch, aa, crcinit);

	NRF_RADIO->TASKS_RXEN = 1UL;
	status |= STATUS_RX;

	return 0;
}

int16_t radio_stop(void)
{
	if (!(status & STATUS_BUSY))
		return -1;

	NRF_RADIO->EVENTS_DISABLED = 0UL;
	NRF_RADIO->TASKS_DISABLE = 1UL;
	while (NRF_RADIO->EVENTS_DISABLED == 0UL);

	status &= ~STATUS_BUSY;

	return 0;
}

void radio_register_handler(radio_handler hdlr)
{
	handler = hdlr;
}

int16_t radio_init(void)
{
	if (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0UL) {
		NRF_CLOCK->TASKS_HFCLKSTART = 1UL;
		while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0UL);
	}

	NRF_RADIO->MODE = RADIO_MODE_MODE_Ble_1Mbit << RADIO_MODE_MODE_Pos;

	NRF_RADIO->TXPOWER = RADIO_TXPOWER_TXPOWER_0dBm
						<< RADIO_TXPOWER_TXPOWER_Pos;

	NRF_RADIO->PCNF1 = (RADIO_PCNF1_WHITEEN_Enabled
						<< RADIO_PCNF1_WHITEEN_Pos)
				| (MAX_BUF_LEN << RADIO_PCNF1_MAXLEN_Pos)
				| (3UL << RADIO_PCNF1_BALEN_Pos);

	NRF_RADIO->RXADDRESSES = 1UL;
	NRF_RADIO->TXADDRESS = 0UL;

	NRF_RADIO->CRCCNF = (RADIO_CRCCNF_LEN_Three << RADIO_CRCCNF_LEN_Pos) |
		(RADIO_CRCCNF_SKIP_ADDR_Skip << RADIO_CRCCNF_SKIP_ADDR_Pos);
	NRF_RADIO->CRCPOLY = 0x100065B;

	/* FIXME: These header sizes only works for advertise channel PDUs */
	NRF_RADIO->PCNF0 = (1UL << RADIO_PCNF0_S0LEN_Pos) |      /* 1 byte */
				(6UL << RADIO_PCNF0_LFLEN_Pos) | /* 6 bits */
				(2UL << RADIO_PCNF0_S1LEN_Pos);  /* 2 bits */

	NRF_RADIO->SHORTS = (RADIO_SHORTS_READY_START_Enabled
					<< RADIO_SHORTS_READY_START_Pos)
					| (RADIO_SHORTS_END_DISABLE_Enabled
					<< RADIO_SHORTS_END_DISABLE_Pos);

	NRF_RADIO->INTENSET = RADIO_INTENSET_END_Msk;

	NVIC_SetPriority(RADIO_IRQn, IRQ_PRIORITY_HIGH);
	NVIC_ClearPendingIRQ(RADIO_IRQn);
	NVIC_EnableIRQ(RADIO_IRQn);

	NRF_RADIO->PACKETPTR = (uint32_t) buf;
	memset(buf, 0, sizeof(buf));

	status = STATUS_INITIALIZED;
	handler = NULL;

	return 0;
}

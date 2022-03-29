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
#include <stdbool.h>

#include <nrf51.h>
#include <nrf51_bitfields.h>

#include <blessed/errcodes.h>

#include "radio.h"
#include "nrf51822.h"

#define MAX_BUF_LEN			RADIO_MAX_PDU
#define MAX_PAYLOAD_LEN			(RADIO_MAX_PDU - 2)

#define STATUS_INITIALIZED		1
#define STATUS_RX			2
#define STATUS_TX			4
#define STATUS_BUSY			(STATUS_RX | STATUS_TX)

#define BASE_SHORTS							\
	(RADIO_SHORTS_READY_START_Enabled				\
		<< RADIO_SHORTS_READY_START_Pos)			\
	| (RADIO_SHORTS_END_DISABLE_Enabled				\
		<< RADIO_SHORTS_END_DISABLE_Pos)

static uint8_t inbuf[MAX_BUF_LEN] __attribute__ ((aligned));
static uint8_t *outbuf;

static radio_recv_cb_t recv_cb;
static radio_send_cb_t send_cb;

static volatile uint8_t status;
static volatile uint32_t flags;

static __inline int8_t ch2freq(uint8_t ch)
{
	/* nRF51 Series Reference Manual v2.1, section 16.2.19, page 91
	 * Link Layer specification section 1.4.1, Core 4.1, page 2502
	 *
	 * The nRF51822 is configured using the frequency offset from
	 * 2400 MHz.
	 */
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
	uint8_t old_status;
	bool active;

	NRF_RADIO->EVENTS_END = 0UL;

	active = false;
	old_status = status;
	status = STATUS_INITIALIZED;

	if (old_status & STATUS_RX) {
		if (flags & RADIO_FLAGS_TX_NEXT) {
			flags &= ~RADIO_FLAGS_TX_NEXT;
			status |= STATUS_TX;
			active = true;
			NRF_RADIO->PACKETPTR = (uint32_t) outbuf;
			NRF_RADIO->SHORTS &= ~RADIO_SHORTS_DISABLED_TXEN_Msk;
		}

		if (recv_cb)
			recv_cb(inbuf, NRF_RADIO->CRCSTATUS, active);
	} else if (old_status & STATUS_TX) {
		if (flags & RADIO_FLAGS_RX_NEXT) {
			flags &= ~RADIO_FLAGS_RX_NEXT;
			status |= STATUS_RX;
			active = true;
			NRF_RADIO->PACKETPTR = (uint32_t) inbuf;
			NRF_RADIO->SHORTS &= ~RADIO_SHORTS_DISABLED_RXEN_Msk;
		}

		if (send_cb)
			send_cb(active);
	}
}

int16_t radio_set_callbacks(radio_recv_cb_t rcb, radio_send_cb_t scb)
{
	recv_cb = rcb;
	send_cb = scb;

	return 0;
}

int16_t radio_prepare(uint8_t ch, uint32_t aa, uint32_t crcinit)
{
	int8_t freq;

	if (!(status & STATUS_INITIALIZED))
		return -ENOREADY;

	if (status & STATUS_BUSY)
		return -EBUSY;

	freq = ch2freq(ch);

	if (freq < 0)
		return -EINVAL;

	NRF_RADIO->DATAWHITEIV = ch & 0x3F;
	NRF_RADIO->FREQUENCY = freq;
	NRF_RADIO->BASE0 = (aa << 8) & 0xFFFFFF00;
	NRF_RADIO->PREFIX0 = (aa >> 24) & RADIO_PREFIX0_AP0_Msk;
	NRF_RADIO->CRCINIT = crcinit;

	return 0;
}

int16_t radio_send(const uint8_t *data, uint32_t f)
{
	status |= STATUS_TX;
	flags |= f;

	if (f & RADIO_FLAGS_RX_NEXT)
		NRF_RADIO->SHORTS |= RADIO_SHORTS_DISABLED_RXEN_Msk;

	NRF_RADIO->PACKETPTR = (uint32_t) data;
	NRF_RADIO->TASKS_TXEN = 1UL;

	return 0;
}

int16_t radio_recv(uint32_t f)
{
	status |= STATUS_RX;
	flags |= f;

	if (f & RADIO_FLAGS_TX_NEXT)
		NRF_RADIO->SHORTS |= RADIO_SHORTS_DISABLED_TXEN_Msk;

	NRF_RADIO->PACKETPTR = (uint32_t) inbuf;
	NRF_RADIO->TASKS_RXEN = 1UL;

	return 0;
}

int16_t radio_stop(void)
{
	if (!(status & STATUS_BUSY))
		return -ENOREADY;

	flags = 0;
	NRF_RADIO->SHORTS = BASE_SHORTS;

	NRF_RADIO->EVENTS_DISABLED = 0UL;
	NRF_RADIO->TASKS_DISABLE = 1UL;
	while (NRF_RADIO->EVENTS_DISABLED == 0UL);

	status &= ~STATUS_BUSY;

	return 0;
}

void radio_set_out_buffer(uint8_t *buf)
{
	outbuf = buf;
}

int16_t radio_set_tx_power(radio_power_t power)
{
	/* nRF51 Series Reference Manual v2.1, section 16.2.6, page 86 */
	switch(power) {
	case RADIO_POWER_4_DBM:
		NRF_RADIO->TXPOWER = RADIO_TXPOWER_TXPOWER_Pos4dBm
						<< RADIO_TXPOWER_TXPOWER_Pos;
		return 0;
	case RADIO_POWER_0_DBM:
		NRF_RADIO->TXPOWER = RADIO_TXPOWER_TXPOWER_0dBm
						<< RADIO_TXPOWER_TXPOWER_Pos;
		return 0;
	case RADIO_POWER_N4_DBM:
		NRF_RADIO->TXPOWER = RADIO_TXPOWER_TXPOWER_Neg4dBm
						<< RADIO_TXPOWER_TXPOWER_Pos;
		return 0;
	case RADIO_POWER_N8_DBM:
		NRF_RADIO->TXPOWER = RADIO_TXPOWER_TXPOWER_Neg8dBm
						<< RADIO_TXPOWER_TXPOWER_Pos;
		return 0;
	case RADIO_POWER_N12_DBM:
		NRF_RADIO->TXPOWER = RADIO_TXPOWER_TXPOWER_Neg12dBm
						<< RADIO_TXPOWER_TXPOWER_Pos;
		return 0;
	case RADIO_POWER_N16_DBM:
		NRF_RADIO->TXPOWER = RADIO_TXPOWER_TXPOWER_Neg16dBm
						<< RADIO_TXPOWER_TXPOWER_Pos;
		return 0;
	case RADIO_POWER_N20_DBM:
		NRF_RADIO->TXPOWER = RADIO_TXPOWER_TXPOWER_Neg20dBm
						<< RADIO_TXPOWER_TXPOWER_Pos;
		return 0;
	case RADIO_POWER_N30_DBM:
		NRF_RADIO->TXPOWER = RADIO_TXPOWER_TXPOWER_Neg30dBm
						<< RADIO_TXPOWER_TXPOWER_Pos;
		return 0;
	}

	return -EINVAL;
}

int16_t radio_init(void)
{
	if (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0UL) {
		NRF_CLOCK->TASKS_HFCLKSTART = 1UL;
		while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0UL);
	}

	/* nRF51 Series Reference Manual v2.1, section 6.1.1, page 18
	 * PCN-083 rev.1.1
	 *
	 * Fine tune BLE deviation parameters.
	 */
	if ((NRF_FICR->OVERRIDEEN & FICR_OVERRIDEEN_BLE_1MBIT_Msk)
					== (FICR_OVERRIDEEN_BLE_1MBIT_Override
					<< FICR_OVERRIDEEN_BLE_1MBIT_Pos)) {
		NRF_RADIO->OVERRIDE0 = NRF_FICR->BLE_1MBIT[0];
		NRF_RADIO->OVERRIDE1 = NRF_FICR->BLE_1MBIT[1];
		NRF_RADIO->OVERRIDE2 = NRF_FICR->BLE_1MBIT[2];
		NRF_RADIO->OVERRIDE3 = NRF_FICR->BLE_1MBIT[3];
		NRF_RADIO->OVERRIDE4 = NRF_FICR->BLE_1MBIT[4] | 0x80000000;
	}

	/* nRF51 Series Reference Manual v2.1, section 16.2.7, page 86 */
	NRF_RADIO->MODE = RADIO_MODE_MODE_Ble_1Mbit << RADIO_MODE_MODE_Pos;

	/* Link Layer specification section 4.1, Core 4.1, page 2524
	 * nRF51 Series Reference Manual v2.1, section 16.2.7, page 92
	 *
	 * Set the inter frame space (T_IFS) to 150 us.
	 */
	NRF_RADIO->TIFS = 150;

	/* nRF51 Series Reference Manual v2.1, section 16.2.9, page 88
	 *
	 * Enable data whitening, set the maximum payload length and set the
	 * access address size (3 + 1 octets).
	 */
	NRF_RADIO->PCNF1 =
		(RADIO_PCNF1_WHITEEN_Enabled << RADIO_PCNF1_WHITEEN_Pos) |
		(MAX_PAYLOAD_LEN << RADIO_PCNF1_MAXLEN_Pos) |
		(3UL << RADIO_PCNF1_BALEN_Pos);

	/* nRF51 Series Reference Manual v2.1, section 16.1.4, page 74
	 * nRF51 Series Reference Manual v2.1, section 16.2.14-15, pages 89-90
	 *
	 * Preset the address to use when receive and transmit packets (logical
	 * address 0, which is assembled by base address BASE0 and prefix byte
	 * PREFIX0.AP0.
	 */
	NRF_RADIO->RXADDRESSES = 1UL;
	NRF_RADIO->TXADDRESS = 0UL;

	/* nRF51 Series Reference Manual v2.1, section 16.1.7, page 76
	 * nRF51 Series Reference Manual v2.1, sections 16.1.16-17, page 90
	 *
	 * Configure the CRC length (3 octets), polynominal and set it to
	 * ignore the access address when calculate the CRC.
	 */
	NRF_RADIO->CRCCNF =
		(RADIO_CRCCNF_LEN_Three << RADIO_CRCCNF_LEN_Pos) |
		(RADIO_CRCCNF_SKIP_ADDR_Skip << RADIO_CRCCNF_SKIP_ADDR_Pos);
	NRF_RADIO->CRCPOLY = 0x100065B;

	/* nRF51 Series Reference Manual v2.1, section 16.1.2, page 74
	 * nRF51 Series Reference Manual v2.1, sections 16.1.8, page 87
	 * Link Layer specification section 2.3, Core 4.1, page 2504
	 * Link Layer specification section 2.4, Core 4.1, page 2511
	 *
	 * Configure the header size. The nRF51822 has 3 fields before the
	 * payload field: S0, LENGTH and S1. These fields can be used to store
	 * the PDU header.
	 */
	NRF_RADIO->PCNF0 = (1UL << RADIO_PCNF0_S0LEN_Pos)
			| (8UL << RADIO_PCNF0_LFLEN_Pos)
			| (0UL << RADIO_PCNF0_S1LEN_Pos);

	/* nRF51 Series Reference Manual v2.1, section 16.1.8, page 76
	 * nRF51 Series Reference Manual v2.1, section 16.1.10-11, pages 78-80
	 * nRF51 Series Reference Manual v2.1, section 16.2.1, page 85
	 *
	 * Enable READY_START short: when the READY event happens, initialize
	 * the START task.
	 *
	 * Enable END_DISABLE short: when the END event happens, initialize the
	 * DISABLE task.
	 */
	NRF_RADIO->SHORTS = BASE_SHORTS;

	/* Trigger RADIO interruption when an END event happens */
	NRF_RADIO->INTENSET = RADIO_INTENSET_END_Msk;

	NVIC_SetPriority(RADIO_IRQn, IRQ_PRIORITY_HIGH);
	NVIC_ClearPendingIRQ(RADIO_IRQn);
	NVIC_EnableIRQ(RADIO_IRQn);

	radio_set_callbacks(NULL, NULL);
	radio_set_tx_power(RADIO_POWER_0_DBM);
	radio_set_out_buffer(NULL);

	NRF_RADIO->PACKETPTR = (uint32_t) inbuf;
	memset(inbuf, 0, sizeof(inbuf));

	status = STATUS_INITIALIZED;

	return 0;
}

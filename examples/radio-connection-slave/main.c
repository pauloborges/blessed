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

#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include <nrf51.h>
#include <nrf51_bitfields.h>

#include <blessed/timer.h>
#include <blessed/log.h>

#include "radio.h"

#define PDU_TYPE_CONNECT_REQ		0x05

#define ADV_CHANNEL_AA			0x8E89BED6
#define ADV_CHANNEL_CRC			0x555555

#define ADV_EVENT			TIMER_MILLIS(1280)
#define ADV_INTERVAL			TIMER_MILLIS(10)
#define T_IFS				500

struct __attribute__ ((packed)) ll_pdu_connect_req {
	uint8_t type:4;
	uint8_t _rfu_0:2;
	uint8_t tx_add:1;
	uint8_t rx_add:1;
	uint8_t length:6;
	uint8_t _rfu_1:2;

	uint8_t inita[6];
	uint8_t adva[6];

	uint32_t aa;
	uint32_t crcinit:24;
	uint8_t winsize;
	uint16_t winoffset;
	uint16_t interval;
	uint16_t latency;
	uint16_t timeout;
	uint64_t chmap:40;
	uint8_t hop:5;
	uint8_t sca:3;
};

struct __attribute__ ((packed)) ll_pdu_adv_ind {
	uint8_t type:4;
	uint8_t _rfu_0:2;
	uint8_t tx_add:1;
	uint8_t rx_add:1;
	uint8_t length:6;
	uint8_t _rfu_1:2;

	uint8_t adva[6];
	uint8_t advdata[31];
};

struct __attribute__ ((packed)) ll_pdu_data {
	uint8_t llid:2;
	uint8_t nesn:1;
	uint8_t sn:1;
	uint8_t md:1;
	uint8_t _rfu_1:3;
	uint8_t length:5;
	uint8_t _rfu_2:3;
	uint8_t payload[34];
	uint8_t mic[3];
};

struct conn_context {
	uint32_t aa;
	uint32_t crcinit;
	uint32_t winsize;		/* us */
	uint32_t timeout;		/* us */
	uint32_t interval;		/* us */
	uint64_t chmap;
	int chmapcnt;
	uint8_t hop;
	uint8_t ch;
};

static uint8_t adv_ind[] = {
	0x40, 0x0F,					/* Header */
	0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,		/* AdvA */
	0x08,						/* AD Length */
	0x08,						/* AD Type */
	0x62, 0x6c, 0x65, 0x73, 0x73, 0x65, 0x64	/* AD Data */
};

static struct ll_pdu_data pdu_data_empty = {
	.llid	= 0x01,
	.nesn	= 0x00,
	.sn	= 0x00,
	.md	= 0x00,
	.length	= 0x00
};

static uint8_t channels[] = { 37, 38, 39 };
static int8_t idx;

static int16_t t_repeated;
static int16_t t_singleshot;
static int16_t t_ifs;

static struct conn_context conn;

static void init_advertise(void);

static void select_next_ch(void)
{
	uint8_t ch = (conn.ch + conn.hop) % 37;

	if (!(conn.chmap & (1ULL << ch))) {
		uint8_t remap = ch % conn.chmapcnt;
		uint8_t idx = 0;

		while (remap > 0) {
			if (conn.chmap & (1 << idx))
				remap--;
			idx++;
		}

		ch = idx;
	}

	conn.ch = ch;
}

static inline uint16_t get_on_air_duration(const uint8_t *pdu)
{
	/* preamble (1 octet) + aa (4 octets) + header (2 octets)
	 * + payload (len field) + crc (3 octets) */
	return (10 + pdu[1]) * 8;
}

////////////////////////////////////////////////////////////////////////////////
// CONNECTION
////////////////////////////////////////////////////////////////////////////////

static void conn_ifs_cb(void)
{
	log_string("  end\r\n");
	radio_stop();
}

static void conn_evt_recv_cb(const uint8_t *pdu, bool crc, bool active)
{
	// NRF_TIMER1->TASKS_CAPTURE[3] = 1UL;
	// log_uint(NRF_TIMER1->CC[3]);
	// log_newline();

	log_string("  pdu\r\n");
	timer_stop(t_ifs);
}

static inline void conn_evt_cb(void)
{
	// log_uint(NRF_RTC0->COUNTER);
	// log_newline();

	select_next_ch();
	radio_prepare(conn.ch, conn.aa, conn.crcinit);
	radio_recv(RADIO_FLAGS_TX_NEXT);

	timer_start(t_ifs, T_IFS, conn_ifs_cb);

	log_string("conn evt\r\n");
}

static void first_conn_evt_cb(void)
{
	timer_start(t_repeated, conn.interval, conn_evt_cb);
	radio_set_callbacks(conn_evt_recv_cb, NULL);

	conn_evt_cb();
}

////////////////////////////////////////////////////////////////////////////////
// TRANSMIT WINDOW
////////////////////////////////////////////////////////////////////////////////

static void transmit_window_recv_cb(const uint8_t *pdu, bool crc, bool active)
{
	// Received a packet in the transmit window. The reply is on the go.
	// 1. stop the end transmit window timer
	// 2. start repeated timer the first connection event

	// NRF_RTC0->TASKS_START = 1UL;

	uint32_t us = conn.interval - get_on_air_duration(pdu) - 6 - 150 - 500;

	timer_start(t_singleshot, us, first_conn_evt_cb);
	timer_stop(t_ifs);

	log_string("  pdu\r\n");
}

static void end_transmit_window_cb(void)
{
	// End of the transmit window. No packets were received.
	// 1. stop the radio
	// 2. go back to advertise state

	log_string("  end\r\n");

	radio_stop();
	init_advertise();
}

static void init_transmit_window_cb(void)
{
	// Start of the transmit window:
	// 1. start receiving packets (with flag to TX right after)
	// 2. start timer to get the end of the transmit window

	radio_recv(RADIO_FLAGS_TX_NEXT);
	timer_start(t_ifs, conn.winsize, end_transmit_window_cb);

	log_string("transmit win\r\n");
}

static inline void init_transmit_window(struct ll_pdu_connect_req *pdu)
{
	timer_stop(t_singleshot);
	timer_stop(t_repeated);

	timer_start(t_singleshot, 1250 + pdu->winoffset * 1250 - 150,
						init_transmit_window_cb);

	log_string("CONNECT_REQ\r\n");

	conn.ch = 0;

	conn.aa = pdu->aa;
	conn.crcinit = pdu->crcinit;

	conn.winsize = pdu->winsize * 1250;
	conn.interval = pdu->interval * 1250;
	conn.timeout = pdu->timeout * 10000;

	conn.hop = pdu->hop;
	conn.chmap = pdu->chmap;
	conn.chmapcnt = __builtin_popcountll(conn.chmap);

	radio_set_out_buffer((uint8_t *) (&pdu_data_empty));
	radio_set_callbacks(transmit_window_recv_cb, NULL);

	select_next_ch();
	radio_prepare(conn.ch, conn.aa, conn.crcinit);

	log_printf("int %u us\r\n", conn.interval);
}

////////////////////////////////////////////////////////////////////////////////
// ADVERTISING
////////////////////////////////////////////////////////////////////////////////

static void t_ifs_cb(void)
{
	radio_stop();
}

static void adv_recv_cb(const uint8_t *pdu, bool crc, bool active)
{
	struct ll_pdu_connect_req *conn_req = (struct ll_pdu_connect_req *) pdu;
	struct ll_pdu_adv_ind *adv = (struct ll_pdu_adv_ind *) adv_ind;

	timer_stop(t_ifs);

	if (conn_req->type != PDU_TYPE_CONNECT_REQ)
		return;

	if (conn_req->rx_add != adv->tx_add)
		return;

	if (memcmp(conn_req->adva, adv->adva, 6))
		return;

	init_transmit_window(conn_req);
}

static void adv_send_cb(bool active)
{
	timer_start(t_ifs, T_IFS, t_ifs_cb);
}

static void adv_interval_cb(void)
{
	radio_stop();
	radio_prepare(channels[idx++], ADV_CHANNEL_AA, ADV_CHANNEL_CRC);
	radio_send(adv_ind, RADIO_FLAGS_RX_NEXT);

	if (idx < 3)
		timer_start(t_singleshot, ADV_INTERVAL, adv_interval_cb);
}

static void adv_event_cb(void)
{
	idx = 0;
	adv_interval_cb();
}

static void init_advertise(void)
{
	DBG("Advertising ADV_IND PDUs");
	DBG("Time between PDUs:   %u ms", ADV_INTERVAL / 1000);
	DBG("Time between events: %u ms", ADV_EVENT / 1000);

	timer_stop(t_singleshot);
	timer_stop(t_repeated);
	timer_stop(t_ifs);

	radio_set_callbacks(adv_recv_cb, adv_send_cb);
	timer_start(t_repeated, ADV_EVENT, adv_event_cb);
	adv_event_cb();
}

////////////////////////////////////////////////////////////////////////////////

int main(void)
{
	log_init();
	timer_init();
	radio_init();

	/* Initialize TIMER1 */
	// NRF_TIMER1->MODE = TIMER_MODE_MODE_Timer;
	// NRF_TIMER1->BITMODE = TIMER_BITMODE_BITMODE_16Bit;
	// NRF_TIMER1->PRESCALER = 4; /* 1 MHz */
	// NRF_TIMER1->TASKS_START = 1UL;

	/* Initialize PPI */
	// NRF_PPI->CH[0].EEP = (uint32_t) (&NRF_RADIO->EVENTS_END);
	// NRF_PPI->CH[0].TEP = (uint32_t) (&NRF_TIMER1->TASKS_CLEAR);
	// NRF_PPI->CHEN = (PPI_CHEN_CH0_Enabled << PPI_CHEN_CH0_Pos);

	/* Initialize RTC0 */
	// NRF_CLOCK->LFCLKSRC = CLOCK_LFCLKSRC_SRC_Xtal
	// 				<< CLOCK_LFCLKSRC_SRC_Pos;
	// NRF_CLOCK->TASKS_LFCLKSTART = 1;
	// while (!NRF_CLOCK->EVENTS_LFCLKSTARTED);
	// NRF_RTC0->PRESCALER = 7; /* 4098 Hz */

	t_singleshot = timer_create(TIMER_SINGLESHOT);
	t_repeated = timer_create(TIMER_REPEATED);
	t_ifs = timer_create(TIMER_SINGLESHOT);

	init_advertise();

	while (1);

	return 0;
}

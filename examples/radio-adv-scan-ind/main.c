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

#define ENABLE_LOG

#include <string.h>
#include <stdint.h>

#include <blessed/timer.h>
#include <blessed/log.h>

#include <nrf51.h>
#include <app_timer.h>

#include "radio.h"

/* Link Layer specification Section 2.1.2, Core 4.1 page 2503 */
#define ADV_CHANNEL_AA			0x8E89BED6

/* Link Layer specification Section 3.1.1, Core 4.1 page 2522 */
#define ADV_CHANNEL_CRC			0x555555

#define ADV_EVENT			150
#define ADV_INTERVAL			10
#define IFS				150	/* us */

/* Link Layer specification section 2.3, Core 4.1, page 2504
 * Link Layer specification section 2.3.1.4, Core 4.1, page 2507
 *
 * ADV_SCAN_IND PDU (39 octets):
 * +--------+--------+-----------+
 * | Header |  AdvA  |  AdvData  |
 * +--------+--------+-----------+
 *  2 octets 6 octets 0-31 octets
 *
 * Header: PDU Type=ADV_SCAN_IND, TxAddr=1, Length=0x16
 * AdvA: FF:EE:DD:CC:BB:AA
 * AdvData: AD structure:
 * LEN: 15 bytes | LOCAL NAME: 0x09 | DATA: "blessed device"
 * 62 6C 65 73 73 65 64 20 64 65 76 69 63 65
 */

static uint8_t pdu[] = {
	0x40, 0x0F, /* Header: see ll_pdu_hdr */
		0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, /* 6 octets: AdvA */
		/* Remaining: AdvData using EIR/ADV Data Format */
		/* EIR: 0x0B | 0x09 | blessed device */
		0x08,	/* EIR Structure 1 Length */
			/* Type: Complete Local Name */
			0x09,
			/* Data: peek-a- */
			0x70, 0x65, 0x65, 0x6B, 0x2D, 0x61, 0x2D };

static uint8_t pdu2[] = {
	0x44, 0x0B, /* Header: see ll_pdu_hdr */
		0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, /* 6 octets: AdvA */
		/* Remaining: AdvData using EIR/ADV Data Format */
		/* EIR: 0x0B | 0x09 | blessed device */
		0x04,	/* EIR Structure 1 Length */
			/* Type: Complete Local Name */
			0x09,
			/* Data: boo */
			0x62, 0x6f, 0x6f };

static uint8_t channels[] = { 37, 38, 39 };
static int8_t idx = 0;

static int16_t adv_event;

static void adv_event_timeout(void *user_data)
{
	int err;

	if (idx == 2)
		idx = 0;
	else
		idx++;

	err = radio_send(channels[idx], ADV_CHANNEL_AA,
				ADV_CHANNEL_CRC, pdu, sizeof(pdu), true);
	if (err < 0)
		ERROR("radio_send_then_recv() returned %d\n", -err);
}

static inline void recv(const uint8_t *pkt)
{
	if (pkt[0] != 0x83)
		return;

	radio_reply(pdu2, sizeof(pdu2));
}

void radio_handler(uint8_t evt, void *data)
{
	switch (evt) {
	case RADIO_EVT_TX_COMPLETED:
		break;
	case RADIO_EVT_TX_COMPLETED_RX_NEXT:
		break;
	case RADIO_EVT_RX_COMPLETED:
		recv(data);
		break;
	}
}

int main(void)
{
	log_init();
	timer_init();
	radio_init(radio_handler);

	adv_event = timer_create(TIMER_REPEATED, adv_event_timeout);

	DBG("Start to advertise");

	timer_start(adv_event, ADV_EVENT, NULL);

	while (1);

	return 0;
}

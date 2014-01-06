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
#include <stdio.h>
#include <string.h>

#include "errcodes.h"
#include "log.h"
#include "radio.h"
#include "timer.h"

#include "ble-common.h"
#include "ll.h"

/* Link Layer specification Section 2.1, Core 4.1 page 2503 */
#define LL_MTU				39

/* Link Layer specification Section 2.3, Core 4.1 pages 2504-2505 */
#define LL_MTU_ADV			37

/* Link Layer specification Section 2.1.2, Core 4.1 page 2503 */
#define LL_ACCESS_ADDRESS_ADV		0x8E89BED6

/* Link Layer specification Section 3.1.1, Core 4.1 page 2522 */
#define LL_CRCINIT_ADV			0x555555

/* Link Layer specification Section 1.1, Core 4.1 page 2499 */
typedef enum ll_states {
	LL_STATE_STANDBY,
	LL_STATE_ADVERTISING,
	LL_STATE_SCANNING,
	LL_INITIATING_SCANNING,
	LL_CONNECTION_SCANNING,
} ll_states_t;

/* Link Layer specification Section 2.3, Core 4.1 page 2505 */
typedef enum ll_pdu_type {
	LL_PDU_ADV_IND,		/* connectable undirected */
	LL_PDU_ADV_DIRECT_IND,	/* connectable directed */
	LL_PDU_ADV_NONCONN_IND,	/* non-connectable undirected */
	LL_PDU_SCAN_REQ,
	LL_PDU_SCAN_RSP,
	LL_PDU_CONNECT_REQ,
	LL_PDU_ADV_SCAN_IND	/* scannable undirected */
} ll_pdu_type_t;

/* Link Layer specification Section 2.3, Core 4.1 pages 2504-2505 */
typedef struct ll_pdu_adv {
	uint8_t pdu_type:4;
	uint8_t _rfu_0:2;
	uint8_t tx_add:1;
	uint8_t rx_add:1;
	uint8_t length:6; /* 6 <= length <= 37 */
	uint8_t _rfu_1:2;
	uint8_t payload[LL_MTU_ADV];
} __attribute__ ((packed)) ll_pdu_adv_t;

static const bdaddr_t *laddr;
static ll_states_t current_state;

static uint8_t adv_chs[] = { 37, 38, 39 };
static uint8_t adv_ch_idx = 0;

static int16_t t_adv_event;
static uint32_t t_adv_event_interval;
static int16_t t_adv_pdu;
static uint32_t t_adv_pdu_interval;

static ll_pdu_adv_t pdu_adv;

static void t_adv_event_cb(void *user_data)
{
	adv_ch_idx = 0;

	radio_send(adv_chs[adv_ch_idx++], LL_ACCESS_ADDRESS_ADV, LL_CRCINIT_ADV,
					(uint8_t *) &pdu_adv, sizeof(pdu_adv));

	timer_start(t_adv_pdu, t_adv_pdu_interval, NULL);
}

static void t_adv_pdu_cb(void *user_data)
{
	radio_send(adv_chs[adv_ch_idx++], LL_ACCESS_ADDRESS_ADV, LL_CRCINIT_ADV,
					(uint8_t *) &pdu_adv, sizeof(pdu_adv));

	if (adv_ch_idx < 3)
		timer_start(t_adv_pdu, t_adv_pdu_interval, NULL);
}

int16_t ll_advertise_start(ll_adv_type_t type, uint8_t *data, uint8_t len)
{
	if (current_state != LL_STATE_STANDBY)
		return -ENOREADY;

	memset(&pdu_adv, 0, sizeof(pdu_adv));

	switch (type) {

	case LL_ADV_NONCONN_UNDIR:
		pdu_adv.pdu_type = LL_PDU_ADV_NONCONN_IND;
		pdu_adv.tx_add = laddr->type;
		pdu_adv.length = len + sizeof(laddr->addr);

		memcpy(pdu_adv.payload, laddr->addr, sizeof(laddr->addr));
		memcpy(pdu_adv.payload + sizeof(laddr->addr), data, len);

		t_adv_event_interval = 100; /* <= 1024ms, Sec 4.4.2.2 pag 2528 */
		t_adv_pdu_interval = 5; /* <= 10ms Sec 4.4.2.6 pag 2534*/

		break;

	case LL_ADV_SCAN_UNDIR:
	case LL_ADV_CONN_UNDIR:
	case LL_ADV_CONN_DIR:
		/* Not implemented */
		return -EINVAL;
		break;
	}

	DBG("Starting advertise: PDU interval %ums, event interval %ums",
				t_adv_pdu_interval, t_adv_event_interval);

	current_state = LL_STATE_ADVERTISING;
	adv_ch_idx = 0;

	timer_start(t_adv_event, t_adv_event_interval, NULL);
	timer_start(t_adv_pdu, t_adv_pdu_interval, NULL);

	radio_send(adv_chs[adv_ch_idx++], LL_ACCESS_ADDRESS_ADV, LL_CRCINIT_ADV,
					(uint8_t *) &pdu_adv, sizeof(pdu_adv));

	return 0;
}

int16_t ll_advertise_stop()
{
	if (current_state != LL_STATE_ADVERTISING)
		return -ENOREADY;

	timer_stop(t_adv_pdu);
	timer_stop(t_adv_event);

	current_state = LL_STATE_STANDBY;

	return 0;
}

int16_t ll_init(const bdaddr_t *addr)
{
	if (addr == NULL)
		return -EINVAL;

	log_init();
	timer_init();
	radio_init();

	laddr = addr;
	current_state = LL_STATE_STANDBY;

	t_adv_event = timer_create(TIMER_REPEATED, t_adv_event_cb);
	t_adv_pdu = timer_create(TIMER_SINGLESHOT, t_adv_pdu_cb);

	return 0;
}

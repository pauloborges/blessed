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
#include <stdint.h>
#include <stdbool.h>

#include <blessed/log.h>
#include <blessed/evtloop.h>

#include "radio.h"
#include "timer.h"

/* Link Layer specification Section 2.1.2, Core 4.1 page 2503 */
#define ADV_CHANNEL_AA			0x8E89BED6

/* Link Layer specification Section 3.1.1, Core 4.1 page 2522 */
#define ADV_CHANNEL_CRC			0x555555

#define ADV_EVENT			TIMER_MILLIS(1280)
#define ADV_INTERVAL			TIMER_MILLIS(10)

/* Link Layer specification section 2.3, Core 4.1, page 2504
 * Link Layer specification section 2.3.1.3, Core 4.1, page 2507
 *
 * ADV_NONCONN_IND PDU (39 octets):
 * +--------+--------+---------+
 * | Header |  AdvA  | AdvData |
 * +--------+--------+---------+
 *  2 octets 6 octets 31 octets
 *
 * Header:	<PDU Type=ADV_NONCONN_IND, TxAddr=RANDOM, Length=22>
 * AdvA:	<FF:EE:DD:CC:BB:AA>
 * AdvData:	<AD: Len=15, Type="Complete Local Name", Data="blessed device">
 */
static uint8_t adv_nonconn_ind[] = {
	0x42, 0x16,					/* Header */
	0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,		/* AdvA */
	0x0F,						/* AD Length */
	0x09,						/* AD Type */
	0x62, 0x6C, 0x65, 0x73, 0x73, 0x65, 0x64, 0x20,	/* AD Data */
	0x64, 0x65, 0x76, 0x69, 0x63, 0x65
};

static uint8_t channels[] = { 37, 38, 39 };
static uint8_t idx;

static int16_t adv_event;
static int16_t adv_interval;


static void adv_interval_timeout(void)
{
	radio_prepare(channels[idx++], ADV_CHANNEL_AA, ADV_CHANNEL_CRC);
	radio_send(adv_nonconn_ind, 0);

	if (idx < 3)
		timer_start(adv_interval, ADV_INTERVAL, adv_interval_timeout);
}

static void adv_event_timeout(void)
{
	idx = 0;
	adv_interval_timeout();
}

int main(void)
{
	log_init();
	timer_init();
	radio_init();

	adv_interval = timer_create(TIMER_SINGLESHOT);
	adv_event = timer_create(TIMER_REPEATED);

	DBG("Advertising ADV_NONCONN_IND PDUs");
	DBG("Time between PDUs:   %u ms", ADV_INTERVAL / 1000);
	DBG("Time between events: %u ms", ADV_EVENT / 1000);

	timer_start(adv_event, ADV_EVENT, adv_event_timeout);
	adv_event_timeout();

	evt_loop_run();

	return 0;
}

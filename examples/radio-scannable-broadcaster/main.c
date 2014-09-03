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

/* The time between packets is 150 us. But we are only notified when a
 * transmission or reception is completed. So we need to consider the time to
 * receive the packet. Empirically, a SCAN_REQ roughly took 100 us to be totally
 * received, which gives us a total timeout of 250 us. But we will consider a
 * bigger window to guarantee the reception.
 */
#define T_IFS				500

#define PDU_TYPE_SCAN_REQ		0x03

/* Link Layer specification section 2.3, Core 4.1, page 2504
 * Link Layer specification section 2.3.1.4, Core 4.1, page 2507
 * Link Layer specification section 2.3.2.1, Core 4.1, page 2508
 * Link Layer specification section 2.3.2.2, Core 4.1, page 2508
 *
 * ADV_SCAN_IND PDU (39 octets):
 * +--------+--------+-----------+
 * | Header |  AdvA  |  AdvData  |
 * +--------+--------+-----------+
 *  2 octets 6 octets 0-31 octets
 *
 * Header:	<PDU Type=ADV_SCAN_IND, TxAddr=RANDOM, Length=15>
 * AdvA:	<FF:EE:DD:CC:BB:AA>
 * AdvData:	<AD: Len=8, Type="Shortened Local Name", Data="blessed">
 *
 * SCAN_REQ PDU (14 octets):
 * +--------+--------+--------+
 * | Header | ScanA  |  AdvA  |
 * +--------+--------+--------+
 *  2 octets 6 octets 6 octets
 *
 * Header:	<PDU Type=SCAN_REQ, TxAddr=???, RxAddr=RANDOM, Length=12>
 * ScanA:	<??:??:??:??:??:??>
 * AdvA:	<FF:EE:DD:CC:BB:AA>
 *
 * SCAN_RSP PDU (39 octets):
 * +--------+--------+---------------+
 * | Header |  AdvA  |  ScanRspData  |
 * +--------+--------+---------------+
 *  2 octets 6 octets   0-31 octets
 *
 * Header:	<PDU Type=SCAN_RSP, TxAddr=RANDOM, Length=22>
 * AdvA:	<FF:EE:DD:CC:BB:AA>
 * ScanRspData:	<AD: Len=15, Type="Complete Local Name", Data="blessed device">
 */

static uint8_t adv_scan_ind[] = {
	0x46, 0x0F,					/* Header */
	0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,		/* AdvA */
	0x08,						/* AD Length */
	0x08,						/* AD Type */
	0x62, 0x6c, 0x65, 0x73, 0x73, 0x65, 0x64	/* AD Data */
};

static uint8_t scan_rsp[] = {
	0x44, 0x16,					/* Header */
	0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,		/* AdvA */
	0x0F,						/* AD Length */
	0x09,						/* AD Type */
	0x62, 0x6c, 0x65, 0x73, 0x73, 0x65, 0x64, 0x20,	/* AD Data */
	0x64, 0x65, 0x76, 0x69, 0x63, 0x65
};

static uint8_t channels[] = { 37, 38, 39 };
static int8_t idx;

static int16_t adv_event;
static int16_t adv_interval;
static int16_t t_ifs;

void t_ifs_timeout(void)
{
	radio_stop();
}

static void adv_interval_timeout(void)
{
	radio_stop();
	radio_prepare(channels[idx++], ADV_CHANNEL_AA, ADV_CHANNEL_CRC);
	radio_send(adv_scan_ind, RADIO_FLAGS_RX_NEXT);

	if (idx < 3)
		timer_start(adv_interval, ADV_INTERVAL, adv_interval_timeout);
}

static void adv_event_timeout(void)
{
	idx = 0;
	adv_interval_timeout();
}

static void radio_recv_cb(const uint8_t *pdu, bool crc, bool active)
{
	const uint8_t *tgt_addr;
	const uint8_t *our_addr;
	uint8_t tgt_rxadd;
	uint8_t our_txadd;

	/* Start replying as soon as possible, if there is something wrong,
	 * cancel it. Also, stop the IFS timer.
	 */
	radio_send(scan_rsp, 0);
	timer_stop(t_ifs);

	/* If the PDU isn't SCAN_REQ, ignore the packet */
	if ((pdu[0] & 0xF) != PDU_TYPE_SCAN_REQ)
		goto stop;

	tgt_addr = pdu + 8;
	our_addr = adv_scan_ind + 2;

	/* If AdvA isn't our address, ignore the packet */
	if (memcmp(tgt_addr, our_addr, 6))
		goto stop;


	tgt_rxadd = (pdu[0] & 0x80) >> 7;
	our_txadd = (adv_scan_ind[0] & 0x40) >> 6;

	/* If RxAdd isn't our address type, ignore the packet */
	if (tgt_rxadd != our_txadd)
		goto stop;

	return;

stop:
	radio_stop();
}

static void radio_send_cb(bool active)
{
	timer_start(t_ifs, T_IFS, t_ifs_timeout);
}

int main(void)
{
	log_init();
	timer_init();
	radio_init();
	radio_set_callbacks(radio_recv_cb, radio_send_cb);

	adv_interval = timer_create(TIMER_SINGLESHOT);
	adv_event = timer_create(TIMER_REPEATED);
	t_ifs = timer_create(TIMER_SINGLESHOT);

	DBG("Advertising ADV_SCAN_IND PDUs");
	DBG("Time between PDUs:   %u us", ADV_INTERVAL);
	DBG("Time between events: %u us", ADV_EVENT);

	timer_start(adv_event, ADV_EVENT, adv_event_timeout);
	adv_event_timeout();

	evt_loop_run();

	return 0;
}

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

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include <blessed/log.h>
#include <blessed/evtloop.h>

#include "radio.h"
#include "timer.h"

#define ADV_CHANNEL_AA			0x8E89BED6
#define ADV_CHANNEL_CRC			0x555555

#define ADV_IND				0
#define ADV_SCAN_IND			6

#define SCAN_WINDOW			TIMER_SECONDS(10)
#define T_IFS				500

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
static uint8_t scan_req[] = {
	0x43, 0x0C,					/* Header */
	0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,		/* ScanA */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00		/* AdvA (empty) */
};

static uint8_t channels[] = { 37, 38, 39 };
static uint8_t idx = 0xFF;

static int16_t scan_window;
static int16_t t_ifs;

static char *pdus[] = {
	"ADV_IND", "ADV_DIRECT_IND", "ADV_NONCONN_IND", "SCAN_REQ",
	"SCAN_RSP", "CONNECT_REQ", "ADV_SCAN_IND"
};

static void t_ifs_timeout(void)
{
	radio_stop();
	radio_recv(RADIO_FLAGS_TX_NEXT);
}

static void scan_window_timeout(void)
{
	idx = (uint8_t) (idx + 1) % sizeof(channels);

	timer_stop(t_ifs);

	radio_stop();
	radio_prepare(channels[idx], ADV_CHANNEL_AA, ADV_CHANNEL_CRC);
	radio_recv(RADIO_FLAGS_TX_NEXT);
}

static void radio_recv_cb(const uint8_t *pdu, bool crc, bool active)
{
	uint8_t pdu_type = pdu[0] & 0xF;
	uint8_t length = pdu[1] & 0x3F;

	timer_stop(t_ifs);

	if (!crc) {
		log_printf("ch%u BAD CRC\r\n", channels[idx]);

		goto next_recv;
	}

	/* The minimum allowed payload is 6 bytes */
	if (length < 6) {
		log_printf("ch%u BAD LENGTH %u\r\n", channels[idx], length);

		goto next_recv;
	}

	log_printf("ch%u %s\r\n", channels[idx], pdus[pdu_type]);

	if (pdu_type == ADV_IND || pdu_type == ADV_SCAN_IND) {
		/* Copy pdu's AdvA to scan_req's AdvA */
		scan_req[8] = pdu[2];
		scan_req[9] = pdu[3];
		scan_req[10] = pdu[4];
		scan_req[11] = pdu[5];
		scan_req[12] = pdu[6];
		scan_req[13] = pdu[7];

		/* Copy pdu's TxAdd to scan_req's RxAdd */
		scan_req[0] |= (pdu[0] << 1) & 0x10;

		return;
	}

next_recv:
	if (active)
		radio_stop();

	radio_recv(RADIO_FLAGS_TX_NEXT);
}

static void radio_send_cb(bool active)
{
	radio_recv(0);
	timer_start(t_ifs, T_IFS, t_ifs_timeout);
}

int main(void)
{
	log_init();
	timer_init();
	radio_init();
	radio_set_callbacks(radio_recv_cb, radio_send_cb);
	radio_set_out_buffer(scan_req);

	scan_window = timer_create(TIMER_REPEATED);
	t_ifs = timer_create(TIMER_SINGLESHOT);

	DBG("Active scanning");
	DBG("Scan window/interval: %u ms", SCAN_WINDOW / 1000);

	timer_start(scan_window, SCAN_WINDOW, scan_window_timeout);
	scan_window_timeout();

	evt_loop_run();

	return 0;
}

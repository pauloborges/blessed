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
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include <blessed/log.h>
#include <blessed/evtloop.h>

#include "radio.h"
#include "timer.h"

#define ADV_CHANNEL_AA			0x8E89BED6
#define ADV_CHANNEL_CRC			0x555555

/* Ideally both timers should have the same value in this example. But
 * scan_window_timeout() must be called first than scan_window_timeout() to not
 * break the radio logic. Since the scan_interval timer is started several
 * microseconds before scan_window, the last must have a smaller timeout value.
 * The link layer implementation must address this corner case.
 */
#define SCAN_WINDOW			TIMER_MILLIS(9900)
#define SCAN_INTERVAL			TIMER_MILLIS(10000)

static uint8_t channels[] = { 37, 38, 39 };
static uint8_t idx = 0xFF;

static char *pdus[] = {
	"ADV_IND", "ADV_DIRECT_IND", "ADV_NONCONN_IND", "SCAN_REQ",
	"SCAN_RSP", "CONNECT_REQ", "ADV_SCAN_IND"
};

static int16_t scan_window;
static int16_t scan_interval;

static __inline const char *format_address(const uint8_t *data)
{
	static char address[18];
	uint8_t i;

	for (i = 0; i < 5; i++)
		sprintf(address + 3*i, "%02x:", data[5-i]);
	sprintf(address + 3*i, "%02x", data[5-i]);

	return address;
}

static void scan_window_timeout(void)
{
	radio_stop();
}

static void scan_interval_timeout(void)
{
	timer_start(scan_window, SCAN_WINDOW, scan_window_timeout);

	idx = (uint8_t) (idx + 1) % sizeof(channels);

	radio_prepare(channels[idx], ADV_CHANNEL_AA, ADV_CHANNEL_CRC);
	radio_recv(0);
}

static void radio_recv_cb(const uint8_t *pdu, bool crc, bool active)
{
	uint8_t pdu_type = pdu[0] & 0xF;
	uint8_t length = pdu[1] & 0x3F;
	const char *address;

	if (!crc) {
		ERROR("ch %u BAD CRC", channels[idx]);
		goto next_recv;
	}

	/* The minimum allowed payload is 6 bytes */
	if (length < 6) {
		ERROR("ch %u BAD LENGTH %u", channels[idx], length);
		goto next_recv;
	}

	address = format_address(pdu + 2);

	DBG("%s ch %u pdu %s", address, channels[idx], pdus[pdu_type]);

next_recv:
	radio_recv(0);
}

int main(void)
{
	log_init();
	timer_init();
	radio_init();
	radio_set_callbacks(radio_recv_cb, NULL);

	scan_window = timer_create(TIMER_SINGLESHOT);
	scan_interval = timer_create(TIMER_REPEATED);

	DBG("Scanning");
	DBG("Scan window:   %u ms", SCAN_WINDOW / 1000);
	DBG("Scan interval: %u ms", SCAN_INTERVAL / 1000);

	timer_start(scan_interval, SCAN_INTERVAL, scan_interval_timeout);
	scan_interval_timeout();

	evt_loop_run();

	return 0;
}

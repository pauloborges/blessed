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

#include "timer.h"
#include "radio.h"
#include "log.h"

#define ADV_CHANNEL_AA			0x8E89BED6
#define ADV_CHANNEL_CRC			0x555555

#define SCAN_WINDOW			1000
#define SCAN_INTERVAL			2000

static uint8_t channels[] = { 37, 38, 39 };
static uint8_t idx = 0;

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

void scan_window_timeout(void *user_data)
{
	radio_stop();
}

void scan_interval_timeout(void *user_data)
{
	idx = (idx + 1) % sizeof(channels);

	radio_recv(channels[idx], ADV_CHANNEL_AA, ADV_CHANNEL_CRC);
	timer_start(scan_window, SCAN_WINDOW, NULL);
}

void radio_hdlr(uint8_t evt, void *data)
{
	struct radio_packet *packet = data;

	if (evt != RADIO_EVT_RX_COMPLETED) {
		ERROR("Unexpected radio evt: %u", evt);
		return;
	}

	if (!packet->crcstatus) {
		DBG("ch %u bad crc", channels[idx]);
		goto recv;
	}

	if ((packet->pdu[1] & 0x3F) < 8) {
		DBG("ch %u bad length", channels[idx]);
		goto recv;
	}

	DBG("ch %u (%s)", channels[idx], format_address(data + 2));

recv:
	radio_recv(channels[idx], ADV_CHANNEL_AA, ADV_CHANNEL_CRC);
}

int main(void)
{
	log_init();
	timer_init();
	radio_init();
	radio_register_handler(radio_hdlr);

	scan_window = timer_create(TIMER_SINGLESHOT, scan_window_timeout);
	scan_interval = timer_create(TIMER_REPEATED, scan_interval_timeout);

	radio_recv(channels[idx], ADV_CHANNEL_AA, ADV_CHANNEL_CRC);
	timer_start(scan_window, SCAN_WINDOW, NULL);
	timer_start(scan_interval, SCAN_INTERVAL, NULL);

	while (1);

	return 0;
}

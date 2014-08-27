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

#include <blessed/bdaddr.h>
#include <blessed/evtloop.h>

#include "ll.h"

#define ADV_INTERVAL			1280000	/* 1280 ms */

static const bdaddr_t addr = { { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF },
							BDADDR_TYPE_RANDOM };

/* AD structure:
 * LEN: 15 bytes | LOCAL NAME: 0x09 | DATA: "blessed device"
 */
static uint8_t data[] = { 0x0F, 0x09, 0x62, 0x6C, 0x65, 0x73, 0x73, 0x65, 0x64,
			  0x20, 0x64, 0x65, 0x76, 0x69, 0x63, 0x65 };

/* AD structure:
 * LEN: 0x03 bytes | Appearance: 0x19 | DATA: 0x00 0x00  (unknown)
 */
static uint8_t scan_data[] = {0x03, 0x19, 0x00, 0x00 };

int main(void)
{
	ll_init(&addr);
	ll_set_advertising_data(data, sizeof(data));
	ll_set_scan_response_data(scan_data, sizeof(scan_data));
	ll_advertise_start(LL_PDU_ADV_SCAN_IND, ADV_INTERVAL, LL_ADV_CH_ALL);

	evt_loop_run();

	return 0;
}

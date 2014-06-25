/**
 *  The MIT License (MIT)
 *
 *  2014 - Tom Magnier <magnier.tom@gmail.com>
 *
 *  Adapted from radio-observer example
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

#include <blessed/bdaddr.h>
#include <blessed/log.h>

#include "nrf_delay.h"

#include "ll.h"

#define SCAN_WINDOW				200
#define SCAN_INTERVAL			500

static const bdaddr_t addr = { { 0x14, 0x20, 0xCC, 0xDD, 0xEE, 0xFF },
							BDADDR_TYPE_RANDOM };

static __inline const char *format_address(const uint8_t *data)
{
	static char address[18];
	uint8_t i;

	for (i = 0; i < 5; i++)
		sprintf(address + 3*i, "%02x:", data[5-i]);
	sprintf(address + 3*i, "%02x", data[5-i]);

	return address;
}

static __inline const char *format_data(const uint8_t *data, uint8_t len)
{
	static char output[LL_ADV_MTU_DATA];

	memcpy(output, data, len);
	memset(output+len, ' ', LL_ADV_MTU_DATA-len);

	return output;
}

void adv_report_cb(ll_pdu_t type, uint8_t addr_type, uint8_t *addr, uint8_t len, uint8_t *data)
{
	DBG("adv type %02x, addr type %02x", type, addr_type);
	DBG("address %s, data %s", format_address(addr), format_data(data, len));
}

int main(void)
{
	log_init();
	ll_init(&addr);

	DBG("End init");

	ll_scan_start(LL_SCAN_PASSIVE, SCAN_INTERVAL, SCAN_WINDOW, adv_report_cb);

	nrf_delay_ms(1100);

	ll_scan_stop();

	while (1);

	return 0;
}

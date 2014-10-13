/**
 *  The MIT License (MIT)
 *
 *  2014 - Tom Magnier <magnier.tom@gmail.com>
 *
 *  Adapted from ll-initiator example
 *
 *  This example demonstrates the connection state, in Central role.
 *  A GATT characteristic (handle 0x000E) is written regularly and notifications
 *  from the battery service (characteristic : handle 0x0015) are received.
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
#include <blessed/events.h>
#include <blessed/evtloop.h>

#include "delay.h"

#include "ll.h"

#define SCAN_WINDOW			200000
#define SCAN_INTERVAL			500000

/* L2CAP specification, Section 3.1, Core 4.1 p.1683
 * L2CAP specification, Section 1.1, Core 4.1 p.1677
 * ATT specification, Section 3.4.5.3, Core 4.1 p. 2148 */
static uint8_t out_buffer[] =  {
				0x04, 0x00,	//L2CAP payload length
				0x04, 0x00,	//L2CAP Channel ID : ATT
				0x52,		//ATT opcode : write cmd
				0x0E, 0x00,	//ATT handle (0x000E)
				0x00};		//ATT value to write

static uint8_t in_buffer[LL_DATA_MTU_PAYLOAD];

/* PDU to send to enable notifications on battery service */
static uint8_t cccd_out_buffer[] = {
				0x05, 0x00,	//L2CAP payload length
				0x04, 0x00,	//L2CAP Channel ID : ATT
				0x52,		//ATT opcode : write cmd
				0x16, 0x00,	//ATT handle (0x0016)
				0x01, 0x00};	//ATT value to write

/* Expected packet on handle value notification ; the 8th byte is the value */
static const uint8_t notification_buffer_value[] = {
				0x04, 0x00, //L2CAP payload length
				0x04, 0x00,	//L2CAP CID : ATT
				0x1B,		//ATT opcode : handle value notification
				0x15, 0x00};	//ATT handle (0x0015)

static const bdaddr_t addr = { { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF },
							BDADDR_TYPE_RANDOM };

static bdaddr_t peer_addr;
uint8_t led_value = 0x00;


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
    static char output[3 * LL_ADV_MTU_DATA + 1];
    uint8_t i;

    for (i = 0; i < len; i++)
        sprintf(output + 3*i, "%02x ", data[i]);

    output[3*i] = '\0';

    return output;
}

void conn_evt_cb(ble_evt_t type, const void *data)
{
	const ble_evt_ll_connection_complete_t* conn_compl = data;
	const ble_evt_ll_disconnect_complete_t* disconn_compl = data;
	const ble_evt_ll_packets_received_t* packets_rx = data;

	switch (type) {
	case BLE_EVT_LL_CONNECTION_COMPLETE:
		DBG("Connection complete, index %u, address %s",
		conn_compl->index, format_address(conn_compl->peer_addr.addr));

		/*Enable notifications on battery service (write 0x0001 on CCCD
		 * characteristic, handle 0x0016) */
		ll_conn_send(cccd_out_buffer, 9);
		break;

	case BLE_EVT_LL_DISCONNECT_COMPLETE:
		DBG("Disconnect complete, index %u, reason %02x",
				disconn_compl->index, disconn_compl->reason);

		evt_loop_run();
		break;

	case BLE_EVT_LL_PACKETS_SENT:
		out_buffer[7] = led_value;
		ll_conn_send(out_buffer, 8);
		break;

	case BLE_EVT_LL_PACKETS_RECEIVED:
		if (!memcmp(in_buffer, notification_buffer_value, 7))
			DBG("Battery value : %u %%", in_buffer[7]);
		else
			DBG("Received packet : %s", format_data(in_buffer,
							packets_rx->length));
		break;
	}
}

void adv_report_cb(struct adv_report *report)
{
	DBG("adv type %02x, addr type %02x", report->type, report->addr.type);
	DBG("address %s, data %s", format_address(report->addr.addr),
					format_data(report->data, report->len));

	memcpy(peer_addr.addr, report->addr.addr, BDADDR_LEN);
	peer_addr.type = report->addr.type;

	ll_scan_stop();
	ll_conn_create(SCAN_INTERVAL, SCAN_WINDOW, &peer_addr, 1, in_buffer,
								conn_evt_cb);
}

int main(void)
{
	log_init();
	ll_init(&addr);

	DBG("End init, connection + battery notification");

	ll_scan_start(LL_SCAN_PASSIVE, SCAN_INTERVAL, SCAN_WINDOW,
							adv_report_cb);
	while (1)
	{
		led_value++;
		delay(10000);
	}

	return 0;
}

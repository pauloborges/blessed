/**
 *  The MIT License (MIT)
 *
 *  Copyright (c) 2013 Paulo B. de Oliveira Filho <pauloborgesfilho@gmail.com>
 *  Copyright (c) 2013 Claudio Takahasi <claudio.takahasi@gmail.com>
 *  Copyright (c) 2013 João Paulo Rechi Vita <jprvita@gmail.com>
 *  Copyright (c) 2014 Tom Magnier <magnier.tom@gmail.com>
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

/* Maximum size of parameters structure (to allocate a static buffer) */
#define BLE_EVT_PARAMS_MAX_SIZE		8

/* HCI Specification, Section 7.7, Core 4.1 p.1132-1239 */
typedef enum ble_evt {
	BLE_EVT_LL_CONNECTION_COMPLETE,		/* Sec 7.7.3 p.1135 */
	BLE_EVT_LL_DISCONNECT_COMPLETE,		/* Sec 7.7.5 p.1138 */
	BLE_EVT_LL_PACKETS_SENT,
	BLE_EVT_LL_PACKETS_RECEIVED
}ble_evt_t;

/* Events parameters structure for BLE_EVT_LL_CONNECTION_COMPLETE event */
typedef struct ble_evt_ll_connection_complete {
	uint8_t		index;		/* Connection index */
	bdaddr_t	peer_addr;	/* Peer address */
} ble_evt_ll_connection_complete_t;

/* Events parameters structure for BLE_EVT_LL_DISCONNECT_COMPLETE event */
typedef struct ble_evt_ll_disconnect_complete {
	uint8_t		index;		/* Connection index */
	uint8_t		reason;		/* Reason for disconnection */
} ble_evt_ll_disconnect_complete_t;

/* Events parameters structure for BLE_EVT_LL_PACKETS_SENT event */
typedef struct ble_evt_ll_packets_sent {
	uint8_t		index;		/* Connection index */
} ble_evt_ll_packets_sent_t;

/* Events parameters structure for BLE_EVT_LL_PACKETS_RECEIVED event */
typedef struct ble_evt_ll_packets_received {
	uint8_t		index;		/* Connection index */
	uint8_t		length;		/* Number of bytes received */
} ble_evt_ll_packets_received_t;


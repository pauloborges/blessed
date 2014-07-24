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
#include <stdbool.h>

#include <blessed/errcodes.h>
#include <blessed/log.h>
#include <blessed/timer.h>
#include <blessed/bdaddr.h>

#include "radio.h"
#include "ll.h"

/* Link Layer specification Section 2.1.2, Core 4.1 page 2503 */
#define LL_ACCESS_ADDRESS_ADV		0x8E89BED6

/* Link Layer specification Section 3.1.1, Core 4.1 page 2522 */
#define LL_CRCINIT_ADV			0x555555

/* Link Layer specification Section 1.1, Core 4.1 page 2499 */
typedef enum ll_states {
	LL_STATE_STANDBY,
	LL_STATE_ADVERTISING,
	LL_STATE_SCANNING,
	LL_STATE_INITIATING,
	LL_STATE_CONNECTION,
} ll_states_t;

/* Link Layer specification Section 2.3, Core 4.1 pages 2504-2505 */
struct __attribute__ ((packed)) ll_pdu_adv {
	uint8_t		type:4;		/* See ll_pdu_t */
	uint8_t		_rfu_0:2;	/* Reserved for future use */
	uint8_t		tx_add:1;	/* public (0) or random (1) */
	uint8_t		rx_add:1;	/* public (0) or random (1) */

	uint8_t		length:6;	/* 6 <= payload length <= 37 */
	uint8_t		_rfu_1:2;	/* Reserved for future use */

	uint8_t		payload[LL_ADV_MTU_PAYLOAD];
};

/* Link Layer specification Section 2.3, Core 4.1 pages 2508 */
struct __attribute__ ((packed)) ll_pdu_scan_req {
	uint8_t scana[6];
	uint8_t adva[6];
};

static const bdaddr_t *laddr;
static ll_states_t current_state;

#define ADV_CH_IDX_37		0
#define ADV_CH_IDX_38		1
#define ADV_CH_IDX_39		2

static uint8_t adv_chs[] = { 37, 38, 39 };
static uint8_t adv_ch_idx;
static uint8_t prev_adv_ch_idx;
static uint8_t adv_ch_map;

static uint32_t t_adv_pdu_interval;
static uint32_t t_scan_window;

static struct ll_pdu_adv pdu_adv;
static struct ll_pdu_adv pdu_scan_rsp;

static bool rx = false;

/** Timers used by the LL
 * Two timers are shared for the various states : one for triggering
 * events at periodic intervals (advertising start / scanning start)
 *
 * The second is used as single shot : change advertising channel or stop
 * scanning at the end of the window
 */
static int16_t t_ll_interval;
static int16_t t_ll_single_shot;

/** Callback function to report advertisers (SCANNING state) */
static adv_report_cb_t ll_adv_report_cb = NULL;

static __inline void scan_req_cb(const struct ll_pdu_adv *pdu)
{
	struct ll_pdu_scan_req *scn;

	/* SCAN_REQ payload: ScanA(6 octets)|AdvA(6 octects) */
	if (pdu->length != 12)
		return;

	if (pdu->rx_add != laddr->type)
		return;

	scn = (struct ll_pdu_scan_req *) pdu->payload;
	if (memcmp(scn->adva, laddr->addr, 6))
		return;

	/* Send SCAN_RSP */
	radio_send((const uint8_t *) &pdu_scan_rsp, pdu_scan_rsp.length, false);
}

/**@brief Function called by the radio driver (PHY layer) on packet RX
 * Dispatch the event according to the LL state
 */
static void ll_on_radio_rx(const uint8_t *pdu, bool crc)
{
	struct ll_pdu_adv *rcvd_pdu = (struct ll_pdu_adv*) pdu;

	switch(current_state) {
		case LL_STATE_SCANNING:
			if(!ll_adv_report_cb) {
				ERROR("No adv. report callback defined");
				return;
			}

			/* Extract information from PDU and call
			 * ll_adv_report_cb
			 */

			ll_adv_report_cb(rcvd_pdu->type, rcvd_pdu->tx_add,
						rcvd_pdu->payload,
						rcvd_pdu->length - BDADDR_LEN,
						rcvd_pdu->payload + BDADDR_LEN);

			/* Receive new packets while the radio is not explicitly
			 * stopped
			 */
			radio_recv();
			break;

		case LL_STATE_ADVERTISING:
			if (pdu_adv.type != LL_PDU_ADV_IND &&
					pdu_adv.type != LL_PDU_ADV_SCAN_IND)
				break;

			if (rcvd_pdu->type != LL_PDU_SCAN_REQ)
				break;

			scan_req_cb(rcvd_pdu);

			break;

		case LL_STATE_INITIATING:
		case LL_STATE_CONNECTION:
		case LL_STATE_STANDBY:
		default:
			/* Nothing to do */
			return;
	}
}

/** Radio driver used by the Link Layer */
static struct radio_driver ll_radio_driver = {
	.rx = ll_on_radio_rx,
	.tx = NULL,
};

static __inline uint8_t first_adv_ch_idx(void)
{
	if (adv_ch_map & LL_ADV_CH_37)
		return ADV_CH_IDX_37;
	else if (adv_ch_map & LL_ADV_CH_38)
		return ADV_CH_IDX_38;
	else
		return ADV_CH_IDX_39;
}

static __inline int16_t inc_adv_ch_idx(void)
{
	if ((adv_ch_map & LL_ADV_CH_38) && (adv_ch_idx == ADV_CH_IDX_37))
		adv_ch_idx = ADV_CH_IDX_38;
	else if ((adv_ch_map & LL_ADV_CH_39) && (adv_ch_idx < ADV_CH_IDX_39))
		adv_ch_idx = ADV_CH_IDX_39;
	else
		return -1;

	return 0;
}

/** Callback function for the "single shot" LL timer
 */
static void t_ll_single_shot_cb(void *user_data)
{
	switch(current_state) {
		case LL_STATE_ADVERTISING:
			radio_stop();
			radio_prepare(adv_chs[adv_ch_idx],
					LL_ACCESS_ADDRESS_ADV, LL_CRCINIT_ADV);
			radio_send((uint8_t *) &pdu_adv, sizeof(pdu_adv), rx);

			prev_adv_ch_idx = adv_ch_idx;
			if (!inc_adv_ch_idx())
				timer_start(t_ll_single_shot,
						t_adv_pdu_interval, NULL);
			break;

		case LL_STATE_SCANNING:
			/* Called at the end of the scan window */
			radio_stop();
			break;

		case LL_STATE_INITIATING:
		case LL_STATE_CONNECTION:
			/* Not implemented */
		case LL_STATE_STANDBY:
		default:
			/* Nothing to do */
			return;
	}
}

/** Callback function for the "interval" LL timer
 */
static void t_ll_interval_cb(void *user_data)
{
	switch(current_state) {
		case LL_STATE_ADVERTISING:
			adv_ch_idx = first_adv_ch_idx();
			t_ll_single_shot_cb(NULL);
			break;

		case LL_STATE_SCANNING:
			if(!inc_adv_ch_idx())
				adv_ch_idx = first_adv_ch_idx();

			radio_prepare(adv_chs[adv_ch_idx],
					LL_ACCESS_ADDRESS_ADV, LL_CRCINIT_ADV);
			radio_recv();
			timer_start(t_ll_single_shot, t_scan_window, NULL);
			break;

		case LL_STATE_INITIATING:
		case LL_STATE_CONNECTION:
			/* Not implemented */
		case LL_STATE_STANDBY:
		default:
			/* Nothing to do */
			return;
	}
}

int16_t ll_advertise_start(ll_pdu_t type, uint32_t interval, uint8_t chmap)
{
	int16_t err_code;

	if (current_state != LL_STATE_STANDBY)
		return -ENOREADY;

	if (!chmap || (chmap & !LL_ADV_CH_ALL))
		return -EINVAL;

	if (interval % LL_ADV_INTERVAL_QUANTUM)
		return -EINVAL;

	if (interval < LL_ADV_INTERVAL_MIN_NONCONN
					|| interval > LL_ADV_INTERVAL_MAX)
			return -EINVAL;

	adv_ch_map = chmap;

	switch (type) {
	case LL_PDU_ADV_IND:
	case LL_PDU_ADV_SCAN_IND:
		rx = true;
		break;

	case LL_PDU_ADV_NONCONN_IND:
		rx = false;
		break;

	case LL_PDU_ADV_DIRECT_IND:
		/* TODO: Not implemented */
	default:
		/* Invalid PDU */
		return -EINVAL;
	}

	pdu_adv.type = type;
	t_adv_pdu_interval = TIMER_MILLIS(10); /* <= 10ms Sec 4.4.2.6 */

	DBG("PDU interval %u ms, event interval %u ms",
				t_adv_pdu_interval / 1000, interval / 1000);

	err_code = timer_start(t_ll_interval, interval, NULL);
	if (err_code < 0)
		return err_code;

	current_state = LL_STATE_ADVERTISING;

	t_ll_interval_cb(NULL);

	return 0;
}

int16_t ll_advertise_stop()
{
	int16_t err_code;

	if (current_state != LL_STATE_ADVERTISING)
		return -ENOREADY;

	err_code = timer_stop(t_ll_interval);
	if (err_code < 0)
		return err_code;

	err_code = timer_stop(t_ll_single_shot);
	if (err_code < 0)
		return err_code;

	current_state = LL_STATE_STANDBY;

	return 0;
}

int16_t ll_set_advertising_data(const uint8_t *data, uint8_t len)
{
	if (current_state != LL_STATE_STANDBY)
		return -EBUSY;

	if (data == NULL)
		return -EINVAL;

	if (len > LL_ADV_MTU_DATA)
		return -EINVAL;

	memcpy(pdu_adv.payload + sizeof(laddr->addr), data, len);
	pdu_adv.length = sizeof(laddr->addr) + len;

	return 0;
}

int16_t ll_set_scan_response_data(const uint8_t *data, uint8_t len)
{
	if (data == NULL)
		return -EINVAL;

	if (len > LL_ADV_MTU_DATA)
		return -EINVAL;

	memcpy(pdu_scan_rsp.payload + sizeof(laddr->addr), data, len);
	pdu_scan_rsp.length = sizeof(laddr->addr) + len;

	return 0;
}

static void init_adv_pdus(void)
{
	pdu_adv.tx_add = laddr->type;
	memcpy(pdu_adv.payload, laddr->addr, sizeof(laddr->addr));

	ll_set_advertising_data(NULL, 0);

	pdu_scan_rsp.type = LL_PDU_SCAN_RSP;
	pdu_scan_rsp.tx_add = laddr->type;
	memcpy(pdu_scan_rsp.payload, laddr->addr, sizeof(laddr->addr));

	ll_set_scan_response_data(NULL, 0);
}

int16_t ll_init(const bdaddr_t *addr)
{
	int16_t err_code;

	if (addr == NULL)
		return -EINVAL;

	err_code = log_init();
	if (err_code < 0 && err_code != -EALREADY)
		return err_code;

	err_code = timer_init();
	if (err_code < 0)
		return err_code;

	err_code = radio_init(&ll_radio_driver);
	if (err_code < 0)
		return err_code;

	t_ll_interval = timer_create(TIMER_REPEATED, t_ll_interval_cb);
	if (t_ll_interval < 0)
		return t_ll_interval;

	t_ll_single_shot = timer_create(TIMER_SINGLESHOT, t_ll_single_shot_cb);
	if (t_ll_single_shot < 0)
		return t_ll_single_shot;

	laddr = addr;
	current_state = LL_STATE_STANDBY;

	init_adv_pdus();

	return 0;
}

/**@brief Set scan parameters and start scanning
 *
 * @note The HCI spec specifies interval in units of 0.625 ms.
 * 	Here we use us directly.
 *
 * @param [in] scan_type: should be LL_SCAN_ACTIVE or LL_SCAN_PASSIVE
 * 		(only the latter implemented at this time)
 * @param [in] interval: the scan Interval in us
 * @param [in] window: the scan Window in us
 * @param [in] adv_report_cb: the function to call for advertising report events
 *
 * @return -EINVAL if window > interval or interval > 10.24 s
 * @return -EINVAL if scan_type != LL_SCAN_PASSIVE
 */
int16_t ll_scan_start(uint8_t scan_type, uint32_t interval, uint32_t window,
						adv_report_cb_t adv_report_cb)
{
	int16_t err_code;

	if(window > interval || interval > LL_SCAN_INTERVAL_MAX)
		return -EINVAL;

	switch(scan_type) {
		case LL_SCAN_PASSIVE:
			/* Setup callback function */
			ll_adv_report_cb = adv_report_cb;
			break;

		case LL_SCAN_ACTIVE:
		/* Not implemented */
		default:
			return -EINVAL;
	}

	/* Setup timer and save window length */
	t_scan_window = window;
	err_code = timer_start(t_ll_interval, interval, NULL);
	if (err_code < 0)
		return err_code;

	current_state = LL_STATE_SCANNING;
	t_ll_interval_cb(NULL);

	DBG("interval %uus, window %uus", interval, window);

	return 0;
}

/**@brief Stop scanning
 */
int16_t ll_scan_stop(void)
{
	int16_t err_code;

	if (current_state != LL_STATE_SCANNING)
		return -ENOREADY;

	err_code = timer_stop(t_ll_interval);
	if (err_code < 0)
		return err_code;

	err_code = timer_stop(t_ll_single_shot);
	if (err_code < 0)
		return err_code;

	/* Call the single shot cb to stop the radio */
	t_ll_single_shot_cb(NULL);

	current_state = LL_STATE_STANDBY;

	DBG("");

	return 0;
}

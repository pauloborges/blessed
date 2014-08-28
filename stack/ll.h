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

/* Link Layer specification Section 2.1, Core 4.1 page 2503 */
#define LL_MTU				39

/* Link Layer specification Section 2.3, Core 4.1 page 2504
 * Link Layer specification Section 2.3, Core 4.1 page 2511
 */
#define LL_HEADER_LEN			2

/* Link Layer specification Section 2.3, Core 4.1 pages 2504-2505 */
#define LL_ADV_MTU_PAYLOAD		(LL_MTU - LL_HEADER_LEN)

/* Link Layer specification Section 2.3.1, Core 4.1 page 2506 */
#define LL_ADV_MTU_DATA			(LL_ADV_MTU_PAYLOAD - BDADDR_LEN)

/* Link Layer specification Section 4.4.2.2, Core 4.1 page 2528 */
#define LL_ADV_INTERVAL_MIN_CONN	20000		/* 20 ms */
#define LL_ADV_INTERVAL_MIN_NONCONN	100000		/* 100 ms */
#define LL_ADV_INTERVAL_MIN_SCAN	LL_ADV_INTERVAL_MIN_NONCONN
#define LL_ADV_INTERVAL_MAX		10240000	/* 10.24 s */
#define LL_ADV_INTERVAL_QUANTUM		625		/* 0.625 ms */

/* Link Layer specification Section 4.4.3, Core 4.1 page 2535 */
#define LL_SCAN_WINDOW_MAX		10240000	/* 10.24 s */
#define LL_SCAN_INTERVAL_MAX		10240000	/* 10.24 s */

/* HCI Funcional Specification Section 7.8.5, Core 4.1 page 1248 */
#define LL_ADV_CH_37			(1 << 0)
#define LL_ADV_CH_38			(1 << 1)
#define LL_ADV_CH_39			(1 << 2)
#define LL_ADV_CH_ALL			(LL_ADV_CH_37 | LL_ADV_CH_38 |	\
 							LL_ADV_CH_39)

/* Link Layer specification Section 1.4, Core 4.1 page 2501 */
#define LL_DATA_CH_ALL			0x1FFFFFFFFFULL

/* HCI Funcional Specification Section 7.8.10, Core 4.1 page 1255 */
#define LL_SCAN_PASSIVE			0x00
#define LL_SCAN_ACTIVE			0x01

/* Link Layer specification Section 2.3, Core 4.1 page 2505 */
typedef enum ll_pdu {
	LL_PDU_ADV_IND,
	LL_PDU_ADV_DIRECT_IND,
	LL_PDU_ADV_NONCONN_IND,
	LL_PDU_SCAN_REQ,
	LL_PDU_SCAN_RSP,
	LL_PDU_CONNECT_REQ,
	LL_PDU_ADV_SCAN_IND
} ll_pdu_t;

/**@brief Connection parameters structure */
/* TODO helper macros to convert to and from us */
typedef struct {
	/** range: 7.5ms -> 4s, unit: 1.25ms */
	uint16_t	conn_interval_min;
	/** range: 7.5ms -> 4s, unit: 1.25ms */
	uint16_t	conn_interval_max;
	/** range : 0 -> 499, slave latency in number of connection events */
	uint16_t	conn_latency;
	/** range: 100ms -> 32s, unit: 10ms */
	uint16_t	supervision_timeout;
	/** range: 0ms -> 40s, unit: 0.625ms, min. length of connection events*/
	uint16_t	minimum_ce_length;
	/** range: 0ms -> 40s, unit: 0.625ms, max. length of connection events*/
	uint16_t	maximum_ce_length;
} ll_conn_params_t;

struct adv_report {
	ll_pdu_t	type;
	bdaddr_t	addr;
	const uint8_t 	*data;
	uint8_t		len;
};

/* Callback function for LE advertising reports (scanning mode)
 * See HCI Funcional Specification Section 7.7.65.2, Core 4.1 page 1220 */
typedef void (*adv_report_cb_t)(struct adv_report *report);

int16_t ll_init(const bdaddr_t *addr);

/* Advertising */
int16_t ll_set_advertising_data(const uint8_t *data, uint8_t len);
int16_t ll_set_scan_response_data(const uint8_t *data, uint8_t len);
int16_t ll_advertise_start(ll_pdu_t type, uint32_t interval, uint8_t chmap);
int16_t ll_advertise_stop(void);

/* Scanning */
int16_t ll_scan_start(uint8_t scan_type, uint32_t interval, uint32_t window,
						adv_report_cb_t adv_report_cb);
int16_t ll_scan_stop(void);

/* Initiating a connection */
int16_t ll_set_conn_params(ll_conn_params_t* conn_params);
int16_t ll_set_data_ch_map(uint64_t ch_map);
int16_t ll_conn_create(uint32_t interval, uint32_t window,
			bdaddr_t* peer_addresses, uint16_t num_addresses);
int16_t ll_conn_cancel(void);

/* LL "platform" interface */
int16_t ll_plat_init(void);
int16_t ll_plat_send_adv_report(adv_report_cb_t cb, struct adv_report *rpt);

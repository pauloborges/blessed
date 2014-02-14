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

#define BCI_ENABLE			1
#define BCI_DISABLE			0

/* HCI Funcional Specification Section 7.8.7, Core 4.1 page 1251 */
#define BCI_ADV_MTU_DATA		31

/* HCI Funcional Specification Section 7.8.5, Core 4.1 page 1247 */
#define BCI_ADV_INTERVAL_MIN_CONN	20
#define BCI_ADV_INTERVAL_MIN_NONCONN	100
#define BCI_ADV_INTERVAL_MAX		10240	/* 10.24 s */

/* HCI Funcional Specification Section 7.8.5, Core 4.1 page 1248 */
#define BCI_ADV_CH_37			(1 << 0)
#define BCI_ADV_CH_38			(1 << 1)
#define BCI_ADV_CH_39			(1 << 2)
#define BCI_ADV_CH_ALL			(BCI_ADV_CH_37 | BCI_ADV_CH_38	\
 							| BCI_ADV_CH_39)

/* HCI Funcional Specification Section 7.8.5, Core 4.1 page 1247 */
typedef enum bci_adv {
	BCI_ADV_CONN_UNDIR,	/* connectable undirected */
	BCI_ADV_CONN_DIR_HIGH,	/* connectable directed (high duty cycle) */
	BCI_ADV_SCAN_UNDIR,	/* scannable undirected */
	BCI_ADV_NONCONN_UNDIR,	/* non-connectable undirected */
	BCI_ADV_CONN_DIR_LOW,	/* connectable directed (low duty cycle) */
} bci_adv_t;

struct bci_adv_params {
	bci_adv_t type;
	uint16_t interval;
	uint8_t chmap;
};

/*
 * From Bluetooth SIG GAP assigned numbers
 * https://www.bluetooth.org/en-us/specification/assigned-numbers/\
 * generic-access-profile
 */

typedef enum {
	BCI_AD_INVALID		= 0x00,
	BCI_AD_FLAGS		= 0x01, /* AD Flags */
	BCI_AD_NAME_SHORT	= 0x08, /* Shortened local name */
	BCI_AD_NAME_COMPLETE	= 0x09, /* Complete local name */
	BCI_AD_TX_POWER		= 0x0A, /* Transmit power level */
	BCI_AD_GAP_APPEARANCE	= 0x19, /* GAP appearance */
	BCI_AD_MFT_DATA		= 0xff,	/* Manufactorer specific data */
} bci_ad_t;

int16_t bci_init(const bdaddr_t *addr);

void bci_get_advertising_params(struct bci_adv_params *params);

int16_t bci_set_advertising_params(const struct bci_adv_params *params);
int16_t bci_set_advertising_data(const uint8_t *data, uint8_t len);
int16_t bci_set_scan_response_data(const uint8_t *data, uint8_t len);
int16_t bci_set_advertise_enable(uint8_t enable);

int8_t bci_ad_put(uint8_t *buffer, bci_ad_t type, ...);
bool bci_ad_get(const uint8_t *buffer, uint8_t len, bci_ad_t type, ...);

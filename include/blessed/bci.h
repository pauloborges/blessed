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

struct bci_adv_params {
	adv_type_t type;
	uint16_t interval;
	uint8_t chmap;
};

int16_t bci_init(const bdaddr_t *addr);

void bci_get_advertising_params(struct bci_adv_params *params);

int16_t bci_set_advertising_params(const struct bci_adv_params *params);
int16_t bci_set_advertising_data(const uint8_t *data, uint8_t len);
int16_t bci_set_advertise_enable(uint8_t enable);

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
#include <stdlib.h>
#include <string.h>

#include <blessed/errcodes.h>
#include <blessed/bdaddr.h>
#include <blessed/bci.h>

#include "ll.h"

static const bdaddr_t *laddr;
static uint8_t adv_data[BCI_ADV_MTU_DATA];
static uint8_t adv_data_len;

static struct bci_adv_params adv_params = {
	.type = BCI_ADV_NONCONN_UNDIR,
	.interval = BCI_ADV_INTERVAL_MIN_NONCONN,
	.chmap = LL_ADV_CH_ALL
};

void bci_get_advertising_params(struct bci_adv_params *params)
{
	memcpy(params, &adv_params, sizeof(adv_params));
}

int16_t bci_set_advertising_params(const struct bci_adv_params *params)
{
	if (!params->chmap || (!params->chmap & !BCI_ADV_CH_ALL))
		return -EINVAL;

	if (params->interval > BCI_ADV_INTERVAL_MAX)
		return -EINVAL;

	/* XXX: Should the lib return an error when the user uses a not yet
	 * implemented type? If yes, fix the code below.
	 */
	switch (params->type) {
	case BCI_ADV_NONCONN_UNDIR:
	case BCI_ADV_SCAN_UNDIR:
		if (params->interval < BCI_ADV_INTERVAL_MIN_NONCONN)
			return -EINVAL;
		break;
	case BCI_ADV_CONN_UNDIR:
	case BCI_ADV_CONN_DIR_LOW:
		if (params->interval < BCI_ADV_INTERVAL_MIN_CONN)
			return -EINVAL;
		break;
	case BCI_ADV_CONN_DIR_HIGH:
		break;
	}

	memcpy(&adv_params, params, sizeof(adv_params));

	return 0;
}

int16_t bci_set_advertising_data(const uint8_t *data, uint8_t len)
{
	if (data == NULL)
		return -EINVAL;

	if (len > BCI_ADV_MTU_DATA)
		return -EINVAL;

	memset(adv_data, 0, BCI_ADV_MTU_DATA);
	memcpy(adv_data, data, len);
	adv_data_len = len;

	/* XXX: If the radio is already advertising, the advertising data
	 * should be refreshed? If yes, then we need to notify the link layer.
	 */

	return 0;
}

static ll_pdu_t adv_type_to_pdu(bci_adv_t type, ll_pdu_t *pdu) {
	if (pdu == NULL)
		return -EINVAL;

	switch (type) {
	case BCI_ADV_CONN_UNDIR:
		*pdu = LL_PDU_ADV_IND;
		break;
	case BCI_ADV_CONN_DIR_HIGH:
	case BCI_ADV_CONN_DIR_LOW:
		*pdu = LL_PDU_ADV_DIRECT_IND;
		break;
	case BCI_ADV_NONCONN_UNDIR:
		*pdu = LL_PDU_ADV_NONCONN_IND;
		break;
	case BCI_ADV_SCAN_UNDIR:
		*pdu = LL_PDU_ADV_SCAN_IND;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

int16_t bci_set_advertise_enable(uint8_t enable)
{
	int16_t err;
	ll_pdu_t pdu;

	if (!enable)
		return ll_advertise_stop();

	err = adv_type_to_pdu(adv_params.type, &pdu);
	if (err < 0)
		return err;

	return ll_advertise_start(pdu, adv_params.interval, adv_params.chmap,
						adv_data, adv_data_len);
}

int16_t bci_init(const bdaddr_t *addr)
{
	int16_t err_code;

	if (addr == NULL || (addr->type != BDADDR_TYPE_PUBLIC &&
					addr->type != BDADDR_TYPE_RANDOM))
		return -EINVAL;

	/* TODO: check bdaddr->addr */

	err_code = ll_init(addr);
	if (err_code != 0)
		return err_code;

	laddr = addr;

	return 0;
}

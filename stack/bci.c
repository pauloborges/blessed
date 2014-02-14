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
#include <stdarg.h>
#include <stdbool.h>

#include <blessed/errcodes.h>
#include <blessed/bdaddr.h>
#include <blessed/bci.h>

#include "ll.h"
#include "assert.h"

/* BCI and LL layers shares common symbols. But since LL can not include BCI
 * and BCI can not expose LL to the user, both layers have definitions of the
 * same symbols. To ensure consistency, here these symbols are asserted in
 * compile time.
 */
STATIC_ASSERT(BCI_ADV_INTERVAL_MIN_CONN == LL_ADV_INTERVAL_MIN_CONN);
STATIC_ASSERT(BCI_ADV_INTERVAL_MIN_NONCONN == LL_ADV_INTERVAL_MIN_NONCONN);
STATIC_ASSERT(BCI_ADV_INTERVAL_MAX == LL_ADV_INTERVAL_MAX);
STATIC_ASSERT(BCI_ADV_CH_37 == LL_ADV_CH_37);
STATIC_ASSERT(BCI_ADV_CH_38 == LL_ADV_CH_38);
STATIC_ASSERT(BCI_ADV_CH_39 == LL_ADV_CH_39);
STATIC_ASSERT(BCI_ADV_CH_ALL == LL_ADV_CH_ALL);

static const bdaddr_t *laddr;

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
	return ll_set_advertising_data(data, len);
}

static ll_pdu_t adv_type_to_pdu(bci_adv_t type, ll_pdu_t *pdu)
{
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

	return ll_advertise_start(pdu, adv_params.interval, adv_params.chmap);
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

int8_t bci_ad_put(uint8_t *buffer, bci_ad_t type, ...)
{
	va_list args;
	uint8_t *data = buffer;
	const char *str;
	const uint8_t *ptr;
	uint16_t u16;
	size_t arglen;

	va_start(args, type);

	while (type != BCI_AD_INVALID) {
		switch (type) {
		/* 1 octet */
		case BCI_AD_FLAGS:
		case BCI_AD_TX_POWER:
			*data = 2;		/* Group length */
			data++;
			*data = type;
			data++;
			*data = va_arg(args, unsigned int);
			data++;
			break;

		/* 2 octets */
		case BCI_AD_GAP_APPEARANCE:
			*data = 3;		/* Group length */
			data++;
			*data = type;
			data++;
			u16 = va_arg(args, unsigned int);
			*data = u16 & 0x00ff;
			data++;
			*data = (u16 >> 1 )& 0x00ff;
			data++;
			break;
		/* Strings */
		case BCI_AD_NAME_SHORT:
		case BCI_AD_NAME_COMPLETE:
			str = va_arg(args, const char *);
			arglen = strlen(str);
			*data = arglen + 1; /* Group length */
			data++;
			*data = type;
			data++;
			strncpy((char *) data, str, arglen);
			data += arglen;
			break;
		/* Fixed size array */
		case BCI_AD_MFT_DATA:
			ptr = va_arg(args, const uint8_t *);
			arglen = va_arg(args, int);
			*data = arglen + 1; /* Group length */
			data++;
			*data = type;
			data++;
			memcpy(data, ptr, arglen);
			data += arglen;
			break;
		default:
			return -EINVAL;
		}

		type = va_arg(args, int);
	}
	va_end(args);

	/* Returns the amount of octets written */
	return data - buffer;
}

bool bci_ad_get(const uint8_t *buffer, uint8_t len, bci_ad_t type, ...)
{
	const uint8_t *data = buffer;
	va_list args;
	unsigned int *flags = NULL, *appear = NULL;
	int8_t *tx = NULL, *mftlen = NULL;
	char *shortname = NULL, *fullname = NULL;
	uint8_t *mft = NULL;
	uint8_t grouplen;

	/* Parsing wanted types */
	va_start(args, type);

	while (type != BCI_AD_INVALID) {
		switch (type) {
		case BCI_AD_FLAGS:
			flags = va_arg(args, unsigned int *);
			break;
		case BCI_AD_TX_POWER:
			tx = va_arg(args, int8_t *);
			break;
		case BCI_AD_GAP_APPEARANCE:
			appear = va_arg(args, unsigned int *);
			break;
		case BCI_AD_NAME_SHORT:
			shortname = va_arg(args, char *);
			break;
		case BCI_AD_NAME_COMPLETE:
			fullname = va_arg(args, char *);
			break;
		case BCI_AD_MFT_DATA:
			mft = va_arg(args, uint8_t *);
			mftlen = va_arg(args, int8_t *);
			break;
		default:
			break;
		}

		type = va_arg(args, int);
	}

	va_end(args);

	/* Parsing the received AD data */
	grouplen = *data;
	if (grouplen == 0)
		return false;

	for (;(data + 1) < (buffer + len); data += (grouplen + 1)) {

		grouplen = *data;

		/* Invalid memory access? */
		if ((data + grouplen) > (buffer + len))
			return false;

		switch (*(data + 1)) {
		case BCI_AD_FLAGS:
			if (flags)
				*flags = *(data + 2);
			break;
		case BCI_AD_TX_POWER:
			if (tx)
				*tx = *(data + 2);
			break;
		case BCI_AD_GAP_APPEARANCE:
			if (!appear)
				break;

			*appear = *(data + 2);
			*appear |= *(data + 3) << 1;
			break;
		case BCI_AD_NAME_SHORT:
			if (!shortname)
				break;
			strncpy(shortname, (const char *) (data + 2),
								grouplen - 1);
			break;
		case BCI_AD_NAME_COMPLETE:
			if (!fullname)
				break;

			strncpy(fullname, (const char *) (data + 2),
								grouplen - 1);
			break;
		case BCI_AD_MFT_DATA:
			if (!mft || !mftlen)
				break;

			memcpy(mft, data + 2, grouplen - 1);
			*mftlen = grouplen - 1;

			break;
		default:
			return false;
		}
	}

	return true;
}

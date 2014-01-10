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

#include "errcodes.h"
#include "ble-common.h"
#include "bci.h"
#include "ll.h"

static const bdaddr_t *laddr;
static uint8_t adv_data[ADVDATA_LEN];

int16_t bci_set_advertising_data(const uint8_t *data, uint8_t len)
{
	if (data == NULL)
		return -EINVAL;

	if (len > ADVDATA_LEN)
		return -EINVAL;

	memset(adv_data, 0, ADVDATA_LEN);
	memcpy(adv_data, data, len);

	/* XXX: If the radio is already advertising, the advertising data
	 * should be refreshed? If yes, then we need to notify the link layer.
	 */

	return 0;
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

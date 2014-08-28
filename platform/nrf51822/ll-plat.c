/**
 *  The MIT License (MIT)
 *
 *  Copyright (c) 2014 Paulo B. de Oliveira Filho <pauloborgesfilho@gmail.com>
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

#include <stdlib.h>
#include <stdint.h>

#include <nrf51.h>

#include <blessed/errcodes.h>
#include <blessed/bdaddr.h>

#include "ll.h"
#include "nrf51822.h"

static adv_report_cb_t adv_report_cb = NULL;
static struct adv_report *adv_report = NULL;

void SWI0_IRQHandler(void)
{
	if (adv_report_cb)
		adv_report_cb(adv_report);
}

int16_t ll_plat_send_adv_report(adv_report_cb_t cb, struct adv_report *rpt)
{
	if (!cb || !rpt)
		return -EINVAL;

	adv_report_cb = cb;
	adv_report = rpt;

	NVIC_SetPendingIRQ(SWI0_IRQn);

	return 0;
}

int16_t ll_plat_init(void)
{
	NVIC_ClearPendingIRQ(SWI0_IRQn);
	NVIC_SetPriority(SWI0_IRQn, IRQ_PRIORITY_LOW);
	NVIC_EnableIRQ(SWI0_IRQn);

	return 0;
}

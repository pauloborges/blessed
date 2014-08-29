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
#include <string.h>

#include <blessed/log.h>
#include <blessed/evtloop.h>

#include "timer.h"

#define TIMER1				TIMER_MILLIS(1000)
#define TIMER2				TIMER_MILLIS(3000)

static uint16_t counter = 0;

static int16_t timer1;
static int16_t timer2;

static void timeout1(void)
{
	DBG("%u second(s)", ++counter);

	if (counter == 5) {
		DBG("timer1 stopped");
		timer_stop(timer1);
	}
}

static void timeout2(void)
{
	DBG("singleshot timer after %d ms", TIMER2 / 1000);
}

int main(void)
{
	log_init();
	timer_init();

	timer1 = timer_create(TIMER_REPEATED);
	timer2 = timer_create(TIMER_SINGLESHOT);

	timer_start(timer1, TIMER1, timeout1);
	timer_start(timer2, TIMER2, timeout2);

	evt_loop_run();

	return 0;
}

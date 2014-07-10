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

#include <string.h>
#include <stdbool.h>

#include <nrf51.h>
#include <nrf51_bitfields.h>

#include <blessed/errcodes.h>
#include <blessed/timer.h>
#include <blessed/log.h>

#include "nrf51822.h"

#define HFCLK				16000000UL
#define TIMER_PRESCALER			4		/* 1 MHz */
#define MAX_TIMERS			4

#define ROUNDED_DIV(A, B)		(((A) + ((B) / 2)) / (B))
#define BIT(n)				(1 << n)

struct timer {
	uint32_t ticks;
	timer_cb cb;
	void *data;
	uint8_t enabled:1;
	uint8_t active:1;
	uint8_t type:1;
};

static struct timer timers[MAX_TIMERS];
static uint8_t active = 0;

#define CLR_SET_MASKS(id, clr, set)					\
	switch (id) {							\
	case 0:								\
		clr = TIMER_INTENCLR_COMPARE0_Msk;			\
		set = TIMER_INTENSET_COMPARE0_Msk;			\
		break;							\
	case 1:								\
		clr = TIMER_INTENCLR_COMPARE1_Msk;			\
		set = TIMER_INTENSET_COMPARE1_Msk;			\
		break;							\
	case 2:								\
		clr = TIMER_INTENCLR_COMPARE2_Msk;			\
		set = TIMER_INTENSET_COMPARE2_Msk;			\
		break;							\
	case 3:								\
		clr = TIMER_INTENCLR_COMPARE3_Msk;			\
		set = TIMER_INTENSET_COMPARE3_Msk;			\
		break;							\
	}

static __inline void update_cc(uint8_t id, uint64_t ticks)
{
	uint32_t clr_mask = 0;
	uint32_t set_mask = 0;

	CLR_SET_MASKS(id, clr_mask, set_mask);

	NRF_TIMER0->INTENCLR = clr_mask;
	NRF_TIMER0->TASKS_CAPTURE[id] = 1UL;
	NRF_TIMER0->CC[id] = ((uint64_t)NRF_TIMER0->CC[id] + ticks)
								% 0xFFFFFFFF;
	NRF_TIMER0->INTENSET = set_mask;
}

void TIMER0_IRQHandler(void)
{
	uint8_t id_mask = 0;
	uint8_t id;

	for (id = 0; id < MAX_TIMERS; id++) {
		if (NRF_TIMER0->EVENTS_COMPARE[id]) {
			NRF_TIMER0->EVENTS_COMPARE[id] = 0UL;

			if (timers[id].active)
				id_mask |= BIT(id);
		}
	}

	for (id = 0; id < MAX_TIMERS; id++) {
		if (id_mask & BIT(id)) {
			if (timers[id].type == TIMER_REPEATED) {
				update_cc(id, (uint64_t) timers[id].ticks);
			} else if (timers[id].type == TIMER_SINGLESHOT) {
				timers[id].active = 0;
			}

			timers[id].cb(timers[id].data);
		}
	}
}

int16_t timer_init(void)
{
	if (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0UL) {
		NRF_CLOCK->TASKS_HFCLKSTART = 1UL;
		while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0UL);
	}

	NRF_TIMER0->MODE = TIMER_MODE_MODE_Timer;
	NRF_TIMER0->BITMODE = TIMER_BITMODE_BITMODE_24Bit;
	NRF_TIMER0->PRESCALER = TIMER_PRESCALER;

	NRF_TIMER0->INTENCLR = TIMER_INTENCLR_COMPARE0_Msk
						| TIMER_INTENCLR_COMPARE1_Msk
						| TIMER_INTENCLR_COMPARE2_Msk
						| TIMER_INTENCLR_COMPARE3_Msk;

	NVIC_SetPriority(TIMER0_IRQn, IRQ_PRIORITY_HIGH);
	NVIC_ClearPendingIRQ(TIMER0_IRQn);
	NVIC_EnableIRQ(TIMER0_IRQn);

	memset(timers, 0, sizeof(timers));

	return 0;
}

int16_t timer_create(uint8_t type, timer_cb cb)
{
	int16_t id;

	if (cb == NULL)
		return -EINVAL;

	if (type != TIMER_SINGLESHOT && type != TIMER_REPEATED)
		return -EINVAL;

	for (id = 0; id < MAX_TIMERS; id++) {
		if (!timers[id].enabled)
			goto create;
	}

	return -ENOMEM;

create:
	timers[id].enabled = 1;
	timers[id].active = 0;
	timers[id].type = type;
	timers[id].cb = cb;

	return id;
}

int16_t timer_start(int16_t id, uint32_t us, void *user_data)
{
	uint32_t ticks;

	if (id < 0)
		return -EINVAL;

	if (!timers[id].enabled)
		return -EINVAL;

	if (timers[id].active)
		return -EALREADY;

	ticks = ROUNDED_DIV((uint64_t)us * HFCLK, TIMER_SECONDS(1)
					* ROUNDED_DIV(2 << TIMER_PRESCALER, 2));

	timers[id].active = 1;
	timers[id].ticks = ticks;
	timers[id].data = user_data;

	update_cc(id, (uint64_t) ticks);

	if (active == 0) {
		NRF_TIMER0->TASKS_START = 1UL;
	}

	active++;

	return 0;
}

int16_t timer_stop(int16_t id)
{
	uint32_t clr_mask = 0;
	uint32_t set_mask = 0;

	UNUSED(set_mask);

	if (id < 0)
		return -EINVAL;

	if (!timers[id].enabled)
		return -EINVAL;

	CLR_SET_MASKS(id, clr_mask, set_mask);
	NRF_TIMER0->INTENCLR = clr_mask;

	timers[id].active = 0;
	active--;

	if (active == 0) {
		NRF_TIMER0->TASKS_STOP = 1UL;
		NRF_TIMER0->TASKS_CLEAR = 1UL;
	}

	return 0;
}

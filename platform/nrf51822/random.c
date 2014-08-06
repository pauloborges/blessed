/**
 *  The MIT License (MIT)
 *
 *  Copyright (c) 2014 Tom Magnier <magnier.tom@gmail.com>
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

#include <nrf51.h>
#include <nrf51_bitfields.h>

int16_t random_init(void)
{
	/* nRF51 Series Reference Manual v2.1, section 20.2, page 118
	 * Enable digital correction algorithm */
	NRF_RNG->CONFIG = RNG_CONFIG_DERCEN_Enabled << RNG_CONFIG_DERCEN_Pos;

	/* Enable shortcut to stop RNG after generating one value */
	NRF_RNG->SHORTS = RNG_SHORTS_VALRDY_STOP_Enabled <<
						RNG_SHORTS_VALRDY_STOP_Pos;

	return 0;
}

uint8_t random_generate(void)
{
	/* nRF5188 Product Specification v2.0, section 8.16, page 50
	 * Time to generate a byte is typically 677 us */
	NRF_RNG->EVENTS_VALRDY = 0;
	NRF_RNG->TASKS_START = 1;

	while(!NRF_RNG->EVENTS_VALRDY);

	return (uint8_t)(NRF_RNG->VALUE);
}

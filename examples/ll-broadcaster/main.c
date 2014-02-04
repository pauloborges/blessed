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

#include <blessed/common.h>

#include "ll.h"

static const bdaddr_t addr = { { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF },
							BDADDR_TYPE_RANDOM };

/* "blessed project says hello!" */
static uint8_t data[] = { 0x62, 0x6c, 0x65, 0x73, 0x73, 0x65, 0x64, 0x20,
			  0x70, 0x72, 0x6f, 0x6a, 0x65, 0x63, 0x74, 0x20,
			  0x73, 0x61, 0x79, 0x73, 0x20, 0x68, 0x65, 0x6c,
			  0x6c, 0x6f, 0x21 };

int main(void)
{
	ll_init(&addr);
	ll_advertise_start(ADV_NONCONN_UNDIR, data, sizeof(data));

	while (1);

	return 0;
}

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

/* Link Layer specification Section 2.1, Core 4.1 page 2503 */
#define LL_MTU				39

/* Link Layer specification Section 2.3, Core 4.1 page 2504
 * Link Layer specification Section 2.3, Core 4.1 page 2511
 */
#define LL_HEADER_LEN			2

/* Link Layer specification Section 2.3, Core 4.1 pages 2504-2505 */
#define LL_ADV_MTU_PAYLOAD		(LL_MTU - LL_HEADER_LEN)

/* Link Layer specification Section 2.3.1, Core 4.1 page 2506 */
#define LL_ADV_MTU_DATA			(LL_ADV_MTU_PAYLOAD - BDADDR_LEN)

/* Link Layer specification Section 4.4.2.2, Core 4.1 page 2528 */
#define LL_ADV_INTERVAL_MIN_CONN	20	/* 20 ms */
#define LL_ADV_INTERVAL_MIN_NONCONN	100	/* 100 ms */
#define LL_ADV_INTERVAL_MAX		10240	/* 10.24 s */

int16_t ll_init(const bdaddr_t *addr);

int16_t ll_advertise_start(adv_type_t type, uint16_t interval,
					const uint8_t *data, uint8_t len);
int16_t ll_advertise_stop(void);

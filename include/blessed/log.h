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

#ifndef CONFIG_LOG_ENABLE
#define CONFIG_LOG_ENABLE		1
#endif

#if CONFIG_LOG_ENABLE

int16_t log_init(void);

int16_t log_int(int32_t n);
int16_t log_uint(uint32_t n);

int16_t log_char(char c);
int16_t log_string(const char *str);

int16_t log_newline(void);
int16_t log_printf(const char *format, ...);

#else

inline int16_t log_init(void)					{ return 0; };
inline int16_t log_int(int32_t n)				{ return 0; };
inline int16_t log_uint(uint32_t n)				{ return 0; };
inline int16_t log_char(char c)					{ return 0; };
inline int16_t log_string(const char *str)			{ return 0; };
inline int16_t log_newline(void)				{ return 0; };
inline int16_t log_printf(const char *format, ...)		{ return 0; };

#endif

#define __ONLYFILE__							\
	(strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define LOG(level, fmt, arg...)						\
		log_printf(level ":%s:%s() " fmt "\r\n",		\
					__ONLYFILE__, __func__, ## arg)

#define DBG(fmt, arg...)		LOG("DEBUG", fmt, ## arg)

#define ERROR(fmt, arg...)		LOG("ERROR", fmt, ## arg)

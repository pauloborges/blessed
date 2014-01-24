/**
 *  The MIT License (MIT)
 *
 *  Copyright (c) 2013 Cidorvan dos Santos Leite
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

#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "app_gpiote.h"
#include "temp-humi-sensor.h"

#define MAX_ITERATIONS  85
#define COUNT_HIGH      6

/**@brief Reads temperature and humidity.
 *
 * @param[in]   pin     Pin where sensor is connected.
 * @param[out]  tempC   Temperature in celsius.
 * @param[out]  tempF   Temperature in fahrenheit.
 * @param[out]  humi    Humidity.
 */
bool read_temperature_humidity(int pin, int *tempC, float *tempF, int *humi)
{
	unsigned char data[6];
	unsigned int i, b = 0, count, state = 1;

	data[0] = data[1] = data[2] = data[3] = data[4] = 0;

	// Send start signal
	GPIO_LED_CONFIG(pin);
	nrf_gpio_pin_clear(pin);
	nrf_delay_ms(20);
	GPIO_BUTTON_CONFIG(pin);

	for (i = 0; i < MAX_ITERATIONS; i++) {
		count = 0;

		// Wait state change
		while (nrf_gpio_pin_read(pin) == state) {
			nrf_delay_us(1);
			if (++count == 255)
				goto validate;
		}

		state = !state;

		// Storage bits
		if (i >= 4 && !(i & 1)) {
			data[b / 8] <<= 1;
			if (count > COUNT_HIGH)
				data[b / 8] |= 1;
			b++;
		}
	}

validate:   // Validate checksum
	if (b >= 40 && ((data[0] + data[1] + data[2] + data[3]) & 0xff) ==
								data[4]) {
		// Celsius temperature
		if (tempC)
			*tempC = data[2];

		// Fahrenheit temperature
		if (tempF)
			*tempF = data[2] * 9 / 5.0 + 32;

		// Humidity
		if (humi)
			*humi = data[0];

		return true;
	}

	return false;
}

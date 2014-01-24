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
#include <stdio.h>

/* Platform specific headers */
#include <boards.h>
#include <string.h>
#include <app_common/app_gpiote.h>
#include "bluetooth.h"

/* Project headers */
#include "temp-humi-sensor.h"
#include "log.h"
#include "timer.h"

#define BLE_ADDRESS_ADDR		{ 0xEF, 0xBE, 0xAD, 0xDE, 0x00, 0x00 }
#define BLE_ADDRESS_TYPE		BDADDR_TYPE_RANDOM
#define BLE_ADDRESS			{ BLE_ADDRESS_ADDR, BLE_ADDRESS_TYPE }

#define TEMP_HUMI_PIN		1

static __inline void read_sensor(void *sensor_data)
{
	float tf;
	int tc, humi, msg_len;
	char data[17];
	static int tc_last = 0;

	if (read_temperature_humidity(TEMP_HUMI_PIN, &tc, &tf, &humi)
							&& tc == tc_last)
		return;

	msg_len = snprintf(data, sizeof(data), "[%dC %dF %d%%]",
							tc, (int)tf, humi);
	memset(&data[msg_len], 0, sizeof(data) - msg_len);

	bci_set_advertise_enable(DISABLE);
	bci_set_advertising_data((uint8_t *)data, sizeof(data));
	bci_set_advertise_enable(ENABLE);

	DBG("%s", data);

	tc_last = tc;
}

static __inline void temp_timer_init(void)
{
	int16_t read_timer;

	read_timer = timer_create(TIMER_REPEATED, read_sensor);

	timer_start(read_timer, 1000, NULL);

	DBG("Reading Sensor...");
}

static __inline void bt_init(void)
{
	int16_t status;

	BLUETOOTH_INIT(status);

	if (status < 0) {
		DBG("BlEStack not initialized. Status = %d", status);
		return;
	}

	DBG("BLEStack initialized");
}

int main(void)
{
	log_init();
	bt_init();
	temp_timer_init();

	while (1)
		__WFI();

	return 0;
}

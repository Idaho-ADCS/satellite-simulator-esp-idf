#include "comm.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"

static const int RX_BUF_SIZE = 1024;
static const char *TAG = "tes-uart";

int uart_enabled;

ADCSdata packet_global;
// int num_packets;

void init_uart(void)
{
	const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_ODD,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    // We won't use a buffer for sending data.
    uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

	uart_enabled = 1;

	// xTaskCreate(rx_task, "uart_rx_task", 1024*2, NULL, configMAX_PRIORITIES, NULL);
}

void disable_uart(void)
{
	uart_enabled = 0;
	uart_driver_delete(UART_NUM_1);
}

int send_command(uint8_t cmd)
{
	TEScommand packet;
	packet._command = cmd;
	packet._crc = 0;

    const int len = COMMAND_LEN;
    const int txBytes = uart_write_bytes(UART_NUM_1, packet._data, len);
    ESP_LOGI(TAG, "Wrote %d bytes", txBytes);
    return txBytes;
}

void rx_task(void *arg)
{
	// ADCSdata packet;
	uint8_t *data = (uint8_t *)malloc(RX_BUF_SIZE + 1);
	int seq = 0;
	int i;
	int rxBytesCopy;

	// num_packets = 0;

	while (1)
	{
		if(uart_enabled)
		{
			const int rxBytes = uart_read_bytes(UART_NUM_1, data, RX_BUF_SIZE, 0);

			rxBytesCopy = rxBytes;

			if (rxBytes > 0)
			{
				ESP_LOGI(TAG, "Read %d bytes", rxBytes);
				ESP_LOG_BUFFER_HEXDUMP(TAG, data, rxBytes, ESP_LOG_INFO);

				if (rxBytesCopy >= PACKET_LEN)
				{
					packet_global._seq = seq;
					seq++;

					for (i = 0; i < PACKET_LEN; i++)
					{
						packet_global._data[i] = data[i];
						// rxBytesCopy--;
					}

					// ESP_LOGI(TAG, "Sequence: %d", packet._seq);

					// if (packet._status == STATUS_OK)
					// 	ESP_LOGI(TAG, "Status: OK");
					// if (packet._status == STATUS_HELLO)
					// 	ESP_LOGI(TAG, "Status: HELLO");
					// if (packet._status == STATUS_COMM_ERROR)
					// 	ESP_LOGI(TAG, "Status: COMM ERROR");
					// if (packet._status == STATUS_ADCS_ERROR)
					// 	ESP_LOGI(TAG, "Status: SYSTEM ERROR");

					// ESP_LOGI(TAG, "Voltage: %f", fixedToFloat(packet._voltage));
					// ESP_LOGI(TAG, "Current: %d", packet._current);
					// ESP_LOGI(TAG, "Motor speed: %d", packet._speed);
					// ESP_LOGI(TAG, "Mag X: %d", packet._magX);
					// ESP_LOGI(TAG, "Mag Y: %d", packet._magY);
					// ESP_LOGI(TAG, "Mag Z: %d", packet._magZ);
					// ESP_LOGI(TAG, "Gyro X: %f", fixedToFloat(packet._gyroX));
					// ESP_LOGI(TAG, "Gyro Y: %f", fixedToFloat(packet._gyroY));
					// ESP_LOGI(TAG, "Gyro Z: %f", fixedToFloat(packet._gyroZ));
					

					// packet_global = packet;
					// num_packets++;

					// if (num_packets > 10)
					// 	num_packets = 10;
				}
			}
		}

		vTaskDelay(10 / portTICK_RATE_MS);
	}

	free(data);
}

/**
 * @brief
 * Converts a floating-point number to a fixed-point number with 5 bits for the
 * integer part and 3 bits for the fraction part. Resulting data cannot be
 * properly interpreted until converted back into a float.
 * 
 * @param[in] f  Float to convert
 * 
 * @return Fixed-point conversion
 */
fixed5_3_t floatToFixed(float f)
{
    fixed5_3_t fix;
    fix = (uint8_t)(f * (1 << 3));
    return fix;
}

/**
 * @brief
 * Converts a fixed-point number with 5 integer bits and 3 fraction bits to a
 * floating-point number. Resulting data can be properly interpreted with no
 * extra conversions.
 * 
 * @param[in] fix  Fixed-point number to convert
 * 
 * @return Float conversion
 */
float fixedToFloat(fixed5_3_t fix)
{
    float f;
    f = ((float)fix) / (1 << 3);
    return f;
}
/* WiFi station Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include <sys/fcntl.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "lwip/sockets.h"

#include "driver/uart.h"
#include "driver/gpio.h"

typedef union
{
	uint32_t data;
	char data_arr[4];

	struct
	{
		uint16_t command;
		uint16_t crc;
	};
} ADCScommand;

/* The examples use WiFi configuration that you can set via project configuration menu

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define ESP_WIFI_SSID "INSPIRATOR-FI" // CONFIG_ESP_WIFI_SSID
#define ESP_WIFI_PASS "p#394B27" // CONFIG_ESP_WIFI_PASSWORD
#define ESP_MAXIMUM_RETRY 10 // CONFIG_ESP_MAXIMUM_RETRY
// #define SERVER_IP CONFIG_SERVER_IP
// #define ESP_MESSAGE "Hello world!\r\n" // CONFIG_ESP_TCP_MESSAGE
char ESP_MESSAGE[64];

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static const char *TAG = "satellite";

static int s_retry_num = 0;

static const int RX_BUF_SIZE = 1024;

#define TXD_PIN (GPIO_NUM_4)
#define RXD_PIN (GPIO_NUM_5)

SemaphoreHandle_t send_semphr;

static void event_handler(void *arg, esp_event_base_t event_base,
						  int32_t event_id, void *event_data)
{
	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
	{
		esp_wifi_connect();
	}
	else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
	{
		if (s_retry_num < ESP_MAXIMUM_RETRY)
		{
			esp_wifi_connect();
			s_retry_num++;
			ESP_LOGI(TAG, "retry to connect to the AP");
		}
		else
		{
			xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
		}
		ESP_LOGI(TAG, "connect to the AP fail");
	}
	else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
	{
		ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
		ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
		s_retry_num = 0;
		xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
	}
}

void wifi_init_sta(void)
{
	s_wifi_event_group = xEventGroupCreate();

	ESP_ERROR_CHECK(esp_netif_init());

	ESP_ERROR_CHECK(esp_event_loop_create_default());
	esp_netif_create_default_wifi_sta();

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	esp_event_handler_instance_t instance_any_id;
	esp_event_handler_instance_t instance_got_ip;
	ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
														ESP_EVENT_ANY_ID,
														&event_handler,
														NULL,
														&instance_any_id));
	ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
														IP_EVENT_STA_GOT_IP,
														&event_handler,
														NULL,
														&instance_got_ip));

	wifi_config_t wifi_config = {
		.sta = {
			.ssid = ESP_WIFI_SSID,
			.password = ESP_WIFI_PASS,
			/* Setting a password implies station will connect to all security modes including WEP/WPA.
			 * However these modes are deprecated and not advisable to be used. Incase your Access point
			 * doesn't support WPA2, these mode can be enabled by commenting below line */
			.threshold.authmode = WIFI_AUTH_WPA2_PSK,

			.pmf_cfg = {
				.capable = true,
				.required = false},
		},
	};
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());

	ESP_LOGI(TAG, "wifi_init_sta finished.");

	/* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
	 * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
	EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
										   WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
										   pdFALSE,
										   pdFALSE,
										   portMAX_DELAY);

	/* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
	 * happened. */
	if (bits & WIFI_CONNECTED_BIT)
	{
		ESP_LOGI(TAG, "connected to ap SSID: %s password: %s",
				 ESP_WIFI_SSID, ESP_WIFI_PASS);
	}
	else if (bits & WIFI_FAIL_BIT)
	{
		ESP_LOGI(TAG, "Failed to connect to SSID: %s, password: %s",
				 ESP_WIFI_SSID, ESP_WIFI_PASS);
	}
	else
	{
		ESP_LOGE(TAG, "UNEXPECTED EVENT");
	}

	/* The event will not be processed after unregister */
	ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
	ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
	vEventGroupDelete(s_wifi_event_group);
}

void tcp_server(void *pvParam)
{
	static const int port = 3000;
	xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, false, true, 0);
	tcpip_adapter_ip_info_t ip_info;
	ESP_ERROR_CHECK(tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip_info));
	ESP_LOGI(TAG, "tcp_server task started at %s:%d\n", ip4addr_ntoa(&ip_info.ip), port);
	struct sockaddr_in tcpServerAddr;
	tcpServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	tcpServerAddr.sin_family = AF_INET;
	tcpServerAddr.sin_port = htons(port);
	int s, r;
	char recv_buf[64];
	static struct sockaddr_in remote_addr;
	static unsigned int socklen;
	socklen = sizeof(remote_addr);
	int cs; // client socket
	xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, false, true, 0);
	xSemaphoreTake(send_semphr, 0);
	while (1)
	{
		s = socket(AF_INET, SOCK_STREAM, 0);
		if (s < 0)
		{
			ESP_LOGE(TAG, "... Failed to allocate socket.\n");
			vTaskDelay(1000 / portTICK_PERIOD_MS);
			continue;
		}
		ESP_LOGI(TAG, "... allocated socket\n");
		if (bind(s, (struct sockaddr *)&tcpServerAddr, sizeof(tcpServerAddr)) != 0)
		{
			ESP_LOGE(TAG, "... socket bind failed errno=%d \n", errno);
			close(s);
			vTaskDelay(4000 / portTICK_PERIOD_MS);
			continue;
		}
		ESP_LOGI(TAG, "... socket bind done \n");
		if (listen(s, TCP_LISTEN_BACKLOG) != 0)
		{
			ESP_LOGE(TAG, "... socket listen failed errno=%d \n", errno);
			close(s);
			vTaskDelay(4000 / portTICK_PERIOD_MS);
			continue;
		}
		while (1)
		{
			cs = accept(s, (struct sockaddr *)&remote_addr, &socklen);
			ESP_LOGI(TAG, "New connection request, Request data:");
			// set O_NONBLOCK so that recv will return, otherwise we need to impliment message end
			// detection logic. If know the client message format you should instead impliment logic
			// detect the end of message
			fcntl(cs, F_SETFL, O_NONBLOCK);
			do
			{
				bzero(recv_buf, sizeof(recv_buf));
				r = recv(cs, recv_buf, sizeof(recv_buf) - 1, 0);
				for (int i = 0; i < r; i++)
				{
					putchar(recv_buf[i]);
				}
			} while (r > 0);

			ESP_LOGI(TAG, "... done reading from socket. Last read return=%d errno=%d\r\n", r, errno);

			while (1)
			{
				xSemaphoreTake(send_semphr, portMAX_DELAY);

				if (write(cs, ESP_MESSAGE, strlen(ESP_MESSAGE)) < 0)
				{
					ESP_LOGE(TAG, "... Send failed \n");
					close(s);
					break;
				}
				ESP_LOGI(TAG, "... socket send success");
				vTaskDelay(1000 / portTICK_PERIOD_MS);
			}
			close(cs);
		}
		ESP_LOGI(TAG, "... server will be opened in 5 seconds");
		vTaskDelay(5000 / portTICK_PERIOD_MS);
	}
	ESP_LOGI(TAG, "...tcp_client task closed\n");
}

void init_uart(void) {
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
}

int sendData(const char* logName, const char* data)
{
    const int len = 4;// strlen(data);
    const int txBytes = uart_write_bytes(UART_NUM_1, data, len);
    ESP_LOGI(logName, "Wrote %d bytes", txBytes);
    return txBytes;
}

static void tx_task(void *arg)
{
	// char command[3] = {0xa0, 0x01, 0x00};

	ADCScommand command;
	command.data = 0;

    static const char *TX_TASK_TAG = "TX_TASK";
    esp_log_level_set(TX_TASK_TAG, ESP_LOG_INFO);

	while (1)
	{
		command.command = 0xa0;
		sendData(TX_TASK_TAG, command.data_arr);
		ESP_LOGI(TAG, "sent test command");
		vTaskDelay(10000 / portTICK_PERIOD_MS);

		command.command = 0xc0;
		sendData(TX_TASK_TAG, command.data_arr);
		ESP_LOGI(TAG, "sent standby command");
		vTaskDelay(10000 / portTICK_PERIOD_MS);
	}
}

static void rx_task(void *arg)
{
	static const char *RX_TASK_TAG = "RX_TASK";
	esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
	uint8_t *data = (uint8_t *)malloc(RX_BUF_SIZE + 1);
	while (1)
	{
		const int rxBytes = uart_read_bytes(UART_NUM_1, data, RX_BUF_SIZE, 1000 / portTICK_RATE_MS);
		if (rxBytes > 0)
		{
			data[rxBytes] = 0;
			ESP_LOGI(RX_TASK_TAG, "Read %d bytes: '%s'", rxBytes, data);
			ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, data, rxBytes, ESP_LOG_INFO);

			// ESP_MESSAGE = "Hello world!\r\n";
			// sprintf(ESP_MESSAGE, "%d ", data);
			xSemaphoreGive(send_semphr);
		}
	}
	free(data);
}

void app_main(void)
{
	send_semphr = xSemaphoreCreateBinary();

	// Initialize NVS
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");

	

	wifi_init_sta();
	init_uart();
	
	xTaskCreate(&tcp_server, "tcp_server", 4096, NULL, 5, NULL);
	xTaskCreate(rx_task, "uart_rx_task", 1024*2, NULL, configMAX_PRIORITIES, NULL);
    xTaskCreate(tx_task, "uart_tx_task", 1024*2, NULL, configMAX_PRIORITIES-1, NULL);
}

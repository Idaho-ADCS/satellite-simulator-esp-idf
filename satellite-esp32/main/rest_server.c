/* HTTP Restful API Server

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "comm.h"

#include <string.h>
#include <fcntl.h>
#include "esp_http_server.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include "cJSON.h"
#include "driver/gpio.h"

#define GPIO_ENABLE 0

static const char *REST_TAG = "tes-rest";

extern ADCSdata packet_global;
// extern int num_packets;

#define REST_CHECK(a, str, goto_tag, ...)                                              \
    do                                                                                 \
    {                                                                                  \
        if (!(a))                                                                      \
        {                                                                              \
            ESP_LOGE(REST_TAG, "%s(%d): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
            goto goto_tag;                                                             \
        }                                                                              \
    } while (0)

#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + 128)
#define SCRATCH_BUFSIZE (10240)

typedef struct rest_server_context {
    char base_path[ESP_VFS_PATH_MAX + 1];
    char scratch[SCRATCH_BUFSIZE];
} rest_server_context_t;

#define CHECK_FILE_EXTENSION(filename, ext) (strcasecmp(&filename[strlen(filename) - strlen(ext)], ext) == 0)

/* Set HTTP response content type according to file extension */
static esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filepath)
{
    const char *type = "text/plain";
    if (CHECK_FILE_EXTENSION(filepath, ".html")) {
        type = "text/html";
    } else if (CHECK_FILE_EXTENSION(filepath, ".js")) {
        type = "application/javascript";
    } else if (CHECK_FILE_EXTENSION(filepath, ".css")) {
        type = "text/css";
    } else if (CHECK_FILE_EXTENSION(filepath, ".png")) {
        type = "image/png";
    } else if (CHECK_FILE_EXTENSION(filepath, ".ico")) {
        type = "image/x-icon";
    } else if (CHECK_FILE_EXTENSION(filepath, ".svg")) {
        type = "text/xml";
    }
    return httpd_resp_set_type(req, type);
}

/* Send HTTP response with the contents of the requested file */
static esp_err_t rest_common_get_handler(httpd_req_t *req)
{
    char filepath[FILE_PATH_MAX];

    rest_server_context_t *rest_context = (rest_server_context_t *)req->user_ctx;
    strlcpy(filepath, rest_context->base_path, sizeof(filepath));
    if (req->uri[strlen(req->uri) - 1] == '/') {
        strlcat(filepath, "/index.html", sizeof(filepath));
    } else {
        strlcat(filepath, req->uri, sizeof(filepath));
    }
    int fd = open(filepath, O_RDONLY, 0);
    if (fd == -1) {
        ESP_LOGE(REST_TAG, "Failed to open file : %s", filepath);
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
        return ESP_FAIL;
    }

    set_content_type_from_file(req, filepath);

    char *chunk = rest_context->scratch;
    ssize_t read_bytes;
    do {
        /* Read file in chunks into the scratch buffer */
        read_bytes = read(fd, chunk, SCRATCH_BUFSIZE);
        if (read_bytes == -1) {
            ESP_LOGE(REST_TAG, "Failed to read file : %s", filepath);
        } else if (read_bytes > 0) {
            /* Send the buffer contents as HTTP response chunk */
            if (httpd_resp_send_chunk(req, chunk, read_bytes) != ESP_OK) {
                close(fd);
                ESP_LOGE(REST_TAG, "File sending failed!");
                /* Abort sending file */
                httpd_resp_sendstr_chunk(req, NULL);
                /* Respond with 500 Internal Server Error */
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
                return ESP_FAIL;
            }
        }
    } while (read_bytes > 0);
    /* Close file after sending complete */
    close(fd);
    ESP_LOGI(REST_TAG, "File sending complete");
    /* Respond with an empty chunk to signal HTTP response completion */
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t adcs_enable_post_handler(httpd_req_t *req)
{
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len) {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0) {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    cJSON *root = cJSON_Parse(buf);
    int enable = cJSON_GetObjectItem(root, "enable")->valueint;

	// if (!enable)
	// 	send_command(CMD_STANDBY);
	
	gpio_set_level(GPIO_ENABLE, enable);
    ESP_LOGI(REST_TAG, "ADCS enable: %d", enable);
	cJSON_Delete(root);

	if (enable)
	{
		init_uart();
		send_command(CMD_HEARTBEAT);
    	httpd_resp_sendstr(req, "Enabled ADCS");
	}
	else
	{
		disable_uart();
		gpio_set_direction(TXD_PIN, GPIO_MODE_OUTPUT);
		gpio_set_level(TXD_PIN, 0);
		httpd_resp_sendstr(req, "Disabled ADCS");
	}

    return ESP_OK;
}

static esp_err_t adcs_mode_post_handler(httpd_req_t *req)
{
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len) {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0) {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    cJSON *root = cJSON_Parse(buf);
    int mode = cJSON_GetObjectItem(root, "mode")->valueint;
    ESP_LOGI(REST_TAG, "ADCS mode: %d", mode);
	cJSON_Delete(root);

	switch (mode)
	{
		case 0:
		send_command(CMD_STANDBY);
    	httpd_resp_sendstr(req, "Set ADCS mode to standby");
		break;

		case 1:
		send_command(CMD_HEARTBEAT);
		httpd_resp_sendstr(req, "Set ADCS mode to measure");
		break;

		case 2:
		send_command(CMD_TST_SIMPLE_DETUMBLE);
		httpd_resp_sendstr(req, "Initiating detumble test");
		break;

		case 3:
		send_command(CMD_TST_BASIC_MOTION);
		httpd_resp_sendstr(req, "Initiating motion test");
		break;

		case 4:
		send_command(CMD_TST_PHOTODIODES);
		httpd_resp_sendstr(req, "Initiating photodiode test");
		break;

		case 5:
		send_command(CMD_TST_SIMPLE_ORIENT);
		httpd_resp_sendstr(req, "Initiating orientation test");
		break;

		default:
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Mode not recognized");
		return ESP_FAIL;
	}
	
    return ESP_OK;
}

static esp_err_t adcs_data_get_handler(httpd_req_t *req)
{
	ADCSdata packet_copy;
	// const int len = num_packets;

	// int i;
	// for (i = 0; i < len; i++)
	// {
	// 	packets_copy[len-i-1] = packets[len-i-1];
	// 	num_packets--;
	// }

	int i;
	for (i = 0; i < PACKET_LEN; i++)
	{
		packet_copy._data[i] = packet_global._data[i];
	}
	packet_copy._seq = packet_global._seq;

    httpd_resp_set_type(req, "application/json");
	// cJSON *arr = cJSON_CreateArray();

	// for (i = 0; i < len; i++)
	// {
		cJSON *obj = cJSON_CreateObject();
		cJSON_AddNumberToObject(obj, "seq", packet_copy._seq);
		
		if (packet_copy._status == STATUS_HELLO)
			cJSON_AddStringToObject(obj, "status", "HELLO");
		if (packet_copy._status == STATUS_OK)
			cJSON_AddStringToObject(obj, "status", "OK");
		if (packet_copy._status == STATUS_COMM_ERROR)
			cJSON_AddStringToObject(obj, "status", "COMM ERROR");
		if (packet_copy._status == STATUS_ADCS_ERROR)
			cJSON_AddStringToObject(obj, "status", "SYSTEM ERROR");
		if (packet_copy._status == STATUS_FUDGED)
			cJSON_AddStringToObject(obj, "status", "FUDGED");

		cJSON_AddNumberToObject(obj, "voltage", fixedToFloat(packet_copy._voltage));
		cJSON_AddNumberToObject(obj, "current", packet_copy._current);
		cJSON_AddNumberToObject(obj, "speed", packet_copy._speed);
		cJSON_AddNumberToObject(obj, "magx", packet_copy._magX);
		cJSON_AddNumberToObject(obj, "magy", packet_copy._magY);
		cJSON_AddNumberToObject(obj, "magz", packet_copy._magZ);
		cJSON_AddNumberToObject(obj, "gyrox", fixedToFloat(packet_copy._gyroX));
		cJSON_AddNumberToObject(obj, "gyroy", fixedToFloat(packet_copy._gyroY));
		cJSON_AddNumberToObject(obj, "gyroz", fixedToFloat(packet_copy._gyroZ));

		// cJSON_AddItemToArray(arr, obj);

		// cJSON_Delete(obj);
	// }



    // cJSON *root = cJSON_CreateObject();
    // esp_chip_info_t chip_info;
    // esp_chip_info(&chip_info);
    // cJSON_AddStringToObject(root, "version", IDF_VER);
    // cJSON_AddNumberToObject(root, "cores", chip_info.cores);
    const char *data = cJSON_Print(obj);
    httpd_resp_sendstr(req, data);
    free((void *)data);
    cJSON_Delete(obj);
    return ESP_OK;
}

/* Simple handler for getting system handler */
static esp_err_t system_info_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateObject();
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    cJSON_AddStringToObject(root, "version", IDF_VER);
    cJSON_AddNumberToObject(root, "cores", chip_info.cores);
    const char *sys_info = cJSON_Print(root);
    httpd_resp_sendstr(req, sys_info);
    free((void *)sys_info);
    cJSON_Delete(root);
    return ESP_OK;
}

/* Simple handler for getting temperature data */
static esp_err_t temperature_data_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "raw", esp_random() % 20);
    const char *sys_info = cJSON_Print(root);
    httpd_resp_sendstr(req, sys_info);
    free((void *)sys_info);
    cJSON_Delete(root);
    return ESP_OK;
}

esp_err_t start_rest_server(const char *base_path)
{
    REST_CHECK(base_path, "wrong base path", err);
    rest_server_context_t *rest_context = calloc(1, sizeof(rest_server_context_t));
    REST_CHECK(rest_context, "No memory for rest context", err);
    strlcpy(rest_context->base_path, base_path, sizeof(rest_context->base_path));

    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;

    ESP_LOGI(REST_TAG, "Starting HTTP Server");
    REST_CHECK(httpd_start(&server, &config) == ESP_OK, "Start server failed", err_start);

    /* URI handler for fetching system info */
    httpd_uri_t system_info_get_uri = {
        .uri = "/api/v1/system/info",
        .method = HTTP_GET,
        .handler = system_info_get_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &system_info_get_uri);

    /* URI handler for fetching temperature data */
    httpd_uri_t temperature_data_get_uri = {
        .uri = "/api/v1/temp/raw",
        .method = HTTP_GET,
        .handler = temperature_data_get_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &temperature_data_get_uri);

	httpd_uri_t adcs_enable_post_uri = {
        .uri = "/api/adcs/enable",
        .method = HTTP_POST,
        .handler = adcs_enable_post_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &adcs_enable_post_uri);

	httpd_uri_t adcs_mode_post_uri = {
        .uri = "/api/adcs/mode",
        .method = HTTP_POST,
        .handler = adcs_mode_post_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &adcs_mode_post_uri);

	httpd_uri_t adcs_data_get_uri = {
        .uri = "/api/adcs/data",
        .method = HTTP_GET,
        .handler = adcs_data_get_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &adcs_data_get_uri);

    /* URI handler for getting web server files */
    httpd_uri_t common_get_uri = {
        .uri = "/*",
        .method = HTTP_GET,
        .handler = rest_common_get_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &common_get_uri);

    return ESP_OK;
err_start:
    free(rest_context);
err:
    return ESP_FAIL;
}

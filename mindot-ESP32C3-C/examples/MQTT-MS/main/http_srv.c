/* HTTP Restful API Server

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <fcntl.h>
#include "esp_http_server.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include "cJSON.h"

#define ROOT_CA_FILE "/spiffs/AmazonRootCA.pem"
#define DATA_MODEL_FILE "/spiffs/data_model.json"
#define MAPPIING_MODEL_FILE "/spiffs/mapping_model.json"
#define PARAM_CONFIG_FILE "/spiffs/param_config.json"
#define DEV_CERT_FILE "/spiffs/dev_cert.pem"
#define DEV_KEY_FILE "/spiffs/dev_key.key"

static const char *REST_TAG = "esp-rest";
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

static void update_json_node(cJSON * node, char * key, cJSON * target)
{
    cJSON * item = cJSON_GetObjectItem(node, key);

    if (item != NULL) {
        cJSON_DetachItemFromObject(target, key);
        cJSON_AddItemToObject(target, key, cJSON_CreateString(item->valuestring));
    }

}
/* Simple handler for light brightness control */
static esp_err_t config_param_post_handler(httpd_req_t *req)
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

    FILE * fd = fopen(PARAM_CONFIG_FILE, "r");
    if (fd == NULL) {
        ESP_LOGE(REST_TAG, "Failed to open file : %s", PARAM_CONFIG_FILE);
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
        return ESP_FAIL;
    }  
    
    char config[1024] = {0};
    fseek(fd, 0, SEEK_SET);
    fread(config, 1, 1024, fd);
    ESP_LOGI(REST_TAG, "param config : %s", config);
    
    fclose(fd);

    cJSON *config_root = cJSON_Parse(config);

    update_json_node(root, "SSID", config_root);
    update_json_node(root, "PASSWD", config_root);
    update_json_node(root, "AP_SSID", config_root);
    update_json_node(root, "AP_PASSWD", config_root);
    update_json_node(root, "Broker", config_root);
    update_json_node(root, "Client_Id", config_root);
    update_json_node(root, "Tenant", config_root);

    char * updated_config = cJSON_Print(config_root);

    fd = fopen(PARAM_CONFIG_FILE, "w");
    if (fd == NULL) {
        ESP_LOGE(REST_TAG, "Failed to open file : %s", PARAM_CONFIG_FILE);
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
        return ESP_FAIL;
    }
    fwrite(updated_config, 1, strlen(updated_config), fd);
    fclose(fd);

    free(updated_config);
    cJSON_Delete(root);
    cJSON_Delete(config_root);
    httpd_resp_sendstr(req, "Post parameter successfully");
    return ESP_OK;
}

static esp_err_t root_ca_post_handler(httpd_req_t *req) 
{
    int total_len = req->content_len;
    int cur_len = 0;
    rest_server_context_t * srv_ctx = (rest_server_context_t *)req->user_ctx;
    char *buf = srv_ctx->scratch;
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
    char * root_ca = cJSON_GetObjectItem(root, "context")->valuestring;
    ESP_LOGI(REST_TAG, "Root CA: %s, %d", root_ca, strlen(root_ca));

    FILE * fd = fopen(ROOT_CA_FILE, "w");
    if (fd == NULL) {
        ESP_LOGE(REST_TAG, "Failed to open file : %s", ROOT_CA_FILE);
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
        return ESP_FAIL;
    }
    fwrite(root_ca, 1, strlen(root_ca), fd);
    
    fclose(fd);

    cJSON_Delete(root);
    httpd_resp_sendstr(req, "Post file successfully");
    return ESP_OK;
}

static esp_err_t data_model_post_handler(httpd_req_t *req) 
{
    int total_len = req->content_len;
    int cur_len = 0;
    rest_server_context_t * srv_ctx = (rest_server_context_t *)req->user_ctx;
    char *buf = srv_ctx->scratch;
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
    char * data_model = cJSON_GetObjectItem(root, "context")->valuestring;
    ESP_LOGI(REST_TAG, "Data Model: %s", data_model);

    FILE * fd = fopen(DATA_MODEL_FILE, "w");
    if (fd == NULL) {
        ESP_LOGE(REST_TAG, "Failed to open file : %s", DATA_MODEL_FILE);
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
        return ESP_FAIL;
    }
    fwrite(data_model, 1, strlen(data_model), fd);

    fclose(fd);

    fd = fopen(DATA_MODEL_FILE, "r");

    char test[16];
    fseek(fd, 0, SEEK_SET);
    fread(test, 1, 16, fd);
    ESP_LOGI(REST_TAG, "test file : %s", test);
    
    fclose(fd);

    cJSON_Delete(root);
    httpd_resp_sendstr(req, "Post file successfully");
    return ESP_OK;
}

static esp_err_t mapping_model_post_handler(httpd_req_t *req) 
{
    int total_len = req->content_len;
    int cur_len = 0;
    rest_server_context_t * srv_ctx = (rest_server_context_t *)req->user_ctx;
    char *buf = srv_ctx->scratch;
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
    char * mapping_model = cJSON_GetObjectItem(root, "context")->valuestring;
    ESP_LOGI(REST_TAG, "Mapping Model: %s", mapping_model);

    FILE * fd = fopen(MAPPIING_MODEL_FILE, "w");
    if (fd == NULL) {
        ESP_LOGE(REST_TAG, "Failed to open file : %s", MAPPIING_MODEL_FILE);
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
        return ESP_FAIL;
    }
    fwrite(mapping_model, 1, strlen(mapping_model), fd);

    fclose(fd);

    fd = fopen(MAPPIING_MODEL_FILE, "r");

    char test[16];
    fseek(fd, 0, SEEK_SET);
    fread(test, 1, 16, fd);
    ESP_LOGE(REST_TAG, "test file : %s", test);
    
    fclose(fd);

    cJSON_Delete(root);
    httpd_resp_sendstr(req, "Post file successfully");
    return ESP_OK;
}

static esp_err_t dev_cert_post_handler(httpd_req_t *req) 
{
    int total_len = req->content_len;
    int cur_len = 0;
    rest_server_context_t * srv_ctx = (rest_server_context_t *)req->user_ctx;
    char *buf = srv_ctx->scratch;
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
    char * dev_cert = cJSON_GetObjectItem(root, "context")->valuestring;
    ESP_LOGI(REST_TAG, "DEV Cert: %s", dev_cert);

    FILE * fd = fopen(DEV_CERT_FILE, "w");
    if (fd == NULL) {
        ESP_LOGE(REST_TAG, "Failed to open file : %s", DEV_CERT_FILE);
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
        return ESP_FAIL;
    }
    fwrite(dev_cert, 1, strlen(dev_cert), fd);

    fclose(fd);

    fd = fopen(DEV_CERT_FILE, "r");

    char test[16];
    fseek(fd, 0, SEEK_SET);
    fread(test, 1, 16, fd);
    ESP_LOGE(REST_TAG, "test file : %s", test);
    
    fclose(fd);

    cJSON_Delete(root);
    httpd_resp_sendstr(req, "Post file successfully");
    return ESP_OK;
}

static esp_err_t dev_key_post_handler(httpd_req_t *req) 
{
    int total_len = req->content_len;
    int cur_len = 0;
    rest_server_context_t * srv_ctx = (rest_server_context_t *)req->user_ctx;
    char *buf = srv_ctx->scratch;
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
    char * dev_key = cJSON_GetObjectItem(root, "context")->valuestring;
    ESP_LOGI(REST_TAG, "DEV Key: %s", dev_key);

    FILE * fd = fopen(DEV_KEY_FILE, "w");
    if (fd == NULL) {
        ESP_LOGE(REST_TAG, "Failed to open file : %s", DEV_KEY_FILE);
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
        return ESP_FAIL;
    }
    fwrite(dev_key, 1, strlen(dev_key), fd);

    fclose(fd);

    fd = fopen(DEV_KEY_FILE, "r");

    char test[16];
    fseek(fd, 0, SEEK_SET);
    fread(test, 1, 16, fd);
    ESP_LOGE(REST_TAG, "test file : %s", test);
    
    fclose(fd);

    cJSON_Delete(root);
    httpd_resp_sendstr(req, "Post file successfully");
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
    config.max_uri_handlers = 9;

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

    /* URI handler for light brightness control */
    httpd_uri_t light_brightness_post_uri = {
        .uri = "/api/v1/config/param",
        .method = HTTP_POST,
        .handler = config_param_post_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &light_brightness_post_uri);

    httpd_uri_t root_ca_post_uri = {
        .uri = "/api/v1/upload/amazon-CA",
        .method = HTTP_POST,
        .handler = root_ca_post_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &root_ca_post_uri);

    httpd_uri_t dev_cert_post_uri = {
        .uri = "/api/v1/upload/dev-cert",
        .method = HTTP_POST,
        .handler = dev_cert_post_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &dev_cert_post_uri);

    httpd_uri_t dev_key_post_uri = {
        .uri = "/api/v1/upload/dev-key",
        .method = HTTP_POST,
        .handler = dev_key_post_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &dev_key_post_uri);

    httpd_uri_t data_model_post_uri = {
        .uri = "/api/v1/upload/data-model",
        .method = HTTP_POST,
        .handler = data_model_post_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &data_model_post_uri);

    httpd_uri_t mapping_model_post_uri = {
        .uri = "/api/v1/upload/mapping-model",
        .method = HTTP_POST,
        .handler = mapping_model_post_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &mapping_model_post_uri);

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

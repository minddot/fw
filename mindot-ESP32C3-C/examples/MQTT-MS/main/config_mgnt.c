#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "cJSON.h"
#include "config_mgnt.h"

#include "esp_log.h"

#define TAG "MQTT-MS"

static char * sta_ssid = "";
static char * sta_passwd = "";
static char * ap_ssid = "";
static char * ap_passwd = "";
static char * tenant = "";
static char * client_id = "";
static char * broker = "";


void init_config(void) 
{
    FILE * fd = fopen("/spiffs/param_config.json", "r");

    char config[1024] = {0};
    fseek(fd, 0, SEEK_SET);
    fread(config, 1, 1024, fd);
    fclose(fd);

    cJSON *config_root = cJSON_Parse(config);

    sta_ssid = cJSON_GetObjectItem(config_root, "SSID")->valuestring;
    sta_passwd = cJSON_GetObjectItem(config_root, "PASSWD")->valuestring;
    ap_ssid = cJSON_GetObjectItem(config_root, "AP_SSID")->valuestring;
    ap_passwd = cJSON_GetObjectItem(config_root, "AP_PASSWD")->valuestring;
    tenant = cJSON_GetObjectItem(config_root, "Client_Id")->valuestring;
    client_id = cJSON_GetObjectItem(config_root, "Tenant")->valuestring;
    broker = cJSON_GetObjectItem(config_root, "Broker")->valuestring;

    cJSON_Delete(config_root);
}

char * get_sta_ssid(void) 
{
    return sta_ssid;
}

char * get_sta_passwd(void) 
{
    return sta_passwd;
}

char * get_ap_ssid(void) 
{
    return ap_ssid;
}

char * get_ap_passwd(void) 
{
    return ap_passwd;
}

char * get_tenant(void) 
{
    return tenant;
}

char * get_client_id(void) 
{
    return client_id;
}

char * get_broker(void) 
{
    return broker;
}

char * read_mqtt_config_info(char * file_path)
{
    ESP_LOGI(TAG, "Reading configuration");

    char * buf = 0;
    // Open for reading hello.txt   "/spiffs/creating_data_model.json"
    FILE* f = fopen(file_path, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open %s", file_path);
        return buf;
    }

    fseek(f, 0, SEEK_END);
    int len = ftell(f);
    fseek(f, 0, SEEK_SET);

    buf = (char *)malloc(len+1);

    memset(buf, 0, len+1);
    fread(buf, 1, len, f);
    fclose(f);

    // Display the read contents from the file
    ESP_LOGI(TAG, "Read from %s: %s", file_path, buf);

    return buf;
}




 //#include <stdio.h>
#include <string.h>

// #include "platform.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "driver/gpio.h"
#include "sdkconfig.h"


#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_spiffs.h"

#include "cJSON.h"
#include "time.h"

#include "ntp_srv.h"
// #include "BME280_drv.h"
// #include "mpu9250_drv.h"

#include "http_app.h"
#include "mqtt_app.h"
#include "config_mgnt.h"


#define DEFAULT_SSID "Xiaomi_E238"
// #define DEFAULT_SSID "GoodLucy"
#define DEFAULT_PWD "19871017"

#define DEFAULT_AP_SSID "MINDOT-WX"
#define DEFAULT_AP_PWD "00000000"

#define TAG "MQTT-MS"



static EventGroupHandle_t wifi_event_group;
// static esp_netif_t *sta_netif = NULL;
const int CONNECTED_BIT = BIT0;

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_ERROR_CHECK(esp_wifi_connect());
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_START");
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_DISCONNECTED");
        ESP_ERROR_CHECK(esp_wifi_connect());
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));

        // xTaskCreate(&mqtt_ms_task, "mqtt_ms_task", 1024 * 12, NULL, 5, NULL);
        ESP_LOGI(TAG, "RAM left %d", esp_get_free_heap_size());

        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
    }
}


static void wifi_scan(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    // assert(sta_netif);
    esp_netif_create_default_wifi_ap();
    // assert(ap_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg));

    char * sta_ssid = NULL;
    char * sta_passwd = NULL;
    char * ap_ssid = NULL;
    char * ap_passwd = NULL;
    
    if (strlen(get_sta_ssid()) == 0) {
        sta_ssid = DEFAULT_SSID;
        sta_passwd = DEFAULT_PASSWD;
    };

    if (strlen(get_ap_ssid()) == 0) {
        ap_ssid = DEFAULT_AP_SSID;
        ap_passwd = DEFAULT_AP_PASSWD;
    }

    wifi_config_t wifi_sta_config = {
        .sta = {
            .ssid = sta_ssid,
            .password = sta_passwd
        },
    };

    wifi_config_t wifi_ap_config = {
        .ap = {
            .ssid = ap_ssid,
            .password = ap_passwd,
            // .channel = ESP_WIFI_AP_CHANNEL,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };

    ESP_ERROR_CHECK( esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL) );
    ESP_ERROR_CHECK( esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL) );
    
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_config));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_config));

    ESP_ERROR_CHECK(esp_wifi_start());

}

void init_fs(void)
{
    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = false
    };

    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }

}


void app_main()
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    init_fs();


    // All done, unmount partition and disable SPIFFS
    // esp_vfs_spiffs_unregister(NULL);
    // ESP_LOGI(TAG, "SPIFFS unmounted");

    wifi_scan();

    
    // MPU9250_init();
    // init_BME280();

    // xTaskCreate(&mqtt_ms_task, "mqtt_ms_task", configMINIMAL_STACK_SIZE, NULL, 5, NULL);
    http_srv_process();
}





#include <stdio.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/apps/sntp.h"
#include "esp_log.h"

#include "ntp_srv.h"


static const char *TAG = "SNTP";

void init_SNTP(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
//    sntp_setservername(0, "cn.pool.ntp.org");
    sntp_setservername(0, "cn.ntp.org.cn");

    // set timezone to China Standard Time
    setenv("TZ", "CST-8", 1);
    tzset();

    sntp_init();
}

void stop_SNTP(void)
{
    sntp_stop();
}

void wait_SNTP_sync(void)
{
    char stmp_str[80] = {0};
    time_t now = 0;
    struct tm time_info = {0};


    while (time_info.tm_year < (2021 - 1900)) {
        ESP_LOGD(TAG, "Waiting for system time to be set...");
        
        time(&now);
        localtime_r(&now, &time_info);

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    sprintf(stmp_str, "%04d-%02d-%02dT%02d:%02d:%02d.000Z", time_info.tm_year+1900, 
        time_info.tm_mon+1, time_info.tm_mday, time_info.tm_hour, time_info.tm_min, time_info.tm_sec);

    ESP_LOGD(TAG, "The current date/time in Shanghai is: %s", stmp_str);
}


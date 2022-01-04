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

#include "mqtt_client.h"
#include "ntp_srv.h"
// #include "BME280_drv.h"
// #include "mpu9250_drv.h"

#include "mqtt_app.h"
#include "config_param.h"


#define DEFAULT_SSID "Xiaomi_E238"
#define DEFAULT_PWD "19871017"

#define DEFAULT_AP_SSID "MINDOT-WX"
#define DEFAULT_AP_PWD "00000000"

#define TAG "MQTT-MS"
// #define DHT_GPIO 4

#define CONFIG_PUBLISH_DATA_MODEL 0

#define CLIENT_ID  "presiot_MindDot_1"

char * DATA_MODEL_C_PUB_TOPIC = NULL;
char * DATA_MODEL_I_PUB_TOPIC = NULL;
char * TS_PUB_TOPIC = NULL;
char * DATA_MODEL_C_SUB_TOPIC = NULL;
char * DATA_MODEL_I_SUB_TOPIC = NULL;

#define BROKER_URI       "mqtts://avwhozk99tqmx-ats.iot.eu-central-1.amazonaws.com:8883"


static esp_mqtt_client_handle_t client = 0;

static char is_connected = 0;
static char is_created = 0;
static char is_instantiated = 0;

void init_topic(void) 
{
    unsigned int len = strlen(get_tenant()) + strlen(get_client_id()) + 15;

    DATA_MODEL_C_PUB_TOPIC = (char *) malloc(len + 1);
    DATA_MODEL_I_PUB_TOPIC = (char *) malloc(len + 1);
    TS_PUB_TOPIC = (char *) malloc(len + 1);
    DATA_MODEL_C_SUB_TOPIC = (char *) malloc(len + 1 + 1);
    DATA_MODEL_I_SUB_TOPIC = (char *) malloc(len + 1 + 1);

    memset(DATA_MODEL_C_PUB_TOPIC, 0, len+1);
    memset(DATA_MODEL_I_PUB_TOPIC, 0, len+1);
    memset(TS_PUB_TOPIC, 0, len+1);
    memset(DATA_MODEL_C_SUB_TOPIC, 0, len+1+1);
    memset(DATA_MODEL_I_SUB_TOPIC, 0, len+1+1);

    sprintf(DATA_MODEL_C_PUB_TOPIC, "tc/%s/%s/o/amo_v3/m", get_tenant(), get_client_id());
    sprintf(DATA_MODEL_I_PUB_TOPIC, "tc/%s/%s/o/amo_v3/i", get_tenant(), get_client_id());
    sprintf(TS_PUB_TOPIC, "tc/%s/%s/o/mc_v3/ts", get_tenant(), get_client_id());
    sprintf(DATA_MODEL_C_SUB_TOPIC, "tc/%s/%s/i/amo_v3/ms", get_tenant(), get_client_id());
    sprintf(DATA_MODEL_I_SUB_TOPIC, "tc/%s/%s/i/amo_v3/ip", get_tenant(), get_client_id());
}


static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) 
{    
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);   
    
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;       
    esp_mqtt_client_handle_t client = event->client;   
    
    int msg_id;    
    // your_context_t *context = event->context;    
    switch (event->event_id) {        
        case MQTT_EVENT_CONNECTED://����MQTT�ɹ�            
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");    
            is_connected = 1;
            break;        
            
        case MQTT_EVENT_DISCONNECTED://�Ͽ�MQTT            
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");        
            is_connected = 0;
            break;     
            
        case MQTT_EVENT_SUBSCRIBED://���ĳɹ�           
            printf("_---------����--------\n");            
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);            
            msg_id = esp_mqtt_client_publish(client, "/World", "data", 0, 0, 0);            
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);            
            break;       
            
        case MQTT_EVENT_UNSUBSCRIBED://ȡ������            
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);            
            break;       
            
        case MQTT_EVENT_PUBLISHED://�����ɹ�          
            printf("_--------����----------\n");            
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);            
            break;        
            
        case MQTT_EVENT_DATA://���ݽ���            
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");            
            printf("���ⳤ��:%d ���ݳ���:%d\n",event->topic_len,event->data_len);             
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);            
            printf("DATA=%.*s\r\n", event->data_len, event->data);   
            if (strncmp(event->topic, DATA_MODEL_C_SUB_TOPIC, strlen(DATA_MODEL_C_SUB_TOPIC)) == 0) {
                is_created = 1;
            }

            if (strncmp(event->topic, DATA_MODEL_I_SUB_TOPIC, strlen(DATA_MODEL_I_SUB_TOPIC)) == 0) {
                is_instantiated = 1;
            }
            break;     
            
        case MQTT_EVENT_ERROR://MQTT����            
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");            
            break;      
            
        default:            
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);            
            break;    
    }
}

void setup_MQTT()
{
    // Read and display the contents of a small text file.
    
    char * root_CA = read_mqtt_config_info("/spiffs/AmazonRootCA.pem");
    char * dev_cert = read_mqtt_config_info("/spiffs/dev_cert.pem");
    char * dev_key = read_mqtt_config_info("/spiffs/dev_key.key");

    const esp_mqtt_client_config_t mqtt_cfg = {
            // no connection takes place, but the uri has to be valid for init() to succeed
            .uri = get_broker(),
            .client_id = get_client_id(),
            .cert_pem = root_CA,
            // .cert_len = strlen(root_CA),
            .client_cert_pem = dev_cert,
            // .client_cert_len = strlen(dev_cert),
            .client_key_pem = dev_key,
            // .client_key_len = strlen(dev_key)
            
    };
    
    client = esp_mqtt_client_init(&mqtt_cfg);
  
    esp_mqtt_client_register_event(client, MQTT_EVENT_ANY, mqtt_event_handler, client);//ע���¼�
  
    esp_mqtt_client_start(client);//����mQTT

    while (is_connected == 0) {
        ESP_LOGI(TAG, "MQTT Connecting ... ..."); 
        
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    
#if CONFIG_PUBLISH_DATA_MODEL
    char * data_model_c_ctx = read_mqtt_config_info("/spiffs/data_model.json");
    char * data_model_i_ctx = read_mqtt_config_info("/spiffs/mapping_model.json");

    if (is_connected > 0) {
        esp_mqtt_client_subscribe(client, DATA_MODEL_C_SUB_TOPIC, 0);
        esp_mqtt_client_subscribe(client, DATA_MODEL_I_SUB_TOPIC, 0);
    }

    if (is_connected > 0) {
        esp_mqtt_client_publish(client, DATA_MODEL_C_PUB_TOPIC, strlen(data_model_c_ctx), 1, 0);
        while (is_created > 0);
    }

    if (is_connected > 0) {
        esp_mqtt_client_publish(client, DATA_MODEL_I_PUB_TOPIC, strlen(data_model_i_ctx), 1, 0);
        while (is_instantiated > 0);
    }
   
#endif

}

void publish_mindot_data(char ** data_point_pairs, unsigned char pair_num)
{
    if (pair_num == 0) {
        return;
    }
    
    cJSON * root =  cJSON_CreateObject();
    cJSON * timeseries =  cJSON_CreateArray();
    cJSON * values =  cJSON_CreateArray();
    cJSON * timeseries_payload =  cJSON_CreateObject();
    cJSON * values_payload;

    cJSON_AddItemToObject(root, "timeseries", timeseries);
    cJSON_AddItemToArray(timeseries, timeseries_payload);

    char i = 0;
    
    char stmp_str[80] = {0};
    time_t ts = 0;
    
    time(&ts);
    struct tm * stmp = gmtime(&ts);

    sprintf(stmp_str, "%04d-%02d-%02dT%02d:%02d:%02d.000Z", stmp->tm_year+1900, 
        stmp->tm_mon+1, stmp->tm_mday, stmp->tm_hour, stmp->tm_min, stmp->tm_sec);
    
    
    cJSON_AddItemToObject(timeseries_payload, "timestamp", cJSON_CreateString(stmp_str));
    cJSON_AddItemToObject(timeseries_payload, "values", values);
    

    for (i = 0; i < pair_num; i++) {
        values_payload =  cJSON_CreateObject();
        cJSON_AddItemToArray(values, values_payload);
        
        cJSON_AddItemToObject(values_payload, "dataPointId", cJSON_CreateString(data_point_pairs[i*2+0]));
        cJSON_AddItemToObject(values_payload, "value", cJSON_CreateString(data_point_pairs[i*2+1]));
        cJSON_AddItemToObject(values_payload, "qualityCode", cJSON_CreateString("0"));
    }

    char * msg_payload = cJSON_Print(root);
    
    ESP_LOGI(TAG, "MSG payload: %s", msg_payload);

    esp_mqtt_client_publish(client, TS_PUB_TOPIC, msg_payload, strlen(msg_payload), 1, 0);

    free(msg_payload);
    
    cJSON_Delete(root);
}


void mqtt_ms_task(void *pvParameter) 
{
    init_SNTP();
    wait_SNTP_sync();
    
    setup_MQTT();


    char temp_val[16] = {0};
    char vib_val[16] = {0};
    // int temp  = 0;
    // unsigned char buff[6];
    // float x = 0;
    char * data_point_pairs[4] = {0};
    while (1) {
        static int m = 0;
        if (is_connected > 0) {
            memset(temp_val, 0, 16);
            // sprintf(temp_val, "%.2f", read_BME280_temp());

            sprintf(temp_val, "%.2f", m);

            // MPU9250_read_accel(buff, 6);    
            // temp = (((int)buff[0] << 8) + buff[1]);   
        
            // x = (temp&0x8000) ? ((0x8000-(temp&0x7fff))&0x7fff)/32767.0*(-16) : (temp&0x7fff)/32767.0*(16);
            memset(vib_val, 0, 16);
            // sprintf(vib_val, "%.2f", x);
            sprintf(vib_val, "%.2f", m);

            data_point_pairs[0] = "dp01-temperature";
            data_point_pairs[1] = temp_val;
            data_point_pairs[2] = "dp04-acceleration";
            data_point_pairs[3] = vib_val;
            
            publish_mindot_data(data_point_pairs, 2);

            m++;
            m %= 1000;
        }
        
        
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    
    ESP_LOGI(TAG, "mqtt_task going to return");
    
    esp_mqtt_client_destroy(client);

    stop_SNTP();
    
    vTaskDelete(NULL);
}



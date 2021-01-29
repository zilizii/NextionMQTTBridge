/*
 * NextionMQTTBridge.cpp
 *
 *  Created on: 2019. Jan. 11.
 *      Author: zilizii
 */

#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string>
#include <map>
#include <vector>
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_intr.h"
#include "esp_intr_alloc.h"
#include "nvs_flash.h"
#include "esp_int_wdt.h"
#include "esp_task_wdt.h"
#include "esp_log.h"

#include "driver/i2c.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_err.h"
#include "esp_intr_alloc.h"
#include "mqtt_client.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "lwip/apps/sntp.h"


#include "soc/cpu.h"
#include "soc/dport_reg.h"
#include "soc/io_mux_reg.h"
#include "soc/rtc_cntl_reg.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "UART.h"
#include "esp_idf_lib_helpers.h"

#include "bmp280.h"


#include "sdkconfig.h"


/// Define section


extern "C" {
void app_main(void);
}

using namespace std;

template < typename Type > std::string to_str (const Type & t)
{
  std::ostringstream os;
  os << t;
  return os.str ();
}


#ifndef __cplusplus
#define nullptr  NULL
#endif




//#define mqtt_user "sensor"
//#define mqtt_password "Zilizii_senseIot"

#define CONFIG_WIFI_SSID "Zilizi_Wifi"
#define CONFIG_WIFI_PASSWORD "ZiliziWifiAtHome_1"

static EventGroupHandle_t wifi_event_group;
const static int CONNECTED_BIT = BIT0;
static const char *TAG = "NextionMQTTBridge";
/// Function section

static const char * mqtt_user = "sensor";
static const char * mqtt_password = "Zilizii_senseIot";
static volatile esp_mqtt_client_handle_t s_client = nullptr;

QueueHandle_t queueTopic;
int topicSize = 20;

static esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id) {
        case SYSTEM_EVENT_STA_START:
            esp_wifi_connect();
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);

            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            esp_wifi_connect();
            xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
            break;
        default:
            break;
    }
    return ESP_OK;
}

static void wifi_init(void)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_init(wifi_event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    /*wifi_sta_config_t sta = {

    		.ssid = CONFIG_WIFI_SSID,
    		.password=CONFIG_WIFI_PASSWORD,
    };*/

    wifi_sta_config_t sta = {};
    strcpy((char*)sta.ssid, CONFIG_WIFI_SSID);
    strcpy((char*)sta.password, CONFIG_WIFI_PASSWORD);

//    sta.ssid = CONFIG_WIFI_SSID;
//    sta.password = CONFIG_WIFI_PASSWORD;
    sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;


    wifi_config_t wifi_config = {};
    wifi_config.sta = sta;



    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_LOGI(TAG, "start the WIFI SSID:[%s] password:[%s]", CONFIG_WIFI_SSID, "******");
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "Waiting for wifi");

    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
}

static void initialize_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
}


static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    mqttTopic x;
    x.sTopic.clear();
    x.sTopic.clear();
   // int msg_id;
    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            esp_mqtt_client_subscribe(client, "home/temperature", 0);
            esp_mqtt_client_subscribe(client, "kidsroom/lightswitch/status",0);
            esp_mqtt_client_subscribe(client, "enterance/lightswitch/status",0);
            esp_mqtt_client_subscribe(client, "Kitchen_Corr/stat/POWER1",0);
            esp_mqtt_client_subscribe(client, "Kitchen_Corr/stat/POWER2",0);
            esp_mqtt_client_subscribe(client, "livingroom/lightswitch/status",0);

           // ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            //msg_id = esp_mqtt_client_publish(client, CONFIG_EMITTER_CHANNEL_KEY"/topic/", "data", 0, 0, 0);
            //ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");

            x.sData.append(event->data,event->data+event->data_len);
            x.sTopic.append(event->topic,event->topic+event->topic_len);
            xQueueSendToBack(queueTopic, &x, portMAX_DELAY);
            //printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            //printf("DATA=%.*s\r\n", event->data_len, event->data);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
        default:
        	break;
    }
    return ESP_OK;
}

void mqtt_app_start(void * parameters)
{
/*
 *     esp_mqtt_client_config_t mqtt_cfg = {
    		.event_handle = mqtt_event_handler,
    		.host = "192.168.1.140",
			.uri = "mqtt://192.168.1.140:1883",
			.port = 1883,
			.client_id = "NextionMQTTBridge",
			.username = mqtt_user,
			.password = mqtt_password,
    };


    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    s_client = client;
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false,true, 5000 / portTICK_PERIOD_MS);
    esp_mqtt_client_start(client);
*/
    while(1){
    	vTaskDelay(2500 / portTICK_PERIOD_MS);
    	xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false,true, 5000 / portTICK_PERIOD_MS);
    	if(s_client != nullptr)
    		esp_mqtt_client_publish(s_client,"test/NextionMQTTBridge","Hello World",11,0,0);
    	ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    }
    vTaskDelete(NULL);



}


void initNextion() {
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(nUART, &uart_config);
    uart_set_pin(nUART, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    // We won't use a buffer for sending data.
    uart_driver_install(nUART, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
}

int sendData(const char* logName, const char* data)
{
    const int len = strlen(data);
    const int txBytes = uart_write_bytes(nUART, data, len);
    ESP_LOGI(logName, "Wrote %d bytes\n", txBytes);
    ESP_LOGI(logName, "#%s#\n", data);
    return txBytes;
}

void TxTask(void * parameters) {
	string store ="";
	//store += "page0.temp1.txt=\"11\"";
	//store += endData;
	//UBaseType_t ip;
	int qL;
	mqttTopic x;
	while (1) {
		// OK so here is where you can play around and change values or add text and/or numeric fields to the Nextion and update them
		qL = uxQueueMessagesWaiting(queueTopic);
		for(int i = 0; i< qL;i++)
		{
			xQueueReceive(queueTopic,&x,portMAX_DELAY);
			store.clear();
			if( x.sTopic == "livingroom/lightswitch/status")
			{
				store += "page2.livingroom.val=";
				store += (x.sData=="ON") ?"1":"0";
				store += endData;
				sendData(TX_TASK_TAG, store.c_str());
			}
			if( x.sTopic == "kidsroom/lightswitch/status")
			{
				store += "page2.kidsroom.val=";
				store += (x.sData=="ON") ?"1":"0";
				store += endData;
				sendData(TX_TASK_TAG, store.c_str());
			}
			if( x.sTopic == "enterance/lightswitch/status")
			{
				store += "page2.enterance.val=";
				store += (x.sData=="ON") ?"1":"0";
				store += endData;
				sendData(TX_TASK_TAG, store.c_str());
			}
			if( x.sTopic == "Kitchen_Corr/stat/POWER1")
			{
				store += "page2.corridor.val=";
				store += (x.sData=="ON") ?"1":"0";
				store += endData;
				sendData(TX_TASK_TAG, store.c_str());
			}
			if( x.sTopic == "Kitchen_Corr/stat/POWER2")
			{
				store += "page2.kitchen.val=";
				store += (x.sData=="ON") ?"1":"0";
				store += endData;
				sendData(TX_TASK_TAG, store.c_str());
			}
			if( x.sTopic == "local/temp1")
			{
				store += "page0.temp1.txt=\"";
				store += x.sData;
				store += "\"";
				store += endData;
				sendData(TX_TASK_TAG, store.c_str());
			}
			if( x.sTopic == "local/hum1")
			{
				store += "page0.hum1.txt=\"";
				store += x.sData;
				store += "\"";
				store += endData;
				sendData(TX_TASK_TAG, store.c_str());
			}
		}
	}
}

void RxTask(void * parameters) {
	 esp_mqtt_client_config_t mqtt_cfg = {};
	 mqtt_cfg.event_handle = mqtt_event_handler;
	 mqtt_cfg.host = "192.168.1.140";
	 mqtt_cfg.uri = "mqtt://192.168.1.140:1883";
	 mqtt_cfg.port = 1883;
	 mqtt_cfg.client_id = "NextionMQTTBridge";
	 mqtt_cfg.username = mqtt_user;
	 mqtt_cfg.password = mqtt_password;



	    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());

	    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
	    s_client = client;
	    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false,true, 5000 / portTICK_PERIOD_MS);
	    esp_mqtt_client_start(client);

	static const char *RX_TASK_TAG = "RX_TASK";
	esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
	uint8_t* data = (uint8_t*) malloc(RX_BUF_SIZE+1);
	xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false,true, 5000 / portTICK_PERIOD_MS);
	string sdata;
	while (1) {
		sdata.clear();
		xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false,true, 5000 / portTICK_PERIOD_MS);
		const int rxBytes = uart_read_bytes(nUART, data, RX_BUF_SIZE, 500 / portTICK_RATE_MS);
		if (rxBytes > 0) {
			data[rxBytes] = 0;
			ESP_LOGI(RX_TASK_TAG, "Read %d bytes: '%s'", rxBytes, data);
			ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, data, rxBytes, ESP_LOG_INFO);
			sdata += (char *)data;
			printf("RxTask: #%s#",sdata.c_str());
			if(sdata == "livingroom ON") {
				esp_mqtt_client_publish(client, "livingroom/lightswitch/switch", "ON", 0, 0, 0);
			}
			if(sdata == "livingroom OFF") {
				esp_mqtt_client_publish(client, "livingroom/lightswitch/switch", "OFF", 0, 0, 0);
			}
			if(sdata == "enterance ON") {
				esp_mqtt_client_publish(client, "enterance/lightswitch/switch", "ON", 0, 0, 0);
			}
			if(sdata == "enterance OFF") {
				esp_mqtt_client_publish(client, "enterance/lightswitch/switch", "OFF", 0, 0, 0);
			}
			if(sdata == "corridor ON") {
				esp_mqtt_client_publish(client, "Kitchen_Corr/cmnd/POWER1", "ON", 0, 0, 0);
			}
			if(sdata == "corridor OFF") {
				esp_mqtt_client_publish(client, "Kitchen_Corr/cmnd/POWER1", "OFF", 0, 0, 0);
			}
			if(sdata == "kitchen ON") {
				esp_mqtt_client_publish(client, "Kitchen_Corr/cmnd/POWER2", "ON", 0, 0, 0);
			}
			if(sdata == "kitchen OFF") {
				esp_mqtt_client_publish(client, "Kitchen_Corr/cmnd/POWER2", "OFF", 0, 0, 0);
			}
			if(sdata == "kidsroom ON") {
				esp_mqtt_client_publish(client, "kidsroom/lightswitch/switch", "ON", 0, 0, 0);
			}
			if(sdata == "kidsroom OFF") {
				esp_mqtt_client_publish(client, "kidsroom/lightswitch/switch", "OFF", 0, 0, 0);
			}
		}
	}
	free(data);
}

void bmp280_test(void *pvParamters)
{
    string sTemp;
    string sHum;
    mqttTopic xT, xH;
    xT.sTopic = "local/temp1";
    xH.sTopic = "local/hum1";
	bmp280_params_t params;
    bmp280_init_default_params(&params);
    bmp280_t dev;

    esp_err_t res;

    while (i2cdev_init() != ESP_OK)
    {
        printf("Could not init I2Cdev library\n");
        vTaskDelay(250 / portTICK_PERIOD_MS);
    }


    while (bmp280_init_desc(&dev, BMP280_I2C_ADDRESS_0, I2C_NUM_0, gpio_num_t::GPIO_NUM_21, gpio_num_t::GPIO_NUM_22) != ESP_OK)
    {
        printf("Could not init device descriptor\n");
        vTaskDelay(250 / portTICK_PERIOD_MS);
    }

    while ((res = bmp280_init(&dev, &params)) != ESP_OK)
    {
        printf("Could not init BMP280, err: %d\n", res);
        vTaskDelay(250 / portTICK_PERIOD_MS);
    }

    bool bme280p = dev.id == BME280_CHIP_ID;
    printf("BMP280: found %s\n", bme280p ? "BME280" : "BMP280");

    float pressure, temperature, humidity;

    while (1)
    {
        vTaskDelay(500 / portTICK_PERIOD_MS);
        if (bmp280_read_float(&dev, &temperature, &pressure, &humidity) != ESP_OK)
        {
            printf("Temperature/pressure reading failed\n");
            continue;
        }
        xT.sData.clear();

        xT.sData += to_str(temperature) ;
        xQueueSendToBack(queueTopic, &xT, portMAX_DELAY);
        esp_mqtt_client_publish(s_client, "controller/temperature", xT.sData.c_str(), 0, 0, 0);
        printf("Pressure: %.2f Pa, Temperature: %.2f C", pressure, temperature);

        if (bme280p){
            printf(", Humidity: %.2f\n", humidity);
            xH.sData.clear();
            xH.sData += to_str(humidity) ;
            xQueueSendToBack(queueTopic, &xH, portMAX_DELAY);
            esp_mqtt_client_publish(s_client, "controller/humidity", xH.sData.c_str(), 0, 0, 0);
        }
        else
            printf("\n");

        vTaskDelay(9500 / portTICK_PERIOD_MS);
    }
}

void app_main(void) {
	nvs_flash_init();
	initNextion();

	queueTopic = xQueueCreate(topicSize, sizeof(mqttTopic));
       wifi_init();
       //xTaskCreate(&mqtt_app_start, "MQTT", 2000,NULL,5,NULL );
       xTaskCreate(&TxTask,"UARTTX",2000,NULL,5,NULL);
       xTaskCreate(&RxTask,"UARTRX",2000,NULL,5,NULL);
       xTaskCreatePinnedToCore(bmp280_test, "bmp280_test", configMINIMAL_STACK_SIZE * 10, NULL, 5, NULL, APP_CPU_NUM);
       //xTaskCreate(&bmp280_test,"BMP280",2000,NULL,5,NULL);
       while(1) {
       		vTaskDelay(1000 / portTICK_PERIOD_MS);
       	}
}

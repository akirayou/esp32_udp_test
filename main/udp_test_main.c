#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include <string.h>
#include <sys/socket.h>

static const char *TAG = "UDPbroadcast";
#include "i2c.h"
#include "ccs811.h"



static void initialise_wifi(void);
static esp_err_t event_handler(void *ctx, system_event_t *event);
const uint16_t port = 13531;
int get_socket_error_code(int socket);
int show_socket_error_reason(const char *str, int socket);
static uint32_t wifi_get_broadcast_ip(void);
static int mysocket;
struct sockaddr_in local_addr, remote_addr;



//task for sensing output
//broad cast to subnet  13533/UDP
xTaskHandle sensTaskHandle;
void sensTask(void *pvParameters)
{
    int sensSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (sensSocket < 0)
    {
        ESP_LOGE(TAG, "... faild to socket");
        show_socket_error_reason(TAG, mysocket);
        return;
    }
    int optval = 1;
    setsockopt(sensSocket, SOL_SOCKET, SO_BROADCAST | SO_REUSEADDR, &optval, sizeof(int));
    struct sockaddr_in sens_addr;

    sens_addr.sin_family = AF_INET;
    sens_addr.sin_addr.s_addr = wifi_get_broadcast_ip();
    sens_addr.sin_port = htons(13533);
    while (true)
    {
        static char strBuf[1000];
        static uint16_t co2,voc;
        getCCS811(&co2,&voc);
        sprintf(strBuf,"{\"c\":%u,\"v\":%u}",co2,voc);
        ESP_LOGI(TAG,"%s",strBuf);
        int len=strlen(strBuf);
        sendto(sensSocket, strBuf,len, 0, (struct sockaddr *)&sens_addr, sizeof(sens_addr));
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

//Loop while Wifi connected
//Command recv on 13531/UDP and ack on port 1352/UDP
static void loop(void)
{
    char databuff[1000];
    size_t socklen = sizeof(remote_addr);

    int len = recvfrom(mysocket, databuff, sizeof(databuff), 0, (struct sockaddr *)&remote_addr, &socklen);
    if (len > 0)
    {
        //just echo
        remote_addr.sin_port = htons(13532);
        ESP_LOGI(TAG, "data come from %s", inet_ntoa(remote_addr.sin_addr.s_addr));
        sendto(mysocket, databuff, len, 0, (struct sockaddr *)&remote_addr, sizeof(remote_addr));
    }
}
//setup for all (IO reset)
static void setup(void)
{
    i2c_setup_master();
    initCCS811();
}










////////////////////////////////////////
//// Fllowings are utility functions
///////////////////////////////////////
#define WIFI_SSID CONFIG_WIFI_SSID
#define WIFI_PASS CONFIG_WIFI_PASSWORD
/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;
/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int CONNECTED_BIT = BIT0;
/* Variable holding number of times ESP32 restarted since first boot.
 * It is placed into RTC memory using RTC_DATA_ATTR and
 * maintains its value when ESP32 wakes from deep sleep.
 */
RTC_DATA_ATTR static int boot_count = 0;


#define STACK_SIZE 2000
//setup after Get IP (socket connection)
static esp_err_t setupWithIP(void)
{
    mysocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (mysocket < 0)
    {
        ESP_LOGE(TAG, "... faild to socket");
        show_socket_error_reason(TAG, mysocket);
        return ESP_FAIL;
    }
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(port);
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(mysocket, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0)
    {
        show_socket_error_reason(TAG, mysocket);
        close(mysocket);
        ESP_LOGE(TAG, "... faild to bind");
        return ESP_FAIL;
    }

    struct timeval receiving_timeout;
    receiving_timeout.tv_sec = 1;
    receiving_timeout.tv_usec = 0;
    if (setsockopt(mysocket, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout, sizeof(receiving_timeout)) < 0)
    {
        ESP_LOGE(TAG, "... failed to set socket receiving timeout");
        return ESP_FAIL;
    }

    xTaskCreate(sensTask, "sens Task", STACK_SIZE, 0, tskIDLE_PRIORITY, &sensTaskHandle);
    return ESP_OK;
}

void app_main()
{
    ++boot_count;
    ESP_LOGI(TAG, "Boot count: %d", boot_count);
    setup();
    initialise_wifi();
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
                        false, true, portMAX_DELAY);
    if (ESP_OK == setupWithIP())
    {
        while (xEventGroupGetBits(wifi_event_group) & CONNECTED_BIT)
            loop();
    }
    ESP_ERROR_CHECK(esp_wifi_stop());
    esp_restart();
    //esp_deep_sleep(1000000LL * deep_sleep_sec);
}

static uint32_t wifi_get_local_ip(void)
{
    int bits = xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, 0, 1, 0);
    tcpip_adapter_if_t ifx = TCPIP_ADAPTER_IF_AP;
    tcpip_adapter_ip_info_t ip_info;
    wifi_mode_t mode;

    esp_wifi_get_mode(&mode);
    if (WIFI_MODE_STA == mode)
    {
        bits = xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, 0, 1, 0);
        if (bits & CONNECTED_BIT)
        {
            ifx = TCPIP_ADAPTER_IF_STA;
        }
        else
        {
            ESP_LOGE(TAG, "sta has no IP");
            return 0;
        }
    }

    tcpip_adapter_get_ip_info(ifx, &ip_info);
    return ip_info.ip.addr;
}
static uint32_t wifi_get_broadcast_ip(void)
{
    int bits = xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, 0, 1, 0);
    tcpip_adapter_if_t ifx = TCPIP_ADAPTER_IF_AP;
    tcpip_adapter_ip_info_t ip_info;
    wifi_mode_t mode;

    esp_wifi_get_mode(&mode);
    if (WIFI_MODE_STA == mode)
    {
        bits = xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, 0, 1, 0);
        if (bits & CONNECTED_BIT)
        {
            ifx = TCPIP_ADAPTER_IF_STA;
        }
        else
        {
            ESP_LOGE(TAG, "sta has no IP");
            return 0;
        }
    }

    tcpip_adapter_get_ip_info(ifx, &ip_info);
    return ip_info.ip.addr | (~ip_info.netmask.addr);
}

static void initialise_wifi(void)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id)
    {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        uint32_t ip = wifi_get_local_ip();
        char *ipstr = inet_ntoa(ip);
        ESP_LOGW(TAG, "IP:%s", ipstr);

        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        /* This is a workaround as ESP32 WiFi libs don't currently
           auto-reassociate. */
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        break;
    default:
        break;
    }
    return ESP_OK;
}
int get_socket_error_code(int socket)
{
    int result;
    u32_t optlen = sizeof(int);
    int err = getsockopt(socket, SOL_SOCKET, SO_ERROR, &result, &optlen);
    if (err == -1)
    {
        ESP_LOGE(TAG, "getsockopt failed:%s", strerror(err));
        return -1;
    }
    return result;
}
int show_socket_error_reason(const char *str, int socket)
{
    int err = get_socket_error_code(socket);

    if (err != 0)
    {
        ESP_LOGW(TAG, "%s socket error %d %s", str, err, strerror(err));
    }

    return err;
}

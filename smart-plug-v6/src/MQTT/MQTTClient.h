//
// Created by iamna on 18/01/2025.
//

#ifndef SMART_PLUG_MQTTCLIENT_H
#define SMART_PLUG_MQTTCLIENT_H

#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "pico/cyw43_arch.h"
#include "lwip/init.h"
#include "lwip/dns.h"
#include "lwip/apps/mqtt.h"
#include "lwip/altcp_tls.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"

#define MQTT_TOPIC_STATE_BIT (1 << 0)
#define MQTT_TOPIC_SETPOINT_BIT (1 << 1)
#define MQTT_TOPIC_CLOCK_BIT (1 << 2)
#define MQTT_TOPIC_WH_BIT (1 << 3)
#define MQTT_TOPIC_TIMER_BIT (1 << 4)
#define MQTT_TOPIC_OTA_BIT (1 << 5)
#define MQTT_TOPIC_ALIVE_BIT (1 << 6)
#define MQTT_TOPIC_TIMER_REMOVE_BIT (1 << 7)

const uint8_t cert[] =
        "-----BEGIN CERTIFICATE-----\n"
        "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\n"
        "-----END CERTIFICATE-----\n";

/*const uint8_t cert2[] =
        "-----BEGIN CERTIFICATE-----\n"
        "MIIFCzCCA/OgAwIBAgISA7NXJMeI1If+CSICLyQe6/LYMA0GCSqGSIb3DQEBCwUA\n"
        "MDMxCzAJBgNVBAYTAlVTMRYwFAYDVQQKEw1MZXQncyBFbmNyeXB0MQwwCgYDVQQD\n"
        "EwNSMTEwHhcNMjQxMjIzMjI0OTI0WhcNMjUwMzIzMjI0OTIzWjAfMR0wGwYDVQQD\n"
        "DBQqLnMxLmV1LmhpdmVtcS5jbG91ZDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCC\n"
        "AQoCggEBAKVuz2sMPmxx2w/f81/YAEKTbNZMJPk2+ooLFg5hxXvReF+AwIT4XvZ+\n"
        "MLhSKvFxmghJF+BB9WyhqrcJLGDCP4s6SOLWTYixEoTcaLUviqqn+06kYqDJ6E83\n"
        "NGsc7T42DlPnzqcZZjPRed9rt4CP3RgeZlWyYZgiD8FoJG9gie8ytihF/FkGZT8T\n"
        "N4Vkl2vQa3mfBWeeKrcuhcLPxqIWDz/30iYfLtEe5JYYScoCKTXcP9SUStjpR8pD\n"
        "vfOWdvasOAuBy7yBbx01/4lcQt50hfbhTR/K14/D4rNkuuvU7ktSQnoxVXC8YDwG\n"
        "zkny10DFt65mVYLNZcBQtOLHHOZGV30CAwEAAaOCAiswggInMA4GA1UdDwEB/wQE\n"
        "AwIFoDAdBgNVHSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIwDAYDVR0TAQH/BAIw\n"
        "ADAdBgNVHQ4EFgQUgsEjDU35+EWJKBsFxJ0lM0PXMi4wHwYDVR0jBBgwFoAUxc9G\n"
        "pOr0w8B6bJXELbBeki8m47kwVwYIKwYBBQUHAQEESzBJMCIGCCsGAQUFBzABhhZo\n"
        "dHRwOi8vcjExLm8ubGVuY3Iub3JnMCMGCCsGAQUFBzAChhdodHRwOi8vcjExLmku\n"
        "bGVuY3Iub3JnLzAzBgNVHREELDAqghQqLnMxLmV1LmhpdmVtcS5jbG91ZIISczEu\n"
        "ZXUuaGl2ZW1xLmNsb3VkMBMGA1UdIAQMMAowCAYGZ4EMAQIBMIIBAwYKKwYBBAHW\n"
        "eQIEAgSB9ASB8QDvAHYAzPsPaoVxCWX+lZtTzumyfCLphVwNl422qX5UwP5MDbAA\n"
        "AAGT9euKuwAABAMARzBFAiEA7kt5ecRQhyl1JsSPpgt4bN14o+/BZspQCq0d46Wy\n"
        "03wCIDZ17LnI6Hh+cIF6SlX64OB4pc18XilUqI7pffaEJEENAHUAzxFW7tUufK/z\n"
        "h1vZaS6b6RpxZ0qwF+ysAdJbd87MOwgAAAGT9euK3wAABAMARjBEAiAlffjlUAeU\n"
        "7T6o7ISkiFGXz5G9tx2BB2C5f+GQdqc59QIgEdxGXKjoAJYYlYeZqig2LQxxkdPZ\n"
        "JaYjkrdr9PBeVcYwDQYJKoZIhvcNAQELBQADggEBADKUM+E39KujX/GS+beQajyU\n"
        "/19CFjB+TXYoaXpRPbXTL9XvhCNWI5ZiGc+uGOFneNBED+24YoC1JTLW3a+bWfuJ\n"
        "hBl8bjJoxbP38MsffsWnQ3CGEO4EdcwqdyYf68qGY9FhxvVAx7nzf8qGzRuN0waQ\n"
        "INpYn6eTjZiCICHPdnQnntVSfTza+mzNEBYqZpHAqkpUywG8pEUytJG7ECw5Z79r\n"
        "bEo3gwI2XCfSfVS57aizBYWeZq0tvAmfy7YD3ubo/IIjB4WhxINwgVAPk5oqSToM\n"
        "ZzKtyDiKUcubGARwO0QPjufvvvmbKB56xNtKELvpSTOUCrei9HQcV+utJWvrZSA=\n"
        "-----END CERTIFICATE-----\n";
*/

extern char dataStr[50]; //global var to be accessed in internet task
//extern SemaphoreHandle_t mqtt_sem;
extern EventGroupHandle_t mqtt_topic_group;
typedef struct {
    char topic[20];
    uint8_t res;
} subscribe_res_s;
extern QueueHandle_t sub_topic_res_q;

class MQTTClient {
public:
    explicit MQTTClient();
    ~MQTTClient();
    int init();
    void mqtt_dns_found(const char *name, ip_addr_t &ipaddr);
    int connect();
    void subscribe(const char *topic);
    void publish( const char *topic, const char *data);
    void disconnect();
    bool is_connected();

private:
    bool connected = false;
    const char *hostname = "4e8b61774358463c87428a6f4f8bf6c3.s1.eu.hivemq.cloud";//"d2ae37053fd34d8db5914cbef1ff7c95.s1.eu.hivemq.cloud" ;
    struct mqtt_connect_client_info_t ci;
    mqtt_client_t *client;
    struct altcp_tls_config *tls_config;

    const char *pub_topic;
    const char *sub_topic;
    ip_addr_t resolved_addr;

    static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status);
    static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags);
    static void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len);
    static void mqtt_publish_cb(void *arg, err_t err);
    static void mqtt_subscribe_cb(void *arg, err_t result);
};


#endif //SMART_PLUG_MQTTCLIENT_H

//
// Created by iamna on 18/01/2025.
//

#include "MQTTClient.h"

QueueHandle_t incoming_data_q = NULL;  // Define it here
char dataStr[50] = {0};
//SemaphoreHandle_t mqtt_sem = xSemaphoreCreateBinary();
EventGroupHandle_t mqtt_topic_group = xEventGroupCreate();

QueueHandle_t sub_topic_res_q = xQueueCreate(7, sizeof(subscribe_res_s));

MQTTClient::MQTTClient(){
    incoming_data_q = xQueueCreate(10, sizeof(char *)); // Create queue once
};

MQTTClient::~MQTTClient(){
};

void MQTTClient::mqtt_dns_found(const char *name, ip_addr_t &ipaddr) {
    bool ip_resolved = false;
    while (!ip_resolved) {
        if (dns_gethostbyname(name, &ipaddr, NULL, NULL) == ERR_OK) {
            printf("Resolved IP: %s\n", ipaddr_ntoa(&ipaddr));
            ip_resolved = true;
        } else {
            printf("DNS resolution failed or in progress.\n");
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }

}

int MQTTClient::init() {
    client = mqtt_client_new();
    if (client == NULL) {
        printf("Failed to create MQTT client.\n");
        return 0;
    }
    return 1;
}

int MQTTClient::connect() {
    if(!connected) {
        int ret = 0;
        while (!ret) {
            ret = init();
        }
        mqtt_dns_found(hostname, resolved_addr);
        tls_config = altcp_tls_create_config_client(cert, sizeof(cert) + 1);
        if (tls_config == NULL) {
            printf("Failed to create TLS config.\n");
            return 0;
        }
        assert(tls_config);
        mbedtls_ssl_conf_authmode((mbedtls_ssl_config *) tls_config, MBEDTLS_SSL_VERIFY_REQUIRED);
        ci = {
                .client_id = "smartplug",
                .client_user = "smartplug",//"nadim",
                .client_pass = "SmartPlug2025@",//"Nadim420",
                .keep_alive = 60,
                .tls_config = tls_config,
                .server_name = hostname
        };
        mqtt_set_inpub_callback(client, mqtt_incoming_publish_cb, mqtt_incoming_data_cb, NULL);
        //printf("resolved IP: %s\n", ipaddr_ntoa(&resolved_addr));
        if (mqtt_client_connect(client, &resolved_addr, 8883, mqtt_connection_cb, 0, &ci) == ERR_OK) {
            printf("MQTT client connected.\n");
            connected = true;
            return 1;
        } else {
            printf("MQTT client connection failed.\n");
            return 0;
        }
    }else{
        mqtt_disconnect(client);
        printf("Disconncting if already connected");
        vTaskDelay(pdMS_TO_TICKS(500));
        mqtt_dns_found(hostname, resolved_addr);
        //tls_config = altcp_tls_create_config_client(cert, sizeof(cert) + 1);
        if (tls_config == NULL) {
            printf("Failed to create TLS config.\n");
            return 0;
        }
        assert(tls_config);
        mbedtls_ssl_conf_authmode((mbedtls_ssl_config *) tls_config, MBEDTLS_SSL_VERIFY_REQUIRED);
        ci = {
                .client_id = "smartplug",
                .client_user = "smartplug",//"nadim",
                .client_pass = "SmartPlug2025@",//"Nadim420",
                .keep_alive = 60,
                .tls_config = tls_config,
                .server_name = hostname
        };
        mqtt_set_inpub_callback(client, mqtt_incoming_publish_cb, mqtt_incoming_data_cb, NULL);
        //printf("resolved IP: %s\n", ipaddr_ntoa(&resolved_addr));
        err_t eti = mqtt_client_connect(client, &resolved_addr, 8883, mqtt_connection_cb, 0, &ci);
        if (eti == ERR_OK) {
            printf("MQTT client connected in retrying.\n");
            connected = true;
            return 1;
        } else {
            printf("MQTT client connection failed retrying.\n");
            printf("ETI: %d\n", eti);
            return 0;
        }
    }
}
void MQTTClient::mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status) {
//    if (status == MQTT_CONNECT_ACCEPTED) {
//        printf("MQTT connection accepted.\n");
//    } else {
//        printf("MQTT connection failed: %d\n", status);
//    }
}

void MQTTClient::mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags) {

    if (len >= sizeof(dataStr)) {
        len = sizeof(dataStr) - 1;
    }
    strncpy(dataStr, (const char *)data, len);
    dataStr[len] = '\0';
    printf("Incoming publish data: %s\n", dataStr);
//    xSemaphoreGive(mqtt_sem);
}

void MQTTClient::mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len) {
    printf("Incoming publish at topic: %s with total length %u\n", topic, tot_len);

    if (strcmp(topic, "state") == 0) {
        xEventGroupSetBits(mqtt_topic_group, MQTT_TOPIC_STATE_BIT);
    } else if (strcmp(topic, "setpoint") == 0) {
        xEventGroupSetBits(mqtt_topic_group, MQTT_TOPIC_SETPOINT_BIT);
    } else if (strcmp(topic, "clock") == 0) {
        xEventGroupSetBits(mqtt_topic_group, MQTT_TOPIC_CLOCK_BIT);
    } else if (strcmp(topic, "wh") == 0) {
        xEventGroupSetBits(mqtt_topic_group, MQTT_TOPIC_WH_BIT);
    } else if (strcmp(topic, "timer") == 0) {
        xEventGroupSetBits(mqtt_topic_group, MQTT_TOPIC_TIMER_BIT);
    } else if (strcmp(topic, "ota") == 0) {
        xEventGroupSetBits(mqtt_topic_group, MQTT_TOPIC_OTA_BIT);
    } else if (strcmp(topic, "alive") == 0) {
        xEventGroupSetBits(mqtt_topic_group, MQTT_TOPIC_ALIVE_BIT);
    } else if (strcmp(topic, "timer_remove") == 0) {
        xEventGroupSetBits(mqtt_topic_group, MQTT_TOPIC_TIMER_REMOVE_BIT);
    }
}

void MQTTClient::subscribe( const char *topic) {
    mqtt_subscribe(client, topic, 0, mqtt_subscribe_cb, (void *)topic);
}

void MQTTClient::mqtt_subscribe_cb(void *arg, err_t result) {
    const char *topic = (const char *)arg;  // Cast arg back to the topic string
    subscribe_res_s sub_result;
    if (result == ERR_OK) {
        printf("Subscribed to '%s'\n", topic);
        sub_result.res = 1;
    } else {
        printf("Failed to subscribe to '%s'\n", topic);
        sub_result.res = 0;
    }
    strcpy(sub_result.topic, topic);
    xQueueSend(sub_topic_res_q, &sub_result, pdMS_TO_TICKS(100));
}

void MQTTClient::publish( const char *topic, const char *data) {
    err_t e = mqtt_publish(client, topic, data, strlen(data), 0, 0, mqtt_publish_cb, NULL);
    if(e == ERR_CONN){
        printf("MQTT Connection error MQTT class 137.\n");
    }
}

void MQTTClient::mqtt_publish_cb(void *arg, err_t err) {
    if (err == ERR_OK) {
        printf("MQTT publish successful.\n");
    }else if(err == ERR_CONN){
        printf("MQTT Connection error MQTT class 142.\n");
    }else {
        printf("MQTT publish failed.\n");
    }
}

void MQTTClient::disconnect() {
    mqtt_disconnect(client);
    mqtt_client_free(client);
    altcp_tls_free_config(tls_config);
}

bool MQTTClient::is_connected() {
    return mqtt_client_is_connected(client);
}
//
// Created by ADMIN on 2/4/2025.
//

#include "internet_task.h"

#define SSID_ADDR 0
#define PASSWORD_ADDR 64

void internet_task(void *pvParameters) {
    auto *data = static_cast<internet_data_s *>(pvParameters);

    credential_s credential = {._ssid = {0}, ._password = {0}};
    uint64_t timer = 0;
    uint64_t datetime = 0;

    uint8_t wifi_connected = 0;
    uint8_t dev_state = 0;
    uint8_t mqtt_connected = 0;
    uint8_t mqtt_subscribed = 0;
    uint8_t state_subscribed = 2;
    uint8_t setpoint_subscribed = 2;
    uint8_t clock_subscribed = 2;
    uint8_t wh_subscribed = 2;
    uint8_t timer_subscribed = 2;
    uint8_t ota_subscribed = 2;
    uint8_t alive_subscribed = 2;
    uint8_t topic_subscribed = 0;
    uint8_t timer_remove_subscribed = 2;

    char wh_data[20] = {0};

    NetworkClass network;
    network.init();

    MQTTClient mqttClient;

    vTaskDelay(pdMS_TO_TICKS(2000));

    // Wait until credentials are retrieved from EEPROM
    if (xQueueReceive(data->credential_q, &credential, portMAX_DELAY) == pdTRUE) {
        network.setCredentials(credential._ssid, credential._password);
    }

    vTaskDelay(pdMS_TO_TICKS(2000));

    while (true) {
        // cyw43_arch_poll();
        //int wifi_status = cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA);   // bullshit
        //printf("wifi_status: %d\n", wifi_status);

        /* if(network.is_wifi_connected()){
             printf("Is_wificonnected True\n");
         }else{
             printf("IS_wificonnected False\n");
         }*/
        int wifi_status = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);
        //printf("Link Status %d\n", wifi_status);
        //vTaskDelay(4000);

        if (wifi_status <= 2) {
            printf("Line 52\n");
            // WiFi is disconnected
            wifi_connected = 0;
            mqtt_connected = 0;
            topic_subscribed = 0;
            mqtt_subscribed = 0;
            state_subscribed = 2;
            setpoint_subscribed = 2;
            clock_subscribed = 2;
            wh_subscribed = 2;
            timer_subscribed = 2;
            ota_subscribed = 2;
            alive_subscribed = 2;
            timer_remove_subscribed = 2;
            xQueueReset(data->wifi_status_q);
            xQueueSend(data->wifi_status_q, &wifi_connected, pdMS_TO_TICKS(100));

            // Attempt to reconnect WiFi
            network.connect();
            vTaskDelay(pdMS_TO_TICKS(1000));
        } else if (network.hasIPAddress()) {
            // WiFi is connected and has an IP address
            if (!wifi_connected) {
                // WiFi just reconnected
                wifi_connected = 1;
            }

            // Check MQTT connection status using is_connected()
            if (!mqtt_connected) {
                printf("MQTT disconnected, attempting to reconnect...\n");
                if (mqttClient.connect()) {
                    mqtt_connected = 1;
                }
            } else {
                if (!topic_subscribed) {
                    mqttClient.subscribe("state");
                    mqttClient.subscribe("setpoint");
                    mqttClient.subscribe("clock");
                    mqttClient.subscribe("wh");
                    mqttClient.subscribe("timer");
                    mqttClient.subscribe("ota");
                    mqttClient.subscribe("alive");
                    mqttClient.subscribe("timer_remove");
                    topic_subscribed = 1;
                } else {
                    if (!state_subscribed) {
                        mqttClient.subscribe("state");
                    }
                    if (!setpoint_subscribed) {
                        mqttClient.subscribe("setpoint");
                    }
                    if (!clock_subscribed) {
                        mqttClient.subscribe("clock");
                    }
                    if (!wh_subscribed) {
                        mqttClient.subscribe("wh");
                    }
                    if (!timer_subscribed) {
                        mqttClient.subscribe("timer");
                    }
                    if (!ota_subscribed) {
                        mqttClient.subscribe("ota");
                    }
                    if (!alive_subscribed) {
                        mqttClient.subscribe("alive");
                    }
                    if (!timer_remove_subscribed) {
                        mqttClient.subscribe("timer_remove");
                    }
                }
            }
            subscribe_res_s sub_result = {.topic = {0}, .res = 2};
            if (xQueueReceive(sub_topic_res_q, &sub_result, pdMS_TO_TICKS(0)) == pdTRUE) {
                if (strcmp(sub_result.topic, "state") == 0) {
                    state_subscribed = sub_result.res;
                } else if (strcmp(sub_result.topic, "setpoint") == 0) {
                    setpoint_subscribed = sub_result.res;
                } else if (strcmp(sub_result.topic, "clock") == 0) {
                    clock_subscribed = sub_result.res;
                } else if (strcmp(sub_result.topic, "wh") == 0) {
                    wh_subscribed = sub_result.res;
                } else if (strcmp(sub_result.topic, "timer") == 0) {
                    timer_subscribed = sub_result.res;
                } else if (strcmp(sub_result.topic, "ota") == 0) {
                    ota_subscribed = sub_result.res;
                } else if (strcmp(sub_result.topic, "alive") == 0) {
                    alive_subscribed = sub_result.res;
                } else if (strcmp(sub_result.topic, "timer_remove") == 0) {
                    timer_remove_subscribed = sub_result.res;
                }
            }
            /* printf("State: %d, Setpoint: %d, Clock: %d, WH: %d, Timer: %d, OTA: %d, Alive: %d, Timer Remove: %d\n",
                    state_subscribed, setpoint_subscribed, clock_subscribed, wh_subscribed, timer_subscribed,
                    ota_subscribed, alive_subscribed, timer_remove_subscribed);*/

            if (state_subscribed == 1 && setpoint_subscribed == 1 && clock_subscribed == 1 && wh_subscribed == 1 &&
                timer_subscribed == 1 && ota_subscribed == 1 && alive_subscribed == 1 && timer_remove_subscribed == 1 && !mqtt_subscribed) {
                mqtt_subscribed = 1;
                printf("All topics subscribed & wifi connected update display %hhu\n", wifi_connected);
                xQueueReset(data->wifi_status_q);
                xQueueSend(data->wifi_status_q, &wifi_connected, pdMS_TO_TICKS(100));
                xSemaphoreGive(data->wifi_connected_sem);

                // query back-end for clock and wh
                mqttClient.publish("smart_plug/clock_query", "1");
                vTaskDelay(pdMS_TO_TICKS(1000));
                mqttClient.publish("smart_plug/wh_query", "1");
                vTaskDelay(pdMS_TO_TICKS(1000));
                mqttClient.publish("smart_plug/state_ok", "0");
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
        }

        // Detect if WiFi reset button is pressed
        if (xSemaphoreTake(data->wifi_reset_sem, pdMS_TO_TICKS(10)) == pdTRUE) {
            printf("Back to AP mode\n");
            network.deinit();
            wifi_connected = 0;
            mqtt_connected = 0;
            mqtt_subscribed = 0;
            topic_subscribed = 0;
            state_subscribed = 2;
            setpoint_subscribed = 2;
            clock_subscribed = 2;
            wh_subscribed = 2;
            timer_subscribed = 2;
            ota_subscribed = 2;
            alive_subscribed = 2;
            timer_remove_subscribed = 2;
            xQueueReset(data->wifi_status_q);
            xQueueSend(data->wifi_status_q, &wifi_connected, pdMS_TO_TICKS(100));
            vTaskDelay(pdMS_TO_TICKS(1000));

            network.init();
            network.ap_init();
            while (!network.areCredentialsSet()) {
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
            network.ap_disconnect();
            printf("network credentials: %s, %s\n", network.ssid, network.password);
            // cahnge credentials
            strcpy(credential._ssid, network.ssid);
            strcpy(credential._password, network.password);
            // clear credentials queue
            xQueueReset(data->credential_q);
            // send new credentials to a queue
            xQueueSend(data->credential_q, &credential, portMAX_DELAY);
            // notify eeprom task
            xSemaphoreGive(data->credential_changed_sem);
        }

        // Check if incoming MQTT data is available
        // incoming 'state' message
        EventBits_t mqtt_bits = xEventGroupWaitBits(mqtt_topic_group,MQTT_TOPIC_STATE_BIT,
                                                    pdTRUE, pdFALSE, pdMS_TO_TICKS(0));
        if (mqtt_bits & MQTT_TOPIC_STATE_BIT) {
            // On/off device
            sscanf(dataStr, "%hhu", &dev_state);
            // empty the queue before adding new data
            xQueueReset(data->dev_state_q);
            // send the new state to a queue
            xQueueSend(data->dev_state_q, &dev_state, portMAX_DELAY);
            // notify output, display, and buzzer task
            xEventGroupSetBits(state_event_group,
                               STATE_OUTPUT_BIT | STATE_DISPLAY_BIT | STATE_BUZZER_BIT);
            // acknowledge the server
            mqttClient.publish("smart_plug/state_ok", dataStr);
        }

        // incoming 'setpoint' message
        mqtt_bits = xEventGroupWaitBits(mqtt_topic_group,MQTT_TOPIC_SETPOINT_BIT,
                                        pdTRUE, pdFALSE, pdMS_TO_TICKS(0));
        if (mqtt_bits & MQTT_TOPIC_SETPOINT_BIT) {
            float temp = 0.0f;
            sscanf(dataStr, "%f", &temp);
            // empty the queue before adding new data
            xQueueReset(data->setpoint_q);
            // send the new setpoint to a queue
            xQueueSend(data->setpoint_q, &temp, portMAX_DELAY);
            //notify eeprrom, display, and adc task
            xEventGroupSetBits(setpoint_event_group,
                               SETPOINT_EEPROM_BIT | SETPOINT_DISPLAY_BIT | SETPOINT_ADC_BIT);
            // acknowledge the server
            mqttClient.publish("smart_plug/setpoint_ok", dataStr);
        }

        // incoming 'clock' message
        mqtt_bits = xEventGroupWaitBits(mqtt_topic_group,MQTT_TOPIC_CLOCK_BIT,
                                        pdTRUE, pdFALSE, pdMS_TO_TICKS(0));
        if (mqtt_bits & MQTT_TOPIC_CLOCK_BIT) {
            sscanf(dataStr, "%llu", &datetime);
            // Send the new time to the RTC task
            xQueueReset(data->time_q);
            xQueueSend(data->time_q, &datetime, portMAX_DELAY);
            printf("New time: %llu\n", datetime);
            //notify rtc task
            xEventGroupSetBits(time_event_group, TIME_UPDATE_TIME_BIT);
        }

        // incoming 'wh' message
        mqtt_bits = xEventGroupWaitBits(mqtt_topic_group,MQTT_TOPIC_WH_BIT,
                                        pdTRUE, pdFALSE, pdMS_TO_TICKS(0));
        if (mqtt_bits & MQTT_TOPIC_WH_BIT) {
            float temp = 0.0f;
            sscanf(dataStr, "%f", &temp);
            xQueueSend(data->pw_consumed_q, &temp, portMAX_DELAY);
        }

        // incoming 'timer' message
        mqtt_bits = xEventGroupWaitBits(mqtt_topic_group,MQTT_TOPIC_TIMER_BIT,
                                        pdTRUE, pdFALSE, pdMS_TO_TICKS(0));
        if (mqtt_bits & MQTT_TOPIC_TIMER_BIT) {
            sscanf(dataStr, "%llu", &timer);
            // reset the queue before adding new data
            xQueueReset(data->time_q);
            // send the new timer to a queue
            xQueueSend(data->time_q, &timer, portMAX_DELAY);
            // notify eeprom task
            xEventGroupSetBits(time_event_group, TIME_EEPROM_INTERNET_BIT);
            // acknowledge the server
            mqttClient.publish("smart_plug/timer_ok", dataStr);
        }

        // incoming 'ota' message
        mqtt_bits = xEventGroupWaitBits(mqtt_topic_group,MQTT_TOPIC_OTA_BIT,
                                        pdTRUE, pdFALSE, pdMS_TO_TICKS(0));
        if (mqtt_bits & MQTT_TOPIC_OTA_BIT) {
            // OTA code here
            xSemaphoreGive(data->ota_sem);
            vTaskDelay(pdMS_TO_TICKS(500));
            uint8_t ota_command = 0;
            sscanf(dataStr, "%hhu", &ota_command);
            printf("command %hhu\n", ota_command);
            xQueueReset(data->ota_state);
            xQueueSend(data->ota_state, &ota_command, portMAX_DELAY);
        }


        // incoming 'alive' message
        mqtt_bits = xEventGroupWaitBits(mqtt_topic_group,MQTT_TOPIC_ALIVE_BIT,
                                        pdTRUE, pdFALSE, pdMS_TO_TICKS(0));
        if (mqtt_bits & MQTT_TOPIC_ALIVE_BIT) {
            mqttClient.publish("smart_plug/alive_ok", dataStr);
        }

        // incoming 'timer_remove' message
        mqtt_bits = xEventGroupWaitBits(mqtt_topic_group,MQTT_TOPIC_TIMER_REMOVE_BIT,
                                        pdTRUE, pdFALSE, pdMS_TO_TICKS(0));
        if (mqtt_bits & MQTT_TOPIC_TIMER_REMOVE_BIT) {
            sscanf(dataStr, "%llu", &timer);
            // reset the queue before adding new data
            xQueueReset(data->time_q);
            // send the new timer to a queue
            xQueueSend(data->time_q, &timer, portMAX_DELAY);
            // notify eeprom task
            xEventGroupSetBits(time_event_group, TIME_REMOVE_EEPROM_INTERNET_BIT);
            // acknowledge the server
            mqttClient.publish("smart_plug/timer_remove_ok", dataStr);
        }

        /* Lower commented code->->-> Devil
         * Creating problem if network is not connected, try to publish and break things
         * Must have to check before publish to any topic
         * @@@Be careful @@@
         * */

        //check if it is time to send wh to cloud
        if (xQueueReceive(data->wh_send_q, &wh_data, pdMS_TO_TICKS(0)) == pdTRUE) {
            if(wifi_connected){
                mqttClient.publish("smart_plug/post_wh", wh_data);
            }
        }

        // check if sate has changed by pressing the button
        if (xSemaphoreTake(data->state_to_cloud_sem, pdMS_TO_TICKS(0)) == pdTRUE) {
            if (xQueueReceive(data->state_to_internet_q, &dev_state, pdMS_TO_TICKS(0)) == pdTRUE) {
                char stateStr[2];
                sprintf(stateStr, "%hhu", dev_state);
                if(wifi_connected){
                    mqttClient.publish("smart_plug/state_ok", stateStr);
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));  // Reduced delay for faster response
    }
}

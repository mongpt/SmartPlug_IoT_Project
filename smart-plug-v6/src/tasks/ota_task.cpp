//
// Created by iamna on 07/03/2025.
//

#include "ota_task.h"

#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include <stdio.h>

#include "lwip/ip4_addr.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"

#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>

#include "OTA/WifiHelper.h"
#include "OTA/Request.h"
#include <md5.h>
#include "common.h"

extern "C"{
#include "pico_fota_bootloader.h"
}

char *buf = NULL;
const size_t BUFLEN = 11000;
const size_t SEGSIZE = 10240;

void otaUpdate(const char * url);
void blinkDelay(uint8_t led);
void runTimeStats();
void vOTA(void *pvParameters) {
    auto *data = static_cast<ota_s*>(pvParameters);

    printf("RELEASE: %s\n", RELEASE);
    printf("Main task started\n");

    if (pfb_is_after_firmware_update()){
        printf("RUNNING ON NEW FIRMWARE\n");
    } else {
        printf("Old firmware\n");
    }
    uint8_t ota_command = 0;

    while (true) {
        if(xQueueReceive(data->ota_state, &ota_command, portMAX_DELAY) == pdTRUE){
            printf("command OTA %hhu\n", ota_command);
            if(ota_command == 1){
                otaUpdate(OTA_URL);
            }
        }
        vTaskDelay(100);
    }
}

void otaUpdate(const char * url){
    MD5 fullMD;
    unsigned char md[16];
    char segNum[10];
    char segSize[10];
    sprintf(segSize, "%d", SEGSIZE);
    buf = (char *)malloc(BUFLEN);
    if (buf == NULL) {
        printf("Failed to allocate buffer\n");
        return;
    }
    int seg = 0;
    int rec = 1;
    pfb_initialize_download_slot();
    while (rec > 0){
        Request req(buf, BUFLEN);
        std::map<std::string, std::string> query;
        query["segSize"] = segSize;
        sprintf(segNum, "%d", seg);
        query["segNum"] = segNum;
        if (!req.get(url, &query)){
            break;
        }
        printf("Seg %d Req Resp: %d : Len %u \t", seg, req.getStatusCode(), req.getPayloadLen());
        rec = req.getPayloadLen();

        if (rec > BUFLEN) {
            printf(" Payload size %d exceeds buffer length %d\n", rec, BUFLEN);
            break;
        }

        fullMD.update((const void *) req.getPayload(),  req.getPayloadLen());

        MD5::hash( (const void *) req.getPayload(),  req.getPayloadLen(),  md );
        for (int i=0; i < 16; i++){
            if (md[i] < 0x10){
                printf("0%X", md[i]);
            } else {
                printf("%X", md[i]);
            }
        }
        printf("\n");


        pfb_write_to_flash_aligned_256_bytes(
                ( uint8_t *) req.getPayload(),
                seg * SEGSIZE,
                req.getPayloadLen());
        seg++;
    }

    free(buf);

    //blinkDelay(LED_GP);

    printf("Full MD5: ");
    fullMD.finalize( md );
    for (int i=0; i < 16; i++){
        if (md[i] < 0x10){
            printf("0%X", md[i]);
        } else {
            printf("%X", md[i]);
        }
    }
    printf("\n");


    pfb_mark_download_slot_as_valid();
    printf("Firmware update complete\n");
    pfb_perform_update();
}

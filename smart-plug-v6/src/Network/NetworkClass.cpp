#include <string>
#include "NetworkClass.h"

// Helper function for URL decoding
static void url_decode(char *dst, const char *src, size_t dst_size) {
    char *d = dst;
    const char *s = src;
    char hex[3] = {0};
    size_t i = 0;

    while (*s && i < dst_size - 1) {
        if (*s == '%' && s[1] && s[2]) {
            hex[0] = s[1];
            hex[1] = s[2];
            *d++ = (char)strtol(hex, NULL, 16);
            s += 3;
        } else if (*s == '+') {
            *d++ = ' ';
            s++;
        } else {
            *d++ = *s++;
        }
        i++;
    }
    *d = '\0'; // Null-terminate the string
}

NetworkClass::NetworkClass() {}

bool NetworkClass::is_credentials_set = false;
char NetworkClass::ssid[32] = {0};
char NetworkClass::password[32] = {0};

void NetworkClass::init() {
    is_credentials_set = false;
    if (cyw43_arch_init()) {
        printf("failed to initialise\n");
        return;
    }
}

void NetworkClass::deinit() {
    printf("Deinitializing network...\n");
    // Stop and deinitialize STA mode
    cyw43_arch_disable_sta_mode();
    vTaskDelay(pdMS_TO_TICKS(1000));  // Allow time for cleanup
    // Deinitialize Wi-Fi chip
    cyw43_arch_deinit();

    printf("Network deinitialized.\n");
}

void NetworkClass::setCredentials(char* SSID, char* PASSWORD) {
    strcpy(ssid, SSID);
    strcpy(password, PASSWORD);
    is_credentials_set = true;
}

int NetworkClass::connect() {
    //init();
    cyw43_arch_enable_sta_mode();
    if (cyw43_arch_wifi_connect_timeout_ms(ssid, password, CYW43_AUTH_WPA2_AES_PSK, 15000)) {
        printf("failed to connect line 63 Network Class\n");
        return 0;
    }
    return 1;
}

void NetworkClass::ap_init() {
    //init();
    cyw43_arch_enable_ap_mode(ap_name, ap_pass, CYW43_AUTH_WPA2_AES_PSK);
    IP4_ADDR(&gw, 192, 168, 4, 1);
    IP4_ADDR(&mask, 255, 255, 255, 0);
    dhcp_server_init(&dhcp_server, &gw, &mask);
    httpd_init();
    httpd_cgi_init();
}

void NetworkClass::httpd_cgi_init() {
    http_set_cgi_handlers(cgi_handlers, sizeof(cgi_handlers) / sizeof(cgi_handlers[0]));
}

const char *NetworkClass::cgi_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]) {
    char gssid[32] = {0};
    char gpassword[64] = {0};
    char decoded_ssid[32] = {0};
    char decoded_password[64] = {0};

    for (int i = 0; i < iNumParams; i++) {
        if (strcmp(pcParam[i], "ssid") == 0) {
            strncpy(gssid, pcValue[i], sizeof(gssid) - 1);
            url_decode(decoded_ssid, gssid, sizeof(decoded_ssid));
        } else if (strcmp(pcParam[i], "password") == 0) {
            strncpy(gpassword, pcValue[i], sizeof(gpassword) - 1);
            url_decode(decoded_password, gpassword, sizeof(decoded_password));
        }
    }
    if (decoded_ssid[0] != '\0' && decoded_password[0] != '\0') {
        printf("Received SSID: %s, Password: %s\n", decoded_ssid, decoded_password);
        // Set the SSID and password in the shared resources
        setCredentials(decoded_ssid, decoded_password);
        return "/confirm.html";
    }

    return "/index.html";
}

bool NetworkClass::ap_disconnect() {
    cyw43_arch_disable_ap_mode();
    return true;
}

// Not working @@@USELESS@@@
bool NetworkClass::hasIPAddress() {
    if (netif_default == NULL) {
        return false;  // No network interface available
    }
    uint32_t ip_addr = ip4_addr_get_u32(&netif_default->ip_addr);
    //printf("Network class has ip: %d\n", ip_addr);
    return ip_addr != 0;  // Returns true if IP is assigned (not 0.0.0.0)
}

bool NetworkClass::areCredentialsSet() {
    return is_credentials_set;
}

bool NetworkClass::is_wifi_connected(void) {
    if (!netif_is_link_up(netif_default)) {
        printf("Link is Down (Not Connected to Wi-Fi)\n");
        return false;
    }else{
        printf("Link is UP (Connected to Wi-Fi)\n");
    }
    return true;
}
#include <string.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <esp_netif.h>
#include <esp_http_server.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define LED_GREEN GPIO_NUM_16
#define LED_RED   GPIO_NUM_17

static const char *TAG = "webserver";

static nvs_handle_t my_nvs_handle;
static int led_green_state = 0;
static int led_red_state = 0;

// HTML Page Template with Toggle Buttons
static const char *html_template =
"<!DOCTYPE html><html><head><title>ESP32 Web Server</title>"
"<style>"
"body{text-align:center;font-family:monospace;}"
".buttonGreen{background-color:yellowgreen;color:white;padding:16px 40px;font-size:32px;cursor:pointer;margin:10px;}"
".buttonRed{background-color:red;color:white;padding:16px 40px;font-size:32px;cursor:pointer;margin:10px;}"
".status{font-size:20px;}"
"</style></head><body><h1>ESP32 Web Server</h1>"
"<p>Green LED is currently: <span class='status'>%s</span></p>"
"<p><a href=\"/green/toggle\"><button class=\"buttonGreen\">Green LED (%s)</button></a></p>"
"<p>Red LED is currently: <span class='status'>%s</span></p>"
"<p><a href=\"/red/toggle\"><button class=\"buttonRed\">Red LED (%s)</button></a></p>"
"</body></html>";	

// Helper: Convert state to string
static const char* led_state_str(int state) {
    return state ? "ON" : "OFF";
}

// Save LED states to NVS
static void save_led_states_to_nvs() {
    nvs_set_i32(my_nvs_handle, "led_green", led_green_state);
    nvs_set_i32(my_nvs_handle, "led_red", led_red_state);
    nvs_commit(my_nvs_handle);
}

// Load LED states from NVS
static void load_led_states_from_nvs() {
    int32_t green = 0, red = 0;
    if (nvs_get_i32(my_nvs_handle, "led_green", &green) == ESP_OK) {
        led_green_state = green;
    }
    if (nvs_get_i32(my_nvs_handle, "led_red", &red) == ESP_OK) {
        led_red_state = red;
    }
}

// Serve HTML page
esp_err_t send_html(httpd_req_t *req) {
    char html[1024];
    snprintf(html, sizeof(html), html_template,
             led_state_str(led_green_state), led_state_str(!led_green_state),
             led_state_str(led_red_state), led_state_str(!led_red_state));

    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, html, HTTPD_RESP_USE_STRLEN);
}

// Handle LED toggle requests
esp_err_t led_toggle_handler(httpd_req_t *req) {
    const char *uri = req->uri;

    if (strcmp(uri, "/green/toggle") == 0) {
        led_green_state = !led_green_state;
        gpio_set_level(LED_GREEN, led_green_state);
    } else if (strcmp(uri, "/red/toggle") == 0) {
        led_red_state = !led_red_state;
        gpio_set_level(LED_RED, led_red_state);
    }

    save_led_states_to_nvs();
    return send_html(req);
}

// Handle root URL "/"
esp_err_t root_handler(httpd_req_t *req) {
    return send_html(req);
}

// Handle favicon to avoid 404
esp_err_t favicon_handler(httpd_req_t *req) {
    return httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "No favicon");
}

// Start HTTP server
void start_webserver() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        // Root
        httpd_uri_t root = { .uri = "/", .method = HTTP_GET, .handler = root_handler };
        httpd_register_uri_handler(server, &root);

        // Toggle paths
        const char *toggle_paths[] = {"/green/toggle", "/red/toggle"};
        for (int i = 0; i < 2; i++) {
            httpd_uri_t uri = { .uri = toggle_paths[i], .method = HTTP_GET, .handler = led_toggle_handler };
            httpd_register_uri_handler(server, &uri);
        }

        // Favicon
        httpd_uri_t favicon = { .uri = "/favicon.ico", .method = HTTP_GET, .handler = favicon_handler };
        httpd_register_uri_handler(server, &favicon);

        ESP_LOGI(TAG, "Web server started");
    }
}

// Initialize Wi-Fi in SoftAP mode
void wifi_init_softap() {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = "ESP32-Network",
            .ssid_len = strlen("ESP32-Network"),
            .password = "Esp32-Password",
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };

    if (strlen((char *)wifi_config.ap.password) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi AP started. SSID: ESP32-Network, Password: Esp32-Password");
}

// Main function
void app_main() {
    // Init NVS
    ESP_ERROR_CHECK(nvs_flash_init());
    if (nvs_open("storage", NVS_READWRITE, &my_nvs_handle) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS");
    }

    // Load previous states
    load_led_states_from_nvs();

    // Setup GPIOs
    gpio_reset_pin(LED_GREEN);
    gpio_reset_pin(LED_RED);
    gpio_set_direction(LED_GREEN, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_RED, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_GREEN, led_green_state);
    gpio_set_level(LED_RED, led_red_state);

    // Start Wi-Fi + Web Server
    wifi_init_softap();
    start_webserver();
}

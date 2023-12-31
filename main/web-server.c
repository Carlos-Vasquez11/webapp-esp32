#include <stdio.h>
#include "string.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"
#include "esp_http_server.h"
#include "driver/ledc.h"

int32_t led_r = 0, led_g = 0, led_b = 0;

extern const char index_start[] asm("_binary_index_html_start");
extern const char index_end[] asm("_binary_index_html_end");
extern const char chroma_start[] asm("_binary_chroma_png_start");
extern const char chroma_end[] asm("_binary_chroma_png_end");

static esp_err_t api_get_handler(httpd_req_t *req) {
  // ip/api?r=0&g=0&b=0

  char* buf;
  size_t buf_len;

  buf_len = httpd_req_get_url_query_len(req) + 1;

  if (buf_len > 1) {
    buf = malloc(buf_len);
    if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
      char param[10];
      if (httpd_query_key_value(buf, "r", param, sizeof(param)) == ESP_OK) {
        led_r = atoi(param);
        printf("r: %d\n", led_r);
      }
      if (httpd_query_key_value(buf, "g", param, sizeof(param)) == ESP_OK) {
        led_g = atoi(param);
        printf("g: %d\n", led_g);
      }
      if (httpd_query_key_value(buf, "b", param, sizeof(param)) == ESP_OK) {
        led_b = atoi(param);
        printf("b: %d\n", led_b);
      }
    }
    free(buf);
  }

  httpd_resp_set_hdr(req, "Content-Type", "application/json");

  char res[30];
  sprintf(res, "{ \"r\": %d, \"g\": %d, \"b\": %d }", led_r, led_g, led_b);
  httpd_resp_send(req, res, HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}

static esp_err_t home_get_handler(httpd_req_t *req){
    httpd_resp_set_type(req, "text/html");
    const uint32_t index_len = index_end - index_start;
    httpd_resp_send(req, index_start, index_len);
    return ESP_OK;
}

static esp_err_t chroma_get_handler(httpd_req_t *req){
    httpd_resp_set_type(req, "image/png");
    const uint32_t chroma_len = chroma_end - chroma_start;
    httpd_resp_send(req, chroma_start, chroma_len);
    return ESP_OK;
}

static const httpd_uri_t api = {
    .uri = "/api",
    .method = HTTP_GET,
    .handler = api_get_handler
};

static const httpd_uri_t home = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = home_get_handler
};

static const httpd_uri_t chroma = {
    .uri = "/chroma.png",
    .method = HTTP_GET,
    .handler = chroma_get_handler
};

void web_server_init(){
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    if (httpd_start(&server, &config) == ESP_OK){
        httpd_register_uri_handler(server, &api);
        httpd_register_uri_handler(server, &home);
        httpd_register_uri_handler(server, &chroma);
        return;
    }
    printf("Error al iniciar el servidor\n");
}

void app_main(void)
{
  nvs_flash_init();
  esp_netif_init();
  esp_event_loop_create_default();

  example_connect();

  esp_netif_ip_info_t ip_info;
  esp_netif_t* netif = NULL;
  netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");

  if (netif == NULL) {
    printf("No hay interfaz\n");
  } else {
    esp_netif_get_ip_info(netif, &ip_info);
    printf("IP: %d.%d.%d.%d\n", IP2STR(&ip_info.ip));
    web_server_init();
  }
}

#include "esp_web_server.h"
#include "esp_log.h"
#include "esp_http_server.h"

static const char *TAG = "web_server";

// Web server GET handler
esp_err_t get_server_handler(httpd_req_t *req) {
    const char resp[] = "<html><body><h1>Air Quality Monitor</h1><p>Temperature: 22.5C</p><p>Humidity: 55%%</p></body></html>";
    httpd_resp_send(req, resp, strlen(resp));
    return ESP_OK;
}

// Configure and start HTTP server
void start_webserver(void) 
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &(httpd_uri_t){
            //only need to specify IP address to access web UI
            .uri = "/",
            .method = HTTP_GET,
            .handler = get_server_handler,
            .user_ctx = NULL
        });
    }
}

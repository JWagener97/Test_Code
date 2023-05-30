#include <stdio.h>              /* Standard input/output operations */
#include <string.h>             /* String manipulation functions */
#include "freertos/FreeRTOS.h"  /* FreeRTOS real-time operating system */
#include "freertos/task.h"      /* FreeRTOS task management */
#include "esp_system.h"         /* ESP32 system functions */
#include "esp_mac.h"            /* ESP32 MAC address functions */
#include "esp_wifi.h"           /* ESP32 Wi-Fi functions */
#include "esp_event.h"          /* ESP32 event handling */
#include "esp_log.h"            /* ESP32 logging functions */
#include "nvs_flash.h"          /* Non-volatile storage (NVS) flash functions */
#include "driver/i2c.h"

#include "driver/gpio.h"        /* GPIO driver for ESP32 */

#include <nvs_flash.h>          /* NVS flash functions */
#include <sys/param.h>          /* System parameters */
#include "esp_netif.h"          /* ESP32 network interface functions */
#include <esp_http_server.h>    /* ESP32 HTTP server function */

#include "lwip/err.h"           /* Lightweight IP stack error codes */
#include "lwip/sys.h"           /* Lightweight IP stack system functions */

#include "ssd1306.h"            /* SSD1306 OLED display functions */
#include "font8x8_basic.h"      /* 8x8 pixel font for SSD1306 display */

#define EXAMPLE_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID         
#define EXAMPLE_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_WIFI_CHANNEL   CONFIG_ESP_WIFI_CHANNEL
#define EXAMPLE_MAX_STA_CONN       CONFIG_ESP_MAX_STA_CONN

#define TAG "CBI - Configurator"
#define LED GPIO_NUM_2                                      /* Define the pin with the led on the development board*/

/**
 * @brief Init function for GPIO pin 2. The pin is first reset and then set as an output.
 * 
 */
static void configure_led (void)
{
	gpio_reset_pin (LED);                                   /* Reset function for the gpio pin which has been defined as LED */
	gpio_set_direction (LED, GPIO_MODE_OUTPUT);             /* Set the LED pin as an output */
}

/**
 * @brief The wifi_event_handler function is a callback function that handles Wi-Fi events related to an access point (AP).
 *        The function takes four parameters: arg, event_base, event_id, and event_data.
 * 
 *        In the first if statement, the code checks if the event_id is equal to WIFI_EVENT_AP_STACONNECTED, indicating that a station has connected to the access point.
 * 
 *        In the else if statement, the code checks if the event_id is equal to WIFI_EVENT_AP_STADISCONNECTED, indicating that a station has disconnected from the access point.
 */
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED)            /* Check if a station has connected to the access point */
    {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data; /* Cast the event_data to the appropriate struct type */
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",MAC2STR(event->mac), event->aid);   /* Print the information about the connected station */
    } 
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)    /*Check if a station has disconnected from the access point */
    {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data; /* Cast the event_data to the appropriate struct type */
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d",MAC2STR(event->mac), event->aid);        /* Print the information about the disconnected station */
    }
}

/**
 * @brief The wifi_init_softap function prepares the ESP32 to operate as a Wi-Fi access point, 
 *        configures its parameters, and starts the access point for other devices to connect to. 
 */
void wifi_init_softap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());                      /* Initialize the default network interface */
    ESP_ERROR_CHECK(esp_event_loop_create_default());       /* Create the default event loop */
    esp_netif_create_default_wifi_ap();                     /* Create the default Wi-Fi access point (AP) */

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();    /* Initialize the Wi-Fi with default configuration */
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));                   /* */

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,         /* Register the Wi-Fi event handler to handle all Wi-Fi events */
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {                          /* Configure the Wi-Fi access point settings */
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,                 /* Set the AP SSID (network name) */
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),     /* Set the length of the SSID */
            .channel = EXAMPLE_ESP_WIFI_CHANNEL,           /* Set the AP channel */
            .password = EXAMPLE_ESP_WIFI_PASS,             /* Set the AP password */
            .max_connection = EXAMPLE_MAX_STA_CONN,        /* Set the maximum number of station connections */
            .authmode = WIFI_AUTH_WPA_WPA2_PSK             /* Set the authentication mode to WPA/WPA2 with a pre-shared key */
        },
    };

    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {              /* Check if the password is an empty string, and if so, change the authentication mode to open */
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));                   /* Set the Wi-Fi mode to access point (AP) mode */
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));     /* Set the Wi-Fi configuration for the access point (AP) interface */
    ESP_ERROR_CHECK(esp_wifi_start());                                  /* Start the Wi-Fi according to the configured mode and configuration*/

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",          /* Log the information about the configured access point (AP) */
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS, EXAMPLE_ESP_WIFI_CHANNEL);
}

/**
 * @brief This function is typically used as a callback for an HTTP server when handling a
 *        specific route or endpoint that triggers the action of turning on an LED.
 */
static esp_err_t ledON_handler(httpd_req_t *req)
{
   esp_err_t error;
   ESP_LOGI(TAG, "LED Turned ON");                             /* Log a message indicating that the LED is being turned ON */
   const char *response = (const char *) req->user_ctx;        /* Get the response message from the user context */
   gpio_set_level(LED,1);                                      /* Set the GPIO pin connected to the LED to a HIGH (ON) state */
   error = httpd_resp_send(req, response, strlen(response));   /* Send the response back to the client */

   if (error != ESP_OK)                                        /* Check if there was an error while sending the response */                                       
   {
	ESP_LOGI(TAG, "Error %d while sending Response", error);   /* Log an error message with the error code */
   }
   else ESP_LOGI(TAG, "Response sent Successfully");           /* Log a message indicating that the response was sent successfully */

   return error;                                               /* Log an error code */
   
}
/**
 * @brief The code defines a static constant variable ledon of type httpd_uri_t, which represents the configuration for an HTTP URI.
 *        - > .uri field: Specifies the URI (path) for the HTTP request. In this case, it is set to "/ledon". This means that the configured URI is "/ledon".
 *        - > .method field: Specifies the HTTP method for the request. It is set to HTTP_GET, indicating that the request should use the GET method.
 *             This means that the handler function will be triggered when a GET request is made to the specified URI.
 *        - > .handler field: Specifies the handler function to be called when the specified URI is requested.
 *            In this case, it is set to ledON_handler. The ledON_handler function will be responsible for handling
 *            the request and performing the necessary actions, such as turning on an LED.
 *        - > .user_ctx field: Specifies the user-specific context data that can be passed to the handler function.
 *            In this code snippet, it is set to NULL, indicating that no specific user context is provided.
 */
static const httpd_uri_t ledon = {
    .uri       = "/ledon",              /* The URI (path) for the HTTP request */
    .method    = HTTP_GET,              /* The HTTP method for the request (GET)*/
    .handler   = ledON_handler,         /*The handler function to be called for this URI */ 
    /* User-specific context data (optional) */
    .user_ctx  = "<!DOCTYPE html><html><head><style type=\"text/css\">html {  font-family: Arial;  display: inline-block;  margin: 0px auto;  text-align: center;}h1{  color: #070812;  padding: 2vh;}.button {  display: inline-block;  background-color: #b30000; //red color  border: none;  border-radius: 4px;  color: white;  padding: 16px 40px;  text-decoration: none;  font-size: 30px;  margin: 2px;  cursor: pointer;}.button2 {  background-color: #364cf4; //blue color}.content {   padding: 50px;}.card-grid {  max-width: 800px;  margin: 0 auto;  display: grid;  grid-gap: 2rem;  grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));}.card {  background-color: white;  box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5);}.card-title {  font-size: 1.2rem;  font-weight: bold;  color: #034078}</style>  <title>ESP32 WEB SERVER</title>  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">  <link rel=\"icon\" href=\"data:,\">  <link rel=\"stylesheet\" href=\"https://use.fontawesome.com/releases/v5.7.2/css/all.css\"    integrity=\"sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr\" crossorigin=\"anonymous\">  <link rel=\"stylesheet\" type=\"text/css\" ></head><body>  <h2>ESP32 WEB SERVER</h2>  <div class=\"content\">    <div class=\"card-grid\">      <div class=\"card\">        <p><i class=\"fas fa-lightbulb fa-2x\" style=\"color:#c81919;\"></i>     <strong>GPIO2</strong></p>        <p>GPIO state: <strong> ON</strong></p>        <p>          <a href=\"/ledon\"><button class=\"button\">ON</button></a>          <a href=\"/ledoff\"><button class=\"button button2\">OFF</button></a>        </p>      </div>    </div>  </div></body></html>"
};

/**
 * @brief This function is typically used as a callback for an HTTP server when handling a 
 *        specific route or endpoint that triggers the action of turning off an LED.
 */
static esp_err_t ledOFF_handler(httpd_req_t *req)
{
   esp_err_t error;
   ESP_LOGI(TAG, "LED Turned OFF");                             /* Log a message indicating that the LED is being turned OFF */
   gpio_set_level(LED,0);                                       /* Set the GPIO pin connected to the LED to a LOW (OFF) state */
   const char *response = (const char *) req->user_ctx;         /* Get the response message from the user context */
   error = httpd_resp_send(req, response, strlen(response));    /* Send the response back to the client*/

   if (error != ESP_OK)                                         /* Check if there was an error while sending the response*/
   {
	ESP_LOGI(TAG, "Error %d while sending Response", error);    /* Log an error message with the error code*/
   }
   else ESP_LOGI(TAG, "Response sent Successfully");            /* Log a message indicating that the response was sent successfully */
   return error;                                                /* Return the error code*/
   
}
/**
 * @brief The code defines a static constant variable ledon of type httpd_uri_t, which represents the configuration for an HTTP URI.
 *        - > .uri field: Specifies the URI (path) for the HTTP request. In this case, it is set to "/ledoff". This means that the configured URI is "/ledoff".
 *        - > .method field: Specifies the HTTP method for the request. It is set to HTTP_GET, indicating that the request should use the GET method.
 *            This means that the handler function will be triggered when a GET request is made to the specified URI.
 *        - > .handler field: Specifies the handler function to be called when the specified URI is requested.
 *            In this case, it is set to ledON_handler. The ledON_handler function will be responsible for handling
 *            the request and performing the necessary actions, such as turning off an LED.
 *        - > .user_ctx field: Specifies the user-specific context data that can be passed to the handler function.
 *            In this code snippet, it is set to NULL, indicating that no specific user context is provided.
 */
static const httpd_uri_t ledoff = {
    .uri       = "/ledoff",             /* The URI (path) for the HTTP request */
    .method    = HTTP_GET,              /* The HTTP method for the request (GET)*/
    .handler   = ledOFF_handler,        /*The handler function to be called for this URI */ 
    /* User-specific context data (optional) */
    .user_ctx  = "<!DOCTYPE html><html><head><style type=\"text/css\">html {  font-family: Arial;  display: inline-block;  margin: 0px auto;  text-align: center;}h1{  color: #070812;  padding: 2vh;}.button {  display: inline-block;  background-color: #b30000; //red color  border: none;  border-radius: 4px;  color: white;  padding: 16px 40px;  text-decoration: none;  font-size: 30px;  margin: 2px;  cursor: pointer;}.button2 {  background-color: #364cf4; //blue color}.content {   padding: 50px;}.card-grid {  max-width: 800px;  margin: 0 auto;  display: grid;  grid-gap: 2rem;  grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));}.card {  background-color: white;  box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5);}.card-title {  font-size: 1.2rem;  font-weight: bold;  color: #034078}</style>  <title>ESP32 WEB SERVER</title>  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">  <link rel=\"icon\" href=\"data:,\">  <link rel=\"stylesheet\" href=\"https://use.fontawesome.com/releases/v5.7.2/css/all.css\"    integrity=\"sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr\" crossorigin=\"anonymous\">  <link rel=\"stylesheet\" type=\"text/css\"></head><body>  <h2>ESP32 WEB SERVER</h2>  <div class=\"content\">    <div class=\"card-grid\">      <div class=\"card\">        <p><i class=\"fas fa-lightbulb fa-2x\" style=\"color:#c81919;\"></i>     <strong>GPIO2</strong></p>        <p>GPIO state: <strong> OFF</strong></p>        <p>          <a href=\"/ledon\"><button class=\"button\">ON</button></a>          <a href=\"/ledoff\"><button class=\"button button2\">OFF</button></a>        </p>      </div>    </div>  </div></body></html>"

};

/**
 * @brief This function is typically used as an error handler for an HTTP server.
 *        When a request results in a 404 Not Found error, this handler is triggered, 
 *        and it sends an appropriate error response to the client with the specified error message. 
 *        The ESP_FAIL return value signifies the error status, indicating that the request handling encountered an issue.
 */
esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Some 404 error message");    /* Send a 404 Not Found error response with a custom error message */
    return ESP_FAIL;                                                            /* Return ESP_FAIL to indicate an error occurred*/
}

/**
 * @brief This function is typically called to start the HTTP server and register the required URI handlers. 
 *        If the server starts successfully, the server handle is returned for further usage, 
 *        such as stopping the server or managing requests.
 */
static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();     /* Create a default configuration for the HTTP server */
    config.lru_purge_enable = true;                     /* Enable LRU (Least Recently Used) cache purge */

    /* Start the HTTP server */
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        /* Set URI handlers */
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &ledoff);
		httpd_register_uri_handler(server, &ledon);
        return server;                                  /* Return the HTTP server handle */
    }

    ESP_LOGI(TAG, "Error starting server!");           /* Log an error message if the sever failed to start*/
    return NULL;                                       /* Return null if the server failed to start */
}

/**
 * @brief This function is typically used to stop the HTTP server when it is no longer needed. It takes
 *        the server handle as an input and stops the server, returning an esp_err_t value 
 *        indicating the success or failure of the operation.
 */
static esp_err_t stop_webserver(httpd_handle_t server)
{
    return httpd_stop(server);      /* Stop the HTTP server*/
}

/**
 * @brief This function is typically used as an event handler for disconnection events. When a 
 *        disconnection event occurs, it checks if the server handle is valid, stops the webserver if it is,
 *        and updates the server handle accordingly.
 */
static void disconnect_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;         /* Cast the argument to httpd_handle_t* type */

    if (*server)                                            /* Check if the server handle is valid */
    {
        ESP_LOGI(TAG, "Stopping webserver");
        if (stop_webserver(*server) == ESP_OK)              /* Attempt to stop the webserver using the stop_webserver function */
        {
            *server = NULL;                                 /* If the server stops successfully, set the server handle to NULL */
        } 
        else 
        {
            ESP_LOGE(TAG, "Failed to stop http server");
        }
    }
}

/**
 * @brief This function is typically used as an event handler for connection events. When a connection
 *        event occurs, it checks if the server handle is NULL, indicating that the webserver is not yet
 *        started. If it is NULL, the function starts the webserver by calling the start_webserver
 *        function and assigns the obtained server handle to the appropriate variable for further
 *        usage.
 */
static void connect_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;     /* Cast the argument to httpd_handle_t* type */
    if (*server == NULL) 
    {
        ESP_LOGI(TAG, "Starting webserver");
        *server = start_webserver();                    /*  Start the webserver using the start_webserver function */
    }
}

/**
 * @brief CBI Energy logo (128 x 32) in hex for use on an 128 x 32 oled I2C display
 */
uint8_t cbi_logo[] = {
	0x1f, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x10, 0x60, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x30, 0x20, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x20, 0x30, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x20, 0x10, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x23, 0x11, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x63, 0x13, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x63, 0x13, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0xe3, 0x13, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0xe3, 0x13, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0xe3, 0x13, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0xe3, 0xf1, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0xe3, 0xe0, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 
	0xe3, 0xf0, 0x21, 0x00, 0xff, 0xf8, 0x30, 0x01, 0xff, 0xff, 0x9f, 0xff, 0x3f, 0xfe, 0x0c, 0x03, 
	0xe3, 0x33, 0x11, 0x01, 0xff, 0xfc, 0x78, 0x01, 0xff, 0xff, 0x9f, 0xff, 0x7f, 0xff, 0x7e, 0x03, 
	0xe3, 0x13, 0x11, 0x03, 0xff, 0xfc, 0x7e, 0x03, 0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xfe, 0x03, 
	0xe3, 0x13, 0x11, 0x03, 0x80, 0x0c, 0x6f, 0x03, 0x70, 0x01, 0xf8, 0x00, 0xe0, 0x03, 0xec, 0x03, 
	0xe3, 0x13, 0x11, 0x13, 0x00, 0x0c, 0xe7, 0xc7, 0x70, 0x01, 0xf0, 0x00, 0xc0, 0x03, 0xec, 0x03, 
	0xe3, 0x13, 0x11, 0x3b, 0x00, 0x0c, 0xc1, 0xe7, 0x70, 0x01, 0xf0, 0x00, 0xc0, 0x03, 0xe0, 0x03, 
	0xe3, 0x13, 0x11, 0x13, 0x7f, 0xfd, 0xd8, 0xf6, 0x7f, 0xff, 0xf0, 0x00, 0xc0, 0x03, 0xe0, 0x03, 
	0xe3, 0x13, 0x11, 0x03, 0x7f, 0xfd, 0xdc, 0x6e, 0x7f, 0xff, 0xb0, 0x00, 0xc0, 0x03, 0xe0, 0x03, 
	0xe3, 0x13, 0x11, 0x13, 0x00, 0x01, 0x9f, 0x0c, 0x70, 0x00, 0x30, 0x00, 0xc0, 0x03, 0xe0, 0x03, 
	0xe0, 0x10, 0x11, 0x3b, 0x00, 0x03, 0x87, 0x8c, 0x70, 0x00, 0x30, 0x00, 0xc0, 0x03, 0xe0, 0x03, 
	0xe0, 0x30, 0x11, 0x13, 0x00, 0x03, 0x83, 0xdc, 0x70, 0x00, 0x30, 0x00, 0xe0, 0x03, 0xe0, 0x03, 
	0xf0, 0x60, 0x31, 0x03, 0xff, 0xf7, 0x00, 0xf8, 0x7f, 0xff, 0xb0, 0x00, 0xff, 0xfb, 0x7f, 0xff, 
	0xf0, 0x60, 0x31, 0x03, 0xff, 0xff, 0x00, 0x78, 0x3f, 0xff, 0xb0, 0x00, 0x7f, 0xfb, 0x7f, 0xff, 
	0xff, 0xff, 0xff, 0x01, 0xff, 0xee, 0x00, 0x30, 0x3f, 0xff, 0xb0, 0x00, 0x00, 0x03, 0x00, 0x03, 
	0xff, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x03, 
	0xff, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xff, 0x7f, 0xff, 
	0xff, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xfe, 0x7f, 0xff, 
	0x7f, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xfc, 0x3f, 0xfc, 
	0x3f, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	
};

/**
 * @brief Battry Empty Icon
 */
uint8_t Batt_Empty[] = {
	0x7f, 0xff, 0x40, 0x01, 0xc0, 0x05, 0xc0, 0x05, 0xc0, 0x05, 0xc0, 0x05, 0x40, 0x01, 0x7f, 0xff
};

/**
 * @brief Battry Low Icon
 */
uint8_t Batt_Low[] = {
	0x7f, 0xff, 0x40, 0x01, 0xc0, 0x3d, 0xc0, 0x3d, 0xc0, 0x3d, 0xc0, 0x3d, 0x40, 0x01, 0x7f, 0xff
};

/**
 * @brief Battry High Icon 
 */
uint8_t Batt_High[] = {
	0x7f, 0xff, 0x40, 0x01, 0xc3, 0xfd, 0xc3, 0xfd, 0xc3, 0xfd, 0xc3, 0xfd, 0x40, 0x01, 0x7f, 0xff
};

/**
 * @brief Battry Full Icon
 */
uint8_t Batt_Full[] = {
	0x7f, 0xff, 0x40, 0x01, 0xdf, 0xfd, 0xdf, 0xfd, 0xdf, 0xfd, 0xdf, 0xfd, 0x40, 0x01, 0x7f, 0xff
};


void app_main(void)
{
	configure_led();        /* Configure the LED Pin*/
	SSD1306_t dev;          /* Initialize SSD1306 display */
	char lineChar[20];

	static httpd_handle_t server = NULL; /* Initialize server handle as NULL */
	


    /**
    * @brief The purpose of this code is to handle potential issues with the NVS flash, such as lack of free 
    *        space or the presence of an incompatible flash version. By erasing and reinitializing the flash
    *        in such cases, it ensures that the NVS flash is in a clean and usable state for storing and 
    *        retrieving data.
    */
	esp_err_t ret = nvs_flash_init();       /* Initialize NVS flash*/
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
    wifi_init_softap();                 /* Initialize and start the WiFi access point*/
	ESP_ERROR_CHECK(esp_netif_init());  /* Register connect_handler function as an event handler for AP_STAIPASSIGNED event*/
	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_AP_STAIPASSIGNED, &connect_handler, &server));

#if CONFIG_I2C_INTERFACE
	ESP_LOGI(TAG, "INTERFACE is i2c");
	ESP_LOGI(TAG, "CONFIG_SDA_GPIO=%d",CONFIG_SDA_GPIO);
	ESP_LOGI(TAG, "CONFIG_SCL_GPIO=%d",CONFIG_SCL_GPIO);
	ESP_LOGI(TAG, "CONFIG_RESET_GPIO=%d",CONFIG_RESET_GPIO);
	i2c_master_init(&dev, CONFIG_SDA_GPIO, CONFIG_SCL_GPIO, CONFIG_RESET_GPIO);
#endif // CONFIG_I2C_INTERFACE

#if CONFIG_FLIP
	dev._flip = true;
	ESP_LOGW(TAG, "Flip upside down");
#endif

#if CONFIG_SSD1306_128x64
	ESP_LOGI(TAG, "Panel is 128x64");
	ssd1306_init(&dev, 128, 64);
#endif // CONFIG_SSD1306_128x64
#if CONFIG_SSD1306_128x32
	ESP_LOGI(TAG, "Panel is 128x32");
	ssd1306_init(&dev, 128, 32);
#endif // CONFIG_SSD1306_128x32
    // Scroll Down
	ssd1306_clear_screen(&dev, false);
	ssd1306_contrast(&dev, 0xff);
	ssd1306_bitmaps(&dev, 0, 0, cbi_logo, 128, 32, false);
	vTaskDelay(5000 / portTICK_PERIOD_MS);
	ssd1306_clear_screen(&dev, false);
	float soc = 0;
    float voltage;
	while (1)
	{
		soc = max17043_read_register_soc(0x04);
        uint16_t voltageRaw = max17043_read_register(0x02);
        voltage = ((voltageRaw >> 4) * 0.00125);
        if (100 < soc)
		{
			soc = 100.00;
        }
        ESP_LOGI(TAG,"SOC:%.2f%% \t Voltage:%3.2fV\n", soc,voltage);
        sprintf(&lineChar, "%3.2fV\t\t%6.2f%%", voltage,soc);
		ssd1306_display_text(&dev,0, lineChar, strlen(lineChar), false);
		if (soc <= 25.00)
		{
			ssd1306_bitmaps(&dev, 112, 0, Batt_Empty, 16, 8, false);
		}
		else if (soc <= 50.00)
		{
			ssd1306_bitmaps(&dev, 112, 0, Batt_Low, 16, 8, false);
		}

		else if (soc <= 75.00)
		{
			ssd1306_bitmaps(&dev, 112, 0, Batt_High, 16, 8, false);
		}

		else if (soc <= 100.00)
		{
			ssd1306_bitmaps(&dev, 112, 0, Batt_Full, 16, 8, false);
		}
		vTaskDelay(500 / portTICK_PERIOD_MS);
		
	}
}
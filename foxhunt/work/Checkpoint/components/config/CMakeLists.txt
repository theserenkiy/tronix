idf_component_register(
    SRCS
        "src/manager.cpp"
        "src/wifi_portal.cpp"
    INCLUDE_DIRS "include"
    REQUIRES
        "nvs_flash"
        "esp_wifi"
        "esp_event"
        "esp_http_server"
        "esp_netif"
        "esp_timer"
        "lwip"
)

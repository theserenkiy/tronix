#pragma once

void uart_logger_init();
esp_err_t uart_logger_send_buffer(uint16_t *buffer, size_t samples);

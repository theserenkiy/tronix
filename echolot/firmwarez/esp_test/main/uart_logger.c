#include <string.h>
#include "common.h"
#include "driver/uart.h"
#include "uart_logger.h"


void uart_logger_init()
{
	ESP_ERROR_CHECK(
		uart_driver_install(
			UART_NUM_0,
			4096,   // RX buffer
			0,      // TX buffer
			0,
			NULL,
			0
		)
	);

	esp_log_level_set("*", ESP_LOG_NONE);

	uart_set_baudrate(UART_PORT, UART_BAUDRATE);
}


esp_err_t uart_logger_send_buffer(uint16_t *buffer,
								 size_t samples)
{
	size_t bytes =
		samples * sizeof(uint16_t);

	uart_write_bytes(UART_NUM_0, ">>DATA", 6);

	int sent = //
		uart_write_bytes(
			UART_PORT,
			(const char *)buffer,
			bytes);

	uart_write_bytes(UART_NUM_0, ">>DATAEND", 9);

	//printf("BUF SENT QQ: %d\n", sent);

	return (sent == bytes)
		   ? ESP_OK
		   : ESP_FAIL;
}
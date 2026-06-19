#include <stdio.h>
#include "cons.h"
#include <stdio.h>
#include <stdarg.h>

// Наша функция-обертка для логирования
void con_log(const char *level, const char *format, ...) {
    // 1. Выводим фиксированную часть (префикс)
    printf("[%s] ", level);

    // 2. Инициализируем va_list
    va_list args;
    va_start(args, format);

    // 3. Передаем формат и список va_list в vprintf
    vprintf(format, args);

    // 4. Очищаем список
    va_end(args);

    printf("\n"); // Добавляем перенос строки для красоты
}

// int main() {
//     int user_id = 42;
//     const char *username = "Alex";

//     // Вызовы нашей функции работают точно так же, как printf
//     my_log("INFO", "User %s connected.", username);
//     my_log("ERROR", "User ID %d not found! Code: 0x%X", user_id, 404);

//     return 0;
// }


void con_err(char *s)
{
	printf("ERROR: %s\n",s);
}


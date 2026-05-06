#include "lora/fox_mode.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_rom_sys.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "FoxMode";

// ——— Таблица Морзе (символ → строка из '.' и '-') ————————————————————————
struct MorseEntry {
    char        ch;
    const char *code;
};

static const MorseEntry MORSE_TABLE[] = {
    {'A', ".-"},   {'B', "-..."},  {'C', "-.-."},  {'D', "-.."},
    {'E', "."},    {'F', "..-."},  {'G', "--."},   {'H', "...."},
    {'I', ".."},   {'J', ".---"},  {'K', "-.-"},   {'L', ".-.."},
    {'M', "--"},   {'N', "-."},    {'O', "---"},   {'P', ".--."},
    {'Q', "--.-"}, {'R', ".-."},   {'S', "..."},   {'T', "-"},
    {'U', "..-"},  {'V', "...-"},  {'W', ".--"},   {'X', "-..-"},
    {'Y', "-.--"}, {'Z', "--.."},
    {'1', ".----"},{'2', "..---"}, {'3', "...--"}, {'4', "....-"},
    {'5', "....."}, {'6', "-...."},{'7', "--..."},
    {'8', "---.."}, {'9', "----."}, {'0', "-----"},
    {' ', " "},
    {'\0', nullptr}
};

static const char *getMorse(char c)
{
    if (c >= 'a' && c <= 'z') c -= 32;
    for (int i = 0; MORSE_TABLE[i].ch != '\0'; i++) {
        if (MORSE_TABLE[i].ch == c) return MORSE_TABLE[i].code;
    }
    return nullptr;
}

namespace lora {

static constexpr uint32_t DIT_MS   = 50;
static constexpr uint32_t DAH_MS   = DIT_MS * 3;
static constexpr uint32_t SYM_GAP  = DIT_MS;
static constexpr uint32_t LET_GAP  = DIT_MS * 3;
static constexpr uint32_t WORD_GAP = DIT_MS * 7;

// ——— Constructor ————————————————————————————————————————————————————————

FoxMode::FoxMode(SX1276 &radio, gpio_num_t dio2_gpio,
                 const config::StationConfig &cfg)
    : radio_(radio), dio2_gpio_(dio2_gpio), cfg_(cfg)
{
    fox_freq_hz_  = cfg_.fox_freq_hz;
    fox_fdev_hz_  = cfg_.fox_fdev_hz;
    fox_tone_hz_  = cfg_.fox_tone_hz;
    lora_freq_hz_ = cfg_.lora_freq_hz;
}

// ——— Генерация тона ——————————————————————————————————————————————————————
//
// Меандр на DIO2 с частотой fox_tone_hz_ Hz:
//   DIO2 = 1 → несущая f0 + Fdev
//   DIO2 = 0 → несущая f0 - Fdev
// FSK-демодулятор FM-приёмника воспроизводит аудио тон fox_tone_hz_.
// Полупериод = 1000000 / (2 * fox_tone_hz_) мкс = 625 мкс для 800 Гц.

void FoxMode::toneBurst(uint32_t duration_ms)
{
    const uint32_t half_us = 1000000UL / (2 * fox_tone_hz_);
    int64_t end_us = esp_timer_get_time() + (int64_t)duration_ms * 1000LL;
    bool level = true;
    while (esp_timer_get_time() < end_us) {
        gpio_set_level(dio2_gpio_, level ? 1 : 0);
        esp_rom_delay_us(half_us);
        level = !level;
    }
    gpio_set_level(dio2_gpio_, 0); // вернуть в «тихое» состояние
}

void FoxMode::silence(uint32_t duration_ms)
{
    gpio_set_level(dio2_gpio_, 0); // DIO2=0 → f0-Fdev → тишина на FM-приёмнике
    vTaskDelay(pdMS_TO_TICKS(duration_ms));
}

// ——— Элементы Морзе ——————————————————————————————————————————————————————

void FoxMode::dit()        { toneBurst(DIT_MS); }
void FoxMode::dah()        { toneBurst(DAH_MS); }
void FoxMode::symbolGap()  { silence(SYM_GAP); }
void FoxMode::letterGap()  { silence(LET_GAP); }
void FoxMode::wordGap()    { silence(WORD_GAP); }

// ——— Передача Морзе ——————————————————————————————————————————————————————

void FoxMode::sendMorseChar(char c)
{
    if (c == ' ') {
        wordGap();
        return;
    }

    const char *code = getMorse(c);
    if (!code) {
        ESP_LOGW(TAG, "No morse for '%c'", c);
        return;
    }

    for (int i = 0; code[i]; i++) {
        if (i > 0) symbolGap();
        if (code[i] == '.') dit();
        else if (code[i] == '-') dah();
    }
}

void FoxMode::sendMorseString(const char *str)
{
    bool first = true;
    for (int i = 0; str[i]; i++) {
        if (!first && str[i] != ' ') letterGap();
        sendMorseChar(str[i]);
        first = false;
    }
}

// ——— switchToFSK / switchToLoRa ——————————————————————————————————————————

void FoxMode::switchToFSK()
{
    radio_.initFSK(fox_freq_hz_, fox_fdev_hz_);

    // Войти в TX режим (FSK continuous, LongRangeMode=0, ModulationType=00)
    radio_.writeReg(lora::Reg::OpMode, 0x03); // FSK, TX mode
    vTaskDelay(pdMS_TO_TICKS(5));

    ESP_LOGI(TAG, "Switched to FSK TX, tone=%lu Hz, Fdev=%lu Hz",
             (unsigned long)fox_tone_hz_, (unsigned long)fox_fdev_hz_);
}

void FoxMode::switchToLoRa()
{
    gpio_set_level(dio2_gpio_, 0);
    radio_.initLoRa(lora_freq_hz_);
    radio_.startReceive();
    ESP_LOGI(TAG, "Switched back to LoRa RX");
}

// ——— Главный цикл Fox ————————————————————————————————————————————————————

void FoxMode::runCycle()
{
    ESP_LOGI(TAG, "Fox TX start: '%s' for %lu sec, tone=%lu Hz",
             cfg_.fox_morse, (unsigned long)cfg_.fox_on_sec, (unsigned long)fox_tone_hz_);

    switchToFSK();

    uint32_t tx_end = esp_timer_get_time() / 1000 + cfg_.fox_on_sec * 1000;

    while (!stop_flag_ && esp_timer_get_time() / 1000 < tx_end) {
        sendMorseString(cfg_.fox_morse);
        wordGap(); // пауза между повторами
    }

    silence(0);
    switchToLoRa();

    ESP_LOGI(TAG, "Fox TX done, pausing %lu sec", (unsigned long)cfg_.fox_off_sec);

    uint32_t pause_ms = cfg_.fox_off_sec * 1000;
    while (!stop_flag_ && pause_ms > 0) {
        uint32_t sleep = pause_ms > 100 ? 100 : pause_ms;
        vTaskDelay(pdMS_TO_TICKS(sleep));
        pause_ms -= sleep;
    }
}

// ——— FreeRTOS задача ————————————————————————————————————————————————————

void FoxMode::foxTask(void *arg)
{
    FoxMode *self = static_cast<FoxMode *>(arg);
    ESP_LOGI(TAG, "Fox task started");

    while (!self->stop_flag_) {
        self->runCycle();
    }

    ESP_LOGI(TAG, "Fox task stopped");
    self->task_handle_ = nullptr;
    vTaskDelete(nullptr);
}

void FoxMode::start()
{
    if (task_handle_ != nullptr) return;
    stop_flag_ = false;

    // Пиним на core 0 чтобы busy-wait меандр не вытеснялся задачами с core 1
    // (rfidScanTask/mainTask работают на core 1 по умолчанию).
    xTaskCreatePinnedToCore(foxTask, "fox_task", 4096, this, 3, &task_handle_, 0);
    ESP_LOGI(TAG, "Fox mode started");
}

void FoxMode::stop()
{
    stop_flag_ = true;
    uint32_t timeout = 100;
    while (task_handle_ != nullptr && timeout--) {
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    gpio_set_level(dio2_gpio_, 0);
}

} // namespace lora

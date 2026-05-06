#pragma once

#include "rfid/rc522.h"
#include "rfid/mifare.h"
#include "rfid/card_processor.h"
#include "lora/sx1276.h"
#include "lora/protocol.h"
#include "lora/fox_mode.h"
#include "sensors/mq3.h"
#include "sensors/button.h"
#include "audio/player.h"
#include "led/status_led.h"
#include "config/manager.h"
#include "checkpoint/logger.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include <cstdint>

namespace checkpoint {

/** Состояния конечного автомата КП. */
enum class CPState : uint8_t {
    STARTUP,
    WIFI_CONFIG,          // WiFi AP запущен, ждём конфигурацию
    INITIALIZING,         // Инициализация после конфига
    SCANNING,             // Сканирование карт (1 раз/сек)
    CARD_FOUND,           // Карта найдена, чтение UID
    WRITING_FIRST_TOUCH,  // Запись первого касания (flags=0x03)
    WAITING_TASK,         // Ожидание начала испытания
    TASK_IN_PROGRESS,     // Испытание выполняется
    TASK_DONE,            // Испытание завершено, ждём второго касания
    WRITING_SECOND_TOUCH, // Запись результата
    ERROR,
};

/** Информация о незавершённом двойном касании. */
struct PendingTask {
    rfid::Uid     uid;
    rfid::SlotInfo slot;
    uint8_t       cp_num;
    uint64_t      ts_arrive;     // timestamp из памяти КП
    uint16_t      reserve_val;   // результат испытания (ADC и др.)
    bool          active = false;
    uint32_t      created_at_sec = 0; // для таймаута
};

/**
 * Главная логика контрольного пункта.
 *
 * Инициализирует все компоненты и запускает задачи.
 * Управляет конечным автоматом.
 */
class CheckpointLogic {
public:
    /**
     * @param spi_host  SPI хост (уже инициализированный с bus_config)
     * @param cfg_mgr   Менеджер конфигурации
     */
    CheckpointLogic(spi_host_device_t spi_host, config::ConfigManager &cfg_mgr);
    ~CheckpointLogic();

    /**
     * Инициализировать только LED (до старта WiFi-портала).
     * Показывает WIFI_CONFIG — красный пульс "ожидание настройки".
     * init() пропустит LED-инициализацию если уже вызван.
     */
    esp_err_t initLed();

    /** Полная инициализация всех компонентов. */
    esp_err_t init();

    /** Запустить задачи (вызывать после init()). */
    void start();

    /** Текущее состояние. */
    CPState getState() const { return state_; }

    CPLogger &getLogger() { return logger_; }

private:
    // ——— Задачи FreeRTOS ——————————————————————————————————————————————
    static void rfidScanTask(void *arg);
    static void loraRxTask(void *arg);
    static void mainTask(void *arg);
    static void tsSaveTask(void *arg);   // периодическое сохранение timestamp в NVS

    // ——— Обработчики состояний ——————————————————————————————————————
    void handleScanning();
    void handleCardFound(const rfid::Uid &uid);
    void handleWriteFirstTouch(const rfid::Uid &uid);
    void handleWaitingTask();
    void handleTaskInProgress();
    void handleTaskDone(const rfid::Uid &uid);
    void handleWriteSecondTouch(const rfid::Uid &uid);

    // ——— Вспомогательные ————————————————————————————————————————————
    void setState(CPState new_state);
    uint64_t getTimestamp();
    bool uidMatch(const rfid::Uid &a, const rfid::Uid &b);
    void logCpVisit(const rfid::Uid &uid, uint8_t cp_num,
                    uint64_t ts, uint8_t flags, uint16_t reserve);

    // ——— Компоненты ————————————————————————————————————————————————
    spi_host_device_t      spi_host_;
    config::ConfigManager &cfg_mgr_;
    CPLogger               logger_;

    rfid::RC522           *pcd_     = nullptr;
    rfid::MifareClassic   *mifare_  = nullptr;
    rfid::CardProcessor   *card_proc_ = nullptr;

    lora::SX1276          *radio_   = nullptr;
    lora::LoRaProtocol    *proto_   = nullptr;
    lora::FoxMode         *fox_     = nullptr;

    sensors::MQ3Sensor    *mq3_    = nullptr;
    sensors::Button       *button_ = nullptr;

    audio::AudioPlayer    *audio_  = nullptr;
    led::StatusLed        *led_    = nullptr;

    // ——— Состояние ——————————————————————————————————————————————————
    volatile CPState state_       = CPState::STARTUP;
    PendingTask      pending_task_;
    rfid::Uid        last_uid_;
    bool             card_present_ = false;

    TaskHandle_t main_task_handle_   = nullptr;
    TaskHandle_t rfid_task_handle_   = nullptr;
    TaskHandle_t lora_task_handle_   = nullptr;
    TaskHandle_t ts_save_task_handle_ = nullptr;
};

} // namespace checkpoint

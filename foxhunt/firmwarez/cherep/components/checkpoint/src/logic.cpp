#include "checkpoint/logic.h"
#include "config/wifi_portal.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstring>
#include <sys/time.h>
#include <cinttypes>

static const char *TAG = "Checkpoint";

// GPIO пины (совпадают с планом)
static constexpr gpio_num_t GPIO_SPI_SCK   = GPIO_NUM_15;
static constexpr gpio_num_t GPIO_SPI_MOSI  = GPIO_NUM_2;
static constexpr gpio_num_t GPIO_SPI_MISO  = GPIO_NUM_4;
static constexpr gpio_num_t GPIO_SPI_RST   = GPIO_NUM_14;
static constexpr gpio_num_t GPIO_RFID_CS   = GPIO_NUM_5;
static constexpr gpio_num_t GPIO_LORA_CS   = GPIO_NUM_13;
static constexpr gpio_num_t GPIO_LORA_DIO0 = GPIO_NUM_12;
static constexpr gpio_num_t GPIO_LORA_DIO2 = GPIO_NUM_16;
static constexpr gpio_num_t GPIO_MQ3_HEAT  = GPIO_NUM_27;
static constexpr gpio_num_t GPIO_DAC_AUDIO = GPIO_NUM_26;
static constexpr gpio_num_t GPIO_WS2812    = GPIO_NUM_33;
static constexpr gpio_num_t GPIO_BUTTON    = GPIO_NUM_32;

// ADC для MQ3: ADC2_CHANNEL_8 = GPIO25
static constexpr adc_channel_t MQ3_ADC_CHANNEL = ADC_CHANNEL_8;
static constexpr adc_unit_t    MQ3_ADC_UNIT    = ADC_UNIT_2;

// Таймаут ожидания второго касания после испытания (1 минута)
static constexpr uint32_t SECOND_TOUCH_TIMEOUT_SEC = 60;

// Пин зуммера (TBD, -1 = не подключён)
static constexpr gpio_num_t GPIO_BUZZER = GPIO_NUM_NC;

namespace checkpoint {

// ——— Constructor / Destructor ———————————————————————————————————————————

CheckpointLogic::CheckpointLogic(spi_host_device_t spi_host,
                                 config::ConfigManager &cfg_mgr)
    : spi_host_(spi_host), cfg_mgr_(cfg_mgr)
{}

CheckpointLogic::~CheckpointLogic()
{
    delete fox_;
    delete proto_;
    delete radio_;
    delete card_proc_;
    delete mifare_;
    delete pcd_;
    delete mq3_;
    delete button_;
    delete audio_;
    delete led_;
}

// ——— initLed —————————————————————————————————————————————————————————————

esp_err_t CheckpointLogic::initLed()
{
    if (led_) return ESP_OK; // уже инициализирован
    led_ = new led::StatusLed(GPIO_WS2812);
    if (led_->init() != ESP_OK) {
        ESP_LOGE(TAG, "LED init failed");
        delete led_;
        led_ = nullptr;
        return ESP_FAIL;
    }
    led_->setStatus(led::LedStatus::WIFI_CONFIG);
    return ESP_OK;
}

// ——— init ————————————————————————————————————————————————————————————————

esp_err_t CheckpointLogic::init()
{
    const config::StationConfig &cfg = cfg_mgr_.get();

    // LED: если initLed() уже вызывался — пропускаем
    if (!led_) {
        led_ = new led::StatusLed(GPIO_WS2812);
        if (led_->init() != ESP_OK) {
            ESP_LOGE(TAG, "LED init failed");
            return ESP_FAIL;
        }
        led_->setStatus(led::LedStatus::WIFI_CONFIG);
    }

    // Audio
    audio_ = new audio::AudioPlayer(GPIO_DAC_AUDIO, GPIO_BUZZER);
    if (audio_->init() != ESP_OK) {
        ESP_LOGE(TAG, "Audio init failed");
        return ESP_FAIL;
    }

    // Перед инициализацией RFID — поднять CS LoRa, чтобы SX1276
    // не слушал SPI шину (оба чипа на одной шине)
    gpio_config_t lora_cs_pre = {};
    lora_cs_pre.pin_bit_mask = (1ULL << GPIO_LORA_CS);
    lora_cs_pre.mode         = GPIO_MODE_OUTPUT;
    lora_cs_pre.intr_type    = GPIO_INTR_DISABLE;
    gpio_config(&lora_cs_pre);
    gpio_set_level(GPIO_LORA_CS, 1);

    // RFID
    pcd_   = new rfid::RC522();
    mifare_= new rfid::MifareClassic(*pcd_);

    esp_err_t ret = pcd_->init(spi_host_, GPIO_RFID_CS, GPIO_SPI_RST);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "RC522 init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    card_proc_ = new rfid::CardProcessor(*mifare_,
                                         cfg.station_key,
                                         cfg.sector_key);

    // LoRa
    radio_ = new lora::SX1276();
    ret = radio_->init(spi_host_, GPIO_LORA_CS, GPIO_LORA_DIO0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SX1276 init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = radio_->initLoRa(cfg.lora_freq_hz);
    if (ret != ESP_OK) return ret;

    radio_->setDio2Gpio(GPIO_LORA_DIO2); // Для Fox режима

    proto_ = new lora::LoRaProtocol(*radio_, cfg.event_id, cfg.cp_num);

    // Привязать лог-провайдер к протоколу
    proto_->setLogProvider([this](lora::LogEntryPacket *buf, size_t max) {
        return logger_.getEntries(buf, max);
    });

    // Привязать RX callback
    radio_->setRxCallback([this](const uint8_t *data, uint8_t len, int8_t rssi) {
        proto_->process(data, len, rssi);
    });

    // Fox режим (если включён)
    if (cfg.is_fox) {
        fox_ = new lora::FoxMode(*radio_, GPIO_LORA_DIO2, cfg);
    }

    // MQ3
    mq3_ = new sensors::MQ3Sensor(GPIO_MQ3_HEAT, MQ3_ADC_CHANNEL, MQ3_ADC_UNIT);
    if (cfg.has_task && cfg.task.type == config::TaskType::ALCO_TEST) {
        mq3_->init();
    }

    // Button
    button_ = new sensors::Button(GPIO_BUTTON);
    button_->init();
    // Пока без handler'ов — добавится позже

    ESP_LOGI(TAG, "Checkpoint init OK: cp_num=%d event_id=%d fox=%d",
             cfg.cp_num, cfg.event_id, cfg.is_fox);

    return ESP_OK;
}

// ——— start ————————————————————————————————————————————————————————————————

void CheckpointLogic::start()
{
    if (cfg_mgr_.get().is_fox) {
        setState(CPState::SCANNING); // Fox логика в foxTask
        fox_->start();
        led_->setStatus(led::LedStatus::FOX_MODE);
    } else {
        setState(CPState::SCANNING);
        led_->setStatus(led::LedStatus::IDLE);
        radio_->startReceive();
    }

    // Задача RFID сканирования
    xTaskCreate(rfidScanTask, "rfid_scan", 4096, this, 4, &rfid_task_handle_);

    // Задача LoRa приёма
    xTaskCreate(loraRxTask, "lora_rx", 4096, this, 3, &lora_task_handle_);

    // Главная задача (обработка состояний)
    xTaskCreate(mainTask, "cp_main", 8192, this, 5, &main_task_handle_);

    // Периодическое сохранение timestamp в NVS (каждые 10 минут)
    xTaskCreate(tsSaveTask, "ts_save", 2048, this, 2, &ts_save_task_handle_);
}

// ——— RFID scan task ——————————————————————————————————————————————————————

void CheckpointLogic::rfidScanTask(void *arg)
{
    CheckpointLogic *self = static_cast<CheckpointLogic *>(arg);

    while (true) {
        if (self->state_ == CPState::SCANNING ||
            self->state_ == CPState::TASK_DONE) {

            rfid::Uid uid;
            if (self->mifare_->readCardSerial(uid) == rfid::Status::OK) {
                // Сразу халтим — карта уходит в HALT.
                // CardProcessor сам сделает reselect когда нужно.
                // Следующий скан: WUPA разбудит HALT карту.
                self->mifare_->haltA();
                self->mifare_->stopCrypto();

                if (self->state_ == CPState::TASK_DONE) {
                    if (self->pending_task_.active &&
                        self->uidMatch(uid, self->pending_task_.uid)) {
                        // Второе касание — та же карта вернулась
                        memcpy(&self->last_uid_, &uid, sizeof(rfid::Uid));
                        self->setState(CPState::WRITING_SECOND_TOUCH);
                    } else if (self->pending_task_.active) {
                        // Чужая карта — ошибка, сбрасываем задание
                        ESP_LOGE(TAG, "Wrong card during second touch wait — task cleared");
                        self->pending_task_.active = false;
                        self->audio_->error();
                        self->led_->setStatus(led::LedStatus::ERROR);
                        vTaskDelay(pdMS_TO_TICKS(2000));
                        self->led_->setStatus(self->cfg_mgr_.get().is_fox
                            ? led::LedStatus::FOX_MODE : led::LedStatus::IDLE);
                        self->setState(CPState::SCANNING);
                    }
                } else if (self->state_ == CPState::SCANNING && !self->card_present_) {
                    self->card_present_ = true;
                    memcpy(&self->last_uid_, &uid, sizeof(rfid::Uid));
                    self->setState(CPState::CARD_FOUND);
                }
            } else {
                self->card_present_ = false;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// ——— Timestamp save task ——————————————————————————————————————————————————

void CheckpointLogic::tsSaveTask(void *arg)
{
    CheckpointLogic *self = static_cast<CheckpointLogic *>(arg);

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(10 * 60 * 1000)); // 10 минут
        uint64_t ts = self->getTimestamp();
        if (ts > 0) {
            self->cfg_mgr_.saveTimestamp((uint32_t)ts);
            ESP_LOGI(TAG, "Timestamp saved to NVS: %" PRIu64, ts);
        }
    }
}

// ——— LoRa RX task ————————————————————————————————————————————————————————

void CheckpointLogic::loraRxTask(void *arg)
{
    CheckpointLogic *self = static_cast<CheckpointLogic *>(arg);

    while (true) {
        // Обрабатываем DIO0 IRQ (флаг устанавливается в ISR)
        self->radio_->handleDio0Isr();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// ——— Main task ————————————————————————————————————————————————————————————

void CheckpointLogic::mainTask(void *arg)
{
    CheckpointLogic *self = static_cast<CheckpointLogic *>(arg);

    while (true) {
        switch (self->state_) {
        case CPState::CARD_FOUND:
            self->handleCardFound(self->last_uid_);
            break;

        case CPState::WAITING_TASK:
            self->handleWaitingTask();
            break;

        case CPState::TASK_DONE:
            // Ждём второго касания; rfidScanTask переведёт в WRITING_SECOND_TOUCH.
            // Таймаут 1 минута — если не приложил карту, сбрасываем.
            if (self->pending_task_.active) {
                uint64_t now = self->getTimestamp();
                if ((now - self->pending_task_.created_at_sec) > SECOND_TOUCH_TIMEOUT_SEC) {
                    ESP_LOGW(TAG, "Second touch timeout — task cleared");
                    self->pending_task_.active = false;
                    self->audio_->error();
                    self->led_->setStatus(led::LedStatus::ERROR);
                    vTaskDelay(pdMS_TO_TICKS(2000));
                    self->led_->setStatus(self->cfg_mgr_.get().is_fox
                        ? led::LedStatus::FOX_MODE : led::LedStatus::IDLE);
                    self->setState(CPState::SCANNING);
                }
            }
            break;

        case CPState::WRITING_SECOND_TOUCH:
            self->handleWriteSecondTouch(self->last_uid_);
            break;

        case CPState::TASK_IN_PROGRESS:
            self->handleTaskInProgress();
            break;

        default:
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// ——— Обработчики состояний ———————————————————————————————————————————————

uint64_t CheckpointLogic::getTimestamp()
{
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return (uint64_t)tv.tv_sec;
}

bool CheckpointLogic::uidMatch(const rfid::Uid &a, const rfid::Uid &b)
{
    return a.size == b.size && memcmp(a.bytes, b.bytes, a.size) == 0;
}

void CheckpointLogic::setState(CPState new_state)
{
    ESP_LOGI(TAG, "State: %d → %d", (int)state_, (int)new_state);
    state_ = new_state;
}

void CheckpointLogic::handleCardFound(const rfid::Uid &uid)
{
    setState(CPState::WRITING_FIRST_TOUCH);
    handleWriteFirstTouch(uid);
}

void CheckpointLogic::handleWriteFirstTouch(const rfid::Uid &uid)
{
    const config::StationConfig &cfg = cfg_mgr_.get();
    uint64_t ts = getTimestamp();

    auto ledIdle = [&]() {
        led_->setStatus(cfg.is_fox ? led::LedStatus::FOX_MODE : led::LedStatus::IDLE);
    };

    if (cfg.has_task) {
        // Двухкасательный КП: первое касание
        rfid::SlotInfo slot;
        esp_err_t ret = card_proc_->writeFirstTouch(uid, cfg.cp_num, ts, slot);

        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "First touch write failed: %s", esp_err_to_name(ret));
            audio_->error();
            led_->setStatus(led::LedStatus::ERROR);
            vTaskDelay(pdMS_TO_TICKS(2000));
            ledIdle();
            setState(CPState::SCANNING);
            return;
        }

        // Если дубль — испытание НЕ предлагаем, просто подтверждаем отметку
        if (slot.was_duplicate) {
            ESP_LOGI(TAG, "Duplicate first touch — skipping task");
            audio_->cardWritten();
            logCpVisit(uid, cfg.cp_num, ts, 0x01, 0x0000);
            ledIdle();
            setState(CPState::SCANNING);
            return;
        }

        // Первое касание (flags=0x03): запоминаем и предлагаем испытание
        audio_->cardFound();
        logCpVisit(uid, cfg.cp_num, ts, 0x03, 0x0000);

        pending_task_.uid            = uid;
        pending_task_.slot           = slot;
        pending_task_.cp_num         = cfg.cp_num;
        pending_task_.ts_arrive      = ts;
        pending_task_.active         = true;
        pending_task_.created_at_sec = (uint32_t)(ts);

        audio_->taskPrompt();
        led_->setStatus(led::LedStatus::WAITING_TASK);
        setState(CPState::WAITING_TASK);

    } else {
        // Простой КП: одна запись (flags=0x00 или 0x01 при дубле)
        rfid::SlotInfo slot;
        esp_err_t ret = card_proc_->writeCpMark(uid, cfg.cp_num, ts, slot);

        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "CP mark write failed: %s", esp_err_to_name(ret));
            audio_->error();
            led_->setStatus(led::LedStatus::ERROR);
            vTaskDelay(pdMS_TO_TICKS(2000));
            ledIdle();
            setState(CPState::SCANNING);
            return;
        }

        audio_->cardWritten();
        logCpVisit(uid, cfg.cp_num, ts, slot.was_duplicate ? 0x01 : 0x00, 0x0000);
        ledIdle();
        setState(CPState::SCANNING);
    }
}

void CheckpointLogic::handleWaitingTask()
{
    const config::StationConfig &cfg = cfg_mgr_.get();

    // Таймаут ожидания
    uint64_t now = getTimestamp();
    if (pending_task_.active &&
        (now - pending_task_.created_at_sec) > SECOND_TOUCH_TIMEOUT_SEC) {
        ESP_LOGW(TAG, "Task timeout, resetting");
        pending_task_.active = false;
        audio_->error();
        led_->setStatus(led::LedStatus::ERROR);
        vTaskDelay(pdMS_TO_TICKS(2000));
        led_->setStatus(cfg.is_fox ? led::LedStatus::FOX_MODE : led::LedStatus::IDLE);
        setState(CPState::SCANNING);
        return;
    }

    // Запуск испытания по типу
    if (cfg.task.type == config::TaskType::ALCO_TEST) {
        setState(CPState::TASK_IN_PROGRESS);
    }
    // Для других типов — добавить здесь
}

void CheckpointLogic::handleTaskInProgress()
{
    const config::StationConfig &cfg = cfg_mgr_.get();

    if (cfg.task.type == config::TaskType::ALCO_TEST) {
        // ── 1. Прогрев 30 сек ────────────────────────────────────────────
        ESP_LOGI(TAG, "Alco: heater ON, warming up 30s...");
        led_->setStatus(led::LedStatus::TASK_IN_PROGRESS);
        mq3_->heatOn();
        vTaskDelay(pdMS_TO_TICKS(30000));

        // ── 2. Приглашение дуть ───────────────────────────────────────────
        ESP_LOGI(TAG, "Alco: BLOW NOW");
        led_->setStatus(led::LedStatus::ALCO_BLOW);
        audio_->taskPrompt();

        // ── 3. Замер 10 сек / 250 мс = 40 сэмплов, нагрев не выключаем ──
        uint16_t max_val = 0;
        for (int i = 0; i < 40; i++) {
            uint16_t v = mq3_->readRaw();
            if (v > max_val) max_val = v;
            vTaskDelay(pdMS_TO_TICKS(250));
        }
        ESP_LOGI(TAG, "Alco: max ADC = %d (0x%04X)", max_val, max_val);

        // ── 4. Нагрев выключить ───────────────────────────────────────────
        mq3_->heatOff();

        // ── 5. Сохранить результат (запишется в reserve при 2м касании) ──
        pending_task_.reserve_val = max_val;

        // ── 6. Пригласить поднести карту ──────────────────────────────────
        // Перезапускаем таймаут — отсчёт идёт от конца испытания
        pending_task_.created_at_sec = (uint32_t)getTimestamp();

        led_->setStatus(led::LedStatus::TASK_DONE);
        audio_->taskDone();
        setState(CPState::TASK_DONE);
        audio_->secondTouch();
    }
}

void CheckpointLogic::handleWriteSecondTouch(const rfid::Uid &uid)
{
    if (!pending_task_.active) {
        setState(CPState::SCANNING);
        return;
    }

    uint16_t reserve_val = pending_task_.reserve_val;

    // CardProcessor сам делает reselect + auth + write + verify + halt
    esp_err_t ret = card_proc_->writeSecondTouch(
        uid, pending_task_.slot,
        pending_task_.cp_num, pending_task_.ts_arrive,
        reserve_val);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Second touch write failed: %s", esp_err_to_name(ret));
        audio_->error();
        led_->setStatus(led::LedStatus::ERROR);
        vTaskDelay(pdMS_TO_TICKS(2000));
    } else {
        audio_->cardWritten();
        logCpVisit(uid, pending_task_.cp_num,
                   pending_task_.ts_arrive, 0x00, reserve_val);
    }

    pending_task_.active = false;
    // Карта может всё ещё быть в поле — предотвращаем немедленную повторную запись
    card_present_ = true;
    led_->setStatus(cfg_mgr_.get().is_fox ? led::LedStatus::FOX_MODE : led::LedStatus::IDLE);
    setState(CPState::SCANNING);
}

void CheckpointLogic::logCpVisit(const rfid::Uid &uid, uint8_t cp_num,
                                   uint64_t ts, uint8_t flags, uint16_t reserve)
{
    lora::LogEntryPacket entry = {};
    memcpy(entry.uid, uid.bytes, 4);
    entry.participant_id = 0; // Можно заполнить после readParticipant
    entry.cp_num         = cp_num;
    entry.ts_arrive      = ts;
    entry.flags          = flags;
    entry.reserve        = reserve;

    logger_.addEntry(entry);
}

} // namespace checkpoint

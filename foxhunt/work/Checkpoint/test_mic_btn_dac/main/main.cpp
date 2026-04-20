/**
 * FoxAss Mic / Button / DAC Test
 *
 * ——— Режимы (переключается одной строкой) ————————————————————————————————
 *
 *   PLAYBACK_MODE_ONESHOT    (0)   8 kHz, 10 сек, ~80 КБ
 *     Запись:          adc_oneshot + esp_timer @ 8 кГц
 *     Воспроизведение: dac_oneshot + esp_timer @ 8 кГц
 *
 *   PLAYBACK_MODE_CONTINUOUS (1)   22 kHz, 5 сек, ~108 КБ   ← активен
 *     Запись:          adc_continuous (I2S0 DMA) — нулевой CPU
 *     Воспроизведение: dac_continuous (I2S0 DMA) — нулевой CPU
 *     Оба захватывают I2S0 → handle уничтожается перед сменой роли.
 *
 *   ВАЖНО (режим 1): adc_lock внутри ESP-IDF — FreeRTOS mutex с priority
 *   inheritance. adc_continuous_start/stop ОБЯЗАНЫ вызываться из одного task.
 *   Кнопки только ставят команду → main task исполняет все ADC/DAC операции.
 *
 * ——— Пины ————————————————————————————————————————————————
 *   G35 = ADC mic input  (ADC1_CH7)
 *   G26 = DAC speaker output (DAC_CHAN_1)
 *   G32 = Button: длинное = начать запись, короткое = стоп / воспроизвести
 *   G33 = WS2812B LED
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "sensors/button.h"
#include "led/status_led.h"
#include <cstdlib>

// ——— Выбор режима ————————————————————————————————————————
#define PLAYBACK_MODE_ONESHOT    0
#define PLAYBACK_MODE_CONTINUOUS 1
#define PLAYBACK_MODE            PLAYBACK_MODE_CONTINUOUS

#if PLAYBACK_MODE == PLAYBACK_MODE_ONESHOT
#  include "esp_timer.h"
#  include "esp_adc/adc_oneshot.h"
#  include "driver/dac_oneshot.h"
#else
#  include "esp_adc/adc_continuous.h"
#  include "driver/dac_continuous.h"
#endif

static const char *TAG = "MicBtnDac";

// ——— Пины ————————————————————————————————————————————————
static constexpr gpio_num_t    PIN_LED    = GPIO_NUM_33;
static constexpr gpio_num_t    PIN_BUTTON = GPIO_NUM_32;
static constexpr adc_channel_t MIC_CH     = ADC_CHANNEL_7;

// ——— Параметры ——————————————————————————————————————————
#if PLAYBACK_MODE == PLAYBACK_MODE_ONESHOT
static constexpr uint32_t SAMPLE_RATE = 8000;
static constexpr uint32_t MAX_REC_SEC = 10;
#else
static constexpr uint32_t SAMPLE_RATE = 22050;
static constexpr uint32_t MAX_REC_SEC = 5;
#endif

static constexpr size_t   MAX_SAMPLES      = SAMPLE_RATE * MAX_REC_SEC;
static constexpr uint32_t SAMPLE_PERIOD_US = 1000000 / SAMPLE_RATE;

#if PLAYBACK_MODE == PLAYBACK_MODE_CONTINUOUS
static constexpr uint32_t DAC_DESC_NUM    = 8;
static constexpr size_t   DAC_BUF_SIZE    = 2048;
static constexpr uint32_t DAC_TAIL_MS     =
    (uint32_t)((uint64_t)DAC_DESC_NUM * DAC_BUF_SIZE * 1000ULL / SAMPLE_RATE);
static constexpr size_t   ADC_FRAME_BYTES = 256;
#endif

// ——— Состояния и команды ————————————————————————————————
enum class AppState : uint8_t { IDLE, RECORDING, HAS_DATA, PLAYING };

// Команды от кнопок → исполняются в main task
// (чтобы adc_continuous_start/stop были в одном task)
enum class Cmd : uint8_t { NONE, REC_START, REC_STOP, PLAY_START };

// ——— Контекст ————————————————————————————————————————————
struct App {
    sensors::Button  *button    = nullptr;
    led::StatusLed   *led       = nullptr;

    uint8_t          *rec_buf   = nullptr;
    volatile size_t   rec_pos   = 0;
    size_t            rec_len   = 0;

    volatile AppState state     = AppState::IDLE;
    volatile Cmd      pending   = Cmd::NONE;
    TaskHandle_t      main_task = nullptr;

#if PLAYBACK_MODE == PLAYBACK_MODE_ONESHOT
    adc_oneshot_unit_handle_t adc_handle  = nullptr;
    dac_oneshot_handle_t      dac_oneshot = nullptr;
    esp_timer_handle_t        rec_timer   = nullptr;
    esp_timer_handle_t        play_timer  = nullptr;
    volatile size_t           play_pos    = 0;
#else
    adc_continuous_handle_t   adc_cont    = nullptr;
#endif
};

static App g_app;

// ——— LED helper ——————————————————————————————————————————
static void updateLed(AppState s)
{
    switch (s) {
        case AppState::IDLE:      g_app.led->setStatus(led::LedStatus::IDLE);             break;
        case AppState::RECORDING: g_app.led->setStatus(led::LedStatus::TASK_IN_PROGRESS); break;
        case AppState::HAS_DATA:  g_app.led->setStatus(led::LedStatus::TASK_DONE);        break;
        case AppState::PLAYING:   g_app.led->setStatus(led::LedStatus::CARD_FOUND);       break;
    }
}

// ═══════════════════════════════════════════════════════════
//  РЕЖИМ 0: adc_oneshot + dac_oneshot + esp_timer
// ═══════════════════════════════════════════════════════════
#if PLAYBACK_MODE == PLAYBACK_MODE_ONESHOT

static void rec_timer_cb(void *arg)
{
    App   *app = static_cast<App *>(arg);
    size_t pos = app->rec_pos;
    if (pos >= MAX_SAMPLES) {
        esp_timer_stop(app->rec_timer);
        app->rec_len = pos;
        app->state   = AppState::HAS_DATA;
        xTaskNotifyGive(app->main_task);
        return;
    }
    int raw = 0;
    adc_oneshot_read(app->adc_handle, MIC_CH, &raw);
    int s = (raw - 2048) / 16 + 128;
    if (s <   0) s =   0;
    if (s > 255) s = 255;
    app->rec_buf[pos] = (uint8_t)s;
    app->rec_pos = pos + 1;
}

static void play_timer_cb(void *arg)
{
    App   *app = static_cast<App *>(arg);
    size_t pos = app->play_pos;
    if (pos >= app->rec_len) {
        esp_timer_stop(app->play_timer);
        dac_oneshot_output_voltage(app->dac_oneshot, 128);
        app->state = AppState::HAS_DATA;
        xTaskNotifyGive(app->main_task);
        return;
    }
    dac_oneshot_output_voltage(app->dac_oneshot, app->rec_buf[pos]);
    app->play_pos = pos + 1;
}

#endif // PLAYBACK_MODE_ONESHOT

// ═══════════════════════════════════════════════════════════
//  РЕЖИМ 1: adc_continuous + dac_continuous (оба I2S0 DMA)
// ═══════════════════════════════════════════════════════════
#if PLAYBACK_MODE == PLAYBACK_MODE_CONTINUOUS

static bool IRAM_ATTR rec_conv_done_cb(adc_continuous_handle_t /*h*/,
                                        const adc_continuous_evt_data_t *edata,
                                        void *user_data)
{
    App *app = static_cast<App *>(user_data);
    if (app->state != AppState::RECORDING) return false;

    const adc_digi_output_data_t *frames =
        reinterpret_cast<const adc_digi_output_data_t *>(edata->conv_frame_buffer);
    size_t n = edata->size / sizeof(adc_digi_output_data_t);
    BaseType_t hp = pdFALSE;

    for (size_t i = 0; i < n; i++) {
        if (frames[i].type1.channel != (uint8_t)MIC_CH) continue;
        size_t pos = app->rec_pos;
        if (pos >= MAX_SAMPLES) {
            // Сигнализируем main task остановить ADC (нельзя из ISR — FreeRTOS mutex)
            app->rec_len = pos;
            app->state   = AppState::HAS_DATA;
            vTaskNotifyGiveFromISR(app->main_task, &hp);
            break;
        }
        int raw = (int)frames[i].type1.data;
        int s   = (raw - 2048) / 16 + 128;
        if (s < 0)   s = 0;
        if (s > 255) s = 255;
        app->rec_buf[pos] = (uint8_t)s;
        app->rec_pos = pos + 1;
    }
    return hp == pdTRUE;
}

// Создаёт handle + конфигурирует + стартует. Вызывать ТОЛЬКО из main task.
static esp_err_t adcStart(App *app)
{
    adc_continuous_handle_cfg_t h = {};
    h.max_store_buf_size = ADC_FRAME_BYTES * 4;
    h.conv_frame_size    = ADC_FRAME_BYTES;
    esp_err_t ret = adc_continuous_new_handle(&h, &app->adc_cont);
    if (ret != ESP_OK) return ret;

    adc_digi_pattern_config_t pat = {};
    pat.atten = ADC_ATTEN_DB_12;  pat.channel = (uint8_t)MIC_CH;
    pat.unit  = ADC_UNIT_1;       pat.bit_width = ADC_BITWIDTH_12;

    adc_continuous_config_t cfg = {};
    cfg.sample_freq_hz = SAMPLE_RATE;
    cfg.conv_mode      = ADC_CONV_SINGLE_UNIT_1;
    cfg.adc_pattern    = &pat;
    cfg.pattern_num    = 1;
    ret = adc_continuous_config(app->adc_cont, &cfg);
    if (ret != ESP_OK) goto fail;

    {
        adc_continuous_evt_cbs_t cbs = {};
        cbs.on_conv_done = rec_conv_done_cb;
        ret = adc_continuous_register_event_callbacks(app->adc_cont, &cbs, app);
        if (ret != ESP_OK) goto fail;
    }
    return adc_continuous_start(app->adc_cont); // захватывает adc_lock в этом task

fail:
    adc_continuous_deinit(app->adc_cont);
    app->adc_cont = nullptr;
    return ret;
}

// Стопит и уничтожает handle. Вызывать из ТОГО ЖЕ task, что вызвал adcStart.
static void adcStop(App *app)
{
    if (app->adc_cont == nullptr) return;
    adc_continuous_stop(app->adc_cont);   // освобождает adc_lock
    adc_continuous_deinit(app->adc_cont); // освобождает I2S0
    app->adc_cont = nullptr;
}

static void playbackTask(void *arg)
{
    App *app = static_cast<App *>(arg);
    ESP_LOGI(TAG, "Playback (dac_continuous): %zu samples (%.2f sec)",
             app->rec_len, (float)app->rec_len / SAMPLE_RATE);

    dac_continuous_handle_t dac = nullptr;
    dac_continuous_config_t cfg = {};
    cfg.chan_mask = DAC_CHANNEL_MASK_CH1;
    cfg.desc_num  = DAC_DESC_NUM;
    cfg.buf_size  = DAC_BUF_SIZE;
    cfg.freq_hz   = SAMPLE_RATE;
    cfg.offset    = 0;
    cfg.clk_src   = DAC_DIGI_CLK_SRC_DEFAULT;
    cfg.chan_mode  = DAC_CHANNEL_MODE_SIMUL;

    esp_err_t ret = dac_continuous_new_channels(&cfg, &dac);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "dac_continuous init failed: %s", esp_err_to_name(ret));
        app->state = AppState::HAS_DATA;
        xTaskNotifyGive(app->main_task);
        vTaskDelete(nullptr);
        return;
    }
    ESP_ERROR_CHECK(dac_continuous_enable(dac));

    size_t loaded = 0;
    dac_continuous_write(dac, app->rec_buf, app->rec_len, &loaded, portMAX_DELAY);
    vTaskDelay(pdMS_TO_TICKS(DAC_TAIL_MS + 300));

    dac_continuous_disable(dac);
    dac_continuous_del_channels(dac); // освобождает I2S0

    app->state = AppState::HAS_DATA;
    xTaskNotifyGive(app->main_task);
    ESP_LOGI(TAG, "Playback done (%zu/%zu bytes)", loaded, app->rec_len);
    vTaskDelete(nullptr);
}

#endif // PLAYBACK_MODE_CONTINUOUS

// ——— Операции (вызываются ТОЛЬКО из main task) ————————————
static void startRecording()
{
    if (g_app.state == AppState::RECORDING) return;
    g_app.rec_pos = 0;
    g_app.state   = AppState::RECORDING;
    updateLed(AppState::RECORDING);

#if PLAYBACK_MODE == PLAYBACK_MODE_ONESHOT
    esp_timer_start_periodic(g_app.rec_timer, SAMPLE_PERIOD_US);
#else
    esp_err_t ret = adcStart(&g_app);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ADC start failed: %s", esp_err_to_name(ret));
        g_app.state = AppState::IDLE;
        updateLed(AppState::IDLE);
        return;
    }
#endif
    ESP_LOGI(TAG, "Recording started @ %lu Hz, max %lu sec",
             (unsigned long)SAMPLE_RATE, (unsigned long)MAX_REC_SEC);
}

static void stopRecording()
{
    if (g_app.state != AppState::RECORDING) return;
    g_app.state   = AppState::HAS_DATA;  // callback/ISR перестанут писать
    g_app.rec_len = g_app.rec_pos;

#if PLAYBACK_MODE == PLAYBACK_MODE_ONESHOT
    esp_timer_stop(g_app.rec_timer);
#else
    adcStop(&g_app); // освобождает adc_lock и I2S0 (в том же task, что и adcStart)
#endif
    updateLed(AppState::HAS_DATA);
    ESP_LOGI(TAG, "Recording stopped: %zu samples (%.2f sec)",
             g_app.rec_len, (float)g_app.rec_len / SAMPLE_RATE);
}

static void startPlayback()
{
    if (g_app.state == AppState::PLAYING) return;
    if (g_app.rec_len == 0) { ESP_LOGW(TAG, "No data to play"); return; }
    g_app.state = AppState::PLAYING;
    updateLed(AppState::PLAYING);
    ESP_LOGI(TAG, "Playback: %zu samples (%.2f sec)",
             g_app.rec_len, (float)g_app.rec_len / SAMPLE_RATE);

#if PLAYBACK_MODE == PLAYBACK_MODE_ONESHOT
    g_app.play_pos = 0;
    esp_timer_start_periodic(g_app.play_timer, SAMPLE_PERIOD_US);
#else
    xTaskCreate(playbackTask, "playback", 4096, &g_app, 5, nullptr);
#endif
}

// ——— Кнопки: только ставим команду, main task исполняет ——
static void onShortPress()
{
    switch (g_app.state) {
        case AppState::RECORDING:
            g_app.pending = Cmd::REC_STOP;
            xTaskNotifyGive(g_app.main_task);
            break;
        case AppState::HAS_DATA:
            g_app.pending = Cmd::PLAY_START;
            xTaskNotifyGive(g_app.main_task);
            break;
        default: break;
    }
}

static void onLongPress()
{
    switch (g_app.state) {
        case AppState::IDLE:
        case AppState::HAS_DATA:
            g_app.pending = Cmd::REC_START;
            xTaskNotifyGive(g_app.main_task);
            break;
        default: break;
    }
}

// ——— app_main ————————————————————————————————————————————
extern "C" void app_main(void)
{
#if PLAYBACK_MODE == PLAYBACK_MODE_ONESHOT
    const char *mode_str = "ONESHOT (adc/dac oneshot+timer, 8 kHz, 10 sec)";
#else
    const char *mode_str = "CONTINUOUS (adc/dac continuous DMA, 22 kHz, 5 sec)";
#endif
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  FoxAss Mic / Button / DAC Test");
    ESP_LOGI(TAG, "  Mode: %s", mode_str);
    ESP_LOGI(TAG, "========================================");

    g_app.main_task = xTaskGetCurrentTaskHandle();

    g_app.rec_buf = (uint8_t *)malloc(MAX_SAMPLES);
    if (!g_app.rec_buf) { ESP_LOGE(TAG, "OOM: %zu bytes", MAX_SAMPLES); return; }
    ESP_LOGI(TAG, "Buffer: %zu bytes (%lu Hz × %lu sec)",
             MAX_SAMPLES, (unsigned long)SAMPLE_RATE, (unsigned long)MAX_REC_SEC);

#if PLAYBACK_MODE == PLAYBACK_MODE_ONESHOT
    {
        adc_oneshot_unit_init_cfg_t u = {};
        u.unit_id = ADC_UNIT_1;  u.ulp_mode = ADC_ULP_MODE_DISABLE;
        ESP_ERROR_CHECK(adc_oneshot_new_unit(&u, &g_app.adc_handle));
        adc_oneshot_chan_cfg_t c = {};
        c.atten = ADC_ATTEN_DB_12;  c.bitwidth = ADC_BITWIDTH_12;
        ESP_ERROR_CHECK(adc_oneshot_config_channel(g_app.adc_handle, MIC_CH, &c));
    }
    {
        dac_oneshot_config_t d = {};  d.chan_id = DAC_CHAN_1;
        ESP_ERROR_CHECK(dac_oneshot_new_channel(&d, &g_app.dac_oneshot));
        dac_oneshot_output_voltage(g_app.dac_oneshot, 128);
    }
    {
        esp_timer_create_args_t t = {};  t.dispatch_method = ESP_TIMER_TASK;
        t.callback = rec_timer_cb;  t.arg = &g_app;  t.name = "rec";
        ESP_ERROR_CHECK(esp_timer_create(&t, &g_app.rec_timer));
        t.callback = play_timer_cb;  t.name = "play";
        ESP_ERROR_CHECK(esp_timer_create(&t, &g_app.play_timer));
    }
    ESP_LOGI(TAG, "ADC/DAC oneshot + timers OK");
#else
    ESP_LOGI(TAG, "ADC/DAC continuous — handle создаётся при каждой записи/воспроизведении");
#endif

    g_app.led = new led::StatusLed(PIN_LED);
    ESP_ERROR_CHECK(g_app.led->init());
    updateLed(AppState::IDLE);
    ESP_LOGI(TAG, "LED OK (G33)");

    g_app.button = new sensors::Button(PIN_BUTTON, 1500);
    g_app.button->onShortPress(onShortPress);
    g_app.button->onLongPress(onLongPress);
    ESP_ERROR_CHECK(g_app.button->init());
    ESP_LOGI(TAG, "Button OK (G32)");

    ESP_LOGI(TAG, "Ready. Long=начать запись  Short=стоп/воспроизвести");

    AppState prev = AppState::IDLE;

    while (true) {
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1000));

        // Выполняем команду от кнопки (в main task — безопасно для adc_lock)
        Cmd cmd      = g_app.pending;
        g_app.pending = Cmd::NONE;
        if (cmd == Cmd::REC_START)  startRecording();
        if (cmd == Cmd::REC_STOP)   stopRecording();
        if (cmd == Cmd::PLAY_START) startPlayback();

        AppState cur = g_app.state;

        // Режим 1: буфер заполнился из ISR — остановить ADC (в main task!)
#if PLAYBACK_MODE == PLAYBACK_MODE_CONTINUOUS
        if (prev == AppState::RECORDING && cur == AppState::HAS_DATA) {
            adcStop(&g_app); // безопасно: тот же task, что вызывал adcStart
            if (g_app.rec_len == 0) g_app.rec_len = g_app.rec_pos;
            ESP_LOGI(TAG, "Buffer full: %zu samples", g_app.rec_len);
        }
#endif

        if (cur != prev) {
            updateLed(cur);
            const char *names[] = {"IDLE", "RECORDING", "HAS_DATA", "PLAYING"};
            ESP_LOGI(TAG, "State: %s  rec=%zu/%zu",
                     names[(uint8_t)cur], g_app.rec_len, MAX_SAMPLES);
            prev = cur;
        }

        if (cur == AppState::RECORDING) {
            size_t pos = g_app.rec_pos;
            ESP_LOGI(TAG, "  recording... %zu/%zu (%.1f%%)",
                     pos, MAX_SAMPLES, pos * 100.0f / MAX_SAMPLES);
        }
#if PLAYBACK_MODE == PLAYBACK_MODE_ONESHOT
        if (cur == AppState::PLAYING) {
            size_t pos = g_app.play_pos;
            ESP_LOGI(TAG, "  playing...   %zu/%zu (%.1f%%)",
                     pos, g_app.rec_len, pos * 100.0f / g_app.rec_len);
        }
#endif
    }
}

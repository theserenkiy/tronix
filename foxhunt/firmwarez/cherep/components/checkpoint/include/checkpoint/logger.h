#pragma once

#include "lora/protocol.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <cstdint>
#include <cstddef>

namespace checkpoint {

static constexpr size_t LOG_CAPACITY = 64;

/**
 * Кольцевой буфер записей посещений КП.
 * Потокобезопасный через мьютекс.
 */
class CPLogger {
public:
    CPLogger();
    ~CPLogger();

    /** Добавить запись. При переполнении перезаписывает самую старую. */
    void addEntry(const lora::LogEntryPacket &entry);

    /**
     * Скопировать до max_count записей в buf.
     * @return Количество скопированных записей
     */
    size_t getEntries(lora::LogEntryPacket *buf, size_t max_count) const;

    /** Количество записей. */
    size_t count() const;

    /** Очистить лог. */
    void clear();

private:
    lora::LogEntryPacket entries_[LOG_CAPACITY];
    size_t               head_  = 0;
    size_t               count_ = 0;
    SemaphoreHandle_t    mutex_ = nullptr;
};

} // namespace checkpoint

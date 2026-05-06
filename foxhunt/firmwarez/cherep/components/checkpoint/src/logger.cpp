#include "checkpoint/logger.h"
#include <cstring>

namespace checkpoint {

CPLogger::CPLogger()
{
    mutex_ = xSemaphoreCreateMutex();
}

CPLogger::~CPLogger()
{
    if (mutex_) vSemaphoreDelete(mutex_);
}

void CPLogger::addEntry(const lora::LogEntryPacket &entry)
{
    xSemaphoreTake(mutex_, portMAX_DELAY);

    entries_[head_] = entry;
    head_ = (head_ + 1) % LOG_CAPACITY;
    if (count_ < LOG_CAPACITY) count_++;

    xSemaphoreGive(mutex_);
}

size_t CPLogger::getEntries(lora::LogEntryPacket *buf, size_t max_count) const
{
    xSemaphoreTake(mutex_, portMAX_DELAY);

    size_t n = (count_ < max_count) ? count_ : max_count;

    if (n == 0) {
        xSemaphoreGive(mutex_);
        return 0;
    }

    // Вернуть последние n записей (самые свежие)
    for (size_t i = 0; i < n; i++) {
        size_t idx = (head_ + LOG_CAPACITY - count_ + i) % LOG_CAPACITY;
        buf[i] = entries_[idx];
    }

    xSemaphoreGive(mutex_);
    return n;
}

size_t CPLogger::count() const
{
    xSemaphoreTake(mutex_, portMAX_DELAY);
    size_t c = count_;
    xSemaphoreGive(mutex_);
    return c;
}

void CPLogger::clear()
{
    xSemaphoreTake(mutex_, portMAX_DELAY);
    head_  = 0;
    count_ = 0;
    xSemaphoreGive(mutex_);
}

} // namespace checkpoint

#pragma once
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include "common.h"

#pragma pack(push, 1)

typedef struct {
    // ---- Блок RIFF ----
    char     chunkID[4] __attribute__((nonstring));       // "RIFF"
    uint32_t chunkSize;       // Размер файла - 8 байт
    char     format[4] __attribute__((nonstring));        // "WAVE"

    // ---- Блок fmt ----
    char     subchunk1ID[4] __attribute__((nonstring));   // "fmt "
    uint32_t subchunk1Size;   // Размер этого подблока (16 для PCM)
    uint16_t audioFormat;     // Формат (1 для PCM)
    uint16_t numChannels;     // Количество каналов
    uint32_t sampleRate;      // Частота дискретизации
    uint32_t byteRate;        // Количество байт в секунду
    uint16_t blockAlign;      // Размер фрейма (Channels * BitsPerSample / 8)
    uint16_t bitsPerSample;   // Битность (Глубина звука)

	char     listID[4] __attribute__((nonstring));       // Всегда "LIST"
    uint32_t listSize;       // Размер этого блока (включая "INFO", тег и текст) = 4 + 4 + 4 + textPaddedSize
    char     listType[4] __attribute__((nonstring));     // Всегда "INFO"
    
    char     tagID[4] __attribute__((nonstring));        // Например, "ICMT" (комментарий) или "INAM" (название)
    uint32_t tagSize;         // Длина текста с учетом выравнивания (четное число)
    char     text[WAV_INFO_SZ];       // Ваш текстовый буфер фиксированного размера	


    // ---- Блок data ----
    char     subchunk2ID[4] __attribute__((nonstring));   // "data"
    uint32_t subchunk2Size;   // Размер аудиоданных в байтах
} WAVHeader;

#pragma pack(pop) // Возвращаем стандартное выравнивание

FILE *wav_open(char *fname, int nsamp, uint32_t fs, char *info);

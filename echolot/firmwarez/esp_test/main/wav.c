#include "wav.h"
#include "esp_timer.h"

// Отключаем выравнивание, чтобы структура занимала ровно 44 байта в памяти
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
    char     text[1024];       // Ваш текстовый буфер фиксированного размера	


    // ---- Блок data ----
    char     subchunk2ID[4] __attribute__((nonstring));   // "data"
    uint32_t subchunk2Size;   // Размер аудиоданных в байтах
} WAVHeader;

#pragma pack(pop) // Возвращаем стандартное выравнивание



FILE *wav_open(char* fname, int nsamp, uint32_t fs, char *info)
{
	uint32_t payload_size_bytes = nsamp << 1;

	WAVHeader hdr = {
		.chunkID = "RIFF",
		.chunkSize = payload_size_bytes + 44 - 8,
		.format = "WAVE",
		.subchunk1ID = "fmt ",
		.subchunk1Size = 16,
		.audioFormat = 1,
		.numChannels = 1,
		.sampleRate = fs,
		.byteRate = fs*2,
		.blockAlign = 2,
		.bitsPerSample = 16,

		.listID = "LIST",
		.listSize = 1024+12,
		.listType = "INFO",
		.tagID = "ICMT",
		.tagSize = 1024,

		.subchunk2ID = "data",
		.subchunk2Size = payload_size_bytes	
	};

	strncpy(hdr.text, info, 1023);

	// static uint8_t io_buf[8192];

	printf(">> OPEN FILE...\n");
	FILE *fp = fopen(fname,"wb");
	// setvbuf(
	// 	fp,
	// 	(char *)io_buf,
	// 	_IOFBF,
	// 	sizeof(io_buf)
	// );
	printf("	open OK\n");
	fwrite(&hdr,1,sizeof(WAVHeader),fp);
	printf("	header OK\n");
	return fp;
}
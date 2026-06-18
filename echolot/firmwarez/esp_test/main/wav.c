#include "wav.h"

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

    // ---- Блок data ----
    char     subchunk2ID[4] __attribute__((nonstring));   // "data"
    uint32_t subchunk2Size;   // Размер аудиоданных в байтах
} WAVHeader;

#pragma pack(pop) // Возвращаем стандартное выравнивание


void save_wav(uint16_t *buf, int nsamp, char* fname, uint32_t fs, uint8_t bitshift)
{
	uint32_t payload_size_bytes = nsamp << 1;

	if(bitshift)
	{
		for(int i=0; i < nsamp; i++)
		{
			buf[i] = buf[i] << bitshift;
		}
	}

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
		.subchunk2ID = "data",
		.subchunk2Size = payload_size_bytes	
	};
	
	FILE *fp = fopen(fname,"w");
	fwrite(&hdr,1,sizeof(WAVHeader),fp);
	fwrite(buf,2,nsamp,fp);
	fclose(fp);
}
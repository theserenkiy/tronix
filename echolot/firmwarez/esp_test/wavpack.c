#include <stdio.h>
#include <inttypes.h>

#include <stdint.h>
#include <string.h>

// Отключаем выравнивание, чтобы структура занимала ровно 44 байта в памяти
#pragma pack(push, 1)

typedef struct {
    // ---- Блок RIFF ----
    char     chunkID[4];       // "RIFF"
    uint32_t chunkSize;       // Размер файла - 8 байт
    char     format[4];        // "WAVE"

    // ---- Блок fmt ----
    char     subchunk1ID[4];   // "fmt "
    uint32_t subchunk1Size;   // Размер этого подблока (16 для PCM)
    uint16_t audioFormat;     // Формат (1 для PCM)
    uint16_t numChannels;     // Количество каналов
    uint32_t sampleRate;      // Частота дискретизации
    uint32_t byteRate;        // Количество байт в секунду
    uint16_t blockAlign;      // Размер фрейма (Channels * BitsPerSample / 8)
    uint16_t bitsPerSample;   // Битность (Глубина звука)

    // ---- Блок data ----
    char     subchunk2ID[4];   // "data"
    uint32_t subchunk2Size;   // Размер аудиоданных в байтах
} WAVHeader;

#pragma pack(pop) // Возвращаем стандартное выравнивание


int save_wav(uint16_t *buf, int nsamp, char* fname, uint32_t fs, uint8_t ampl)
{
	uint32_t payload_size_bytes = nsamp << 1;

	if(ampl)
	{
		for(int i=0; i < nsamp; i++)
		{
			buf[i] = buf[i] * ampl;
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


int main()
{
	uint16_t buf[20000];

	FILE *fp = fopen("ac.bin","r");
	fread(buf,2,20000,fp);
	fclose(fp);

	save_wav(buf, 20000, "test3.wav", 6250, 8);

	return 0;
}
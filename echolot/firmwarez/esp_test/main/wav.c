#include "common.h"
#include "wav.h"
#include "esp_timer.h"

// Отключаем выравнивание, чтобы структура занимала ровно 44 байта в памяти




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
		.listSize = WAV_INFO_SZ+12,
		.listType = "INFO",
		.tagID = "ICMT",
		.tagSize = WAV_INFO_SZ,

		.subchunk2ID = "data",
		.subchunk2Size = payload_size_bytes	
	};

	strncpy(hdr.text, info, WAV_INFO_SZ-1);

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
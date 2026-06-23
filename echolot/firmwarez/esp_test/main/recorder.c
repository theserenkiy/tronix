#include "common.h"
#include "sonar.h"
#include "chirp.h"
#include "wav.h"
#include "recorder.h"


void recorder_make_record()
{
	wav_open("test.wav", ADC_RECORD_SAMPLES, 3125, "Test info");
	chirp_gen(178, 190, 250);
	sonar_ping(sonar_buffer,6);
}
#include <string>
#include <vector>
#include <cassert>
#include <csignal>
#include <iostream>

#ifdef __cplusplus
extern "C" {
#endif
//#include <portaudio.h>
#include "portaudio.h"
#include "pa_ringbuffer.h"
#include "pa_util.h"

#ifdef __cplusplus
}
#endif


//snowboyclass
class PortAudioWrapper {
 public:
  // Constructor.
  PortAudioWrapper(int sample_rate, int num_channels, int bits_per_sample);
  // Reads data from ring buffer.

  void Read(std::vector<int16_t>* data);
  void Stop(void);
  bool Start(void);

  int Callback(const void* input, void* output, unsigned long frame_count, const PaStreamCallbackTimeInfo* time_info, PaStreamCallbackFlags status_flags);

  ~PortAudioWrapper();

 private:
  // Initialization.
  bool Init(int sample_rate, int num_channels, int bits_per_sample);

 private:
  // Pointer to the ring buffer memory.
  char* ringbuffer_;

  // Ring buffer wrapper used in PortAudio.
  PaUtilRingBuffer pa_ringbuffer_;

  // Pointer to PortAudio stream.
  PaStream* pa_stream_;

  // Number of lost samples at each Read() due to ring buffer overflow.
  int num_lost_samples_;

  // Wait for this number of samples in each Read() call.
  int min_read_samples_;

  //data
  int sample_rate_;
  int num_channels_;
  int bits_per_sample_;
  int ringbuffer_size;

public:
  //initialisation ok ?
  bool ready;
};

/******************************************************************************************************************/
/* Class to record sound */

/* Select sample format. */
#if 0
#define PA_SAMPLE_TYPE  paFloat32
typedef float SAMPLE;
#define SAMPLE_SILENCE  (0.0f)
#define PRINTF_S_FORMAT "%.8f"
#elif 1
#define PA_SAMPLE_TYPE  paInt16
typedef short SAMPLE;
#define SAMPLE_SILENCE  (0)
#define PRINTF_S_FORMAT "%d"
#elif 0
#define PA_SAMPLE_TYPE  paInt8
typedef char SAMPLE;
#define SAMPLE_SILENCE  (0)
#define PRINTF_S_FORMAT "%d"
#elif 0
#define PA_SAMPLE_TYPE  paInt32
typedef long SAMPLE;
#define SAMPLE_SILENCE  (0)
#define PRINTF_S_FORMAT "%ld"
#else
#define PA_SAMPLE_TYPE  paUInt8
typedef unsigned char SAMPLE;
#define SAMPLE_SILENCE  (128)
#define PRINTF_S_FORMAT "%d"
#endif


void AudioRecordConfig(int,int);


typedef struct
{
	char RIFF_marker[4];
	uint32_t data_size;
	char filetype_header[4];
	char format_marker[4];
	uint32_t data_header_length;
	uint16_t format_type;
	uint16_t number_of_channels;
	uint32_t sample_rate;
	uint32_t bytes_per_second;
	uint16_t bytes_per_frame;
	uint16_t bits_per_sample;
} WaveHeader;

typedef struct
{
	int frameIndex;  /* Index into sample array. */
	int maxFrameIndex;
	SAMPLE *recordedSamples;
} PAData;



class cRecord
{
public:

	//constructeur
	cRecord();

	// Méthodes
	char * RecordSound(uint32_t duration, size_t *size);
	void Stop(void);
	bool Start(void);
	void SetSampleRate(int);

	void SetSpectro(long);

	//destructeur
	~cRecord();

private:

	// Attributs
	WaveHeader *hdr;
	PAData data;
	PaStream* stream;

	int Defaut_sample_rate;

	//config



};

class cPlay
{
public:

	//constructeur
	cPlay();

	// Méthodes
	int PlayWav(char * file);
	bool portAudioOpen(void);
	int readFmtChunk(uint32_t chunkLen);

	//destructeur
	~cPlay();

private:

	// Attributs
	PaStream* stream;

	int sampleRate;
	PaSampleFormat sampleFormat;
	bool ready;



};

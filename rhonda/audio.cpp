#include <stdlib.h>
#include <stdint.h>
#include <string.h>


//#include "record.h"
#include "flac.h"
#include "stdio.h"
#include "hardware.h"
//#include <pa_ringbuffer.h>
#include "audio.h"

#include "prog.h"

#ifndef _WIN32
#include <unistd.h>
#endif

#pragma comment(lib,"libs/libsnowboy-detect.a")
//#pragma comment(lib,"libs/portaudio/install/lib/libportaudio.so")


#define NUM_CHANNELS    (1)
#define SAMPLE_RATE  (44100)
#define FRAMES_PER_BUFFER (512)
#define MAXSILENCE 900
#define MINSILENCE 500
#define MINFRAME 1.1f

#define MAGICNUMBER 8000

int fd = 0;

bool PortAudioInitialised = false;

int Config_Gain=2;
int Min_Amplitude = 2000;


WaveHeader *genericWAVHeader(WaveHeader*hdr, uint32_t sample_rate, uint16_t bit_depth, uint16_t channels)
{
	if (!hdr) return NULL;

	memcpy(&hdr->RIFF_marker, "RIFF", sizeof(hdr->RIFF_marker));
	memcpy(&hdr->filetype_header, "WAVE", sizeof(hdr->filetype_header));
	memcpy(&hdr->format_marker, "fmt ", sizeof(hdr->format_marker));
	hdr->data_header_length = 16;
	hdr->format_type = 1;
	hdr->number_of_channels = channels;
	hdr->sample_rate = sample_rate;
	hdr->bytes_per_second = sample_rate * channels * bit_depth / 8;
	hdr->bytes_per_frame = channels * bit_depth / 8;
	hdr->bits_per_sample = bit_depth;

	return hdr;
}


int writeWAVHeader(FILE* fd, WaveHeader *hdr)
{
	uint32_t file_size;
	file_size = hdr->data_size + 36;

	if (!hdr) return -1;

	fwrite(hdr->RIFF_marker, sizeof(hdr->RIFF_marker), 1, fd);
	fwrite(&file_size, sizeof(hdr->data_size), 1, fd);
	fwrite(hdr->filetype_header, sizeof(hdr->filetype_header), 1, fd);
	fwrite(hdr->format_marker, sizeof(hdr->format_marker), 1, fd);
	fwrite(&hdr->data_header_length, sizeof(hdr->data_header_length), 1, fd);
	fwrite(&hdr->format_type, sizeof(hdr->format_type), 1, fd);
	fwrite(&hdr->number_of_channels, sizeof(hdr->number_of_channels), 1, fd);
	fwrite(&hdr->sample_rate, sizeof(hdr->sample_rate), 1, fd);
	fwrite(&hdr->bytes_per_second, sizeof(hdr->bytes_per_second), 1, fd);
	fwrite(&hdr->bytes_per_frame, sizeof(hdr->bytes_per_frame), 1, fd);
	fwrite(&hdr->bits_per_sample, sizeof(hdr->bits_per_sample), 1, fd);
	fwrite("data", 4, 1, fd);
	fwrite(&hdr->data_size, sizeof(hdr->data_size), 1, fd);

	return 0;
}


int writeWAVHeaderBuffer(char* buff2, WaveHeader *hdr)
{
	uint32_t file_size;

	char *buff = buff2;
	char t[5] = "data";

	file_size = hdr->data_size + 36;

	if (!hdr) return -1;

	memcpy(buff, hdr->RIFF_marker, sizeof(hdr->RIFF_marker));
	buff += sizeof(hdr->RIFF_marker);
	memcpy(buff, &file_size, sizeof(hdr->data_size));
	buff += sizeof(hdr->data_size);
	memcpy(buff, hdr->filetype_header, sizeof(hdr->filetype_header));
	buff += sizeof(hdr->filetype_header);
	memcpy(buff, hdr->format_marker, sizeof(hdr->format_marker));
	buff += sizeof(hdr->format_marker);
	memcpy(buff, &hdr->data_header_length, sizeof(hdr->data_header_length));
	buff += sizeof(hdr->data_header_length);
	memcpy(buff, &hdr->format_type, sizeof(hdr->format_type));
	buff += sizeof(hdr->format_type);
	memcpy(buff, &hdr->number_of_channels, sizeof(hdr->number_of_channels));
	buff += sizeof(hdr->number_of_channels);
	memcpy(buff, &hdr->sample_rate, sizeof(hdr->sample_rate));
	buff += sizeof(hdr->sample_rate);
	memcpy(buff, &hdr->bytes_per_second, sizeof(hdr->bytes_per_second));
	buff += sizeof(hdr->bytes_per_second);
	memcpy(buff, &hdr->bytes_per_frame, sizeof(hdr->bytes_per_frame));
	buff += sizeof(hdr->bytes_per_frame);
	memcpy(buff, &hdr->bits_per_sample, sizeof(hdr->bits_per_sample));
	buff += sizeof(hdr->bits_per_sample);
	memcpy(buff, t, 4);
	buff += 4;
	memcpy(buff, &hdr->data_size, sizeof(hdr->data_size));
	buff += sizeof(hdr->data_size);

	return 0;
}

/**************************************************************************/
/******                        Common fonction                       ******/
/**************************************************************************/

int InitPortAudio(void)
{

	if (PortAudioInitialised) return true;


#ifndef _WIN32
	//disable output error
	fd = dup(fileno(stdout));
	freopen("/dev/null", "w", stderr);
#endif


	// Initializes PortAudio.
	PaError pa_init_ans = Pa_Initialize();

	if (pa_init_ans != paNoError) {
		wprintf(L"Fail to initialize PortAudio, error message is %s\n", Pa_GetErrorText(pa_init_ans));
		return false;
	}


		{
			int i;

			PaDeviceIndex numDevices = Pa_GetDeviceCount();
			if (numDevices < 0)
			{
				wprintf(L"ERROR: Pa_GetDeviceCount returned 0x%x\n", numDevices);
			}

			wprintf(L"Number of devices = %d\n", numDevices);
			for (i = 0; i<numDevices; i++)
			{
				const PaDeviceInfo * deviceInfo = Pa_GetDeviceInfo(i);
				wprintf(L"[Device] %s", deviceInfo->name);

				if (i == Pa_GetDefaultInputDevice())
				{
					wprintf(L"[ Default Input ]");
				}
				if (i == Pa_GetDefaultOutputDevice())
				{
					wprintf(L"[ Default Output ]");
				}
				wprintf(L"\n");
			}
		}

	PortAudioInitialised = true;

#ifndef _WIN32
	//clear and restore it
	fflush(stderr);
	dup2(fd, fileno(stderr));
#endif

	return true;
}


/**************************************************************************/
/******                        Snowboy fonction                      ******/
/**************************************************************************/



int PortAudioCallback(const void* input, void* output, unsigned long frame_count, const PaStreamCallbackTimeInfo* time_info, PaStreamCallbackFlags status_flags, void* user_data);



PortAudioWrapper::PortAudioWrapper(int sample_rate, int num_channels, int bits_per_sample) {
	num_lost_samples_ = 0;
	pa_stream_ = NULL;
	ready = false;
	min_read_samples_ = (int)(sample_rate * 0.1);
	ringbuffer_ = NULL;

	ringbuffer_size = 16384;

	wprintf(L"\033[0;31mInitialise Snowboy\033[0;37m\n");

	InitPortAudio();

	if (Init(sample_rate, num_channels, bits_per_sample)) ready = true;
}



void PortAudioWrapper::Read(std::vector<int16_t>* data) {
	assert(data != NULL);

	if (!ready) return;

	// Checks ring buffer overflow.
	if (num_lost_samples_ > 0) {
		wprintf(L"Lost %d samples due to ring buffer overflow.\n", num_lost_samples_);
		num_lost_samples_ = 0;
	}

	ring_buffer_size_t num_available_samples = 0;

	while (true) {
		num_available_samples = PaUtil_GetRingBufferReadAvailable(&pa_ringbuffer_);
		if (num_available_samples >= min_read_samples_) {
			break;
		}
		Pa_Sleep(5);
	}

	// Reads data.
	num_available_samples = PaUtil_GetRingBufferReadAvailable(&pa_ringbuffer_);
	data->resize(num_available_samples);

	ring_buffer_size_t num_read_samples = PaUtil_ReadRingBuffer(&pa_ringbuffer_, data->data(), num_available_samples);

	if (num_read_samples != num_available_samples)
	{
		wprintf(L"%d samples were available,  but only %d samples were read.\n", num_available_samples, num_read_samples);
	}
}

int PortAudioWrapper::Callback(const void* input, void* output,	unsigned long frame_count, const PaStreamCallbackTimeInfo* time_info, PaStreamCallbackFlags status_flags)
{
	// Input audio.
	ring_buffer_size_t num_written_samples = PaUtil_WriteRingBuffer(&pa_ringbuffer_, input, frame_count);
	num_lost_samples_ += frame_count - num_written_samples;
	return paContinue;
}

PortAudioWrapper::~PortAudioWrapper() {
	if (pa_stream_)
	{
		//Pa_StopStream(pa_stream_);
		//Pa_CloseStream(pa_stream_);
		Stop();
	}
	Pa_Terminate();
	PaUtil_FreeMemory(ringbuffer_);
}


bool PortAudioWrapper::Init(int sample_rate, int num_channels, int bits_per_sample)
{
	// Allocates ring buffer memory.
	if (ringbuffer_) PaUtil_FreeMemory(ringbuffer_);
	ringbuffer_ = static_cast<char*>( PaUtil_AllocateMemory(bits_per_sample / 8 * ringbuffer_size));

	if (ringbuffer_ == NULL) {
		wprintf(L"Fail to allocate memory for ring buffer.\n");
		return false;
	}

	sample_rate_ = sample_rate;
	num_channels_ = num_channels;
	bits_per_sample_ = bits_per_sample;

	return Start();
}


void PortAudioWrapper::Stop()
{
	if (pa_stream_)
	{
		Pa_StopStream(pa_stream_);
		//Pa_AbortStream(pa_stream_);
		Pa_CloseStream(pa_stream_);
		pa_stream_ = NULL;
	}
}

bool PortAudioWrapper::Start()
{

	PaError pa_open_ans;

	if (pa_stream_) return false;

	if (bits_per_sample_ == 8)
	{
		pa_open_ans = Pa_OpenDefaultStream(&pa_stream_, num_channels_, 0, paUInt8, sample_rate_, paFramesPerBufferUnspecified, PortAudioCallback, this);
	}
	else if (bits_per_sample_ == 16)
	{
		pa_open_ans = Pa_OpenDefaultStream(&pa_stream_, num_channels_, 0, paInt16, sample_rate_, paFramesPerBufferUnspecified, PortAudioCallback, this);
	}
	else if (bits_per_sample_ == 32)
	{
		pa_open_ans = Pa_OpenDefaultStream(&pa_stream_, num_channels_, 0, paInt32, sample_rate_, paFramesPerBufferUnspecified, PortAudioCallback, this);
	}
	else {
		wprintf(L"Unsupported BitsPerSample: %d\n", bits_per_sample_);
		return false;
	}
	if (pa_open_ans != paNoError) {
		wprintf(L"Fail to open PortAudio stream, error message is %s\n", Pa_GetErrorText(pa_open_ans));
		return false;
	}

	PaError pa_stream_start_ans = Pa_StartStream(pa_stream_);
	if (pa_stream_start_ans != paNoError) {
		wprintf(L"Fail to start PortAudio stream, error message is %s\n", Pa_GetErrorText(pa_stream_start_ans));
		return false;
	}

	// Initializes PortAudio ring buffer.
	ring_buffer_size_t rb_init_ans = PaUtil_InitializeRingBuffer(&pa_ringbuffer_, bits_per_sample_ / 8, ringbuffer_size, ringbuffer_);

	if (rb_init_ans == -1) {
		wprintf(L"Ring buffer size is not power of 2");
		return false;
	}

	return true;
}

int PortAudioCallback(const void* input,void* output,unsigned long frame_count,	const PaStreamCallbackTimeInfo* time_info,	PaStreamCallbackFlags status_flags,	void* user_data)
{
	PortAudioWrapper* pa_wrapper = reinterpret_cast<PortAudioWrapper*>(user_data);
	pa_wrapper->Callback(input, output, frame_count, time_info, status_flags);
	return paContinue;
}




/**************************************************************************/
/******                      Recorder fonction                       ******/
/**************************************************************************/


#define ENR_ATTENTE 0
#define ENR_ENCOURS 1
#define ENR_FINI 2
#define ENR_RATE 3

long amplitude;
int Enr_etat;
int silence;
long temposilence;
long tempobruit;

void Initchecker(void)
{
	Enr_etat = ENR_ATTENTE;
	amplitude = 0;
	temposilence = 0;
	tempobruit = 0;
}

int Checkamplitude(long value)
{
	//wprintf(L"Debug %d.\n", value);

	if (Enr_etat == ENR_RATE) return Enr_etat;

	if (value > Min_Amplitude)
	{
		if (Enr_etat == ENR_ATTENTE)
		{
			wprintf(L"Sound detected, Start recording. Value : %ld\n",value);
			_DisplaySpectro(-1);

			Enr_etat = ENR_ENCOURS;

		}
		temposilence = 0;
	}
	else
	{
		temposilence += 1;

		if (((temposilence > MAXSILENCE) && (Enr_etat == ENR_ATTENTE)) || ((temposilence > MINSILENCE) && (Enr_etat == ENR_ENCOURS)))
		{
			if (Enr_etat != ENR_FINI)
			{
				wprintf(L"Too much silence, stop recording.\n");
				if (Enr_etat == ENR_ENCOURS) Enr_etat = ENR_FINI;
				else Enr_etat = ENR_RATE;
			}
		}
	}

	return Enr_etat;
}

long compteur;
void SetSpectro(long val)
{
	unsigned short int spect;

	compteur++;

	if (Enr_etat == ENR_ATTENTE) return;

	if (compteur % 30 == 0)
	{
		spect = (unsigned short int)(val * MAGICNUMBER / 65537);
		spect = (int)(spect * 100 / 65537);
		//wprintf(L"Spectro %ld %d\n", val, spect);
		_DisplaySpectro(spect);
	}
}

/* This routine will be called by the PortAudio engine when audio is needed.
** It may be called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
static int recordCallback(const void *inputBuffer, void *outputBuffer,	unsigned long framesPerBuffer,	const PaStreamCallbackTimeInfo* timeInfo,	PaStreamCallbackFlags statusFlags,	void *userData)
{
	PAData *data = (PAData*)userData;
	const SAMPLE *rptr = (const SAMPLE*)inputBuffer;
	SAMPLE *wptr = &data->recordedSamples[data->frameIndex * NUM_CHANNELS];
	long framesToCalc;
	long i;
	int finished;

	long totalamplitude = 0;
	long v;
	int detection;

	unsigned long framesLeft = data->maxFrameIndex - data->frameIndex;

	(void)outputBuffer; /* Prevent unused variable warnings. */
	(void)timeInfo;
	(void)statusFlags;
	(void)userData;


	if (framesLeft < framesPerBuffer)
	{
		wprintf(L"Recorded sound too long, stop recording\n");
		framesToCalc = framesLeft;
		finished = paComplete;
	}
	else
	{
		framesToCalc = framesPerBuffer;
		finished = paContinue;
	}

	//check amplitude
	if (inputBuffer == NULL)
	{
		totalamplitude = 0;
	}
	else
	{
		for (i = 0; i < framesToCalc * NUM_CHANNELS; i += NUM_CHANNELS)
		{
			v = (long)rptr[i];
			if (v < 0) v = -v;
			totalamplitude += v;
		}
	}

	totalamplitude = totalamplitude / NUM_CHANNELS;

	detection = Checkamplitude(totalamplitude / framesToCalc);
	//stop if too much silence
	if ((detection == ENR_FINI) || (detection == ENR_RATE)) finished = paComplete;


	//Display spectrographe
	SetSpectro(totalamplitude);

	//buffer copy
	//all the time or only if there is sound ??? TODO : need to test
	if (1 == 1) //(Enr_etat == ENR_ENCOURS)
	{
		if (inputBuffer == NULL)
		{
			for (i = 0; i < framesToCalc; i++)
			{
				*wptr++ = SAMPLE_SILENCE;  /* left */
				if (NUM_CHANNELS == 2) *wptr++ = SAMPLE_SILENCE;  /* right */

			}
		}
		else
		{
			for (i = 0; i < framesToCalc; i++)
			{
				*wptr++ = (*rptr++) * Config_Gain;  /* left */
				if (NUM_CHANNELS == 2) *wptr++ = *rptr++;  /* right */
			}

		}

		data->frameIndex += framesToCalc;
	}

	return finished;
}

/**************************************/

void AudioRecordConfig(int g,int ma)
{
	if ((g > 0) && (g < 10))
	{
		Config_Gain = g;
	}
	if ((ma > 0) && (ma < 20000))
	{
		Min_Amplitude = ma;
	}

}


cRecord::cRecord()
{
	stream = NULL;
	data.recordedSamples = NULL;
	hdr = NULL;

	Defaut_sample_rate = SAMPLE_RATE;

	hdr = (WaveHeader *)malloc(sizeof(*hdr));

	wprintf(L"\033[0;31mInitialise sound recorder\033[0;37m\n");

	InitPortAudio();

	//making header
	hdr = genericWAVHeader(hdr, Defaut_sample_rate, 8 * sizeof(SAMPLE), NUM_CHANNELS);
	if (!hdr)
	{
		wprintf(L"Error allocating WAV header.\n");
	}

	//initialising var

}

cRecord::~cRecord()
{
	Pa_Terminate();
	if (data.recordedSamples)       /* Sure it is NULL or valid. */
		free(data.recordedSamples);

	if (hdr) free(hdr);
}

bool cRecord::Start()
{
	PaStreamParameters inputParameters;
	PaError err = paNoError;

	inputParameters.device = Pa_GetDefaultInputDevice(); /* default input device */
	if (inputParameters.device == paNoDevice) {
		wprintf(L"Error: No default input device.\n");
		return 1;
	}
	inputParameters.channelCount = NUM_CHANNELS;
	inputParameters.sampleFormat = PA_SAMPLE_TYPE;
	inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
	inputParameters.hostApiSpecificStreamInfo = NULL;

	/* Record some audio. -------------------------------------------- */
	err = Pa_OpenStream(&stream, &inputParameters, NULL, hdr->sample_rate, paFramesPerBufferUnspecified, paClipOff, recordCallback, &data);
	if (err)
	{
		wprintf(L"Can't open record stream\n");
		return false;
	}

	err = Pa_StartStream(stream);
	if (err)
	{
		wprintf(L"Can't start record stream\n");
		return false;
	}


	return true;

}

void cRecord::Stop()
{
	if (stream)
	{
		Pa_CloseStream(stream);
	}
}

void cRecord::SetSampleRate(int v)
{
	Defaut_sample_rate = v;

	//Remake header
	if (hdr) free(hdr);
	hdr = (WaveHeader *)malloc(sizeof(*hdr));
	hdr = genericWAVHeader(hdr, Defaut_sample_rate, 8 * sizeof(SAMPLE), NUM_CHANNELS);
	if (!hdr)
	{
		wprintf(L"Error allocating WAV header.\n");
	}

	wprintf(L"Setting Sample rate : %d\n", v);

}


char * cRecord::RecordSound(uint32_t duration,size_t *sizeflac)
{

	PaError err = paNoError;

	int MaxFrames;
	int numSamples;
	int numBytes;
	int i;

	Initchecker();

	//ok start to recording
	data.maxFrameIndex = MaxFrames = duration * hdr->sample_rate; /* Record for a few seconds. */
	data.frameIndex = 0;
	numSamples = MaxFrames * NUM_CHANNELS;
	numBytes = numSamples * sizeof(SAMPLE);
	data.recordedSamples = (SAMPLE *) malloc( numBytes ); /* From now on, recordedSamples is initialised. */
	if(!data.recordedSamples)
	{
		wprintf(L"Could not allocate record array.\n");
		return NULL;
	}

	for(i = 0; i < numSamples; i++) data.recordedSamples[i] = 0;

	if (!(Start())) return NULL;


	wprintf(L"Now recording!! Please speak into the microphone.\n");

	_DisplayIcone(MICRO);

	while((err = Pa_IsStreamActive(stream)) == 1)
	{
		Pa_Sleep(1000);
		//wprintf(L"index = %d\n", data.frameIndex);
	}
	if (err < 0) return NULL;

	Stop();

	wprintf(L"Nbre de frames = %d\n",data.frameIndex);
	if (data.frameIndex < (hdr->sample_rate* MINFRAME))
	{
		wprintf(L"Fichier trop petit, nbre frame = %d < %d\n",data.frameIndex,MINFRAME);
		return NULL;
	}

	if (Enr_etat == ENR_RATE)
	{
		wprintf(L"Recording cancelled\n");
		return NULL;
	}


#if 0
	/* Measure maximum peak amplitude. */
	max = 0;
	average = 0.0;
	for(i = 0; i < numSamples; i++)
	{
		val = data.recordedSamples[i];
		if( val < 0 ) val = -val; /* ABS */
		if( val > max )
		{
			max = val;
		}
		average += val;
	}

	average /= (double)numSamples;
#endif

//Raw output
#if 0
	{
		FILE  *fid;
		fid = fopen("recorded.raw", "wb");
		if( fid == NULL )
		{
			printf("Could not open file.");
			err = 1;
		}
		else
		{
			fwrite( data.recordedSamples, NUM_CHANNELS * sizeof(SAMPLE), data.frameIndex, fid );
			fclose( fid );
			printf("Wrote data to 'recorded.raw'\n");
		}
		return NULL;
	}
#endif
//File output
#if 0
	{
		FILE* fid = fopen("output.wav", "wb");
		if(!fid)
		{
			printf("Could not open file.");
			err = 1;
		}
		else
		{
			hdr->data_size = data.frameIndex * (NUM_CHANNELS * sizeof(SAMPLE));
			writeWAVHeader(fid, hdr);
			fwrite(data.recordedSamples, NUM_CHANNELS * sizeof(SAMPLE), data.frameIndex, fid);
			fclose(fid);
		}

		return NULL;
	}
#endif
//Buffer output
#if 1
	{
		char *WavBuffer;
		int size;

		hdr->data_size = data.frameIndex * (NUM_CHANNELS * sizeof(SAMPLE));
		size = hdr->data_size + 44;

		//Save wav in buffer
		WavBuffer = (char*) malloc (size * sizeof(char));
		writeWAVHeaderBuffer(WavBuffer,hdr);
		memcpy(WavBuffer+44,data.recordedSamples,NUM_CHANNELS * sizeof(SAMPLE) * data.frameIndex);

		//return size
		*sizeflac = NUM_CHANNELS * sizeof(SAMPLE) * data.frameIndex + 44;

		//Return buffer
		return WavBuffer;
	}
#endif

	return NULL;
}

/**************************************************************************/
/******                    Wav player fonction                       ******/
/**************************************************************************/

int bytesPerSample, bitsPerSample;
FILE* wavfile;
int numChannels;

#define CHECK(x) { if(!(x)) { wprintf(L"%s:%i: failure at: %s\n", __FILE__, __LINE__, #x); return 0; } }


std::string freadStr(FILE* f, size_t len) {
	std::string s(len, '\0');
	CHECK(fread(&s[0], 1, len, f) == len);
	return s;
}

template<typename T>
T freadNum(FILE* f) {
	T value;
	CHECK(fread(&value, sizeof(value), 1, f) == 1);
	return value; // no endian-swap for now... WAV is LE anyway...
}

cPlay::cPlay()
{
	stream = NULL;
	ready = false;

	wprintf(L"\033[0;31mInitialise sound output\033[0;37m\n");

	InitPortAudio();

	ready = true;

}

cPlay::~cPlay()
{
	if (stream)	Pa_CloseStream(stream);
	Pa_Terminate();
}

int cPlay::PlayWav(char * file)
{

	if (!ready) return false;

	wavfile = fopen(file, "r");
	CHECK(wavfile != NULL);

	CHECK(freadStr(wavfile, 4) == "RIFF");
	uint32_t wavechunksize = freadNum<uint32_t>(wavfile);
	CHECK(freadStr(wavfile, 4) == "WAVE");
	while (true) {
		std::string chunkName = freadStr(wavfile, 4);
		uint32_t chunkLen = freadNum<uint32_t>(wavfile);
		if (chunkName == "fmt ")
			readFmtChunk(chunkLen);
		else if (chunkName == "data") {
			CHECK(sampleRate != 0);
			CHECK(numChannels > 0);
			CHECK(bytesPerSample > 0);
			//wprintf(L"len: %.0f secs\n", double(chunkLen) / sampleRate / numChannels / bytesPerSample);
			break; // start playing now
		}
		else {
			// skip chunk
			CHECK(fseek(wavfile, chunkLen, SEEK_CUR) == 0);
		}
	}

	if (!portAudioOpen())
	{
		wprintf(L"Can't initialise sound output\n");
	}

	wprintf(L"Playing wav : %s\n",file);

	// wait until stream has finished playing
	while (Pa_IsStreamActive(stream) > 0)
	{
		//usleep(1000);
		Pa_Sleep(100);
	}

	if (stream)
	{
		Pa_StopStream(stream);
		Pa_CloseStream(stream);
	}

	fclose(wavfile);

	return true;
}

int cPlay_paStreamCallback(const void *input, void *output,unsigned long frameCount,const PaStreamCallbackTimeInfo* timeInfo,PaStreamCallbackFlags statusFlags,void *userData)
{
	size_t numRead = fread(output, bytesPerSample * numChannels, frameCount, wavfile);
	output = (uint8_t*)output + numRead * numChannels * bytesPerSample;
	frameCount -= numRead;

	if (frameCount > 0) {
		memset(output, 0, frameCount * numChannels * bytesPerSample);
		return paComplete;
	}

	return paContinue;
}

bool cPlay::portAudioOpen() {
	//CHECK(Pa_Initialize() == paNoError);

	PaStreamParameters outputParameters;

	outputParameters.device = Pa_GetDefaultOutputDevice();
	CHECK(outputParameters.device != paNoDevice);

	outputParameters.channelCount = numChannels;
	outputParameters.sampleFormat = sampleFormat;
	outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;//->defaultHighOutputLatency;
	outputParameters.hostApiSpecificStreamInfo = NULL;

	PaError ret = Pa_OpenStream(
		&stream,
		NULL, // no input
		&outputParameters,
		sampleRate,
		paFramesPerBufferUnspecified, // framesPerBuffer
		0, // flags
		&cPlay_paStreamCallback,
		NULL //void *userData
		);

	if (ret != paNoError) {
		wprintf(L"(SO) Pa_OpenStream failed: (err %i) %s\n", ret, Pa_GetErrorText(ret));
		if (stream)
			Pa_CloseStream(stream);
		return false;
	}

	CHECK(Pa_StartStream(stream) == paNoError);
	return true;
}

int cPlay::readFmtChunk(uint32_t chunkLen) {
	CHECK(chunkLen >= 16);
	uint16_t fmttag = freadNum<uint16_t>(wavfile); // 1: PCM (int). 3: IEEE float
	CHECK(fmttag == 1 || fmttag == 3);
	numChannels = freadNum<uint16_t>(wavfile);
	CHECK(numChannels > 0);
	//wprintf(L"%i channels\n", numChannels);
	sampleRate = freadNum<uint32_t>(wavfile);
	//wprintf(L"%i Hz\n", sampleRate);
	uint32_t byteRate = freadNum<uint32_t>(wavfile);
	uint16_t blockAlign = freadNum<uint16_t>(wavfile);
	bitsPerSample = freadNum<uint16_t>(wavfile);
	bytesPerSample = bitsPerSample / 8;
	CHECK(byteRate == sampleRate * numChannels * bytesPerSample);
	CHECK(blockAlign == numChannels * bytesPerSample);
	if (fmttag == 1 /*PCM*/) {
		switch (bitsPerSample) {
		case 8: sampleFormat = paInt8; break;
		case 16: sampleFormat = paInt16; break;
		case 32: sampleFormat = paInt32; break;
		default: CHECK(false);
		}
		//wprintf(L"PCM %ibit int\n", bitsPerSample);
	}
	else {
		CHECK(fmttag == 3 /* IEEE float */);
		CHECK(bitsPerSample == 32);
		sampleFormat = paFloat32;
		//wprintf(L"32bit float\n");
	}
	if (chunkLen > 16) {
		uint16_t extendedSize = freadNum<uint16_t>(wavfile);
		CHECK(chunkLen == 18 + extendedSize);
		fseek(wavfile, extendedSize, SEEK_CUR);
	}

	return true;
}

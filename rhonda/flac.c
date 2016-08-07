/*
*
*       This is a modified version (2016)
*       ---------------------------------
*
*
*/
/* example_c_encode_file - Simple FLAC file encoder using libFLAC
* Copyright (C) 2007,2008,2009  Josh Coalson
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

/*
* This example shows how to use libFLAC to encode a WAVE file to a FLAC
* file.  It only supports 16-bit stereo files in canonical WAVE format.
*
* Complete API documentation can be found at:
*   http://flac.sourceforge.net/api/
*/

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "flac.h"
#include <wchar.h>

//#include "FLAC/metadata.h"
#include "FLAC/stream_encoder.h"

static void progress_callback(const FLAC__StreamEncoder *encoder, FLAC__uint64 bytes_written, FLAC__uint64 samples_written, unsigned frames_written, unsigned total_frames_estimate, void *client_data);

#define READSIZE 1024

static unsigned total_samples = 0; /* can use a 32-bit number due to WAVE size limitations */
static FLAC__byte buffer[READSIZE/*samples*/ * 2/*bytes_per_sample*/ * 1/*channels*/]; /* we read the WAVE data into here */
static FLAC__int32 pcm[READSIZE/*samples*/ * 1/*channels*/];


/******************************************************************/

typedef struct sprec_encoder_state
{
	char *buf;
	size_t length;
} sprec_encoder_state;

static FLAC__StreamEncoderWriteStatus flac_write_callback(const FLAC__StreamEncoder *encoder, const FLAC__byte buffer[],	size_t bytes, unsigned samples, unsigned current_frame,	void *client_data )
{
	sprec_encoder_state *flac_data = client_data;

	flac_data->buf = realloc(flac_data->buf, flac_data->length + bytes);
	memcpy(flac_data->buf + flac_data->length, buffer, bytes);
	flac_data->length += bytes;

	return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
}



char * ConvertWavBufferToFlacBuffer(char *buff_wave, size_t size_wave,size_t *size)
{

		FLAC__bool ok = true;
		FLAC__StreamEncoder *encoder = 0;
		FLAC__StreamEncoderInitStatus init_status;

		unsigned sample_rate = 0;
		unsigned channels = 0;
		unsigned bps = 0;

		sprec_encoder_state flac_data;
		flac_data.buf = NULL;
		flac_data.length = 0;


		if (size_wave < 44)
		{
			wprintf(L"ERROR: Buffer problem\n");
		}
		memcpy(buffer, buff_wave, 44);
		size_wave -= 44;
		buff_wave += 44;

		sample_rate = ((((((unsigned)buffer[27] << 8) | buffer[26]) << 8) | buffer[25]) << 8) | buffer[24];
		channels = 1;
		bps = 16;

		total_samples = (((((((unsigned)buffer[43] << 8) | buffer[42]) << 8) | buffer[41]) << 8) | buffer[40]) / 2;// par 2 au lieu de 4
		/* allocate the encoder */
		if ((encoder = FLAC__stream_encoder_new()) == NULL) {
			wprintf(L"ERROR: allocating encoder\n");
			return 0;
		}



		//fprintf(stderr, "Total samples %d \n",total_samples);
		ok &= FLAC__stream_encoder_set_verify(encoder, true);
		ok &= FLAC__stream_encoder_set_compression_level(encoder, 5);
		ok &= FLAC__stream_encoder_set_channels(encoder, channels);
		ok &= FLAC__stream_encoder_set_bits_per_sample(encoder, bps);
		ok &= FLAC__stream_encoder_set_sample_rate(encoder, sample_rate);
		ok &= FLAC__stream_encoder_set_total_samples_estimate(encoder, total_samples);


		ok = FLAC__stream_encoder_init_stream(
			encoder,
			flac_write_callback,
			NULL, // seek() stream
			NULL, // tell() stream
			NULL, // metadata writer
			&flac_data
			);

		if (ok != FLAC__STREAM_ENCODER_INIT_STATUS_OK) {
			fprintf(stderr, "ERROR: initializing encoder: %s\n", FLAC__StreamEncoderInitStatusString[init_status]);
			ok = false;

			free(flac_data.buf);
			FLAC__stream_encoder_delete(encoder);
			return NULL;
		}
		ok = true;

		/* read blocks of samples from WAVE file and feed to encoder */
		if (ok) {
			size_t left = (size_t)total_samples;
			while (ok && left) {
				size_t need = (left>READSIZE ? (size_t)READSIZE : (size_t)left);
				if (size_wave < (unsigned long)(channels*(bps / 8) * need)) {
					fprintf(stderr, "ERROR: reading from WAVE file\n");
					ok = false;
				}
				else {
					/* convert the packed little-endian 16-bit PCM samples from WAVE into an interleaved FLAC__int32 buffer for libFLAC */
					size_t i;

					memcpy(buffer, buff_wave, channels*(bps / 8) * need);
					size_wave -= channels*(bps / 8) * need;
					buff_wave += channels*(bps / 8) * need;


					for (i = 0; i < need*channels; i++) {
						/* inefficient but simple and works on big- or little-endian machines */
						pcm[i] = (FLAC__int32)(((FLAC__int16)(FLAC__int8)buffer[2 * i + 1] << 8) | (FLAC__int16)buffer[2 * i]);
					}
					/* feed samples to encoder */
					ok = FLAC__stream_encoder_process_interleaved(encoder, pcm, need);
				}
				left -= need;
			}
		}

		ok &= FLAC__stream_encoder_finish(encoder);

		if (!ok)
		{
			wprintf(L"encoding: %s\n", ok ? L"succeeded" : L"FAILED");
			wprintf(L"state: %s\n", FLAC__StreamEncoderStateString[FLAC__stream_encoder_get_state(encoder)]);
		}
		FLAC__stream_encoder_delete(encoder);


#if 0
		//To make file
		{
			FILE *fout;
			fout = fopen("test2.flac", "wb");
			fwrite(flac_data.buf, 1, flac_data.length, fout);
			fclose(fout);
		}
#endif


		*size = flac_data.length;

		return flac_data.buf;

}

int ConvertWaveToFlacBuffer(char * sour, long size, const char * dest)
{
	FLAC__bool ok = true;
	FLAC__StreamEncoder *encoder = 0;
	FLAC__StreamEncoderInitStatus init_status;

	unsigned sample_rate = 0;
	unsigned channels = 0;
	unsigned bps = 0;

	/* read wav header and validate it */
	/*
	if(
	fread(buffer, 1, 44, fin) != 44 ||
	memcmp(buffer, "RIFF", 4) ||
	memcmp(buffer+8, "WAVEfmt \020\000\000\000\001\000\002\000", 16) ||
	memcmp(buffer+32, "\004\000\020\000data", 8)
	) {
	fprintf(stderr, "ERROR: invalid/unsupported WAVE file, only 16bps stereo WAVE in canonical form allowed\n");
	fclose(fin);
	return 1;
	}
	*/
	if (size < 44)
	{
		wprintf(L"ERROR: Buffer problem\n");
	}
	memcpy(buffer,sour,44);
	size -= 44;
	sour += 44;

	sample_rate = ((((((unsigned)buffer[27] << 8) | buffer[26]) << 8) | buffer[25]) << 8) | buffer[24];
	channels = 1;
	bps = 16;

	total_samples = (((((((unsigned)buffer[43] << 8) | buffer[42]) << 8) | buffer[41]) << 8) | buffer[40]) / 2;// par 2 au lieu de 4
	/* allocate the encoder */
	if((encoder = FLAC__stream_encoder_new()) == NULL) {
		wprintf(L"ERROR: allocating encoder\n");
		return 0;
	}

	//fprintf(stderr, "Total samples %d \n",total_samples);

	ok &= FLAC__stream_encoder_set_verify(encoder, true);
	ok &= FLAC__stream_encoder_set_compression_level(encoder, 5);
	ok &= FLAC__stream_encoder_set_channels(encoder, channels);
	ok &= FLAC__stream_encoder_set_bits_per_sample(encoder, bps);
	ok &= FLAC__stream_encoder_set_sample_rate(encoder, sample_rate);
	ok &= FLAC__stream_encoder_set_total_samples_estimate(encoder, total_samples);


	/* initialize encoder */
	if(ok) {
		init_status = FLAC__stream_encoder_init_file(encoder, dest, progress_callback, /*client_data=*/NULL);
		if(init_status != FLAC__STREAM_ENCODER_INIT_STATUS_OK) {
			wprintf(L"ERROR: initializing encoder: %s\n", FLAC__StreamEncoderInitStatusString[init_status]);
			ok = false;
		}
	}

	/* read blocks of samples from WAVE file and feed to encoder */memcpy(buffer,sour,44);
	if(ok) {
		size_t left = (size_t)total_samples;
		while(ok && left) {
			size_t need = (left>READSIZE? (size_t)READSIZE : (size_t)left);
			//if(fread(buffer, channels*(bps/8), need, fin) != need) {
			if (size < (long)(channels*(bps/8) * need )) {
				wprintf(L"ERROR: reading from buffer\n");
				ok = false;
			}
			else {
				size_t i;

				memcpy(buffer,sour, channels*(bps/8) * need);
				size -= channels*(bps/8) * need;
				sour += channels*(bps/8) * need;

				/* convert the packed little-endian 16-bit PCM samples from WAVE into an interleaved FLAC__int32 buffer for libFLAC */
				for(i = 0; i < need*channels; i++) {
					/* inefficient but simple and works on big- or little-endian machines */
					pcm[i] = (FLAC__int32)(((FLAC__int16)(FLAC__int8)buffer[2*i+1] << 8) | (FLAC__int16)buffer[2*i]);
				}
				/* feed samples to encoder */
				ok = FLAC__stream_encoder_process_interleaved(encoder, pcm, need);
			}
			left -= need;
		}
	}

	ok &= FLAC__stream_encoder_finish(encoder);

	if (!ok)
	{
		wprintf(L"encoding: %s\n", ok ? L"succeeded" : L"FAILED");
		wprintf(L"state: %s\n", FLAC__StreamEncoderStateString[FLAC__stream_encoder_get_state(encoder)]);
	}

	FLAC__stream_encoder_delete(encoder);

	return 1;
}

#if 0
int ConvertWaveFileToFlacFile(char * sour, char * dest)
{
	FLAC__bool ok = true;
	FLAC__StreamEncoder *encoder = 0;
	FLAC__StreamEncoderInitStatus init_status;
	FILE *fin;
	unsigned sample_rate = 0;
	unsigned channels = 0;
	unsigned bps = 0;
	if ((fin = fopen(sour, "rb")) == NULL) {
		fprintf(stderr, "ERROR: opening %s for input\n", sour);
		return 0;
	}
	/* read wav header and validate it */
	/*
	if(
	fread(buffer, 1, 44, fin) != 44 ||
	memcmp(buffer, "RIFF", 4) ||
	memcmp(buffer+8, "WAVEfmt \020\000\000\000\001\000\002\000", 16) ||
	memcmp(buffer+32, "\004\000\020\000data", 8)
	) {
	fprintf(stderr, "ERROR: invalid/unsupported WAVE file, only 16bps stereo WAVE in canonical form allowed\n");
	fclose(fin);
	return 1;
	}
	*/
	if (fread(buffer, 1, 44, fin) != 44)
	{
		fprintf(stderr, "ERROR: opening %s for read\n", sour);
		return 0;
	}
	sample_rate = ((((((unsigned)buffer[27] << 8) | buffer[26]) << 8) | buffer[25]) << 8) | buffer[24];
	channels = 1;
	bps = 16;
	total_samples = (((((((unsigned)buffer[43] << 8) | buffer[42]) << 8) | buffer[41]) << 8) | buffer[40]) / 2;// par 2 au lieu de 4
	/* allocate the encoder */
	if ((encoder = FLAC__stream_encoder_new()) == NULL) {
		fprintf(stderr, "ERROR: allocating encoder\n");
		fclose(fin);
		return 0;
	}
	//fprintf(stderr, "Total samples %d \n",total_samples);
	ok &= FLAC__stream_encoder_set_verify(encoder, true);
	ok &= FLAC__stream_encoder_set_compression_level(encoder, 5);
	ok &= FLAC__stream_encoder_set_channels(encoder, channels);
	ok &= FLAC__stream_encoder_set_bits_per_sample(encoder, bps);
	ok &= FLAC__stream_encoder_set_sample_rate(encoder, sample_rate);
	ok &= FLAC__stream_encoder_set_total_samples_estimate(encoder, total_samples);
	/* initialize encoder */
	if (ok) {
		init_status = FLAC__stream_encoder_init_file(encoder, dest, progress_callback, /*client_data=*/NULL);
		if (init_status != FLAC__STREAM_ENCODER_INIT_STATUS_OK) {
			fprintf(stderr, "ERROR: initializing encoder: %s\n", FLAC__StreamEncoderInitStatusString[init_status]);
			ok = false;
		}
	}
	/* read blocks of samples from WAVE file and feed to encoder */
	if (ok) {
		size_t left = (size_t)total_samples;
		while (ok && left) {
			size_t need = (left>READSIZE ? (size_t)READSIZE : (size_t)left);
			if (fread(buffer, channels*(bps / 8), need, fin) != need) {
				fprintf(stderr, "ERROR: reading from WAVE file\n");
				ok = false;
			}
			else {
				/* convert the packed little-endian 16-bit PCM samples from WAVE into an interleaved FLAC__int32 buffer for libFLAC */
				size_t i;
				for (i = 0; i < need*channels; i++) {
					/* inefficient but simple and works on big- or little-endian machines */
					pcm[i] = (FLAC__int32)(((FLAC__int16)(FLAC__int8)buffer[2 * i + 1] << 8) | (FLAC__int16)buffer[2 * i]);
				}
				/* feed samples to encoder */
				ok = FLAC__stream_encoder_process_interleaved(encoder, pcm, need);
			}
			left -= need;
		}
	}
	ok &= FLAC__stream_encoder_finish(encoder);
	fprintf(stderr, "encoding: %s\n", ok ? "succeeded" : "FAILED");
	fprintf(stderr, "state: %s\n", FLAC__StreamEncoderStateString[FLAC__stream_encoder_get_state(encoder)]);
	FLAC__stream_encoder_delete(encoder);
	fclose(fin);
	return 1;
}
#endif


void progress_callback(const FLAC__StreamEncoder *encoder, FLAC__uint64 bytes_written, FLAC__uint64 samples_written, unsigned frames_written, unsigned total_frames_estimate, void *client_data)
{
	(void)encoder, (void)client_data;

#ifdef _MSC_VER
	//fprintf(stderr, "wrote %I64u bytes, %I64u/%u samples, %u/%u frames\n", bytes_written, samples_written, total_samples, frames_written, total_frames_estimate);
#else
	//fprintf(stderr, "wrote %llu bytes, %llu/%u samples, %u/%u frames\n", bytes_written, samples_written, total_samples, frames_written, total_frames_estimate);
#endif
}

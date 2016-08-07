#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

//int ConvertWaveFileToFlacFile(char * sour, char * dest);
int ConvertWaveToFlacBuffer(char * sour, long size, const char * dest);
char *ConvertWavBufferToFlacBuffer(char *buff_wave, size_t size_wave, size_t *size);

#ifdef __cplusplus
}
#endif
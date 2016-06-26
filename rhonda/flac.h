#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

int ConvertFlac(char * sour, char * dest);
int ConvertFlacBuffer(char * sour, long size,const char * dest);

#ifdef __cplusplus
}
#endif
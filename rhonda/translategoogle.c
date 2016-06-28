
// Aralox - http://stackoverflow.com/questions/25307487/how-to-use-libcurl-with-google-speech-api-what-is-the-equivalent-for-data-bin/25310710#25310710

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include <curl/curl.h> 
//#include <direct.h>

#include "translategoogle.h"

#include "libs/slre.h"


char GoogleApiKey[40];

void SetGoogleApiKey(char *s)
{
	int l = strlen(s);
	if (l > 39) l = 39;
	strncpy(GoogleApiKey, s, l);
}


struct WriteThis {
	const char *readptr;
	long sizeleft;
	long totalsize;
};

struct MemoryStruct {
	char *memory;
	size_t size;
};


static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct *)userp;

	mem->memory = (char *)realloc(mem->memory, mem->size + realsize + 1);
	if(mem->memory == NULL) {
		/* out of memory! */ 
		printf("not enough memory (realloc returned NULL)\n");
		return 0;
	}

	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}


int lastpercent = -1;
static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *userp)
{

	struct WriteThis *pooh = (struct WriteThis *)userp;

	int percent = 100*(pooh->totalsize - pooh->sizeleft)/pooh->totalsize;
	if ((percent != lastpercent) && (percent % 5 == 0))
	{
		printf("Avancement %d\n",percent);
		lastpercent = percent;
	}

	if (size*nmemb < 1)	return 0;

	if (pooh->sizeleft)
    { 
		long written = (long)(size * nmemb);
		if (pooh->sizeleft < written) written = pooh->sizeleft;
        memcpy(ptr, ((char*)(pooh->readptr)), written);
		pooh->readptr += written;
        pooh->sizeleft -= written;
        return written; 
    } 

/*
	if (pooh->sizeleft) {
		*(char *)ptr = pooh->readptr[0]; // copy one single byte
		pooh->readptr++;                 // advance pointer
		pooh->sizeleft--;                // less data left
		return 1;						 // we return 1 byte at a time!

	}
*/
	return 0;                          /* no more data left to deliver */
}


int TranslateGoggle(char *filename,char *resultat)
{
	CURL *curl;			// curl handle
	CURLcode res;

	int bestscore = 0;

	struct MemoryStruct data;
	data.memory = (char *)malloc(1);  /* will be grown as needed by the realloc above */ 
	data.size = 0;    /* no data at this point */

	resultat[0] = '\0';

	curl = curl_easy_init();
	if (curl) 
	{
		FILE *file;
		int fileSize = 0;
		struct curl_slist *chunk = NULL;

		char *audioData;
		struct WriteThis pooh;

		char sizeHeader[255];

		char apiurl[255];
		strcpy(apiurl, "https://www.google.com/speech-api/v2/recognize?output=json&lang=FR-fr&key=");
		strcat(apiurl, GoogleApiKey);

		file = fopen(filename, "r");
		fseek(file, 0, SEEK_END);
		fileSize = ftell(file);
		fseek(file, 0, SEEK_SET);

		if (fileSize == 0) return 0;

		wprintf(L"File size %d\n",fileSize);
		audioData = (char*)malloc(fileSize);

		//chunk = curl_slist_append(chunk, "Content-Type: audio/l16; rate=44100");
		chunk = curl_slist_append(chunk, "Content-Type: audio/x-flac; rate=44100");

		fread(audioData, fileSize, 1, file);
		fclose(file);

		pooh.readptr = audioData;
		pooh.sizeleft = fileSize;
		pooh.totalsize = fileSize;

		sprintf(sizeHeader,"Content-Length: %d",fileSize);
		chunk = curl_slist_append(chunk, sizeHeader);

		//disalbe Expect: 100-continue
		chunk = curl_slist_append(chunk, "Expect:");

		curl_easy_setopt(curl, CURLOPT_POST, 1L);
		curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
		curl_easy_setopt(curl, CURLOPT_READDATA, &pooh);
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&data);

		curl_easy_setopt(curl, CURLOPT_CAINFO, 0L);
		curl_easy_setopt(curl, CURLOPT_CAPATH, 0L);
		//curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

		curl_easy_setopt(curl, CURLOPT_URL, apiurl);

		res = curl_easy_perform(curl);

		wprintf(L"***********************************\n");
		wprintf(L"Resultat From Google\n %s \n",data.memory);
		wprintf(L"***********************************\n");

		{
			//parsing with regex
			struct slre_cap caps[2];

			//Have a result with confidence ?
			if (slre_match("{\"transcript\":\"([^\"]+)\",\"confidence\":([0-9\.]+)}", data.memory, data.size, caps, 2, 0) > 0)
			{
				char *tmp2;

				strncpy(resultat, caps[0].ptr, caps[0].len);
				strncpy(resultat + caps[0].len, "\0", 1);

				tmp2 = (char *)malloc((caps[1].len + 1) * sizeof(char));
				strncpy(tmp2, caps[1].ptr, caps[1].len);
				strncpy(tmp2 + caps[1].len, "\0", 1);

				bestscore = 1 + (100 * atof(tmp2));
				
			}
			else
			{
				// ok nevermind, add all the other results
				while (slre_match("{\"transcript\":\"([^\"]+)\"}", data.memory, data.size, caps, 1, 0) > 0)
				{
					char *tmp1;
					int l;

					tmp1 = (char *)malloc((caps[0].len + 1) * sizeof(char));
					strncpy(tmp1, caps[0].ptr, caps[0].len);
					strncpy(tmp1 + caps[0].len, "\0", 1);

					//TODO : Need to check word and not complete string
					if (!strstr(resultat, tmp1))
					{
						int l = strlen(resultat) + strlen(tmp1) + 1;
						if (l < 254)
						{
							strcat(resultat," ");
							strcat(resultat, tmp1);
						}
					}

					l = caps[0].ptr + caps[0].len - data.memory;
					data.memory += l;
					data.size -= l;

					bestscore = 50;
				}
			}
		}

		curl_easy_cleanup(curl);

	}

	return bestscore;
}

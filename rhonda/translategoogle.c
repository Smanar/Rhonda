
// Aralox - http://stackoverflow.com/questions/25307487/how-to-use-libcurl-with-google-speech-api-what-is-the-equivalent-for-data-bin/25310710#25310710

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include <curl/curl.h> 
//#include <direct.h>

#include "translategoogle.h"
#include "libs/jsmn.h"


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

static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
	if (tok->type == JSMN_STRING && (int) strlen(s) == tok->end - tok->start &&
		strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
			return 0;
	}
	return -1;
}

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
		wprintf(L"Resultat \n %s \n",data.memory);
		wprintf(L"***********************************\n");

		{
			//JSON parser
			jsmn_parser p;
			int r;
			int i;

			jsmntok_t t[128]; /* We expect no more than 128 tokens */
			jsmn_init(&p);


			r = jsmn_parse(&p, data.memory, data.size, t, sizeof(t)/sizeof(t[0]));
			if (r < 0)
			{
				wprintf(L"Failed to parse JSON: %d\n", r);
				return 0;
			}

			/* Assume the top-level element is an object */
			if (r < 1 || t[0].type != JSMN_OBJECT)
			{
				wprintf(L"Object expected\n");
				return 0;
			}

			/* Loop over all keys of the root object */
			for (i = 1; i < r; i++)
			{
				if (jsoneq(data.memory, &t[i], "result") == 0)
				{
					//printf("- Result: %.*s\n", t[i+1].end-t[i+1].start,data.memory + t[i+1].start);
					i++;
				}
				else if (jsoneq(data.memory, &t[i], "alternative") == 0)
				{
					int end;
					//list all alternatives
					//printf("- alternative: %.*s\n", t[i+1].end-t[i+1].start,data.memory + t[i+1].start);
					
					end = t[i+1].end;

					while (t[i].start < end)//to correct size bug
					{
						if (jsoneq(data.memory, &t[i], "transcript") == 0)
						{
							wprintf(L"- transcript: %.*s\n", t[i+1].end-t[i+1].start,data.memory + t[i+1].start);
							if ((bestscore == 0) && ( t[i+1].end-t[i+1].start < 255 ))
							{
								strncpy(resultat,data.memory + t[i+1].start,t[i+1].end-t[i+1].start);
								resultat[ t[i+1].end-t[i+1].start ] = '\0';
								bestscore = 50;
							}
						}
						if (jsoneq(data.memory, &t[i], "confidence") == 0)
						{
							char tmp[255];
							wprintf(L"- confidence: %.*s\n", t[i+1].end-t[i+1].start,data.memory + t[i+1].start);
							strncpy(tmp,data.memory + t[i+1].start, t[i+1].end-t[i+1].start);
							tmp[ t[i+1].end-t[i+1].start ] = '\0';
							bestscore = 1 + (int)(100 * atof(tmp));
						}

						i++;
					}
					i--;

				}
				else if (jsoneq(data.memory, &t[i], "final") == 0)
				{
					//comparaison ok ?
					wprintf(L"- final: %.*s\n", t[i+1].end-t[i+1].start,data.memory + t[i+1].start);
					if (bestscore == 0) bestscore = 50;
					i++;
				}

				else
				{
					//printf("Unexpected key: %.*s\n", t[i].end-t[i].start,data.memory + t[i].start);
				}
			}

		}

		curl_easy_cleanup(curl);

	}

	return bestscore;
}
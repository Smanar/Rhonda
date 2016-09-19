/*

Code to use Speech to text engine, this code can use Google API or Bing API

By defaut I use 8820 Hz for sample rate for Bing Engine to have smaller file sier but works too with other sample rate.









*/

// Aralox - http://stackoverflow.com/questions/25307487/how-to-use-libcurl-with-google-speech-api-what-is-the-equivalent-for-data-bin/25310710#25310710

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <string>


#ifdef WIN32
#include <Rpc.h>
#else
#include <uuid/uuid.h>
#endif

#include <curl/curl.h> 
//#include <direct.h>

#include "STTEngine.h"
#include "libs/slre.h"
#include "fonction.h"
#include "prog.h"


//#pragma comment(lib, "Rpcrt4.lib")

std::string Uid;
std::string ApiKey;


void SetSTTApiKey(char *s)
{
	ApiKey = s;
#ifdef WIN32
	{
		UUID id;
		RPC_CSTR szUuid = NULL;
		UuidCreate(&id);
		UuidToStringA(&id, &szUuid);
		Uid = (char*)szUuid;
		RpcStringFreeA(&szUuid);
	}
#else
	{
		uuid_t uuid;
		uuid_generate_random(uuid);
		char s[37];
		uuid_unparse(uuid, s);
		Uid = (char*)s;
	}
#endif
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

int TranslateGoggle(char *Buff_flac, size_t s, char *resultat)
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
		struct curl_slist *chunk = NULL;

		struct WriteThis pooh;

		char sizeHeader[255];

		std::string apiurl;
		apiurl = "https://www.google.com/speech-api/v2/recognize?output=json&lang=";

		if (GetLanguage() == 0) apiurl = apiurl + "FR-fr";
		else apiurl = apiurl + "EN-en";

		apiurl = apiurl + "&key=" + ApiKey;

		if (s == 0) return 0;

		wprintf(L"File size %d Kb\n",s/1000);

		//chunk = curl_slist_append(chunk, "Content-Type: audio/l16; rate=44100");
		chunk = curl_slist_append(chunk, "Content-Type: audio/x-flac; rate=8820");

		pooh.readptr = Buff_flac;
		pooh.sizeleft = s;
		pooh.totalsize = s;

		sprintf(sizeHeader,"Content-Length: %d",s);
		chunk = curl_slist_append(chunk, sizeHeader);

		//disalbe Expect: 100-continue
		chunk = curl_slist_append(chunk, "Expect:");

		curl_easy_setopt(curl, CURLOPT_POST, 1L);
		curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
		curl_easy_setopt(curl, CURLOPT_READDATA, &pooh);
		//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);// To debug

		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&data);

		curl_easy_setopt(curl, CURLOPT_CAINFO, 0L);
		curl_easy_setopt(curl, CURLOPT_CAPATH, 0L);
		//curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

		curl_easy_setopt(curl, CURLOPT_URL, apiurl.c_str());

		res = curl_easy_perform(curl);

		Mywprintf(L"Resultat From Google\n %s \n",data.memory);


		{
			//parsing with regex
			struct slre_cap caps[2];
			

			//Have a result with confidence ?
			if (slre_match("{\"transcript\":\"([^\"]+)\",\"confidence\":([0-9\.]+)}", data.memory, data.size, caps, 2, 0) > 0)
			{
				char *tmp2;
				int l;

				strncpy(resultat, caps[0].ptr, caps[0].len);
				resultat[caps[0].len] = '\0';

				l = caps[1].len;
				if (l > 4) l = 4;
				tmp2 = (char *)malloc((l + 1) * sizeof(char));
				strncpy(tmp2, caps[1].ptr + 2, l - 2);
				tmp2[l - 2] = '\0';

				bestscore = 1 + (atoi(tmp2));

				free(tmp2);
				
			}
			else
			{
				//There is no more thing to check for bing engine
				bestscore = 0;
			}
		}

		curl_easy_cleanup(curl);

	}

	free(data.memory);

	return bestscore;
}

//https://github.com/Microsoft/Cognitive-Documentation/blob/master/Content/en-us/Speech/API-Reference-REST/BingVoiceRecognition.md
//https://www.microsoft.com/cognitive-services/en-us/subscriptions
int TranslateBing(char *Buff_file, size_t s, char *resultat)
{
	CURL *curl;			// curl handle
	CURLcode res;

	int bestscore = 0;

	struct MemoryStruct data;
	data.memory = (char *)malloc(1);  /* will be grown as needed by the realloc above */
	data.size = 0;    /* no data at this point */

	resultat[0] = '\0';

	//No size Return directly
	if (s == 0) return 0;

	curl = curl_easy_init();
	if (curl)
	{

		struct WriteThis pooh;

		char sizeHeader[600];
		char *Token = NULL;

		std::string apiurl;
		std::string apiPostData;

		//**************************
		//       First api call
		//**************************

		apiurl = "https://oxford-speech.cloudapp.net/token/issueToken";

		apiPostData = "grant_type=client_credentials";
		apiPostData = apiPostData + "&client_id=" + ApiKey;
		apiPostData = apiPostData + "&client_secret=" + ApiKey;
		apiPostData = apiPostData + "&scope=https%3A%2F%2Fspeech.platform.bing.com";

		curl_easy_setopt(curl, CURLOPT_POST, 1L);

		//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);// To debug

		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&data);

		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);

		curl_easy_setopt(curl, CURLOPT_URL, apiurl.c_str());
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, apiPostData.c_str());

		res = curl_easy_perform(curl);

		{
			//parsing with regex
			struct slre_cap caps[1];

			//Have a result with confidence ?
			if (slre_match("\"access_token\":\"([^\"]+)\"", data.memory, data.size, caps, 1, 0) > 0)
			{
				Token = (char *)malloc((caps[0].len + 1) * sizeof(char));
				strncpy(Token, caps[0].ptr, caps[0].len);
				Token[caps[0].len] = '\0';
			}
		}

		curl_easy_cleanup(curl);

		if (!Token) return 0;

		//************************************
		//          Second API call
		//************************************

		free(data.memory);
		data.memory = (char *)malloc(1);  /* will be grown as needed by the realloc above */
		data.size = 0;    /* no data at this point */

		curl = curl_easy_init();
		if (curl)
		{
			struct curl_slist *chunk = NULL;

			apiurl = "https://speech.platform.bing.com/recognize?scenarios=ulm&appid=f84e364c-ec34-4773-a783-73707bd9a585&device.os=wp7&version=3.0&format=json&locale=";
	
			if (GetLanguage() == 0) apiurl = apiurl + "fr-FR";
			else apiurl = apiurl + "en-US";

			//Device ID
			apiurl = apiurl + "&instanceid=1d4b6030-9099-11e0-91e4-0800200c9a66";
			//Client ID generated
			apiurl = apiurl + "&requestid=" + Uid;

			wprintf(L"File size %d Kb\n", s / 1000);

			pooh.readptr = Buff_file;
			pooh.sizeleft = s;
			pooh.totalsize = s;

			curl_easy_setopt(curl, CURLOPT_POST, 1L);

			//Header part
			snprintf(sizeHeader, sizeof(sizeHeader), "Authorization: Bearer %s", Token);
			chunk = curl_slist_append(chunk, sizeHeader);
			chunk = curl_slist_append(chunk, "Content-Type: audio/wav; samplerate=8000");//samplerate = 8000/16000
			sprintf(sizeHeader, "Content-Length: %d", s);
			chunk = curl_slist_append(chunk, sizeHeader);
			//disalbe Expect: 100-continue
			chunk = curl_slist_append(chunk, "Expect:");
			//Set headers
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

			//curl_easy_setopt(curl, CURLOPT_POST, 1L);
			curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
			curl_easy_setopt(curl, CURLOPT_READDATA, &pooh);
			//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);// To debug

			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&data);

			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);


			curl_easy_setopt(curl, CURLOPT_URL, apiurl.c_str());

			res = curl_easy_perform(curl);

			Mywprintf(L"Resultat From Bing\n %s \n", data.memory);

			{
				//parsing with regex
				struct slre_cap caps[2];


				//Have a result with confidence ?
				if (slre_match("\"lexical\":\"([^\"]+)\",\"confidence\":\"([^\"]+)\"", data.memory, data.size, caps, 2, 0) > 0)
				{
					char *tmp2;
					int l;

					strncpy(resultat, caps[0].ptr, caps[0].len);
					resultat[caps[0].len] = '\0';

					l = caps[1].len;
					if (l > 4) l = 4;
					tmp2 = (char *)malloc((l + 1) * sizeof(char));
					strncpy(tmp2, caps[1].ptr + 2, l - 2);
					tmp2[l - 2] = '\0';

					bestscore = 1 + (atoi(tmp2));

					free(tmp2);

				}
				else
				{
					char *Pmemory = data.memory;

					// ok nevermind, add all the other results
					while (slre_match("\"lexical\":\"([^\"]+)\"", Pmemory, data.size, caps, 1, 0) > 0)
					{
						char *tmp1;
						char *tmp2;
						int l;

						tmp1 = (char *)malloc((caps[0].len + 1) * sizeof(char));
						strncpy(tmp1, caps[0].ptr, caps[0].len);
						strncpy(tmp1 + caps[0].len, "\0", 1);

						tmp2 = tmp1;
						while (tmp2)
						{
							while ((tmp2[0] != ' ') && (tmp2[0] != '\0')) tmp2++;
							if (tmp2[0] != '\0')
							{
								tmp2[0] = '\0';
								tmp2++;
							}
							else tmp2 = NULL;

							if (!mystrstr(resultat, tmp1))
							{
								int l = strlen(resultat) + strlen(tmp1) + 1;
								if (l < 254)
								{
									strcat(resultat, " ");
									strcat(resultat, tmp1);
								}
							}

							if (tmp2) tmp1 = tmp2;
						}

						l = caps[0].ptr + caps[0].len - data.memory;
						Pmemory += l;
						data.size -= l;

						bestscore = 50;
					}
				}
			}

			curl_easy_cleanup(curl);

		}

		free(data.memory);
	}

	return bestscore;
}
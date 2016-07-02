#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wchar.h>
#include <ctype.h>


#include <curl/curl.h>
#include <curl/easy.h>


//http://nicolasj.developpez.com/articles/regex/
//https://github.com/cesanta/slre
#include "libs/slre.h"


//https://curl.haxx.se/libcurl/c/example.html

#include "fonction.h"


#ifdef _WIN32
#define snprintf _snprintf
#else
#include <unistd.h>
#endif



/********************************************************/

//Structure recevant la sortie de LibCurl
struct BufferStruct
{
  char* buffer;
  size_t size;
};

//Met le contenu de la page web dans la struct
static size_t WriteMemoryCallback(void* ptr, size_t size, size_t nmemb, void* data)
{
	size_t realsize = size * nmemb;
	struct BufferStruct* mem = (struct BufferStruct*) data;
	mem->buffer = (char*)realloc(mem->buffer, mem->size + realsize + 1);

	if ( mem->buffer )
	{
		memcpy(&(mem->buffer[mem->size]), ptr, realsize );
		mem->size += realsize;
		mem->buffer[ mem->size ] = 0;

		return realsize;
	}

	return 0;
}


//Lecture de la page web
char* LectureWeb(char* URL)
{

  char *Chaine;
 
  CURL *myHandle;
  CURLcode result;
  struct BufferStruct LectureLC;
  LectureLC.buffer = NULL;
  LectureLC.size = 0;

  curl_global_init(CURL_GLOBAL_ALL);
 
  myHandle = curl_easy_init();
  curl_easy_setopt(myHandle, CURLOPT_SSL_VERIFYPEER, 0);
  curl_easy_setopt(myHandle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
  curl_easy_setopt(myHandle, CURLOPT_WRITEDATA, (void*)&LectureLC);
  curl_easy_setopt(myHandle, CURLOPT_URL, URL);
  result = curl_easy_perform(myHandle);  //voir la doc pour une gestion minimal des erreurs
  curl_easy_cleanup(myHandle);
 
  if(result!=0)
  {
	  /*Error */
	  LectureLC.size=1;
  }

  if(LectureLC.buffer)
  {

	Chaine = (char*)malloc((LectureLC.size + 1) * sizeof (char));

	strcpy(Chaine, LectureLC.buffer); 
	strcat(Chaine,"\0");

	free(LectureLC.buffer);
	LectureLC.buffer = NULL;

	return Chaine;
  }
  
 
  return NULL;
}


int check_gmail(char *username, char *password)
{

	CURL *myHandle;
	CURLcode result;

	// sendRequest 
	myHandle = curl_easy_init();
	curl_easy_setopt(myHandle, CURLOPT_URL, "https://mail.google.com/mail/feed/atom");
	curl_easy_setopt(myHandle, CURLOPT_FOLLOWLOCATION, 1);
	//curl_easy_setopt(myHandle, CURLOPT_RETURNTRANSFER, 1);
	curl_easy_setopt(myHandle, CURLOPT_SSL_VERIFYPEER, 0);

	//curl_setopt(myHandle, CURLOPT_USERPWD, username . ":".password);
	curl_easy_setopt(myHandle, CURLOPT_USERNAME, username);
	curl_easy_setopt(myHandle, CURLOPT_PASSWORD, password);

	curl_easy_setopt(myHandle, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
	curl_easy_setopt(myHandle, CURLOPT_ENCODING, "");
	result = curl_easy_perform(myHandle);
	curl_easy_cleanup(myHandle);

	curl_global_cleanup();

	//returning retrieved feed
	return 1;
}


int OpenMailServer(char *username,char *password)
{
	char *m_popsAccount = "pop3s://pop.gmail.com:995/"; // try with /1
    struct BufferStruct chunk;

	int mail = 0;

	CURL *myHandle;
	CURLcode result;

    chunk.buffer = (char*) malloc(1);  //crecerá según sea necesario con el realloc
    chunk.size = 0;    //no hay datos en este punto

    //inicializacion
    curl_global_init(CURL_GLOBAL_ALL);
    myHandle = curl_easy_init();


    //login
    curl_easy_setopt(myHandle,CURLOPT_USERNAME,username);
    curl_easy_setopt(myHandle,CURLOPT_PASSWORD,password);      

    curl_easy_setopt(myHandle, CURLOPT_URL, m_popsAccount);
    curl_easy_setopt(myHandle, CURLOPT_USE_SSL, CURLUSESSL_ALL); 
    curl_easy_setopt(myHandle, CURLOPT_SSL_VERIFYPEER, 0); 
    curl_easy_setopt(myHandle, CURLOPT_SSL_VERIFYHOST, 0);

	//some servers needs this validation
    curl_easy_setopt(myHandle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

	curl_easy_setopt(myHandle, CURLOPT_VERBOSE, 0); // debug infos

    curl_easy_setopt(myHandle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(myHandle, CURLOPT_WRITEDATA, (void *)&chunk);

	//curl_easy_setopt(myHandle, CURLOPT_CUSTOMREQUEST, "STAT");//pr les stats
	//curl_easy_setopt(myHandle, CURLOPT_NOBODY, 1L);
	//curl_easy_setopt(myHandle, CURLOPT_CUSTOMREQUEST, "QUIT");//pr les stats

    result = curl_easy_perform(myHandle); 

    if(result != CURLE_OK)
    {
        Mywprintf(L"curl_easy_perform() failed: %s\n", curl_easy_strerror(result));
    }
    else 
    {
        //printf("%s\n",chunk.buffer);
        //printf("%lu bytes retrieved\n", (long)chunk.size);
		int i;
		for (i=0; chunk.buffer[i]; chunk.buffer[i]=='\n' ? i++ : *chunk.buffer++);
		//printf("Vous avez %i messages\n",i);
		mail = i;
  }

    //se libera la memoria si hay datos
    if(chunk.buffer)
        free(chunk.buffer);
    /* always cleanup */ 

	curl_easy_cleanup(myHandle);
    curl_global_cleanup();

	return mail;
}



void SauvegarderFichier(char *Chaine, char *Chemin, char *NomFichier)
{
  //chdir(Chemin);
  FILE* Flot = NULL;
  Flot = fopen(NomFichier, "wt");
  if(Flot!=NULL)
  {
    fputs(Chaine, Flot);
    fclose(Flot);
  }
}
/*******************************************************/



/******************************************************************/

void Mywprintf(const wchar_t* format,const char* wc)
{
#ifdef _WIN32
	wchar_t* format2;
	wchar_t *p;
	format2 = (wchar_t*)malloc( (wcslen(format) + 1) * sizeof(wchar_t) );
	wcscpy(format2, format);
	p = (wchar_t*)format2;
	while(*p){
		wchar_t c = *p;
		if (c == L'%')
			switch(p[1]){
			case(L'S'):
				p[1] = L's';
				break;
			case(L's'):
				p[1] = L'S';
				break;
		}
		p++;
	}

	wprintf(format2,wc);

	free(format2);
#else
	wprintf(format,wc);
#endif

}

void Myswprintf(wchar_t *ws, size_t len, const wchar_t* format,const wchar_t* wc)
{
#ifdef _WIN32
	wchar_t* format2;
	wchar_t *p;
	format2 = (wchar_t*)malloc( (wcslen(format) + 1) * sizeof(wchar_t) );
	wcscpy(format2, format);
	p = (wchar_t*)format2;
	while(*p){
		wchar_t c = *p;
		if (c == L'%')
			switch(p[1]){
			case(L'S'):
				p[1] = L's';
				break;
			case(L's'):
				p[1] = L'S';
				break;
		}
		p++;
	}

	swprintf(ws,len,format2,wc);

	free(format2);
#else
	swprintf(ws,len,format,wc);
#endif

}

/******************************************************/

/* Converts a hex character to its integer value */
char from_hex(char ch) {
  return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
}

/* Converts an integer value to its hex character*/
char to_hex(char code) {
  static char hex[] = "0123456789abcdef";
  return hex[code & 15];
}

/* Returns a url-encoded version of str */
/* IMPORTANT: be sure to free() the returned string after use */
char *url_encode(char *str) {
  char *pstr = str, *buf = (char*)malloc(strlen(str) * 3 + 1), *pbuf = buf;
  while (*pstr) {
    if (isalnum(*pstr) || *pstr == '-' || *pstr == '_' || *pstr == '.' || *pstr == '~') 
      *pbuf++ = *pstr;
    else if (*pstr == ' ') 
      *pbuf++ = '+';
    else 
      *pbuf++ = '%', *pbuf++ = to_hex(*pstr >> 4), *pbuf++ = to_hex(*pstr & 15);
    pstr++;
  }
  *pbuf = '\0';
  return buf;
}

/* Returns a url-decoded version of str */
/* IMPORTANT: be sure to free() the returned string after use */
char *url_decode(char *str) {
  char *pstr = str, *buf = (char*)malloc(strlen(str) + 1), *pbuf = buf;
  while (*pstr) {
    if (*pstr == '%') {
      if (pstr[1] && pstr[2]) {
        *pbuf++ = from_hex(pstr[1]) << 4 | from_hex(pstr[2]);
        pstr += 2;
      }
    } else if (*pstr == '+') { 
      *pbuf++ = ' ';
    } else {
      *pbuf++ = *pstr;
    }
    pstr++;
  }
  *pbuf = '\0';
  return buf;
}

/******************************************************/

BOOL charisinstring(char *s,char c)
{
	char *a = s;
	while (a[0] != '\0')
	{
		if (a[0] == c) return TRUE;
		a++;
	}
	return FALSE;
}

char* mystrstr(char *s, char *subs) {
	char *a = s;
	char *b = subs;
	char *c = subs;
	char *d;

	// Ya t 'il des parenthses
	while (c[0] != '\0')
	{
		if (c[0] == '(')
		{
			c[0] = '\0';
			c++;
			
			d = c;
			while (d[0] != ')') d++;
			d[0] = '\0';

			break;
		}

		c++;
	}

	for( ; *s != '\0'; s++)
	{
		if(*s != *b)
		{
			continue;
		}

		a = s;
		while(1)
		{
			if(*b == '\0')
			{
				// ok le mot y est mais le char suivant est il un espace,une fin de chaine ou un pluriel ? */
				if ((*a == ' ') || (*a == '\0') || (*a == 's') )
				{
					return s;
				}
#if 1
				//Est ce que tout les caracteres suivant sont dans la partie entre parenthses ?
				while (charisinstring(c, *a)) a++;

				//Deuxieme test
				if ((*a == ' ') || (*a == '\0') || (*a == 's'))
				{
					return s;
				}

#endif
				//Sinon c'est mort
				break;

			}
			else if(*a++ != *b++)
			{
				break;
			}
		}
		b = subs;
	}

	return (char *) NULL;
}

void mymbstowcs(wchar_t *ws,char *s,int len)
{
	char *pr = s;
	wchar_t *pw = ws;
	unsigned char ch;

    while ( ((*pr) != 0) && (len > 0) )
	{
        ch = (*pr);

		if ( ch == '\\' )
		{
			if ( *(pr+1) == 'u')
			{

				char h[5]={'\0'};
				strncpy(h,pr + 2,4);

				ch = (int)strtol(h, NULL, 16);

				pr = pr + 5;
				len = len - 5;
			}
		}

		(*pw) = ch;

		++pw;
		++pr;
		len --;
	}
	*pw = '\0';

	return;
}


/*************************************************************************/
void UnicodeToAnsi(char *str, char *str2)
{
	/* Impossible de faire fonctionner pour une chaine complete, ne marche que pour 1 cracteres d'ou le bordel */
	unsigned char ch;

	char *pr = str;
	char *pw = str2;

    while ( (*pr) != 0 )
	{
        ch = (*pr);
        if ( ch == '\\' )
		{
			if ( *(pr+1) == 'u')
			{

				char szANSIString [2] = {'\0'};
				wchar_t wcsString[2] = {0,'\0'};

				char h[5]={'\0'};
				int v;
				strncpy(h,pr + 2,4);


				v = (int)strtol(h, NULL, 16);
#if 0
				wcsString[0] = v;

#ifndef _WIN32
				WideCharToMultiByte ( CP_ACP, // ANSI code page
                WC_COMPOSITECHECK,     // Check for accented characters
                wcsString,         // Source Unicode string
                -1,                    // -1 means string is zero-terminated
                szANSIString,          // Destination char string
                sizeof(szANSIString),  // Size of buffer
                NULL,                  // No default character
                NULL );                // Don't care about this flag
#else
	            wcstombs(szANSIString, wcsString, sizeof(szANSIString));
#endif
				ch = *szANSIString;
				if (ch == '\0') ch = '?';
#endif

				//bored with this fucking unicode, easy way

				ch = '-';
				if (v == 232) ch = 138;
				if (v == 233) ch = 130;
				if (v == 234) ch = 136;
				if (v == 224) ch = 133;
				if (v == 225) ch = 'a';
				if (v == 226) ch = 'a';
				if (v == 257) ch = 'a';
				if (v == 231) ch = 135;

				pr = pr + 5;
			}

        }
		(*pw) = ch;

		++pw;
		++pr;
    }

	*pw = '\0';

    return;

}		

/***************************************************************/

int GetWord(char * reg, char *str, char *res, int l)
{
	struct slre_cap caps[1];

	if (slre_match(reg,str, strlen(str), caps, 1, 0) > 0)
	{

		/* Max 255 char */
		l--;
		if (caps[0].len > l) caps[0].len = l;

		strncpy(res,caps[0].ptr,caps[0].len);
		strncpy(res + caps[0].len ,"\0", 1);

		return 1;
	}

	return 0;
}

//retourne temps en minutes
int Findhour(char *str)
{
	/* La lib ne gere pas (?:) donc 2 tests a faire */
	struct slre_cap caps[2];

	int hour;
	int minut;

	if (slre_match(" ([a-z]+) heures* ([a-z]+)",str, strlen(str), caps, 2, 0) > 0)
	{
		char *tmp;

		tmp = (char *)malloc((caps[0].len + 1) * sizeof(char));
		strncpy(tmp,caps[0].ptr,caps[0].len);
		strncpy(tmp + caps[0].len ,"\0", 1);
		hour = ConvertionChiffre(tmp);
		free(tmp);

		tmp = (char *)malloc((caps[1].len + 1) * sizeof(char));
		strncpy(tmp,caps[1].ptr,caps[1].len);
		strncpy(tmp + caps[1].len ,"\0", 1);
		minut = ConvertionChiffre(tmp);
		free(tmp);

		return minut + 60* hour;
	}

	if (slre_match(" ([a-z]+) heures*",str, strlen(str), caps, 2, 0) > 0)
	{
		char *tmp = (char *)malloc((caps[0].len + 1) * sizeof(char));
		strncpy(tmp,caps[0].ptr,caps[0].len);
		strncpy(tmp + caps[0].len ,"\0", 1);
		hour = ConvertionChiffre(tmp);
		free(tmp);

		return hour * 60;
	}

	return 0;
}

int Findhour2(char *str)
{
	/* La lib ne gere pas (?:) donc 2 tests a faire */
	struct slre_cap caps[2];

	int hour;
	int minut;

	//au cas ou
	ConvertionChiffre(str);

	//Petit raccourcis pour les valeur speciale */
	if (mystrstr(str, "midi")) return 12 * 60;
	if (mystrstr(str, "un quart d'heure")) return 15;
	if (mystrstr(str, "demie heure")) return 30;

	//heure et minutes
	if (slre_match(" ([0-9 ]+)h([0-9 ]+)", str, strlen(str), caps, 2, 0) > 0)
	{
		char *tmp;

		tmp = (char *)malloc((caps[0].len + 1) * sizeof(char));
		strncpy(tmp, caps[0].ptr, caps[0].len);
		strncpy(tmp + caps[0].len, "\0", 1);
		hour = atoi(tmp);
		free(tmp);

		tmp = (char *)malloc((caps[1].len + 1) * sizeof(char));
		strncpy(tmp, caps[1].ptr, caps[1].len);
		strncpy(tmp + caps[1].len, "\0", 1);
		minut = atoi(tmp);
		free(tmp);

		return minut + 60 * hour;
	}
	//heures seules
	else if (slre_match(" ([0-9]+)h", str, strlen(str), caps, 2, 0) > 0)
	{
		char *tmp = (char *)malloc((caps[0].len + 1) * sizeof(char));
		strncpy(tmp, caps[0].ptr, caps[0].len);
		strncpy(tmp + caps[0].len, "\0", 1);
		hour = atoi(tmp);
		free(tmp);

		return hour * 60;
	}
	//minutes seules
	else if (slre_match(" ([0-9]+) minute", str, strlen(str), caps, 2, 0) > 0)
	{
		char *tmp = (char *)malloc((caps[0].len + 1) * sizeof(char));
		strncpy(tmp, caps[0].ptr, caps[0].len);
		strncpy(tmp + caps[0].len, "\0", 1);
		minut = atoi(tmp);
		free(tmp);

		return minut;
	}

	return 0;
}

/****************************************************************************************************/
static char *ListDizaine[]=
{
	("dix"),
	("vingt"),
	("trente"),
	("quarante"),
	("cinquante"),
	NULL
};

static char *ListUnite[]=
{
	("zero"),
	("une"),
	("deux"),
	("trois"),
	("quatre"),
	("cinq"),
	("six"),
	("sept"),
	("huit"),
	("neuf"),
	NULL
};

// char '-' need to be removed before
//Dangerous fonction but every time the Number have lower len than letter number, so I m using same char.
int ConvertionChiffre(char * string)
{
	char * NewString;
	char * Stringtok;
	int len = strlen(string);
	char Buff[4]; //max 2 number + 1 espace + \0

	char * NewStringMem;
	char * MemStringtok;
	int lenMem;

	char *chainWord;

	int dizaine = 0;
	int unite = 0;
	int chiffre;
	


	NewStringMem = (char *)malloc((len +1 ) * sizeof(char));
	MemStringtok = (char *)malloc((len + 1) * sizeof(char));

	Stringtok = MemStringtok;


	NewString = NewStringMem;
	memset(NewString,'\0',len);


	strncpy(Stringtok,string,len);
	Stringtok[len] = '\0'; //just to be sure

	chainWord = Stringtok;
	while ((Stringtok[0] != ' ') && (Stringtok[0] != '\0')) Stringtok++;
	if (Stringtok[0] == ' ')
	{
		Stringtok[0] = '\0';
		Stringtok++;
	}
	else chainWord = NULL;

	while (chainWord != NULL)
	{
		int i;

		int len2 = strlen(chainWord);

		chiffre = 0;

		//dizaine ?
		for (i = 0; i < 5; i++)
		{
			if (strcmp(chainWord,ListDizaine[i]) == 0)
			{
				dizaine = (i + 1) * 10;
				chiffre = 1;
			}
		}

		//unites
		for (i = 0; i < 10; i++)
		{
			if (strcmp(chainWord,ListUnite[i]) == 0)
			{
				unite = i;
				chiffre = 1;
			}
		}

		//special
		if (strcmp(chainWord, "heure") == 0) { chainWord[1] = '\0'; }
		if (strcmp(chainWord,"quart") == 0)	{ unite = 15; chiffre = 1; }
		if (strcmp(chainWord,"demi") == 0) { unite = 30; chiffre = 1; }
		if (strcmp(chainWord,"onze") == 0)	{ unite = 11; chiffre = 1; }
		if (strcmp(chainWord,"douze") == 0) { unite = 12; chiffre = 1; }
		if (strcmp(chainWord,"treize") == 0)	{ unite = 13; chiffre = 1; }
		if (strcmp(chainWord,"quatorze") == 0) { unite = 14; chiffre = 1; }
		if (strcmp(chainWord,"quinze") == 0)	{ unite = 15; chiffre = 1; }
		if (strcmp(chainWord,"seize") == 0) { unite = 16; chiffre = 1; }

		if ( (chiffre == 0 ) && ( (unite > 0) || (dizaine > 0) ) )
		{

			sprintf(Buff,"%d ",dizaine + unite);
			len2 = strlen(Buff);

			strncpy(NewString,Buff,len2);
			NewString = NewString + len2;

			dizaine = 0;
			unite = 0;

		}

		if (chiffre == 0 )
		{
			if (!strcmp(chainWord, "et") == 0)
			{
				len2 = strlen(chainWord);
				strncpy(NewString, chainWord, len2);
				NewString[len2] = '\0';
				NewString = NewString + len2;
			}

		}

		//Forced to code my own strtok function ...
		if (Stringtok)
		{
			chainWord = Stringtok;
			while ((Stringtok[0] != ' ') && (Stringtok[0] != '\0')) Stringtok++;
			if (Stringtok[0] != '\0')
			{
				Stringtok[0] = '\0';
				Stringtok++;
			}
			else Stringtok = NULL;
		}
		else chainWord = NULL;

		if ((chainWord != NULL) && (chiffre == 0))
		{
			NewString[0] = ' ';
			NewString++;
		}


		/* plus de chaine mais encore un chiffre memorise */
		if ((chainWord == NULL)  && ( (unite > 0) || (dizaine > 0) ) )
		{
			sprintf(Buff,"%d",dizaine + unite);
			len2 = strlen(Buff);

			strncpy(NewString,Buff,len2);
			NewString = NewString + len2;
		}

	}

	lenMem = strlen(NewStringMem);
	strncpy(string,NewStringMem,lenMem);
	string[lenMem] = '\0';

	free(NewStringMem);
	free(MemStringtok);

	return 1;
}

/****************************************/

wchar_t wbufhour[20];
const wchar_t * wtimestamp(void)
{
	time_t crt = time(NULL);
	struct tm * timeinfo;

	timeinfo = localtime(&crt);

	wcsftime(wbufhour, 20, L"%H:%M", timeinfo);
	//wprintf(L"%S",wbufhour);

	return wbufhour;
}


char bufhour[20];
const char * timestamp(void)
{
	time_t crt = time(NULL);
	struct tm * timeinfo;

	timeinfo = localtime(&crt);

	strftime(bufhour, 20, "%H:%M", timeinfo);

	return bufhour;
}


/* sleep for X milliSeconds */
void Wait(int sec)
{
#ifdef _WIN32
	Sleep(sec);
#else
	usleep(sec * 1000);
#endif
}

void SP(void)
{
	wprintf(L"***********************************************************\n");
}
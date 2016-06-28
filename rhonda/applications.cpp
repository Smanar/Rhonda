#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <wchar.h>

#include "applications.h"
#include "fonction.h"

#include "libs/slre.h"

#ifdef _WIN32
#define snprintf _snprintf
#else
#include <unistd.h>
#endif

int PlayWave(char * file);

/*************************************/

time_t Alarme1 = 0;

void SetAlarm(time_t t)
{
	char buff[255];

	strftime(buff, sizeof(buff), "%H heure et %M minute", localtime(&t));
	Mywprintf(L"heure alarme : %s\n",buff);

	Alarme1 = t;
}

void Checkalarm(void)
{
	time_t crt = time(NULL);
	double diff =  difftime(crt, Alarme1);

	wprintf(L"Verifiacation des alarme\n");

	if ((Alarme1 < crt) && (Alarme1 > 0))
	{
		parle(L"Alarme declenchee");
		Alarme1 = 0;
	}
}

/******************************************************/
char ville[20];

void SetCity(char *s)
{
	int l = strlen(s);
	if (l > 19) l = 19;
	strncpy(ville, s, l);
}

int GetMeteo(wchar_t *s)
{
	struct slre_cap caps[1];
	
	char url[255];
	strcpy(url, "http://www.prevision-meteo.ch/services/json/");
	strcat(url, ville);
	
	char *html = LectureWeb(url);

	if (slre_match("fcst_day_1.+?\"condition\":\"([^\"]+)\"",html, strlen(html), caps, 1, 0) > 0)
	{
		if (caps[0].len < 255)
		{
			char *tmp = (char *)malloc((caps[0].len + 1) * sizeof(char));
			strncpy(tmp,caps[0].ptr,caps[0].len);
			strncpy(tmp + caps[0].len ,"\0", 1);
			//mbstowcs(s, tmp, caps[0].len+1);
			mymbstowcs(s, tmp, caps[0].len+1);
			//swprintf(s, 100, L"%hs", caps[0].ptr);
		}
	}

	free(html);

	return 0;

}

/*******************************************************************************/

int GetDefinition(char *mot, wchar_t * def)
{
	struct slre_cap caps[1];
	char url[255];
    char *html;

	int Maxlen = 800;

	char *search;

	Mywprintf(L"Recherche de la definition du mot : %s\n",mot);

	search = url_encode(mot);
	if (strlen(search) > 100)
	{
		free(search);
		return false;
	}

	snprintf(url,255,"https://fr.wikipedia.org/w/api.php?action=opensearch&limit=1&format=json&search=%s",search);
	free(search);
	html = LectureWeb(url);

	if (!html) return false;

	if (slre_match(",.\"([^\"]+)\".,.\"http",html, strlen(html), caps, 1, 0) > 0)
	{
		char buff[800];

		/* Max 800 char */
		if (caps[0].len > 800) caps[0].len = 800;

		strncpy(buff,caps[0].ptr,caps[0].len);
		strncpy(buff + caps[0].len ,"\0", 1);

#ifdef _WIN32
		//UnicodeToAnsi(buff,tmp);
#endif
		mymbstowcs(def, buff, caps[0].len);

	}

	free(html);

	return true;

}


/*******************************************************/

char MailUser[50];
char MailPass[50];

void SetMailUserPass(char *u, char *p)
{
	int l = strlen(u);
	if (l > 49) l = 49;
	strncpy(MailUser, u, l);

	l = strlen(p);
	if (l > 49) l = 49;
	strncpy(MailPass, p, l);

}
int CheckMail(void)
{
	int mail = 0;
	mail = OpenMailServer(MailUser, MailPass);

	return mail;
}


/******************************************************/
//http://stackoverflow.com/questions/478898/how-to-execute-a-command-and-get-output-of-command-within-c-using-posix


#include <string>


std::string exec(const char* cmd) {
	char buffer[128];
	std::string result = "";

#ifndef _WIN32
	FILE* pipe = popen(cmd, "r");
	if (!pipe) throw wprintf(L"Probleme lancement fichier shell\n");
	try {
		while (!feof(pipe)) {
			if (fgets(buffer, 128, pipe) != NULL)
				result += buffer;
		}
	}
	catch (...) {
		pclose(pipe);
		throw;
	}
	pclose(pipe);
#endif
	return result;
}
/***********************************************/

void executeshell(char *command)
{
	Mywprintf(L"Execution du fichier shell %s:\n",command);
#ifndef _WIN32
	//execlp("/ressources/shell", "/bin/sh", "-c", command, NULL);
	//system(command);
	std::string ret = exec(command);
	Mywprintf(L"retour %s:\n", ret.c_str());
#endif
}

void lireshell(char *command)
{
	Mywprintf(L"Lecture du fichier shell %s:\n", command);
#ifndef _WIN32
	std::string ret = exec(command);
	std::wstring widestr = std::wstring(ret.begin(), ret.end());
	parle(widestr.c_str());
#endif
}

int parle(const wchar_t *texte)
{
	//char commande[400];
	wprintf(L"\033[1;33mPhrase a dire %ls\033[0;37m\n",texte);

#ifndef _WIN32
	char commande[501];
	int l = wcslen(texte) + 2;
	char *char_text = (char *)malloc(l*sizeof(char) );
	wcstombs(char_text, texte, l);
	snprintf(commande, 500, "pico2wave -l=fr-FR -w=/dev/shm/test.wav \"<pitch level='100'><speed level='80'>%s</speed></pitch>\"", char_text);
	//wprintf(L"Commande : %s\n",commande);
	system(commande);
	//system("aplay /dev/shm/test.wav");
	PlayWave("/dev/shm/test.wav");
#endif

	return 1;
}

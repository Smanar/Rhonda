#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <wchar.h>

#include "applications.h"
#include "fonction.h"

#include "libs/slre.h"

#include "libs/pugixml.hpp"

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

	strftime(buff, sizeof(buff), "%H heure et %M minute le %x", localtime(&t));
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

int GetFilmCinema(wchar_t *s, int len)
{
	struct slre_cap caps[1];
	char url[255];
	char *html;
	char buff[300];
	char *posbuf = buff;
	char *posHtml;

	int Maxlen = 800;
	len -= 1;

	html = LectureWeb("http://www.commeaucinema.com/rsspage.php?feed=cine");

	if (!html) return false;

	posHtml = html;

	while (slre_match("<title><!.CDATA.([^>]+)", posHtml, strlen(posHtml), caps, 1, 0) > 0)
	{
		caps[0].len -= 2;

		if (caps[0].len > len) break;
		len -= caps[0].len;

		strncpy(posbuf, caps[0].ptr, caps[0].len);
		strncpy(posbuf + caps[0].len, ", ", 2);
		posbuf = posbuf + caps[0].len + 2;

		posHtml = (char*)(caps[0].ptr + caps[0].len);


	}
	posbuf[-1] = '\0';

#ifdef _WIN32
	//UnicodeToAnsi(buff,tmp);
#endif
	mymbstowcs(s, buff, strlen(buff));

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
	SP();
	wprintf(L"\033[1;33mPhrase a dire %ls\033[0;37m\n", texte);
	SP();

#ifndef _WIN32
	char commande[511];
	int l = wcslen(texte) + 2;
	//if the strng is too long, we cut it
	if (l > 400)
	{
		l = 400;
	}
	char *char_text = (char *)malloc(l*sizeof(char));
	wcstombs(char_text, texte, l);

	snprintf(commande, 510, "pico2wave -l=fr-FR -w=/dev/shm/test.wav \"<pitch level='100'><speed level='80'>%s</speed></pitch>\"", char_text);
	//wprintf(L"Commande : %s\n",commande);

	system(commande);
	//system("aplay /dev/shm/test.wav");
	PlayWave("/dev/shm/test.wav");
#endif

	return 1;
}


/***************************************************************************************/

void Savedata(void)
{

	// Generate new XML document within memory
	pugi::xml_document doc;
	// Alternatively store as shared pointer if tree shall be used for longer
	// time or multiple client calls:
	// std::shared_ptr<pugi::xml_document> spDoc = std::make_shared<pugi::xml_document>();
	// Generate XML declaration
	auto declarationNode = doc.append_child(pugi::node_declaration);
	declarationNode.append_attribute("version") = "1.0";
	declarationNode.append_attribute("encoding") = "ISO-8859-1";
	declarationNode.append_attribute("standalone") = "yes";
	// A valid XML doc must contain a single root node of any name
	auto root = doc.append_child("MyRoot");



	// Append some child elements below root
	// Add as last element
	pugi::xml_node nodeChild = root.append_child("Alarm");
	nodeChild.append_attribute("time") = "inserted as last child";
	nodeChild.append_attribute("day") = "111";
#if 0
	// Add as last element
	nodeChild = root.append_child("Alarm");
	nodeChild.append_attribute("hint") = "also inserted as last child";
	nodeChild.append_attribute("doubleVal") = "222";
	// Add as first element
	nodeChild = root.prepend_child("Alarm");
	nodeChild.append_attribute("hint") = "inserted at front";
	nodeChild.append_attribute("boolVal") = "333";
#endif

#if 0
	pugi::xml_node childrenWithValues = root.append_child("ChildrenWithValue");
	// Add child of type integer
	nodeChild = childrenWithValues.append_child("MyChildWithIntValue");
	nodeChild.append_child(pugi::node_pcdata).set_value("147");
	// Add child of type double
	nodeChild = childrenWithValues.append_child("MyChildWithDoubleValue");
	nodeChild.append_child(pugi::node_pcdata).set_value("478");
	// Add child of type bool
	nodeChild = childrenWithValues.append_child("MyChildWithBoolValue");
	nodeChild.append_child(pugi::node_pcdata).set_value("222");
#endif


	// Save XML tree to file.
	// Remark: second optional param is indent string to be used;
	// default indentation is tab character.
	bool saveSucceeded = doc.save_file("data.xml", "  ");
}
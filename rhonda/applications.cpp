#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctime>
#include <stdio.h>
#include <wchar.h>

#include "applications.h"
#include "fonction.h"
#include "prog.h"

#include "libs/slre.h"

#include "libs/pugixml.hpp"

#ifdef _WIN32
#define snprintf _snprintf
#else
#include <unistd.h>
#endif

int PlayWave(char * file);



/********************************************/
#define MAX_ALARM 50

time_t Event_time = 0;
char *Event_Message;

class cAlarm
{
public:

	//constructeur
	cAlarm();

	// Méthodes
	int AddAlarm(char *,char *);
	int SetUpAlarm(void);

	time_t ComputeAlarm(char *t);
	//destructeur
	~cAlarm();

private:
	// Attributs
	char *ActionAlarm[MAX_ALARM];
	time_t TimeAlarm[MAX_ALARM];
	char MemTimer[MAX_ALARM][15];
};

int cAlarm::SetUpAlarm(void)
{
	int i;
	time_t crt = time(NULL);

	Event_time = 0;

	for (i = 0; i < MAX_ALARM; i++)
	{
		if (TimeAlarm[i] != 0)
		{
			//alarme perimee
			if (TimeAlarm[i] < crt)
			{
				//Est elle renouvelable ?
				if (charisinstring(MemTimer[i],'X') )
				{
					//on recalcule la nouvelle heure
					TimeAlarm[i] = ComputeAlarm(MemTimer[i]);
					//prbleme le calcule nous donne une huere plus petite
					if (TimeAlarm[i] < crt)
					{
						wprintf(L"Wrong calcul for new event\n");
						//forget it
						TimeAlarm[i] = 0;
						free(ActionAlarm[i]);

					}
				}
				else
				{
					TimeAlarm[i] = 0;
					free(ActionAlarm[i]);
				}
			}

			//alarme plus precoce que celle memorisee ?
			if (((TimeAlarm[i] < Event_time) || (Event_time == 0)) && TimeAlarm[i] != 0 )
			{
				Event_time = TimeAlarm[i];
				Event_Message = ActionAlarm[i];
			}
		}
	}

	//There is a new alarm ?
	if (TimeAlarm[i] != 0)
	{
		Mywprintf(L"Next event : %s", Event_Message);
		Mywprintf(L" at %s", asctime(localtime(&Event_time)));
	}


	return true;
}

/* Format YY/MM/DD/HH/MM     */
//tm_wday tm_yday ignoree pr mktime
time_t cAlarm::ComputeAlarm(char *t)
{
	time_t crt = time(NULL);
	struct tm T;
	char tmp[3] = { '\0' };
	int lastundefined = 0;
	time_t tmptime;

	if (strlen(t) != 14)
	{
		wprintf(L"wrong format for alarm time\n");
		return 0;
	}

	T = *localtime(&crt);

	strncpy(tmp, t, 2);
	if (tmp[0] != 'X') T.tm_year = atoi(tmp) + 100;
	else lastundefined = 1;
	t += 3;
	strncpy(tmp, t, 2);
	if (tmp[0] != 'X') T.tm_mon = atoi(tmp) - 1;
	else lastundefined = 2;
	t += 3;
	strncpy(tmp, t, 2);
	if (tmp[0] != 'X') T.tm_mday = atoi(tmp);
	else lastundefined = 3;
	t += 3;
	strncpy(tmp, t, 2);
	if (tmp[0] != 'X') T.tm_hour = atoi(tmp);
	else lastundefined = 4;
	t += 3;
	strncpy(tmp, t, 2);
	if (tmp[0] != 'X') T.tm_min = atoi(tmp);
	else lastundefined = 5;

	tmptime = mktime(&T);

	if (tmptime > crt) return tmptime;

	//ok le nouveau temps calcule est deja passee
	switch (lastundefined)
	{
		case 1: T.tm_year += 1; break;
		case 2: T.tm_mon += 1; break;
		case 3: T.tm_mday += 1; break;
		case 4: T.tm_hour += 1; break;
		case 5: return 0;
		default: return 0;
	}
	
	return mktime(&T);
}


int cAlarm::AddAlarm(char *t, char * action)
{
	int index = 0;
	int l;

	while (ActionAlarm[index] != NULL) index++;
	if (index > MAX_ALARM - 1)
	{
		wprintf(L"Too much alarms set\n");
		return false;
	}

	strncpy(MemTimer[index], t, 15);
	TimeAlarm[index] = ComputeAlarm(t);

	l = strlen(action) + 1;
	ActionAlarm[index] = (char*)malloc(l * sizeof(char));
	strncpy(ActionAlarm[index], action, l);

	Mywprintf(L"Add event : %s", ActionAlarm[index]);
	Mywprintf(L" at %s", asctime(localtime(&TimeAlarm[index])));

	return true;
}

cAlarm::cAlarm()
{
	int i;

	for (i = 0; i < MAX_ALARM; i++)
	{
		ActionAlarm[i] = NULL;
		TimeAlarm[i] = 0;
	}
}

cAlarm::~cAlarm()
{
	int i;
	for (i = 0; i < MAX_ALARM; i++)
	{
		if (ActionAlarm[i]) free(ActionAlarm[i]);
	}
}


class cAlarm cAlarm;


/*************************************/


void SetAlarm(char * t,char * m)
{
	cAlarm.AddAlarm(t, m);
}

void ResetAlarm(void)
{
	cAlarm.SetUpAlarm();
}

void Checkalarm(void)
{
	time_t crt = time(NULL);
	//double diff = difftime(crt, Event_time);

	//wprintf(L"Verifiacation des alarme %d\n", Event_time - crt);

	if ((Event_time < crt) && (Event_time > 0))
	{
		Mywprintf(L"%s Programmed event triggered\n", timestamp());
		ManageEvent(Event_Message);
		cAlarm.SetUpAlarm();
	}
}
/*******/

bool LoadData(void)
{
	//not enabled yet
	return true;

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file("data.xml");
	wprintf(L"\033[0;31mLoading Alarm XML %s\033[0;37m\n", result.description());
	if (result.status != 0) return false;

	pugi::xml_node panels = doc.child("MyRoot");

	//alarm
	for (pugi::xml_node panel = panels.child("Alarm").first_child(); panel; panel = panel.next_sibling())
	{
		cAlarm.AddAlarm((char *)panel.attribute("time").value() , (char *)panel.attribute("action").value());
	}

	return true;
}

/******************************************************/
std::string ville;
void SetCity(char *s)
{
	ville = s;
}

int GetMeteo(wchar_t *s)
{
	struct slre_cap caps[1];
	
	char url[255];
	strcpy(url, "http://www.prevision-meteo.ch/services/json/");
	strcat(url, ville.c_str());
	
	char *html = LectureWeb(url);


	if (slre_match("fcst_day_1.+?\"condition\":\"([^\"]+)\"",html, strlen(html), caps, 1, 0) > 0)
	{
		if (caps[0].len < 255)
		{
			char *tmp = (char *)malloc((caps[0].len + 10) * sizeof(char));
			strncpy(tmp,caps[0].ptr,caps[0].len);
			tmp[caps[0].len] = '\0';
			//mbstowcs(s, tmp, caps[0].len+1);
			mymbstowcs(s, tmp, caps[0].len);
			//swprintf(s, 100, L"%hs", caps[0].ptr);
			free(tmp);
		}
	}

	free(html);

	return 0;

}

/*******************************************************************************/
#ifdef _WIN32
char * FindFile(char * path, char *f, char *ret)
{
	return NULL;
}
#else
#include <dirent.h>

char * FindFile(char * path, char *f, char *ret)
{
	DIR           *d;
	struct dirent *dir;

	d = opendir("path");
	if (d)
	{
		while ((dir = readdir(d)) != NULL)
		{
			if (strstr(dir->d_name, f))
			{
				wprintf(L"%s\n", dir->d_name);
			}
		}

		closedir(d);
	}
}

#endif

/*******************************************************************************/

int GetDefinition(char *mot, wchar_t * def)
{
	struct slre_cap caps[1];
	char url[255];
    char *html;

	//int Maxlen = 800;

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

strncpy(buff, caps[0].ptr, caps[0].len);
strncpy(buff + caps[0].len, "\0", 1);

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
	char *html;
	char buff[300];
	char *posbuf = buff;
	char *posHtml;

	//int Maxlen = 800;
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
bool bMusicactive;
void PlayMusic(bool b)
{
	if (b)
	{
		executesCommand("mpg123 http://rfm-live-mp3-64.scdn.arkena.com/rfm.mp3 >/dev/null 2>&1 &");
		bMusicactive = true;
	}
	else
	{
		executesCommand("killall mpg123");
		bMusicactive = false;
	}
}

void ClearMusic(void)
{
	if (bMusicactive) PlayMusic(false);
}

/*******************************************************/

void SendRequest(char *url)
{
	char *html = LectureWeb(url);

	free(html);
}

/*******************************************************/

std::string MailUser_and_pass;

void SetMailUserPass(char *s)
{
	MailUser_and_pass = s;
}
int CheckMail(void)
{

	if (MailUser_and_pass.empty()) return -1;

	int mail = 0;
	if (strstr(MailUser_and_pass.c_str(), "@gmail") != NULL)
	{
		mail = check_gmail((char*)MailUser_and_pass.c_str());
	}
	else
	{
		mail = 0;// OpenMailServer(MailUser_and_pass);
	}

	return mail;
}

/******************************************************/

std::string GitHub_Account;
void SetGitHubUserPass(char *s)
{
	GitHub_Account = s;
}
int CheckGitHubNotification(void)
{
	if (GitHub_Account.empty()) return -1;

	return check_github((char *)GitHub_Account.c_str());
}
/******************************************************/
int RSS_Mem_lengh = 0;
std::string RSS_site;

void SetRSS_Site(char *s)
{
	RSS_site = s;
	RSSMonitor();
}

int RSSMonitor()
{
	int l;
	char *tmp;

	if (RSS_site.empty()) return -1;

	tmp = LectureWeb((char *)RSS_site.c_str());
	l = strlen(tmp);
	free(tmp);

	if (RSS_Mem_lengh == 0)
	{
		Mywprintf(L"Memorisation du RSS du site : %s\n", (char *)RSS_site.c_str());
		RSS_Mem_lengh = l;
	}
	else if (RSS_Mem_lengh != l)
	{
		return 1;
	}

	return 0;
}

/******************************************************/
//http://stackoverflow.com/questions/478898/how-to-execute-a-command-and-get-output-of-command-within-c-using-posix


#include <string>


std::string exec(const char* cmd) {

	std::string result = "";

#ifndef _WIN32
	char buffer[128];
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

void executesCommand(char *command)
{
	Mywprintf(L"Execution de la commande %s:\n", command);
#ifndef _WIN32
	system(command);
#endif
}

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
	ClearMusic();

	//char commande[400];
	SP();
	wprintf(L"\033[1;33mString to say : %ls\033[0;37m\n", texte);
	SP();

#ifndef _WIN32
	std::string commande;

	int l = wcslen(texte) + 2;
	//if the strng is too long, we cut it
	if (l > 400)
	{
		l = 400;
	}
	char *char_text = (char *)malloc(l*sizeof(char));
	wcstombs(char_text, texte, l);

	commande = "pico2wave -l=";

	if (GetLanguage() == 0) commande = commande + "fr-FR";
	else commande = commande + "en-EN";

	commande = commande + " -w=/dev/shm/test.wav \"<pitch level='100'><speed level='80'>" + char_text + "</speed></pitch>\"";

	free(char_text);

	//Daemon mode
	//commande = commande + " >/dev/null 2>&1 &";

	//Mywprintf(L"Commande : %s\n",commande.c_str());

	system(commande.c_str());
	//system("aplay /dev/shm/test.wav");
	PlayWave("/dev/shm/test.wav");
#endif

	return 1;
}


/***************************************************************************************/

void Savedata(void)
{
	//Not enabled yet
	return;

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

	pugi::xml_node nodeChild2;
	pugi::xml_node nodeChild;
		
	nodeChild = root.append_child("Remarque");
	nodeChild.append_child(pugi::node_pcdata).set_value("Laisser des XX a la place des valeurs pr les repetions, ex : XX/XX/XX/07/00 se declenchera tout les jours a 7h00");
		
    nodeChild2 = root.append_child("Alarm");

	// Append some child elements below root
	// Add as last element
	nodeChild = nodeChild2.append_child("Al");
	nodeChild.append_attribute("time") = "14/06/16/05/41";
	nodeChild.append_attribute("action") = "DIRE Test";

	nodeChild = nodeChild2.append_child("Al");
	nodeChild.append_attribute("time") = "XX/XX/XX/07/00";
	nodeChild.append_attribute("action") = "DIRE Test tout les jours";
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
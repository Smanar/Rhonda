#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <wchar.h>

#include "traitement.h"
#include "hardware.h"
#include "fonction.h"
#include "applications.h"
#include "prog.h"


//#define DEBUG_OUPUT

/***************************************************************************/

cTraitement::cTraitement()
{
	int i;
	NbreCommand = 0;
	CommandeL = NULL;
	ActionComL = NULL;
	CommandeSpecialL = NULL;

	for (i = 0; i < 50; i++) ListActions[i] = NULL;
}

cTraitement::~cTraitement()
{
	int i;
	if (CommandeL)
	{
		for (i = 0; i < NbreCommand; i++)
		{
			free(CommandeL[i]);
		}
		free(CommandeL);
	}
	if (CommandeSpecialL)
	{
		for (i = 0; i < NbreSpecial; i++)
		{
			free(CommandeSpecialL[i]);
			free(WordSpecialL[i]);
		}
		free(CommandeSpecialL);
		free(WordSpecialL);
	}
	for (i = 0; i < 50; i++)
	{
		if (ListActions[i]) free(ListActions[i]);
	}

	if (ActionComL) free(ActionComL);
}

void cTraitement::AddAction(char* data, int index)
{
	int len = strlen(data) + 1;

	if (index > 49) return;

	ListActions[index] = (char*)malloc(len * sizeof(char));

	strcpy(ListActions[index], data);
}

void cTraitement::AddCommand(char *s, int v)
{
	int l = strlen(s);
	CommandeL[comptcommande] = (char *)malloc((l +1) * sizeof(char));
	strncpy(CommandeL[comptcommande], s , l);
	CommandeL[comptcommande][l] = '\0';
	ActionComL[comptcommande] = v;
	comptcommande++;
}

void cTraitement::SetMaxCommand(int v)
{
	NbreCommand = v;

	CommandeL = (char **)malloc(v * sizeof(char*));
	ActionComL = (int *)malloc(v * sizeof(int));

	comptcommande = 0;
}

void cTraitement::AddCommandSpecial(char *c, char *w)
{
	int l = strlen(c);
	CommandeSpecialL[comptcommande] = (char *)malloc((l + 1) * sizeof(char));
	strncpy(CommandeSpecialL[comptcommande], c, l);
	CommandeSpecialL[comptcommande][l] = '\0';

	l = strlen(w);
	WordSpecialL[comptcommande] = (char *)malloc((l + 1) * sizeof(char));
	strncpy(WordSpecialL[comptcommande], w, l);
	WordSpecialL[comptcommande][l] = '\0';

	comptcommande++;
}

void cTraitement::SetMaxCommandSpecial(int v)
{
	NbreSpecial = v;
	CommandeSpecialL = (char **)malloc(v * sizeof(char*));
	WordSpecialL = (char **)malloc(v * sizeof(char*));

	comptcommande = 0;
}

void cTraitement::ReconconizeWord(char *wordchain, int *nbre, int *maxmot, char *commande)
{
	char *ptr = wordchain;
	char mot[255];
	int j;

	if (!ptr) return;

	while (*ptr != '\0')
	{
		j = 0;
		while ((j < 254) && (*ptr != ' ') && (*ptr != '\0'))
		{
			mot[j] = *ptr;
			ptr++;
			j++;
		}

		if (j == 0) return;

		mot[j] = '\0';

		if (*ptr != '\0')
			ptr++;

		/*Liste speciale ?*/
		if (mot[0] == '*')
		{
			int i;
			char *p=NULL;
			char* word;
			char* token;

			for (i = 0; i < NbreSpecial; i++)
			{
				if (strcmp(CommandeSpecialL[i], mot) == 0)
				{
					p = WordSpecialL[i];
					break;
				}
			}

			if (p == NULL) break;

			token = (char *)malloc((strlen(p) + 1) * sizeof(char));
			strcpy(token, p);
			for (word = strtok(token, " "); word; word = strtok(NULL, " "))
			{

				if (mystrstr(commande, word) != NULL)
				{
					*nbre = *nbre + 1;
#ifdef DEBUG_OUPUT
					Mywprintf(L"Mot reconnus : %s\n", mot);
#endif
				}
			}
			free(token);

		}
		else
		{
			/* Simple recherche de mot */
			if (mystrstr(commande, mot) !=  NULL)
			{
				*nbre = *nbre + 1;
#ifdef DEBUG_OUPUT
				Mywprintf(L"Mot reconnus : %s\n", mot);
#endif
			}
		}
		*maxmot = *maxmot + 1;
	}

	return;
}

void cTraitement::ManageAction(char *c)
{
	char * TokenCommand;
	char * Com;
	char * Key;
	char *Param;

	wchar_t wbuf[255];
	char buf[255];

	TokenCommand = (char *)malloc((strlen(c) + 1) * sizeof(char));
	strcpy(TokenCommand, c);

	Mywprintf(L"Action to do : %s\n", c);

	for (Com = strtok(TokenCommand, "|"); Com; Com = strtok(NULL, "|"))
	{
		int i = 0;
		int l = strlen(Com);

		//Pas d'espace en debut
		while (Com[0] == ' ') Com++;
		Key = Com;

		//On s'arrette a la fin du premier mot
		while ((i < l) && (Com[i] != ' ')) i++;
		Com[i] = '\0';
		Param = (&Com[i + 1]);

		//Debut des tests
		if (strcmp("DIRE", Key) == 0)
		{
			const size_t cSize = strlen(Param) + 1;
			wchar_t* wc = new wchar_t[cSize];
			mbstowcs(wc, Param, cSize);
			parle(wc);
			delete wc;
		}
		else if (strcmp("DIREHEURE", Key) == 0)
		{
			time_t crt = time(NULL);
			struct tm * timeinfo;
			timeinfo = localtime(&crt);
			wcsftime(wbuf, 254, L"Il est %H heure %M", timeinfo);
			parle(wbuf);
			}
		else if (strcmp("DIREDATE", Key) == 0)
		{
			time_t crt = time(NULL);
			struct tm * timeinfo;
			timeinfo = localtime(&crt);
			wcsftime(wbuf, 254, L"On est %A %d %B", timeinfo);
			parle(wbuf);
		}
		else if (strcmp("PROGRAMMETV", Key) == 0)
		{
			parle(L"A faire");
		}
		else if (strcmp("METEO", Key) == 0)
		{
			wchar_t Str[400];
			wchar_t tmp[255];
			GetMeteo(tmp);
			swprintf(Str, 400, L"La m\u00e9t\u00e9o de demain sera %S", tmp);
			parle(Str);
		}
		else if (strcmp("DEFINITION", Key) == 0)
		{
			wchar_t Str[500];
			char mot[50];
			wchar_t def[800];

			mot[49] = '\0';
			/* Mot a recuperer */
			if (GetWord(" ([a-zA-Z]+$)", commande, mot, 49))
			{
				wcsncpy(Str, L"Ce mot veut dire ", 500);

				GetDefinition(mot, def);

				wcscat(Str, def);
				parle(Str);
			}
			else
			{
				parle(L"Je n'ai pas compris le mot a d\u00e9finir");
			}
		}
		else if ((strcmp("RAPPELA", Key) == 0) || (strcmp("RAPPELDANS", Key) == 0) )
		{
			wchar_t Str[400];

			int temps = Findhour2(commande);

			if (temps)
			{
				char format[20];
				time_t crt = time(NULL);
				time_t crtAlarme;
				struct tm T;

				/* Rajoute du temps en minutes */
				if (strcmp("RAPPELDANS", Key) == 0) crtAlarme = crt + temps *60;
				else
				{
					struct tm *Al = localtime(&crt);

					if (Al->tm_hour > (temps / 60)) Al->tm_yday++;

					Al->tm_hour = (int)(temps / 60);
					Al->tm_min = temps - Al->tm_hour * 60;
					crtAlarme = mktime(Al);
				}
				
				T = *localtime(&crtAlarme);
				strftime(format, 20, "%y/%m/%d/%H/%M", &T);

				SetAlarm(format, "DIRE Alarme programm\u00e9e");
				ResetAlarm();

				strftime(buf, sizeof(buf), "%H heure et %M minute", localtime(&crtAlarme));
				Myswprintf(Str, 400, L"Alarme rajout\u00e9 pour %s", (wchar_t *)buf);
				parle(Str);
			}
			else
			{
				parle(L"Je n'ai pas compris l'heure a retenir");
			}
		}
		else if (strcmp("CHECKMAIL", Key) == 0)
		{
			wchar_t Str[400];
			int m = 0;
			m = CheckMail();

			if (m == -1)
			{
				parle(L"probleme de configuration");
				break;
			}
			//if nothing and silence needed, say nothing
			if ((strstr(Param, "0") != NULL) && (m == 0)) break;
			
			swprintf(Str, 400, L"Vous avez %i nouveaux m\u00e8ils", m);
			parle(Str);
			
		}
		else if (strcmp("CHECKGITHUB", Key) == 0)
		{
			wchar_t Str[400];
			int m = 0;
			m = CheckGitHubNotification();

			if (m == -1)
			{
				parle(L"probleme de configuration");
				break;
			}
			//if nothing and silence needed, say nothing
			if ((strstr(Param, "0") != NULL) && (m == 0)) break;


			swprintf(Str, 400, L"Vous avez %i nouvelles notifications", m);
			parle(Str);

		}
		else if (strcmp("CHECKRSS", Key) == 0)
		{
			int m = 0;
			m = RSSMonitor();
			if (m == -1)
			{
				parle(L"probleme de configuration");
				break;
			}

			//if nothing and silence needed, say nothing
			if ((strstr(Param, "0") != NULL) && (m == 0)) break;

			else if (m == 1)
			{
				parle(L"Le site a chang\u00e9");
			}
			else
			{
				parle(L"Le site n'a re\u00e7u aucunes modifications");
			}
			
		}
		else if (strcmp("MUSIQUE", Key) == 0)
		{
			if (strcmp("ON", Param) == 0)
			{
				parle(L"D\u00e9marrage de la radio");
				PlayMusic(true);
			}
			else
			{
				parle(L"Arret de la radio");
				PlayMusic(false);
			}
		}
		else if (strcmp("CINEMA", Key) == 0)
		{
			wchar_t Str[400];
			wchar_t Liste_film[300];

			wcscpy(Str, L"Les films de la semaine sont ");

			GetFilmCinema(Liste_film,300);

			wcscat(Str, Liste_film);
			parle(Str);
			
		}
		else if (strcmp("SENDREQUEST", Key) == 0)
		{
			SendRequest(Param);
		}
		else if (strcmp("SHELLEXECUTE", Key) == 0)
		{
			executeshell(Param);
		}
		else if (strcmp("SHELLLIRE", Key) == 0)
		{
			lireshell(Param);
		}
		else if (strcmp("SHOWICON", Key) == 0)
		{
			_DisplayIcone(atoi(Param));
		}
		else if (strcmp("TRANSMITTER", Key) == 0)
		{
			int pin;
			int sender;
			int interuptor;
			char *tmp = Param;

			while (*Param != ' ') Param++;
			*Param = '\0';
			pin = atoi(tmp);
			tmp = Param + 1;
			while (*Param != ' ') Param++;
			*Param = '\0';
			sender = atoi(tmp);
			tmp = Param + 1;
			while (*Param != ' ') Param++;
			*Param = '\0';
			interuptor = atoi(tmp);
			tmp = Param + 1;

			TestTransmitter(pin, sender, interuptor, tmp);

		}
		else if (strcmp("EXIT", Key) == 0)
		{
			Exit();
		}
		else
		{
			parle(GetCommonString(1)); // = "je n'ai pas compris" ?
		}

	}

	free(TokenCommand);

}

void cTraitement::action(int choix)
{
	if (ListActions[choix])
		ManageAction(ListActions[choix]);
	else
		parle(GetCommonString(1)); // = "je n'ai pas compris" ?
}

int cTraitement::traite(char *commande2)
{

	int i;
	char *ptr;

	int Betterscore = 0;
	int choix = 0;

	//limite a 255 caractere
	if (strlen(commande2) > 254) commande2[254] = '\0';

	UnicodeToAnsi(commande2, commande);

	commande[strlen(commande) + 1] = '\0';
	CleanCommand(commande);

	SP();
	Mywprintf(L"\033[0;31mString recognized : %s\033[0;37m\n", commande);
	SP();

	for (i = 0; i < NbreCommand; i++)
	{

		char *chainWord;
		int maxmot;
		int NbrePrioritaire, NbrePossible, NbreOption, NbreInterdit;
		int maxmot2;
		int mottotal;
		char Listcommand2[255]; //need a copy for strtok


		ptr = CommandeL[i];

#ifdef DEBUG_OUPUT
		Mywprintf(L"Test chaine : %s\n", ptr);
#endif

		maxmot = 0;
		NbrePrioritaire = 0;
		NbreOption = 0;
		NbreInterdit = 0;
		NbrePossible = 0;

		maxmot2 = 0;
		mottotal = 0;

		//On commence par les mots obligatoire
		strncpy(Listcommand2, ptr, 255);
		chainWord = strtok(Listcommand2, "/");
		ReconconizeWord(chainWord, &NbrePrioritaire, &maxmot, commande);
		//wprintf(L"Test obligatoire : %i / %i\n",NbrePrioritaire,maxmot);
		//Si il n y sont pas, meme pas la peine de continuer
		if (NbrePrioritaire < maxmot)
			continue;
		mottotal += NbrePrioritaire;

		//les mots possibles
		chainWord = strtok(NULL, "/");
		ReconconizeWord(chainWord, &NbrePossible, &maxmot, commande);
		//wprintf(L"Test possible : %i / %i\n",NbrePossible,maxmot);
		mottotal += NbrePossible;

		//option
		chainWord = strtok(NULL, "/");
		ReconconizeWord(chainWord, &NbreOption, &maxmot, commande);
		//wprintf(L"Test option : %i / %i\n",NbreOption,maxmot);
		mottotal += NbreOption;

		//interdit
		chainWord = strtok(NULL, "/");
		ReconconizeWord(chainWord, &NbreInterdit, &maxmot2, commande);
		//wprintf(L"Test interdit : %i / %i\n",NbreInterdit,maxmot2);
		mottotal -= NbreInterdit;
		maxmot -= maxmot2;

		if (mottotal > Betterscore)
		{
			Mywprintf(L"\033[0;31mCorrespondence found for chain : %s\033[0;37m\n", ptr);
			Betterscore = mottotal;
			choix = ActionComL[i];
		}

	}

	if (choix == 0)
	{
		_DisplayIcone(INTERROGATION);
		parle(GetCommonString(1)); // = "je n'ai pas compris" ?
	}
	else
	{
		_DisplayIcone(SMILEY);
		action(choix);
	}

	return 0;
}

//vire accents/pluriels
void cTraitement::CleanCommand(char* str)
{
	//               "ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖ×ØÙÚÛÜÝÞßàáâãäåæçèéêëìíîïðñòóôõö÷øùúûüýþÿ"
	const char* tr = "AAAAAAECEEEEIIIIDNOOOOOx0UUUUYPsaaaaaaeceeeeiiiiOnooooo/0uuuuypy";

	unsigned char ch;

	char *pw = str;
	char *pr = str;

	while ((*pr) != 0)
	{
		ch = (*pr);
		if (ch == 195)
		{
			++pr;
			ch = (*pr);
			ch = tr[ch - 127];
		}

		if (ch >= 192) {
			ch = tr[ch - 192];
		}
		if (ch == '-') ch = ' ';
		if (ch == '\'') ch = ' ';
#if 0
		if ( (ch == 's') && (*(pr +1) == ' ') )
		{
			ch = ' ';
			++pr;
		}
#endif
		(*pw) = ch;
		++pw;
		++pr;
	}

	(*pw) = '\0';

	return;

}


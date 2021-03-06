/*

No comment yet

*/

#ifdef _WIN32
#include <windows.h>
#define _CRTDBG_MAP_ALLOC #include <stdlib.h> #include <crtdbg.h>
#endif

/* Special options*/
//#define DEBUG
#define SNOWBOY


#include <cassert>
#include <csignal>
#include <iostream>
#include <pa_ringbuffer.h>
#include <pa_util.h>
#include <portaudio.h>
#include <string>
#include <vector>


#ifdef SNOWBOY
	#include "libs/snowboy-detect.h"
#else
    #include "uart.h"
#endif

#include "prog.h"

#include "audio.h"
#include "hardware.h"
#include "flac.h"
#include "STTEngine.h"
#include "traitement.h"
#include "fonction.h"
#include "applications.h"


#include "libs/pugixml.hpp"

/***********************************************************************/

// Parameter section for Snowboy.
// If you have multiple hotword models (e.g., 2), you should set
// <model_filename> and <sensitivity_str> as follows:
//   model_filename = "resources/snowboy.umdl,resources/alexa.pmdl";
//   sensitivity_str = "0.4,0.4";
std::string resource_filename = "resources/common.res";
std::string model_filename = "resources/snowboy.umdl";
std::string sensitivity_str = "0.5";
float audio_gain = 1;
int HotWordModel = 1;

/***********************************************************************/
//  Global variables
bool bExit = false;
int language = 0; // 0 = FR 1 = EN
int STTEngine = 0; // 0 = google  -  1 = Bing
/***********************************************************************/



//Initailisation classes
class cTraitement cTraitement;
class cMatrixLed cMatrixLed;
class cPlay cPlay;
class cRecord cRecord;


#ifdef _WIN32
#include <conio.h>

#else

#endif



int main(int argc, char* argv[]) {

	char Resultat[255];
	int err = 0;
	int cycle = 0; //compteur de cycle
	int anim = 0; // compteur pr animation

	/* For sound engine */
	std::vector<int16_t> data;
	int Samplerate = 44100;
	int Numchannel = 1;
	int Bitpersample = 16;

	bool HotWord = false;

	size_t size_soundbuffer;
	char *buff_soundbuffer = NULL;

	Resultat[0] = '\0';

	/************************************************/


	if (argc > 1)
	{
		wprintf(L"\033[0;32mPassage d'arguments\033[0;37m\n");
		if (argv[1][1] = 't')
		{
			wprintf(L"Forcage transmetter eg -t 2 1478162 0 on\n");
			TestTransmitter(atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), argv[5]);
		}
		else
		{
			Mywprintf(L"Non compris : %s\n", argv[1]);
		}
		return 0;
	}


	//load config
	if (!LoadConfig()) return 0;


#ifndef _WIN32
#ifdef SNOWBOY

	wprintf(L"Snowboy sensivity %s\n", sensitivity_str.c_str());
	wprintf(L"Snowboy modele %s\n", model_filename.c_str());

	if ((int)(model_filename.find(",")) > 0)
	{
		wprintf(L"Snowboy will works with 2 hotwords mode\n");
		HotWordModel = 2;
	}

	// Initializes Snowboy detector.A faire avant setlocale sinon bug !!
	snowboy::SnowboyDetect detector(resource_filename, model_filename);
	detector.SetSensitivity(sensitivity_str);
	detector.SetAudioGain(audio_gain);

	Samplerate = detector.SampleRate();
	Numchannel = detector.NumChannels();
	Bitpersample = detector.BitsPerSample();

	wprintf(L"Samplerate %i Numchannel %i, Bitpersampple %i\n", Samplerate, Numchannel, Bitpersample);
#else
	//Enable UART
	printf("Initialising UART\n");
	err = InitUART();
	if (err == 0) return 0;
#endif


#else
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif


	//Works on some Pi but not all ?????
	//if (setlocale(LC_ALL, "fr_FR") == NULL) setlocale(LC_ALL, "fr_FR.utf8");
	setlocale(LC_ALL, "fr_FR.utf8");

	
	// Initializes PortAudio wrapper for snowboy. You may use other tools to capture the audio.
	PortAudioWrapper pa_wrapper(Samplerate, Numchannel, Bitpersample);
	if (!(pa_wrapper.ready))
	{
		wprintf(L"Audio problem for snowboy\n");
	}


#ifdef DEBUG
	//to debug
	//TranslateGoggle("vvv", Resultat);
	//TestTransmitter(0,12325261,1,"on");
	//parle(L"test m\u00e9t\u00e9o");

	//cTraitement.traite("recherche le fichier test");

	//CheckGitHubNotification();
#if 0
	{
		char *source = NULL;
		FILE *fp;
		long bufsize;
		size_t newLen;
		char res[255];

		fp = fopen("c:\\testfile.wav", "rb");
		fseek(fp, 0L, SEEK_END);
		bufsize = ftell(fp);

		source = (char*)malloc(sizeof(char) * (bufsize + 1));

		fseek(fp, 0L, SEEK_SET);
		newLen = fread(source, sizeof(char), bufsize, fp);

		fclose(fp);

		TranslateBing(source, newLen, res);
	}
#endif
	bExit = true;

#endif
	 
	LoadData();
	ResetAlarm();

	/****************************************************/
	/***********             Main loop                 **/
	/****************************************************/

	SP();

	wprintf(L"\033[0;32mStarting Main loop\033[0;37m\n");

	while (!bExit)
	{

		/*Waiting loop*/
		Mywprintf(L"\033[1;37m%s => Waiting for trigger\033[0;37m\n", timestamp());

		HotWord = false;
		pa_wrapper.Start();


		while (!HotWord)
		{
			cycle = cycle + 1;

			//Animation
			if (cycle % 10 == 0)
			{
				anim = 1 - anim;
				if (anim == 1) cMatrixLed.DisplayIcone(ALIEN1);
				else cMatrixLed.DisplayIcone(ALIEN2);
			}

#ifndef _WIN32
#ifdef SNOWBOY
			pa_wrapper.Read(&data);
		
			if (data.size() != 0) {
				int result = detector.RunDetection(data.data(), data.size());
				if (result == 1) {
					int vide = 10;
					wprintf(L"Hotword detected %d\n",result);

					ClearMusic();

					PlayWave("resources/ding.wav");

					if (HotWordModel == 1)
					{
						HotWord = true;
					}
					else
					{

						cMatrixLed.DisplayIcone(SMILEY);
						//Now we wait for the second hotword
						while (vide > 0)
						{
							pa_wrapper.Read(&data);
							result = detector.RunDetection(data.data(), data.size()); // =-2 if silence
							if (result == 2)
							{
								wprintf(L"Hotword detected %d\n",result);
								HotWord = true;
								break;
							}
							Wait(100);
							vide--;
						}
					}
					if (!HotWord) cMatrixLed.DisplayIcone(ALIEN1);
				}
			}
#else
			if (GetUART())
			{
				HotWord = true;
			}
#endif
#else		
			//NOT PORTABLE, JUST TO DEBUG
			if (kbhit())
			{
				HotWord = true;
			}
#endif

			//Event Verification, I don't use special thread to prevent multiples actions in same time
			if (cycle % 800 == 0) Checkalarm();

			//Alerte programmee

			//Temporisation pr processus
			Wait(100);

		}

		Mywprintf(L"%s Trigger activated\n", timestamp());

		pa_wrapper.Stop();

		parle(GetCommonString(0)); // = oui ?
		//Wait(800);

		wprintf(L"Recording\n");
		SP();

		err = 0;
		size_soundbuffer = 0;
		
		//Google STT engine
		if (STTEngine == 0)
		{
			size_t size_tmpbuffer = 0;
			char *buff_tmp = NULL;

			/*   Recording sound and convert it to flac file   */
			buff_tmp = cRecord.RecordSound(5, &size_tmpbuffer);

			if ((!buff_tmp) || (size_tmpbuffer == 0))
			{
				err = 0;
				break;
			}

			//convert wav buffer to flac buffer
			printf("Sound recorded, convertion to flac\n");
			buff_soundbuffer = ConvertWavBufferToFlacBuffer(buff_tmp, size_tmpbuffer, &size_soundbuffer);

			if (buff_tmp) free(buff_tmp);

			if ((buff_soundbuffer) && (size_soundbuffer > 0))
			{

				wprintf(L"Send to google\n");
				cMatrixLed.DisplayIcone(SABLIER);

				err = TranslateGoggle(buff_soundbuffer, size_soundbuffer, Resultat);
			}
		}
		//Bing STT engine
		else if (STTEngine == 1)
		{
			/*   Recording sound  */
			buff_soundbuffer = cRecord.RecordSound(5, &size_soundbuffer);

			if ((buff_soundbuffer) && (size_soundbuffer > 0))
			{
				wprintf(L"Send to Bing\n");
				cMatrixLed.DisplayIcone(SABLIER);

				err = TranslateBing(buff_soundbuffer, size_soundbuffer, Resultat);
			}
		}

		if (buff_soundbuffer != NULL)
		{
     		free(buff_soundbuffer);
			buff_soundbuffer = NULL;
		}

		if (err > 0)
		{
			SP();
			wprintf(L"Resultat with a score of (%d) : ", err);
			Mywprintf(L"%s\n", Resultat);
			SP();

			if (err != 0)
			{
				wprintf(L"Processing\n");
				cTraitement.traite(Resultat);
			}
			else
			{
				cMatrixLed.DisplayIcone(INTERROGATION);
				parle(GetCommonString(1)); // = "je n'ai pas compris" ?
			}
		}

#ifdef _WIN32
		bExit = true;
#endif

	}

	Savedata();

	wprintf(L"Exiting\n");

#ifdef _WIN32
	system("pause");
#endif

	return 0;

}






/****************************************************/
//http://www.dreamincode.net/forums/topic/244725-how-to-parse-xml-in-c/
//https://purusothamana.wordpress.com/2011/06/03/pugi-a-simple-xml-parser-api-from-google-labs/

#include <iostream>
bool LoadConfig(void)
{

	int max;

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file("config.xml");
	wprintf(L"\033[0;31mLoading config XML %s\033[0;37m\n", result.description());
	if (result.status != 0) return false;

	pugi::xml_node panels = doc.child("mesh");

	//config
	SetSTTApiKey((char *)panels.child("config").child_value("api"));
	SetSTTMode(atoi((char *)panels.child("config").child_value("STTMode")));
	SetCity((char *)panels.child("config").child_value("ville"));
	SetLanguage((char *)panels.child("config").child_value("language"));
	SetMailUserPass((char *)(panels.child("config").child_value("mailuser_and_pass")));
	SetGitHubUserPass((char *)(panels.child("config").child_value("githubaccount")));
	SetRSS_Site((char *)(panels.child("config").child_value("RSS_Site")));

	//sound engine
	sensitivity_str = std::string(panels.child("sound_engine").child_value("sensibility"));
	model_filename = std::string(panels.child("sound_engine").child_value("model"));
	AudioRecordConfig(atoi((char *)(panels.child("sound_engine").child_value("Gain_record"))), atoi((char *)(panels.child("sound_engine").child_value("Min_amplitude"))));


	//command list
	max = std::distance(panels.child("commandlist").begin(), panels.child("commandlist").end());
	cTraitement.SetMaxCommand(max);
	for (pugi::xml_node panel = panels.child("commandlist").first_child(); panel; panel = panel.next_sibling())
	{
		//security
		if (max < 0) break;
		max--;

		cTraitement.AddCommand((char *)panel.attribute("command").value(), atoi((char *)panel.attribute("action").value()));
	}

	//Special dictionnary
	max = std::distance(panels.child("special").begin(), panels.child("special").end());
	cTraitement.SetMaxCommandSpecial(max);
	for (pugi::xml_node panel = panels.child("special").first_child(); panel; panel = panel.next_sibling())
	{
		//security
		if (max < 0) break;
		max--;

		cTraitement.AddCommandSpecial((char *)panel.attribute("command").value(), (char *)panel.attribute("word").value());
	}

	//Icons for matrix
	for (pugi::xml_node panel = panels.child("matrixicon").first_child(); panel; panel = panel.next_sibling())
	{
		cMatrixLed.MakeIcon(atoi(panel.attribute("index").value()), (char *)panel.attribute("data").value());
	}

	//list of possibles actions
	for (pugi::xml_node panel = panels.child("action").first_child(); panel; panel = panel.next_sibling())
	{
		cTraitement.AddAction((char *)panel.attribute("action").value(), atoi((char *)panel.attribute("index").value()));
	}

	//Programmable event, repetive only
	for (pugi::xml_node panel = panels.child("Event").first_child(); panel; panel = panel.next_sibling())
	{
		SetAlarm((char *)panel.attribute("time").value(), (char *)panel.attribute("action").as_string());
	}

	//Common string
	for (pugi::xml_node panel = panels.child("commonstring").first_child(); panel; panel = panel.next_sibling())
	{
		SetCommonString(atoi(panel.attribute("id").value()), (char *)panel.attribute("string").as_string());
	}

	return true;
}



/***************************************************************************/

/* fonctions to use main fonction on other part */

int _DisplaySpectro(int val)
{
	return cMatrixLed.DisplaySpectro(val);
}
int _DisplayIcone(int val)
{
	return cMatrixLed.DisplayIcone(val);
}

int PlayWave(char * file)
{
	return cPlay.PlayWav(file);
}

int ManageEvent(char* c)
{
	cTraitement.ManageAction(c);
	return true;
}

void Exit(void)
{
	bExit = true;
}

int GetLanguage(void)
{
	return language;
}
void SetLanguage(char *s)
{
	if (strcmp("FR", s) == 0)  language = 0;
	else if (strcmp("fr", s) == 0)  language = 0;
	else language = 1;
}

#define MAXSTRING 2
std::wstring CommonString[MAXSTRING];
void SetCommonString(int index,char *s)
{
	const size_t cSize = strlen(s) + 1;
	wchar_t* wc;

	if ((index < 0) || (index > MAXSTRING)) return;

	wc = new wchar_t[cSize];
	mbstowcs(wc, s, cSize);
	CommonString[index] = wc;
	free(wc);
}
wchar_t * GetCommonString(int index)
{
	if ((index < 0) || (index > MAXSTRING)) return L"";
	return (wchar_t*)CommonString[index].c_str();
}

void SetSTTMode(int v)
{
	STTEngine = v;
	if (STTEngine > 1) STTEngine = 0;

	if (STTEngine == 0)
	{
		wprintf(L"Use Google STT\n");
	}
	else if (STTEngine == 1)
	{
		wprintf(L"Use Bing STT\n");
		cRecord.SetSampleRate(8820);
		//cRecord.SetSampleRate(44100);
	}
}
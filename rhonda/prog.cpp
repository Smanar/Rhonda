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



#include "audio.h"



#include "hardware.h"
#include "flac.h"
#include "translategoogle.h"
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

//Initailisation classe
class cTraitement cTraitement;
class cMatrixLed cMatrixLed;
class cPlay cPlay;


#ifdef _WIN32
#include <conio.h>
char RamTmpFile[] = "testfile.flac";
#else
char RamTmpFile[] = "/dev/shm/testfile.flac";
#endif


bool LoadConfig(void);
int PlayWave(char * file);


bool bExit = false;
void Exit(void)
{
	bExit = true;
}


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

	Resultat[0] = '\0';

	//load config
	if (!LoadConfig()) return 0;


#ifndef _WIN32
#ifdef SNOWBOY

	wprintf(L"Snowboy sensivity %s\n", sensitivity_str.c_str());
	wprintf(L"Snowboy modele %s\n", model_filename.c_str());

	if ((int)(model_filename.find(",")) > 0) HotWordModel = 2;

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

	if (setlocale(LC_ALL, "fr_FR") == NULL) setlocale(LC_ALL, "fr_FR.utf8");


#ifdef DEBUG
	//to debug

	//TestTransmitter(0,12325261,1,"on");
	//parle(L"test m\u00e9t\u00e9o");
	cTraitement.traite("testes le fichier de commande");
#ifdef _WIN32
	system("pause");
#endif
	return 0;
#endif

	/****************************************************/

	//Init Audio engine
	wprintf(L"Initialising audio\n");
	class cRecord cRecord;
	// Initializes PortAudio. You may use other tools to capture the audio.
	PortAudioWrapper pa_wrapper(Samplerate, Numchannel, Bitpersample);
	if (!(pa_wrapper.ready))
	{
		wprintf(L"Audio problem for snowboy\n");
#ifndef _WIN32
		return 0;
#endif
	}

	/************************************************/


	//Main loop

	wprintf(L"\033[0;31mStarting Main loop\033[0;37m\n");

	while (!bExit)
	{

		/*Waiting loop*/
		Mywprintf(L"\033[0;31m%s => Waiting for trigger\033[0;37m\n", timestamp());

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
				if (result > 0) {
					int vide = 10;
					wprintf(L"Hotword detected %d\n",result);

					if (HotWordModel == 1)
					{
						HotWord = true;
						PlayWave("resources/ding.wav");
					}
					else
					{

						cMatrixLed.DisplayIcone(SMILEY);
						//Now on attend le second hotword
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

			//Verification alarme
			if (cycle % 800 == 0) Checkalarm();

			//Alerte programmee

			//Temporisation pr processus
			Wait(100);

		}

		Mywprintf(L"%s Trigger declenche\n", timestamp());

		pa_wrapper.Stop();

		parle(L"Oui");
		//Wait(800);

		wprintf(L"Recording\n");

		/*   /dev/shm is ram folder   */
		err = cRecord.RecordFLAC(RamTmpFile, 5);
		//err = 0;
		if (err != 0)
		{

			wprintf(L"Send to google\n");
			cMatrixLed.DisplayIcone(SABLIER);
			err = TranslateGoggle(RamTmpFile, Resultat);

			wprintf(L"***********************************\n");
			wprintf(L"Resultat avec un score de (%d): %s\n", err , Resultat);
			wprintf(L"***********************************\n");

			if (err != 0)
			{
				cMatrixLed.DisplayIcone(SMILEY);
				wprintf(L"Traitement\n");
				cTraitement.traite(Resultat);

				wprintf(L"Done\n");

			}
			else
			{
				cMatrixLed.DisplayIcone(INTERROGATION);
				parle(L"Je n'ai pas compris");
			}
		}

#ifdef _WIN32
		bExit = true;
		system("pause");
#endif

	}

	wprintf(L"Exiting\n");

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
	wprintf(L"\033[0;31mLoading config XML %s\n\033[0;37m\n", result.description());
	if (result.status != 0) return false;

	pugi::xml_node panels = doc.child("mesh");

	//config
	SetGoogleApiKey((char *)panels.child("config").child_value("api"));
	SetCity((char *)panels.child("config").child_value("ville"));
	sensitivity_str = std::string(panels.child("config").child_value("sensibility"));
	model_filename = std::string(panels.child("config").child_value("model"));


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

	return true;
}

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

/*******************************************/

#if 0
int main2(int argc, char* argv[]) {
	/* For sound engine */
	std::vector<int16_t> data;
	int Samplerate = 44100;
	int Numchannel = 1;
	int Bitpersample = 16;

	if (!LoadConfig()) return 0;

	wprintf(L"Snowboy sensivity %s\n", sensitivity_str.c_str());
	wprintf(L"Snowboy modele %s\n", model_filename.c_str());

	if ((int)(model_filename.find(",")) > 0) HotWordModel = 2;

	// Initializes Snowboy detector.A faire avant setlocale sinon bug !!
	snowboy::SnowboyDetect detector(resource_filename, model_filename);
	detector.SetSensitivity(sensitivity_str);
	detector.SetAudioGain(audio_gain);

	Samplerate = detector.SampleRate();
	Numchannel = detector.NumChannels();
	Bitpersample = detector.BitsPerSample();

	wprintf(L"Samplerate %i Numchannel %i, Bitpersampple %i\n", Samplerate, Numchannel, Bitpersample);

	PortAudioWrapper pa_wrapper(Samplerate, Numchannel, Bitpersample);
	if (!(pa_wrapper.ready))
	{
		wprintf(L"Audio problem for snowboy\n");
	}

	while (true)
	{
		pa_wrapper.Read(&data);

		if (data.size() != 0) {
			int result = detector.RunDetection(data.data(), data.size());
			wprintf(L"xxx %d\n", result);
			if (result > 0) {
				wprintf(L"Hotword detected %d\n", result);
			}
		}
	}
}
#endif
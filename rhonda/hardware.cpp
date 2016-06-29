/*
FROM

my2cset.c - First try at controlling Adafruit 8x8matrix.
Mark A. Yoder, 14-Aug-2012.
Page numbers are from the HT16K33 manual
http://www.adafruit.com/datasheets/ht16K33v110.pdf

i2cset.c - A user-space program to write an I2C register.
Copyright (C) 2001-2003 Frodo Looijaard <frodol@dds.nl>, and
Mark D. Studebaker <mdsxyz123@yahoo.com>
Copyright (C) 2004-2010 Jean Delvare <khali@linux-fr.org>

*/

#define BRIGHTNESS   15 // 0=min, 15=max
#define I2C_ADDR   "0x70" // Edit if backpack A0/A1 jumpers set
#define I2C_PORT   "1" // Edit if backpack A0/A1 jumpers set


#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

#include "hardware.h"


static int check_funcs(int file, int size);
int ClearMatrix(void);

#ifdef _WIN32
#include <windows.h>
#define I2C_SMBUS_BLOCK_MAX 32
#else
#include <unistd.h>
#ifdef __cplusplus
extern "C" {
#endif
#include "libs/i2c-dev.h"
#include "libs/i2cbusses.h"
#ifdef __cplusplus
	}
#endif
#endif



unsigned short int block[I2C_SMBUS_BLOCK_MAX*2];


//http://blog.riyas.org/2013/12/online-led-matrix-font-generator-with.html


cMatrixLed::cMatrixLed(void)
{
	int i;

	Ready = false;

	wprintf(L"\033[0;31mInitialise matrix\033[0;37m\n");
	InitMatrix();

	for (i = 0; i < 50; i++) DataIcon[i] = NULL;

}

cMatrixLed::~cMatrixLed(void)
{
	int i;
	for (i = 0; i < 50; i++)
	{
		if (DataIcon[i]) free(DataIcon[i]);
	}

	CloseMatrix();
}
#if 0
short int rolb(short int v)
{
	v = v & 0xff;
	v = (v << 1) | (v >> 7);
	return v & 0xff;
}
#endif
bool cMatrixLed::MakeIcon(int index,char* datar)
{
	DataIcon[index] = (unsigned short int*)malloc(8 * sizeof(short int));

	int x,y;
	char *p = datar;
	short int val;
	char r;

	if (index > 50) return false;

	for (y = 7; y >= 0; y--)
	{
		val = 0;
		for (x = 0; x < 8; x++)
		{
			r = (*p);
			val = val << 1;

			if (r == 'G') val = val + 1;
			if (r == 'R') val = val + 256;
			if (r == 'Y') val = val + 257;
			//if (r != '0') val = val + 1;
			
			p++;
		}
		//need 1 more rollbit to be compatible
		//val = rolb(val) | (rolb((val & 0xff00) >> 8) << 8);

		DataIcon[index][y] = val;
		p++;
	}

	return true;
}


int cMatrixLed::displayImage(unsigned short int bmp[], int res, int daddress, int file)
{

#ifndef _WIN32

	int i;

	if (!Ready) return false;

	for(i=0; i<8; i++)
    {
		//val = (bmp[i]&0xfe) >> 1 | (bmp[i]&0x01) << 7 | (bmp[i]&0xfffe) >> 1 | (bmp[i]&0x0100) << 7  ;
		//block[i] = val;
		block[i] = bmp[i];
    }
	res = i2c_smbus_write_i2c_block_data(file, daddress, 16,(unsigned char *)block);

#endif
	return true;
}


int cMatrixLed::InitMatrix(void)
{
#ifndef	_WIN32

	//int res, i2cbus, address, size, file;
	int value = 0, daddress;
	char filename[20];
	int force = 0;
	//__u16 block[I2C_SMBUS_BLOCK_MAX];
	int len = 0;

	i2cbus = lookup_i2c_bus(I2C_PORT);
	wprintf(L"i2cbus = %d\n", i2cbus);
	if (i2cbus < 0) return false;

	address = parse_i2c_address(I2C_ADDR);
	wprintf(L"address = 0x%2x\n", address);
	if (address < 0) return false;

	size = I2C_SMBUS_BYTE;

	daddress = 0x21;
	if (daddress < 0 || daddress > 0xff) {
		wprintf(L"Error: Data address invalid!\n");
		return false;
	}

	file = open_i2c_dev(i2cbus, filename, sizeof(filename), 0);
	wprintf(L"file = %d\n", file);
	if (file < 0 || check_funcs(file, size) || set_slave_addr(file, address, force)) return false;


	switch (size)
	{
		case I2C_SMBUS_BYTE:
			daddress = 0x21; // Start oscillator (page 10)
			//printf("writing: 0x%02x\n", daddress);
			res = i2c_smbus_write_byte(file, daddress);

			daddress = 0x81; // Display on, blinking off (page 11)
			//printf("writing: 0x%02x\n", daddress);
			res = i2c_smbus_write_byte(file, daddress);

			daddress = 0xe0 | BRIGHTNESS; // brightness (page 15)
			//printf("writing: 0x%02x\n", daddress);
			res = i2c_smbus_write_byte(file, daddress);

			//clear matrice
			//res = i2c_smbus_write_byte(file, 0x20);

			daddress = 0x00; // Start writing to address 0 (page 13)
			//printf("writing: 0x%02x\n", daddress);
			res = i2c_smbus_write_byte(file, daddress);

			break;

		case I2C_SMBUS_WORD_DATA:
			res = i2c_smbus_write_word_data(file, daddress, value);
			break;
		case I2C_SMBUS_BLOCK_DATA:
			res = i2c_smbus_write_block_data(file, daddress, len, (const __u8 *)block);
			break;
		case I2C_SMBUS_I2C_BLOCK_DATA:
			res = i2c_smbus_write_i2c_block_data(file, daddress, len, (const __u8 *)block);
			break;
		default: /* I2C_SMBUS_BYTE_DATA */
			res = i2c_smbus_write_byte_data(file, daddress, value);
			break;
	}

	if (res < 0) {
		wprintf(L"Error: Write failed\n");
		close(file);
		return false;
	}

#if 0
	switch (size) {
		case I2C_SMBUS_BYTE:
			res = i2c_smbus_read_byte(file);
			value = daddress;
			break;
		case I2C_SMBUS_WORD_DATA:
			res = i2c_smbus_read_word_data(file, daddress);
			break;
		default: /* I2C_SMBUS_BYTE_DATA */
			res = i2c_smbus_read_byte_data(file, daddress);
		}
#endif

#endif
	//ClearMatrix();

	Ready = true;

	return true;

}

int cMatrixLed::ClearMatrix(void)
{

#ifndef _WIN32

		int daddress = 0x20;

		if (!Ready) return false;

		res = i2c_smbus_write_byte(file, daddress);
#endif
		return true;
}


int cMatrixLed::CloseMatrix(void)
{
	// Closing file and turning off Matrix
	wprintf(L"\033[0;31mClosing file and turning off the LED Matrix\033[0;37m\n");

#ifndef _WIN32

	int daddress = 0x20;		
	//for(daddress = 0xef; daddress >= 0xe0; daddress--) {
	//printf("writing: 0x%02x\n", daddress);
	res = i2c_smbus_write_byte(file, daddress);

	close(file);
#endif

	return true;
}


static int check_funcs(int file, int size)
{
#ifndef _WIN32

	unsigned long funcs;

	/* check adapter functionality */
	if (ioctl(file, I2C_FUNCS, &funcs) < 0) {
		fprintf(stderr, "Error: Could not get the adapter "
		"functionality matrix: %s\n", strerror(errno));
		return -1;
	}

	switch (size) {
	case I2C_SMBUS_BYTE:
		if (!(funcs & I2C_FUNC_SMBUS_WRITE_BYTE)) {
			fprintf(stderr, MISSING_FUNC_FMT, "SMBus send byte");
			return -1;
		}
		break;

	case I2C_SMBUS_BYTE_DATA:
		if (!(funcs & I2C_FUNC_SMBUS_WRITE_BYTE_DATA)) {
			fprintf(stderr, MISSING_FUNC_FMT, "SMBus write byte");
			return -1;
		}
		break;

	case I2C_SMBUS_WORD_DATA:
		if (!(funcs & I2C_FUNC_SMBUS_WRITE_WORD_DATA)) {
			fprintf(stderr, MISSING_FUNC_FMT, "SMBus write word");
			return -1;
		}
		break;

	case I2C_SMBUS_BLOCK_DATA:
		if (!(funcs & I2C_FUNC_SMBUS_WRITE_BLOCK_DATA)) {
			fprintf(stderr, MISSING_FUNC_FMT, "SMBus block write");
			return -1;
		}
		break;
	case I2C_SMBUS_I2C_BLOCK_DATA:
		if (!(funcs & I2C_FUNC_SMBUS_WRITE_I2C_BLOCK)) {
			fprintf(stderr, MISSING_FUNC_FMT, "I2C block write");
			return -1;
		}
		break;
	}
#endif

	return 0;
}

int cMatrixLed::DisplayIcone(int icon)
{

#ifndef _WIN32

	int daddress;
	daddress = 0x00; // Start writing to address 0 (page 13)

	if (!Ready) return false;

	if (icon == CLEAR)
	{
		ClearMatrix();
	}
	
	else
	{
		if (DataIcon[icon]) displayImage(DataIcon[icon], res, daddress, file);
		else displayImage(DataIcon[INTERROGATION], res, daddress, file);
	}

	//sleep(1);

#endif

	return true;
}

static unsigned short int spe[] = { 0, 2, 6, 14, 30, 62, 126, 254, 255 };

int cMatrixLed::DisplaySpectro(int val)
{
	int daddress;
	daddress = 0x00;
	int i;

	if (!Ready) return false;

	if (val == -1) memset(spectro_bmp, 0, 8);

	for (i = 0; i < 7; i++)
	{
		spectro_bmp[i] = spectro_bmp[i + 1];
	}
	val = (int)(val * 8 /100);
	if (val > 7) val = 7;

	spectro_bmp[7] = spe[val];

	displayImage(spectro_bmp, res, daddress, file);
	//sleep(1);
	return true;
}

/************************************************************************************************************************************/
/* Modified file From */
/*
Par Idleman (idleman@idleman.fr - http://blog.idleman.fr)
Licence : CC by sa
Toutes question sur le blog ou par mail, possibilité de m'envoyer des bières via le blog
*/



//#include <stdio.h>
#include <iostream>
//#include <stdlib.h>
#include <sstream>

using namespace std;

#ifdef _WIN32

int TestTransmitter(int _pin, int _sender, int _interruptor, string _onoff)
{
	wprintf(L"Using transmitter %d %d %d %s\n", _pin, _sender, _interruptor, _onoff.c_str());
	return 0;
}

#else
#include <wiringPi.h>
#include <sys/time.h>
#include <sched.h>




#include <time.h>








//int pin;
bool bit2[26] = {};              // 26 bit Identifiant emetteur
bool bit2Interruptor[4] = {};
int pin;
int interruptor;
int sender;
string onoff;

void log(string a){
	//Décommenter pour avoir les logs

	//cout << a << endl;
}




void scheduler_realtime() {

	struct sched_param p;
	p.__sched_priority = sched_get_priority_max(SCHED_RR);
	if (sched_setscheduler(0, SCHED_RR, &p) == -1) {
		perror("Failed to switch to realtime scheduler.");
	}
}

void scheduler_standard() {

	struct sched_param p;
	p.__sched_priority = 0;
	if (sched_setscheduler(0, SCHED_OTHER, &p) == -1) {
		perror("Failed to switch to normal scheduler.");
	}
}



//Envois d'une pulsation (passage de l'etat haut a l'etat bas)
//1 = 310µs haut puis 1340µs bas
//0 = 310µs haut puis 310µs bas
void sendBit(bool b) {
	if (b) {
		digitalWrite(pin, HIGH);
		delayMicroseconds(310);   //275 orinally, but tweaked.
		digitalWrite(pin, LOW);
		delayMicroseconds(1340);  //1225 orinally, but tweaked.
	}
	else {
		digitalWrite(pin, HIGH);
		delayMicroseconds(310);   //275 orinally, but tweaked.
		digitalWrite(pin, LOW);
		delayMicroseconds(310);   //275 orinally, but tweaked.
	}
}

//Calcul le nombre 2^chiffre indiqué, fonction utilisé par itob pour la conversion decimal/binaire
unsigned long power2(int power){
	unsigned long integer = 1;
	for (int i = 0; i<power; i++){
		integer *= 2;
	}
	return integer;
}

//Convertis un nombre en binaire, nécessite le nombre, et le nombre de bits souhaité en sortie (ici 26)
// Stocke le résultat dans le tableau global "bit2"
void itob(unsigned long integer, int length)
{
	for (int i = 0; i<length; i++){
		if ((integer / power2(length - 1 - i)) == 1){
			integer -= power2(length - 1 - i);
			bit2[i] = 1;
		}
		else bit2[i] = 0;
	}
}

void itobInterruptor(unsigned long integer, int length)
{
	for (int i = 0; i<length; i++){
		if ((integer / power2(length - 1 - i)) == 1){
			integer -= power2(length - 1 - i);
			bit2Interruptor[i] = 1;
		}
		else bit2Interruptor[i] = 0;
	}
}




//Envoie d'une paire de pulsation radio qui definissent 1 bit réel : 0 =01 et 1 =10
//c'est le codage de manchester qui necessite ce petit bouzin, ceci permet entre autres de dissocier les données des parasites
void sendPair(bool b) {
	if (b)
	{
		sendBit(true);
		sendBit(false);
	}
	else
	{
		sendBit(false);
		sendBit(true);
	}
}


//Fonction d'envois du signal
//recoit en parametre un booleen définissant l'arret ou la marche du matos (true = on, false = off)
void transmit(int blnOn)
{
	int i;

	// Sequence de verrou anoncant le départ du signal au recepeteur
	digitalWrite(pin, HIGH);
	delayMicroseconds(275);     // un bit de bruit avant de commencer pour remettre les delais du recepteur a 0
	digitalWrite(pin, LOW);
	delayMicroseconds(9900);     // premier verrou de 9900µs
	digitalWrite(pin, HIGH);   // high again
	delayMicroseconds(275);      // attente de 275µs entre les deux verrous
	digitalWrite(pin, LOW);    // second verrou de 2675µs
	delayMicroseconds(2675);
	digitalWrite(pin, HIGH);  // On reviens en état haut pour bien couper les verrous des données

	// Envoie du code emetteur (272946 = 1000010101000110010  en binaire)
	for (i = 0; i<26; i++)
	{
		sendPair(bit2[i]);
	}

	// Envoie du bit définissant si c'est une commande de groupe ou non (26em bit)
	sendPair(false);

	// Envoie du bit définissant si c'est allumé ou eteint 27em bit)
	sendPair(blnOn);

	// Envoie des 4 derniers bits, qui représentent le code interrupteur, ici 0 (encode sur 4 bit donc 0000)
	// nb: sur  les télécommandes officielle chacon, les interrupteurs sont logiquement nommés de 0 à x
	// interrupteur 1 = 0 (donc 0000) , interrupteur 2 = 1 (1000) , interrupteur 3 = 2 (0100) etc...
	for (i = 0; i<4; i++)
	{
		if (bit2Interruptor[i] == 0){
			sendPair(false);
		}
		else{
			sendPair(true);
		}
	}

	digitalWrite(pin, HIGH);   // coupure données, verrou
	delayMicroseconds(275);      // attendre 275µs
	digitalWrite(pin, LOW);    // verrou 2 de 2675µs pour signaler la fermeture du signal

}




int TestTransmitter(int _pin, int _sender, int _interruptor, string _onoff)
{

	if (setuid(0))
	{
		perror("setuid");
		return 1;
	}

	wprintf(L"Using transmitter %d %d %d %s\n",_pin,_sender,_interruptor,_onoff.c_str());

	scheduler_realtime();

	pin = _pin;
	sender = _sender;
	interruptor = _interruptor;
	onoff = _onoff;


	//Si on ne trouve pas la librairie wiringPI, on arrête l'execution
	if (wiringPiSetup() == -1)
	{
		wprintf(L"Librairie Wiring PI introuvable, veuillez lier cette librairie...\n");
		return -1;

	}
	pinMode(pin, OUTPUT);
	log("Pin GPIO configure en sortie");

	itob(sender, 26);            // convertion du code de l'emetteur (ici 8217034) en code binaire
	itobInterruptor(interruptor, 4);


	if (onoff == "on"){
		wprintf(L"envois du signal ON\n");
		for (int i = 0; i<5; i++){
			transmit(true);            // envoyer ON
			delay(10);                 // attendre 10 ms (sinon le socket nous ignore)
		}

	}
	else{
		wprintf(L"envois du signal OFF\n");
		for (int i = 0; i<5; i++){
			transmit(false);           // envoyer OFF
			delay(10);                // attendre 10 ms (sinon le socket nous ignore)
		}
	}

	scheduler_standard();

	return true;
}

#endif
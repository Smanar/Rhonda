//Header file for matrixLEDi2c
#include <string>


#define CLEAR 0
#define SABLIER 1
#define MICRO 2
#define SMILEY 3
#define INTERROGATION 4
#define ALIEN1 5
#define ALIEN2 6

int TestTransmitter(int _pin, int _sender, int _interruptor, std::string _onoff);

class cMatrixLed
{
public:

	//constructeur
	cMatrixLed();

	// Méthodes
	int displayImage(unsigned short int bmp[], int res, int daddress, int file);
	int InitMatrix(void);
	int CloseMatrix(void);
	int DisplayIcone(int icon);
	int DisplaySpectro(int val);
	int ClearMatrix(void);

	bool MakeIcon(int index,char* data);

	//destructeur
	~cMatrixLed();

private:

	// Attributs
	unsigned short int *DataIcon[50]; // 50 icon max
	int NbreIcon;

	int res, i2cbus, address, size, file;

	unsigned short int spectro_bmp[8];

};
/**************************************************

**************************************************/

#include <stdlib.h>
#include <stdio.h>

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include "libs/rs232.h"

int cport_nr=22;
int bdrate=115200;

#ifdef _WIN32

#include <windows.h>

int InitUART(void) { return 1; }
int GetUART(void) {return 1; }

#else

int InitUART(void)
{


	char mode[]={'8','N','1',0};


	if(RS232_OpenComport(cport_nr, bdrate, mode))
	{
		printf("Can not open comport\n");

		return(0);
	}

	printf("Open port %i a la vitesse %i\n",cport_nr,bdrate);

	return (1);
}

int GetUART(void)
{
	int n;
	unsigned char buf[4096];


	n = RS232_PollComport(cport_nr, buf, 4095);

	if(n > 0)
	{
		return 1;
	}

	return 0;
}
#endif

#if 0
int main()
{
	int i, n;
	unsigned char buf[4096];

	while(1)
	{
		n = RS232_PollComport(cport_nr, buf, 4095);

		if(n > 0)
		{
			buf[n] = 0;   /* always put a "null" at the end of a string! */

			for(i=0; i < n; i++)
			{
				if(buf[i] < 32)  /* replace unreadable control-codes by dots */
				{
					//buf[i] = '.';
				}
			}

			printf("received %i bytes: %s\n", n, (char *)buf);
			if (i == 1)
			{
				printf("Value : %d\n", buf[0]);
			}
			if (i == 2)
			{
				printf("Value : %d\n", buf[1]*255+buf[0]);
			}
		}

#ifdef _WIN32
		Sleep(100);
#else
		usleep(100000);  /* sleep for 100 milliSeconds */
#endif
	}

	return(0);
}
#endif
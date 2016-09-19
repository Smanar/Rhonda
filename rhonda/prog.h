#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool LoadConfig(void);

int _DisplaySpectro(int val);
int _DisplayIcone(int val);
int PlayWave(char * file);
int ManageEvent(char* c);
void SetMusic(bool b);
void Exit(void);
void SetLanguage(char *s);
int GetLanguage(void);
void SetCommonString(int index, char *s);
wchar_t * GetCommonString(int index);
void SetSTTMode(int v);
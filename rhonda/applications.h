#include <stdio.h>

void SetAlarm(time_t t);
void Checkalarm(void);

void SetCity(char *s);
int GetMeteo(wchar_t *s);


int GetDefinition(char *,wchar_t *);
int parle(const wchar_t *texte);

void executeshell(char *command);
void lireshell(char *command);

int CheckMail(void);
void SetMailUserPass(char *u, char *p);
int GetFilmCinema(wchar_t *s,int l);

void Savedata(void);


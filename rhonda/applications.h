#include <stdio.h>
#include <wchar.h>

void SetAlarm(char *, char *);
void Checkalarm(void);
void ResetAlarm(void);

bool LoadData(void);

void PlayMusic(bool b);
void ClearMusic(void);

void SetCity(char *s);
int GetMeteo(wchar_t *s);

int GetDefinition(char *,wchar_t *);
int parle(const wchar_t *texte);

void executeshell(char *command);
void lireshell(char *command);
void executesCommand(char*);

int CheckMail(void);
void SetMailUserPass(char *u, char *p);
int GetFilmCinema(wchar_t *s,int l);

void Savedata(void);


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int BOOL;
#define TRUE  1
#define FALSE 0

#ifdef _WIN32
#define snprintf _snprintf
#endif

#ifdef __cplusplus
extern "C" {
#endif

char* LectureWeb(char* URL);
int OpenMailServer(char *username,char *password);

int GetWord(char * reg, char *str, char *res);
int Findhour(char *str);
int Findhour2(char *str);
char* mystrstr(char *s, char *subs);
BOOL charisinstring(char *s, char c);
void UnicodeToAnsi(char *str, char *str2);
char *url_encode(char *str);

void Mywprintf(const wchar_t* format, const char* wc);
void Myswprintf(wchar_t *ws, size_t len, const wchar_t* format,const wchar_t* wc);
void mymbstowcs(wchar_t *ws,char *s,int len);

int ConvertionChiffre(char * string);
const wchar_t *wtimestamp(void);
const char * timestamp(void);

void Wait(int sec);


#ifdef __cplusplus
}
#endif
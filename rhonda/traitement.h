//#include <stdio.h>
#include <string>

class cTraitement
{
	public:

	//constructeur
	cTraitement();

    // Méthodes
    int traite(char *);
	void ReconconizeWord(char *wordchain,int *nbre, int *maxmot,char *commande);
	void action(int choix);
	void CleanCommand( char* str );

	void SetMaxCommand(int);
	void AddCommand(char *, int);
	void SetMaxCommandSpecial(int v);
	void AddCommandSpecial(char *c, char *w);

	void AddAction(char *c, int);

	void ManageAction(char *c);

	//destructeur
	~cTraitement();

	private:

    // Attributs
	char commande[255];
	int comptcommande;

    int NbreCommand;
	char **CommandeL;
	int *ActionComL;

	int NbreSpecial;
	char **CommandeSpecialL;
	char **WordSpecialL;

	int NbreActions;
	char *ListActions[50]; // 50 actions max


};
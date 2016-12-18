#ifndef _STABLA_H_
#define _STABLA_H_ 0

#define BACKLOG 5
#define BUFFER_SIZE 1000
#define MAX_DESCRIPTOR 20

char niz_parametara[10][30];


typedef struct Provider{
	char ip[20];
	int port;
	int opc;
	int dpc;
	int id;
	char gt_number[20];
	int rc;
	int ni;
}provider;


/* struktura koja sadrzi informacije o cvoru */
typedef struct Cvor{
	char prva[20];
	char druga[50];
	char treca[500];
	char cetvrta[500];
	struct Cvor* levo;
	struct Cvor* desno;

}cvor;

typedef struct CvorH{
	int opc;
	int dpc;
	int client_id;
	unsigned long int global_id;
	struct CvorH* levo;
	struct CvorH* desno;

}cvorH;


/* struktura cvor sadrzi buffer i pokazivac na sledeci cvor */
typedef struct cvor_liste{
	unsigned char buffer[1000];
	struct cvor_liste* sledeci_cvor;
	int velicinapaketa;
}Cvor_liste;


cvor* kreiraj_cvor(char* prva, char *druga, char* treca, char* cetvrta);
cvor* dodaj_cvor(cvor* stablo, char* prva,char* druga,char* treca, char* cetvrta);
cvor* pronadji_cvor(cvor* stablo, char* prva, char* druga);
cvor* pronadji_cvor_po_vrednosti(cvor* stablo, char* vrednost);
void oslobodi_memoriju(cvor* stablo);
void ispisi_stablo(cvor* stablo);



void error_fatal (char *format, ...);
int hctoi(const char h);
void funkcija_konverzija (int c, char *br,int sirina);
int konverzija_u_dekadno(char* broj);
int koliko_ima_parametara (char* parametri);
int indeks_karaktera(char* parametri, char c, int poc_poz);
void rasparcaj (char* parametri);
void izvuci_parametre(cvor* stablo,char* ime_paketa,char* parametri);
void obradi_broj (char *broj);
int indeks_a0(unsigned char* buffer, int count);


Cvor_liste* napravi_cvor_liste(unsigned char* paket, int velicina);
void dodaj_u_red(Cvor_liste** pocetak_reda, Cvor_liste** kraj_reda,unsigned char* paket, int velicina);
int uzmi_iz_reda(Cvor_liste** pocetak_reda, Cvor_liste** kraj_reda,unsigned char* paket);
int procitaj_prvi_u_redu (Cvor_liste** pocetak_reda, Cvor_liste** kraj_reda,unsigned char* paket);
void ispisi_red(Cvor_liste* pocetak_reda);
void oslobodi_red(Cvor_liste** pocetak_reda, Cvor_liste** kraj_reda);

cvorH* uzmiIDizHash(cvorH* pocetak,unsigned long int gid);
cvorH* dodajIDuHash(cvorH* pocetak, int gid, int cid, int opc, int dpc);
cvorH* kreiraj_cvorHM(int gid, int cid, int opc, int dpc);
void oslobodi_memorijuHM(cvorH* stablo);
void ispisiHM(cvorH* stablo);


#endif
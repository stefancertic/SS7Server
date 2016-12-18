#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include "stabla.h"



cvor* kreiraj_cvor(char* prva, char *druga, char* treca, char* cetvrta){
	
	cvor* nov_cvor=(cvor*)malloc(sizeof(cvor));
	if(nov_cvor==NULL){
		fprintf(stderr,"Problem sa alokacijom memorije!\n");
		exit(EXIT_FAILURE);
	}

	strcpy(nov_cvor->prva, prva);
	strcpy(nov_cvor->druga, druga);
	strcpy(nov_cvor->treca, treca);
	strcpy(nov_cvor->cetvrta, cetvrta);
	nov_cvor->levo=NULL;
	nov_cvor->desno=NULL;

	return nov_cvor;
}

cvor* dodaj_cvor(cvor* stablo, char* prva,char* druga,char* treca, char* cetvrta){

	if(stablo==NULL)
		return kreiraj_cvor(prva, druga, treca, cetvrta);

	int poredi=strcmp(stablo->prva, prva);

	if(poredi>0){
			stablo->levo=dodaj_cvor(stablo->levo, prva, druga, treca, cetvrta);
		}
		else if (poredi < 0){
			stablo->desno=dodaj_cvor(stablo->desno, prva, druga, treca, cetvrta);
		}
		else{
		  poredi=strcmp(stablo->druga, druga);
		  if(poredi>0){
			stablo->levo=dodaj_cvor(stablo->levo, prva, druga, treca, cetvrta);
		}
		else if (poredi < 0){
			stablo->desno=dodaj_cvor(stablo->desno, prva, druga, treca, cetvrta);
		}
		else{
		  poredi=strcmp(stablo->treca, treca);
		  if(poredi>0){
			stablo->levo=dodaj_cvor(stablo->levo, prva, druga, treca, cetvrta);
		}
		else {
			stablo->desno=dodaj_cvor(stablo->desno, prva, druga, treca, cetvrta);
		}
		
		}
		}
	
	return stablo;
}

cvor* pronadji_cvor(cvor* stablo, char* prva, char* druga){

	if(stablo==NULL)
		return NULL;
	  

	int poredi=strcmp(prva, stablo->prva);

	if(poredi==0){

	  	 if (strcmp(druga, "")==0)
		   return stablo;
		else
		{
		  int poredi2=strcmp(druga, stablo->druga);

		if(poredi2==0){
			return stablo;
		}
		if(poredi2<0){
			return pronadji_cvor(stablo->levo, prva, druga);		}
		else{
			return pronadji_cvor(stablo->desno, prva, druga);
		}
		}
	}
	else{
		if(poredi<0){
			return pronadji_cvor(stablo->levo, prva, druga);
		}
		else{
			return pronadji_cvor(stablo->desno, prva, druga);	
		}
	}
}


cvor* pronadji_cvor_po_vrednosti(cvor* stablo, char* vrednost)
{

	if(stablo==NULL)
		return NULL;
	
	if((strcmp(stablo->prva, vrednost)==0) || (strcmp(stablo->druga, vrednost)==0) || (strcmp(stablo->treca, vrednost)==0)  || (strcmp(stablo->cetvrta, vrednost)==0))
	  return stablo;
	
	if (pronadji_cvor_po_vrednosti(stablo->levo, vrednost)==NULL)
	  return pronadji_cvor_po_vrednosti(stablo->desno, vrednost);
	else 
	  return pronadji_cvor_po_vrednosti(stablo->levo, vrednost);
}
void oslobodi_memoriju(cvor* stablo){

	if(stablo==NULL)
		return;
	
	  oslobodi_memoriju(stablo->levo);
	  oslobodi_memoriju(stablo->desno);

	free(stablo);
}

void ispisi_stablo(cvor* stablo){

	if(stablo==NULL)
		return;

	ispisi_stablo(stablo->levo);
	if (strcmp(stablo->cetvrta, "")==0)
	printf("%s %s %s\n", stablo->prva, stablo->druga, stablo->treca);
	else if (strcmp(stablo->treca, "")==0)
	printf("%s %s\n", stablo->prva, stablo->druga);
	else
	printf("%s %s %s %s\n", stablo->prva, stablo->druga, stablo->treca, stablo->cetvrta);
	ispisi_stablo(stablo->desno);	
}



void error_fatal (char *format, ...)
{
  va_list arguments;		
  va_start (arguments, format);
  vfprintf (stderr, format, arguments);
  va_end (arguments);
  exit(1);
}

void obradi_broj (char *broj)
{
 int i, j;
 int d=strlen(broj);
 if (d==11)
 {
   strcat(broj, "f");
   d++;
 }

 char pom;
 
 for(i=0; i<d-1; i+=2)
 {
   j=i+1;
    pom=broj[i];
    broj[i]=broj[j];
    broj[j]=pom;
 }
}

int hctoi(const char h){
    if(isdigit(h))
        return h - '0';
    else
        return toupper(h) - 'A' + 10;
}

void funkcija_konverzija (int c, char *br, int sirina)
{

br[sirina]='\0';
sirina--;
while(c>0)
{
  
if ((c%16)<10)
br[sirina]=(c%16)+'0';
else if ((c%16)==10)
br[sirina]='a';
else if ((c%16)==11)
br[sirina]='b';
else if ((c%16)==12)
br[sirina]='c';
else if ((c%16)==13)
br[sirina]='d';
else if ((c%16)==14)
br[sirina]='e';
else if ((c%16)==15)
br[sirina]='f';
else;

c=c/16;
sirina--;

}

while(sirina>-1)
{
br[sirina]='0';
sirina--;
}

}

int konverzija_u_dekadno(char* broj)
{
  int i;
  int br=0;
  
  for(i=2; i<=5; i++)
  {
    br*=16;
    if(broj[i]<='9')
    br+=broj[i]-'0';
    else   if(broj[i]=='a')
    br+=10;
    else   if(broj[i]=='b')
    br+=11;
    else   if(broj[i]=='c')
    br+=12;
    else   if(broj[i]=='d')
    br+=13;
    else   if(broj[i]=='e')
    br+=14;
    else   if(broj[i]=='f')
    br+=15;
    else;
    
  }
  return br;
  
}




void izvuci_parametre(cvor* stablo,char* ime_paketa,char* parametri){
 
  cvor* pomocni;
  pomocni=pronadji_cvor_po_vrednosti(stablo, ime_paketa);
  
  if (strcmp(pomocni->prva, ime_paketa)==0)
      strcpy(parametri, pomocni->druga);
   if (strcmp(pomocni->druga, ime_paketa)==0)
      strcpy(parametri, pomocni->treca);
    if (strcmp(pomocni->treca, ime_paketa)==0)
      strcpy(parametri, pomocni->cetvrta);
  
}

int koliko_ima_parametara (char* parametri)
{
  int i;
  int br=0;
  
  for(i=0; parametri[i]!='\0'; i++)
  {
    if (parametri[i]=='_')
	br++;
  }
    
    return ++br;
    
}



int indeks_karaktera(char* parametri, char c, int poc_poz)
{
   int i;  
  for(i=poc_poz; parametri[i]!='\0'; i++)
    if (parametri[i]==c)
      return i;
    
    
    return -1;
}

void rasparcaj (char* parametri)
{
 
  int indeks=0;
  int prethodni=0;

  int br_par=koliko_ima_parametara(parametri);
  int j;

  for (j=0; j<br_par; j++)
  {
  indeks=indeks_karaktera(parametri,'_', prethodni+1);
  if (prethodni>0)
    prethodni++;
  if ((j+1)==br_par)
  {
    strncpy(niz_parametara[j], parametri+prethodni, strlen(parametri)-prethodni+1);    
  }
  else
  {
      strncpy(niz_parametara[j], parametri+prethodni, indeks-prethodni);
      niz_parametara[j][indeks-prethodni]='\0';
  }
  prethodni=indeks;
  }
  
}


int indeks_a0(unsigned char* buffer, int count)
{
 
  int i;
  for(i=count; i>0; i--)
  {
    if(buffer[i]==160)
      return i;
  }
}


Cvor_liste* napravi_cvor_liste(unsigned char* paket, int velicina){

	Cvor_liste* novi_cvor;
	int i;
	
	
	novi_cvor=(Cvor_liste*)malloc(sizeof(Cvor_liste));
	if(novi_cvor==NULL){
		fprintf(stderr, "Problem sa alokacijom!\n");
		exit(EXIT_FAILURE);
	}
	novi_cvor->sledeci_cvor=NULL;
	novi_cvor->velicinapaketa=velicina;
	for (i=0; i<velicina; i++)
		novi_cvor->buffer[i]=paket[i];
	 novi_cvor->sledeci_cvor=NULL;

	return novi_cvor;
}


void dodaj_u_red(Cvor_liste** pocetak_reda, Cvor_liste** kraj_reda,unsigned char* paket, int velicina){

	Cvor_liste* novi_cvor=napravi_cvor_liste(paket, velicina);
		/* a ako je red prazan */

	 if(*pocetak_reda==NULL){
		/* sada je jedini cvor i pocetni i krajnji */
		*pocetak_reda=novi_cvor;
		*kraj_reda=novi_cvor;
	}
	
	/* ako red nije prazan */
	if(*kraj_reda!=NULL){
		
		/* novi cvor dodajemo na kraj reda */
		(*kraj_reda)->sledeci_cvor=novi_cvor;
		novi_cvor->sledeci_cvor=NULL;
		*kraj_reda=novi_cvor;
	}
	
}



int uzmi_iz_reda(Cvor_liste** pocetak_reda, Cvor_liste** kraj_reda,unsigned char* paket){

	/* proveravamo da li je red prazan*/
	if(*pocetak_reda==NULL)
		/* i ako jeste, vracamo nulu kao naznaku da je operaciju nemoguce izvrsiti */
		return 0;

	/*inace, ocitavamo  sadrzaj u prvom cvoru */
	int i;
	int vel;
	
	
	vel=(*pocetak_reda)->velicinapaketa;
	for (i=0; i<vel; i++)
	  paket[i]=(*pocetak_reda)->buffer[i];

	/* i izbacujemo taj cvor iz reda tako sto pomerimo pokazivac na pocetak reda */
	Cvor_liste* pomocni_cvor=*pocetak_reda;
	*pocetak_reda=(*pocetak_reda)->sledeci_cvor;
	free(pomocni_cvor);

	/* proveravamo da li je posle brisanja prvog cvora red ostao prazan */
	if(*pocetak_reda==NULL)
		/* i ako jeste, kraj reda je takodje NULL pokazivac */
		*kraj_reda=NULL;

	/* vracamo velicinu paketa  kao indikaciju da je operacija uspesno izvrsena */
	return vel;
}




int procitaj_prvi_u_redu (Cvor_liste** pocetak_reda, Cvor_liste** kraj_reda,unsigned char* paket){

	/* proveravamo da li je red prazan*/
	if(*pocetak_reda==NULL)
		/* i ako jeste, vracamo nulu kao naznaku da je operaciju nemoguce izvrsiti */
		return 0;

	/*inace, ocitavamo  sadrzaj u prvom cvoru */
	int i;
	int vel;
	
	
	vel=(*pocetak_reda)->velicinapaketa;
	for (i=0; i<vel; i++)
	  paket[i]=(*pocetak_reda)->buffer[i];

	/* vracamo velicinu paketa  kao indikaciju da je operacija uspesno izvrsena */
	return vel;
}



/* funkcija ispisuje red cvor po cvor od pocetka reda do kraja reda */
void ispisi_red(Cvor_liste* pocetak_reda){

	Cvor_liste* tekuci_cvor=pocetak_reda;

	while(tekuci_cvor!=NULL){
	  int i;
		for (i=0; i<tekuci_cvor->velicinapaketa; i++)
		printf("%x ", tekuci_cvor->buffer[i]);
		tekuci_cvor=tekuci_cvor->sledeci_cvor;
	}

	printf("\n");
}


/* funkcija oslobadja memoriju koju je red zauzimao */
void oslobodi_red(Cvor_liste** pocetak_reda, Cvor_liste** kraj_reda){

	Cvor_liste* tekuci_cvor=*pocetak_reda;

	while(tekuci_cvor!=NULL){
		Cvor_liste* pomocni_cvor=tekuci_cvor;
		tekuci_cvor=tekuci_cvor->sledeci_cvor;
		free(pomocni_cvor);
	}

	*kraj_reda=NULL;
}


cvorH* uzmiIDizHash(cvorH* pocetak,unsigned long int gid)
{
  if(pocetak==NULL)
  {
    return NULL;
  }

   int poredi= pocetak->global_id - gid;
	if (poredi==0)
	{
	  return pocetak;
	}
	else if(poredi>0){
	return uzmiIDizHash(pocetak->levo, gid);
		}
	else {
	return uzmiIDizHash(pocetak->desno, gid);
	}
}



cvorH* dodajIDuHash(cvorH* pocetak, int gid, int cid, int opc, int dpc)
{
  if(pocetak==NULL)
  {
	 return kreiraj_cvorHM(gid,cid, opc, dpc);
	 
  }
	int poredi=pocetak->global_id - gid;
	if(poredi>0){
			pocetak->levo=dodajIDuHash(pocetak->levo, gid,cid, opc, dpc);
		}
		else if (poredi < 0){

			pocetak->desno=dodajIDuHash(pocetak->desno,  gid,cid, opc, dpc);
		}
		else{
		 printf("vec postoji dtid u HashMapi\n"); 
		}

    return pocetak;
    
    

    
    
}




cvorH* kreiraj_cvorHM(int gid, int cid, int opc, int dpc){
	
	cvorH* nov_cvor=(cvorH*)malloc(sizeof(cvorH));
	if(nov_cvor==NULL){
		fprintf(stderr,"Problem sa alokacijom memorije!\n");
		exit(EXIT_FAILURE);
	}

	
	nov_cvor->global_id=gid;
	nov_cvor->opc=opc;
	nov_cvor->dpc=dpc;
	nov_cvor->client_id=cid;
	nov_cvor->levo=NULL;
	nov_cvor->desno=NULL;

	
	return nov_cvor;

}




void oslobodi_memorijuHM(cvorH* stablo){

	if(stablo==NULL)
		return;
	
	  oslobodi_memorijuHM(stablo->levo);
	  oslobodi_memorijuHM(stablo->desno);

	free(stablo);
}


void ispisiHM(cvorH* stablo){

	if(stablo==NULL)
	{
		return;
	}

	ispisiHM(stablo->levo);
	printf("cvor HM %d %d %d %d\n", stablo->global_id, stablo->client_id, stablo->opc, stablo->dpc);
	ispisiHM(stablo->desno);	
}






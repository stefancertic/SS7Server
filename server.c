#include <mysql/mysql.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/select.h>
#include "stabla.h"
#include <netinet/sctp.h>

/* Maksimalan broj istovremenih konekcija koje server moze da prihvati. */
#define BACKLOG 5

static pthread_mutex_t mutex;

/* stabla za implementaciju hash mapa*/
cvor* messageClassValues=NULL;
cvor* ParametarValue=NULL;
cvor* MessageTypeValue=NULL;
cvor* ParametarOrder=NULL;

pthread_t tid[100];		/* Identifikator niti. */ 
pthread_t tidclient[100];
provider providers[10];
int count_providers=0;
char *program;

cvorH* hash=NULL;

Cvor_liste* pocetak_reda=NULL;
Cvor_liste* kraj_reda=NULL;

Cvor_liste* pocetak_reda2=NULL;
Cvor_liste* kraj_reda2=NULL;

   MYSQL *conn;
   MYSQL_RES *res;
   MYSQL_ROW row;

unsigned long long int global_id=100000000;

FILE* logfile;


void loguj(char* poruka)
{
 logfile=fopen("ulaz.txt", "a");
 fprintf(logfile, "%s\n", poruka); 
 fclose(logfile);
}
   
void obradi (char* buffer, int count, char* ime_paketa)
{
  
  char class[20];
  char type[20];
  int i;
          //printf("Primljen je paket velicine %d\n", count);
          sprintf(class, "%d", buffer[2]);
	  class[1]='\0';
	  sprintf(type, "%d", buffer[3]);
	  type[1]='\0';	  
	  cvor* pomocni = pronadji_cvor (MessageTypeValue, class , type);
	  strcpy(ime_paketa, pomocni->treca);
         // printf("Tip paketa je  %s\n", ime_paketa);
}

int izvadi_RoutingContext(char* buffer)
{
   char RoutingContext[10];
   int duzina=sprintf(RoutingContext, "%d", buffer[23]);
    RoutingContext[duzina]='\0';
    return atoi(RoutingContext);
  
}

void salje_paket (char* ime_paketa, int server, int poz)
{
    char cdata[80];
    char br[30];
    char parametri[500];
    cvor *pomocni = pronadji_cvor_po_vrednosti(MessageTypeValue, ime_paketa);  
   
    /*Version*/
    funkcija_konverzija(1, br, 2);
    strcpy(cdata, br);
    /*Reserved*/
    funkcija_konverzija(0, br, 2);
    strcat(cdata, br);     
    /*Message class*/
    funkcija_konverzija(atoi(pomocni->prva), br, 2);
    strcat(cdata, br);
     /*Message Type*/
    funkcija_konverzija(atoi(pomocni->druga), br, 2);
    strcat(cdata, br);    
    
    /*Message length*/
    if (strcmp(ime_paketa, "ASPUP")==0)
    {
    funkcija_konverzija(36, br, 8);
    }
    else if (strcmp(ime_paketa, "ASPAC")==0)
    {
    funkcija_konverzija(24, br,8);
    }
    else if (strcmp(ime_paketa, "DAUD")==0)
    {
    funkcija_konverzija(16, br,8);
    }
    else
      ;
    
    strcat(cdata, br);
        
       /* zaglavlje za parametre*/    
     izvuci_parametre(ParametarOrder,ime_paketa, parametri);
     rasparcaj (parametri);

     int pom;
    for (pom=0; pom<koliko_ima_parametara(parametri); pom++)
     {
       /*Parametar tag*/
    pomocni = pronadji_cvor_po_vrednosti(ParametarValue, niz_parametara[pom]); 
    funkcija_konverzija(konverzija_u_dekadno(pomocni->prva), br, 4);
    strcat(cdata, br);
    /*Parametar length*/
    if (strcmp(niz_parametara[pom], "InfoString")==0)
    {
    funkcija_konverzija(18, br,4);
    strcat(cdata, br);
        /*Info String*/
    strcat(cdata, "416c65784353277320415350210a");
    }
      else
      {
    funkcija_konverzija(8, br,4);
    strcat(cdata, br);
      }
      
/* treci deo zaglavlja za parametar*/

if (strcmp(niz_parametara[pom], "ASPIdentifier")==0)
{
    funkcija_konverzija(0, br,8);
    strcat(cdata, br);
}
    
     if (strcmp( niz_parametara[pom], "TrafficModeType")==0)
     {
    funkcija_konverzija(1, br,8);
    strcat(cdata, br);
     }
    
    
    /* ovde treba voditi racuna, na jednu adresu malte se salje RC=100,  a na drugu (194) se salje RC=101*/
    if (strcmp( niz_parametara[pom], "RoutingContext")==0)
    {
	funkcija_konverzija(providers[poz].rc, br,8);
        strcat(cdata, br);
    }
    
       if (strcmp( niz_parametara[pom], "AffectedPointCode")==0)
       {
    funkcija_konverzija(0, br,2);
    strcat(cdata, br);
        /* ovde treba voditi racuna, na jednu adresu malte se salje APC=8460,  a na drugu se salje APC=8461*/
      funkcija_konverzija(providers[poz].dpc, br,6);
      strcat(cdata, br);
       }
 } 
    if (strcmp(ime_paketa, "ASPUP")==0)
    {
    /*Padding*/
    funkcija_konverzija(0, br,6);
    strcat(cdata, br);
    }
    else
        {
    /*Padding*/
    funkcija_konverzija(0, br,2);
    strcat(cdata, br);
    }    
    
    unsigned char udata[(strlen(cdata)-1)/2];
    
    const char *p;
    unsigned char *up;

    for(p=cdata,up=udata;*p;p+=2,++up)
    {
        *up = hctoi(p[0])*16 + hctoi(p[1]);
    }
      int count = sizeof (udata);
      int i;
      if (send(server, udata, count, 0) != count)
	error_fatal ("%s send() error\n", "client"); 
}



void salje_paket2 (char* ime_paketa, int client, int RC, int prviNTFY)
{
  
    char cdata[500];
    char br[50];
    int i, j, proba;
    char parametri[500];
    
    cvor *pomocni = pronadji_cvor_po_vrednosti(MessageTypeValue, ime_paketa);  
    /*Version*/
    funkcija_konverzija(1, br, 2);
    strcpy(cdata, br);
    /*Reserved*/
    funkcija_konverzija(0, br, 2);
    strcat(cdata, br);
    /*Message class*/
    funkcija_konverzija(atoi(pomocni->prva), br, 2);
    strcat(cdata, br);
    /*Message Type*/
    funkcija_konverzija(atoi(pomocni->druga), br, 2);
    strcat(cdata, br);

    /*Message length*/
    if (strcmp(ime_paketa, "ASPUP_ACK")==0)
    funkcija_konverzija(64, br,8);
    if (strcmp(ime_paketa, "ASPAC_ACK")==0)
    funkcija_konverzija(24, br,8);
    if (strcmp(ime_paketa, "DAVA")==0)
    funkcija_konverzija(24, br,8);
    if (strcmp(ime_paketa, "NTFY")==0)
    funkcija_konverzija(16, br,8);
    strcat(cdata, br);
        
     izvuci_parametre(ParametarOrder,ime_paketa, parametri);
     rasparcaj (parametri);

     int pom;
     
    for (pom=0; pom<koliko_ima_parametara(parametri); pom++)
     {
    
    /*Parametar tag*/
    pomocni = pronadji_cvor_po_vrednosti(ParametarValue, niz_parametara[pom]);  
    funkcija_konverzija(konverzija_u_dekadno(pomocni->prva), br, 4);
    strcat(cdata, br);
    
    /*Parametar length*/
    if (strcmp(niz_parametara[pom], "InfoString")==0)
    {
      
    funkcija_konverzija(56, br,4);
        strcat(cdata, br);
        /*Info String*/
    strcat(cdata,"47656e65726174656420627920506c61622076657273696f6e20312e302062756975696c742032312e382e3230313120506c6162");
    }
      else
      {
    funkcija_konverzija(8, br,4);
    strcat(cdata, br);
      }
      
 
/* treci deo zaglavlja za parametar*/

     if (strcmp( niz_parametara[pom], "TrafficModeType")==0)
     {
    funkcija_konverzija(2, br,8);
    strcat(cdata, br);
     }
    
    if (strcmp( niz_parametara[pom], "RoutingContext")==0)
    {
    funkcija_konverzija(RC, br,8);
    strcat(cdata, br);
    }
   
    if (strcmp( niz_parametara[pom], "Status")==0)
    {
       if(prviNTFY==1)
       {
    funkcija_konverzija(1, br,4);
    strcat(cdata, br);
    funkcija_konverzija(2, br,4);
    strcat(cdata, br);
       }
       else
       {
	 funkcija_konverzija(1, br,4);
    strcat(cdata, br);
    funkcija_konverzija(3, br,4);
    strcat(cdata, br); 
	 
       }
       
    }
    
    
       if (strcmp( niz_parametara[pom], "AffectedPointCode")==0)
       {
    funkcija_konverzija(0, br,2);
    strcat(cdata, br);
     funkcija_konverzija(8080, br,6);
    strcat(cdata, br);
       }
 } 

   
    /*Padding*/
    
   if (strcmp(ime_paketa, "ASPUP_ACK")==0)
    {
   funkcija_konverzija(0, br,2);
   strcat(cdata, br);
    }
    else
        {
    funkcija_konverzija(0, br,2);
    strcat(cdata, br);
    }

    unsigned char udata[(strlen(cdata)-1)/2];
    
    const char *p;
    unsigned char *up;

    for(p=cdata,up=udata;*p;p+=2,++up)
    {
        *up = hctoi(p[0])*16 + hctoi(p[1]);
    }
      
      int count = sizeof (udata);
      if (send(client, udata, count, 0) != count)
	error_fatal ("%s send() error\n", "server"); 

          
}

void  izgradi_stek(int server, int poz)
{
    int count;
    char buffer[BUFFER_SIZE];
    char ime_paketa[15];

    salje_paket("ASPUP", server, poz);
  
    if ((count = recv (server, buffer, 200, 0)) < 0)
	error_fatal ("%s readline() error\n", program);
      buffer[count] = 0;
    
    obradi(buffer, count, ime_paketa);
    
    if (strcmp(ime_paketa, "ASPUP_ACK")==0)
	loguj("primili smo ASPUP_ACK paket");
      else
      {
	  loguj("Ocekivan je ASPUP_ACK paket"); 
	  exit(1);
      }
       salje_paket("ASPAC", server, poz);
       
 if ((count = recv (server, buffer, 200, 0)) < 0)
	error_fatal ("%s readline() error\n", program);
      buffer[count] = 0;    
    obradi(buffer, count, ime_paketa);
 
    if (strcmp(ime_paketa, "NTFY")==0)
	loguj("primili smo NTFY paket\n");
      else
      {
	  loguj("Ocekivan je NTFY paket\n"); 
	  exit(1);
      }
       if ((count = recv (server, buffer, 200, 0)) < 0)
	error_fatal ("%s readline() error\n", program);
      buffer[count] = 0;    
    obradi(buffer, count, ime_paketa);
    if (strcmp(ime_paketa, "ASPAC_ACK")==0)
	loguj("primili smo ASPAC_ACK paket\n");
      else
      {
	  loguj("Ocekivan je ASPAC_ACK paket\n"); 
	  exit(1);
      }
       if ((count = recv (server, buffer, 200, 0)) < 0)
	error_fatal ("%s readline() error\n", program);
      buffer[count] = 0;
      //printf("primili paket\n");
    
    obradi(buffer, count, ime_paketa);
    if (strcmp(ime_paketa, "NTFY")==0)
	loguj("primili smo NTFY paket\n");
      else
      {
	  loguj("Ocekivan je NTFY paket\n"); 
	  exit(1);
      }
      salje_paket("DAUD", server, poz);
       if ((count = recv (server, buffer, 200, 0)) < 0)
	error_fatal ("%s readline() error\n", program);
      buffer[count] = 0;
      //printf("primili paket\n");
    
    obradi(buffer, count, ime_paketa);
    if (strcmp(ime_paketa, "DAVA")==0)
	loguj("primili smo DAVA paket\n");
      else
      {
	  loguj("Ocekivan je DAVA paket\n"); 
	  exit(1);
      }   
     
      return;
}

void izgradi_stek2 (int client)
{
 
 int count;
 char buffer [BUFFER_SIZE];
 char ime_paketa[15];
 
  
      if ((count =   recv (client, buffer, BUFFER_SIZE, 0)) < 0)
	error_fatal ("%s recv() error\n", program);
   printf("Primljen je prvi paket\n");
	obradi(buffer, count, ime_paketa);
        printf("primljeni paket je tipa %s\n", ime_paketa);
	 
      if(strcmp(ime_paketa, "ASPUP")==0)
       salje_paket2("ASPUP_ACK", client, 0,0);
       printf("Poslat je prvi paket\n");
      
      
       if ((count =   recv (client, buffer, BUFFER_SIZE, 0)) < 0)
	error_fatal ("%s recv() error\n", program);
	 obradi(buffer, count, ime_paketa);
        printf("primljeni paket je tipa %s\n", ime_paketa);
	
         if (strcmp(ime_paketa,"ASPAC")==0)
	 {
        int rc=izvadi_RoutingContext(buffer);
	salje_paket2("NTFY", client, rc, 1);
	salje_paket2("ASPAC_ACK", client, rc, 0);
	salje_paket2("NTFY", client, rc, 0);
	 }
	
	/*if ((count =   recv (client, buffer, BUFFER_SIZE, 0)) < 0)
	error_fatal ("%s recv() error\n", program);
	 obradi(buffer, count, ime_paketa);
	 printf("primljeni paket je tipa %s\n", ime_paketa);
	
         if (strcmp(ime_paketa,"DAUD")==0)
	 {
	      izvadi_APC(buffer);
	      salje_paket2("DAVA", client);
	 }*/
  
}

int pocetak_ID (unsigned char* buffer, int count, int indikator)
{
  
  // indikator 1 -> paket od clienta
  //indikator 2 -> paket od operatora
  
 int poz=-1;
 int i;
 char br[5];
 for(i=0; i<count; i++)
 {
     if((indikator==1) && (buffer[i+1]==04) && (buffer[i]==72))
 return i;
     if((indikator==2) && (buffer[i+1]==04) && (buffer[i]==73))
 return i;
 }
 return -1;
  
}

int uzmiIDpaketa (unsigned char* buffer, int count, char *dtid, int indikator)
{
  
  int poz=pocetak_ID(buffer, count, indikator);
  if (poz==-1)
    return -1;
  
  int i;
  char br[5];
  for(i=poz+2; i<poz+6; i++)
  {
	funkcija_konverzija(buffer[i], br,2);
	if (i==poz+2)
	  strcpy(dtid, br);
	else
	  strcat(dtid, br);
     }
  
     printf("dtid: %s\n", dtid); 
     return 1;
   
   
  
}

void promeni_gt(unsigned char* buffer,int count,int client, char* gt_number)
{
 // client =1 paket dosao od korisnika
  //client =2 paket dosao od operatora
  int i;
  //printf("menjamo GT!\n");
  if(client==1)
   for(i=0; i<count; i++)
   {
     if((buffer[i]==131) && (buffer[i+1]==97)&& (buffer[i+2]==0)&& (buffer[i+3]==22) && (buffer[i+4]==148) && (buffer[i+5]==0))
     {
   //printf("-----------------------menjamo GT--------\n");
   buffer[i]=16*(gt_number[1]-'0')+gt_number[0]-'0';
   buffer[i+1]=16*(gt_number[3]-'0')+gt_number[2]-'0';
   buffer[i+2]=16*(gt_number[5]-'0')+gt_number[4]-'0';
   buffer[i+3]=16*(gt_number[7]-'0')+gt_number[6]-'0';
   buffer[i+4]=16*(gt_number[9]-'0')+gt_number[8]-'0';
  if(strlen(gt_number)==12)
   buffer[i+5]=16*(gt_number[11]-'0')+gt_number[10]-'0';     
  else
       buffer[i+5]=gt_number[10]-'0'+0;     
     }
     
     if((buffer[i]==131) && (buffer[i+1]==97)&& (buffer[i+2]==0)&& (buffer[i+3]==22) && (buffer[i+4]==148) && (buffer[i+5]==240))
     {
          //printf("-----------------------menjamo GT 2 --------\n");

   buffer[i]=16*(gt_number[1]-'0')+gt_number[0]-'0';
   buffer[i+1]=16*(gt_number[3]-'0')+gt_number[2]-'0';
   buffer[i+2]=16*(gt_number[5]-'0')+gt_number[4]-'0';
   buffer[i+3]=16*(gt_number[7]-'0')+gt_number[6]-'0';
   buffer[i+4]=16*(gt_number[9]-'0')+gt_number[8]-'0';
  if(strlen(gt_number)==12)
   buffer[i+5]=16*(gt_number[11]-'0')+gt_number[10]-'0';     
  else
       buffer[i+5]=16*15+gt_number[10]-'0';      
   }
   }
   
   
   
  if(client==2)
   for(i=0; i<count; i++)
     if((buffer[i]==(16*(gt_number[1]-'0')+gt_number[0]-'0')) 
       && (buffer[i+1]==(16*(gt_number[3]-'0')+gt_number[2]-'0')) 
       && (buffer[i+2]==(16*(gt_number[5]-'0')+gt_number[4]-'0'))
       && (buffer[i+3]==(16*(gt_number[7]-'0')+gt_number[6]-'0')) 
       && (buffer[i+4]==(16*(gt_number[9]-'0')+gt_number[8]-'0'))) 
     {
   buffer[i]=131;
   buffer[i+1]=97;
   buffer[i+2]=0;
   buffer[i+3]=22;
   buffer[i+4]=148;
   buffer[i+5]=0;  
   }
   return;
  
}

void promeni_paket (unsigned char* buffer, int count, int client, int opcprov, int dpcprov, char* gt_number, int rcprov, int niprov)
{
  //printf("menajmo paket...\n");
 char opc[10];
 char dpc[10];
 char rc[10];
 char ni[5];
 unsigned char idpaketa[20];
  char client_opc[10];
 char client_dpc[10];
 int i;
 char br[5];
 int pr=1;
 char finalid[10];
 int opc_dekadno=0, dpc_dekadno=0, id_dekadno=0;
 int new_id, mod=0;
 int poz_pc;
 
 
 funkcija_konverzija(opcprov, opc, 8);
 funkcija_konverzija(dpcprov, dpc, 8);
 funkcija_konverzija(rcprov, rc, 8);
 funkcija_konverzija(niprov, ni, 2);

 //printf ("pozivamo fju za id\n");
 uzmiIDpaketa(buffer, count, idpaketa,1);
 printf("paket id----\n");
 printf("%s\n", idpaketa);
 if (strcmp(idpaketa, "noID")==0)
 {
   printf("do promene nece doci jer je paket bez id-ja\n");
   return;
 }
 for(i=0; i<count; i++)
 {
   if((buffer[i]==2) && (buffer[i+1]==16))
   {
     poz_pc=i+4;
   break;
   }
 }
 
 for (i=poz_pc; i<poz_pc+4; i++)
 {
   funkcija_konverzija(buffer[i], br, 2);
    if(i==poz_pc)
      strcpy(client_opc, br);
    else
      strcat(client_opc, br);
  }
  //printf ("opc paketa= %s\n", client_opc);
  
  for (i=poz_pc+4; i<poz_pc+8; i++)
  {
    funkcija_konverzija(buffer[i], br, 2);
    if(i==poz_pc+4)
      strcpy(client_dpc, br);
    else
      strcat(client_dpc, br);
  }
 //printf ("dpc paketa= %s\n", client_dpc); 
 
 
 for(i=7; i>=0; i--)
 {
   if((idpaketa[i]>='0') && (idpaketa[i]<='9'))
   id_dekadno+=pr*(idpaketa[i]-48);
   else
   id_dekadno+=pr*(idpaketa[i]-'a'+10);
   pr*=16;
 }
 
 pr=1;
  for(i=7; i>=0; i--)
 {
   if((client_opc[i]>='0') && (client_opc[i]<='9'))
   opc_dekadno+=pr*(client_opc[i]-48);
   else
   opc_dekadno+=pr*(client_opc[i]-'a'+10);
   pr*=16;
 }
 
  pr=1;
  for(i=7; i>=0; i--)
 {
   if((client_dpc[i]>='0') && (client_dpc[i]<='9'))
   dpc_dekadno+=pr*(client_dpc[i]-48);
   else
   dpc_dekadno+=pr*(client_dpc[i]-'a'+10);
   pr*=16;
 }
 
 printf("dekadno id je %d\n", id_dekadno);
  printf("dekadno opc je %d\n", opc_dekadno);
 printf("dekadno dpc je %d\n", dpc_dekadno);

pthread_mutex_lock (&mutex);
hash= dodajIDuHash(hash, global_id, id_dekadno, opc_dekadno, dpc_dekadno);
//ispisiHM(hash);
pthread_mutex_unlock (&mutex);

 //menjamo rc
    const char *p;
    unsigned char *up;
    char udata[100];
    for(up=udata,i=0;i<8;i+=2,++up)
    {
        *up = hctoi(rc[i])*16 + hctoi(rc[i+1]);
    }
     for (i=0; i<4; i++)
	buffer[i+12]=udata[i];
 
 //menjamo opc i dpc

    for(up=udata,i=0;i<8;i+=2,++up)
    {
        *up = hctoi(opc[i])*16 + hctoi(opc[i+1]);
    }
     for (i=0; i<4; i++)
	buffer[i+poz_pc]=udata[i];
     
     for(up=udata,i=0;i<8;i+=2,++up)
    {
        *up = hctoi(dpc[i])*16 + hctoi(dpc[i+1]);
    }
     for (i=0; i<4; i++)
	buffer[i+poz_pc+4]=udata[i];

     
     /* menjamo i NI */
       for(up=udata,i=0;i<2;i+=2,++up)
    {
        *up = hctoi(ni[i])*16 + hctoi(ni[i+1]);
    }
    buffer[poz_pc+9]=udata[0];
     
     
 funkcija_konverzija(global_id, finalid, 8);
 global_id++;
 
 /* menjamo id*/
   int poz=pocetak_ID(buffer, count, 1);
   for(up=udata,i=0;i<8;i+=2,++up)
    {
        *up = hctoi(finalid[i])*16 + hctoi(finalid[i+1]);
    }
     for (i=0; i<4; i++)
	buffer[i+poz+2]=udata[i];    
     
   /* menjamo gt_number*/
     promeni_gt(buffer, count,client, gt_number);
}

int promeni_paket_inverzno (unsigned char* buffer, int count, int client, char* gt_number, int rcuser, int niuser)
{
  
  printf("menjamo paket inverzno .....");
 int new_id, mod=0;
 char opc[10];
 char dpc[10];
 int i;
 char br[10];
 const char *p;
 unsigned char *up;
 char udata[100];
 unsigned char idpaketa[20];
 int poz_pc;
 int pot_id=0;
char rc[10], ni[10];
 funkcija_konverzija(rcuser, rc, 8);
 funkcija_konverzija(niuser, ni, 2);
int r=uzmiIDpaketa(buffer, count, idpaketa,2);
  printf ("pozivamo fju za id\n");
 if (r==-1)
 {
   printf("do inverzne promene nece doci jer je paket bez id-ja\n");
   return -1;
 }
 printf("id je ok\n");
 
 int pr=1;
 for(i=7; i>0;i--)
 {
  if((idpaketa[i]>='0') && (idpaketa[i]<='9'))
   pot_id+=pr*(idpaketa[i]-'0');
  else
   pot_id+=pr*(idpaketa[i]-'a'+10);
   pr*=16; 
 }
 
  cvorH* pom;
   //printf("da vidimo cime cemo menjati paket %ld\n", pot_id);

  pthread_mutex_lock (&mutex);
  //ispisiHM(hash);
  pom=uzmiIDizHash(hash, pot_id);
  //printf("treba promeniti u %d %d\n", pom->opc, pom->dpc);
  pthread_mutex_unlock (&mutex);

  for(i=0; i<count; i++)
 {
   printf("%d ",buffer[i]);
   if((buffer[i]==2) && (buffer[i+1]==16))
   {
     poz_pc=i+4;
   break;
   }
 }
 
  funkcija_konverzija(pom->dpc, opc, 8);

    for(up=udata,i=0;i<8;i+=2,++up)
    {
        *up = hctoi(opc[i])*16 + hctoi(opc[i+1]);
    }
     for (i=0; i<4; i++)
	buffer[i+poz_pc]=udata[i];
 
  
  funkcija_konverzija(pom->opc, dpc, 8);
   for(up=udata,i=0;i<8;i+=2,++up)
    {
        *up = hctoi(dpc[i])*16 + hctoi(dpc[i+1]);
    }
     for (i=0; i<4; i++)
	buffer[i+poz_pc+4]=udata[i];
     
   int poz=pocetak_ID(buffer, count, 2);
   funkcija_konverzija(pom->client_id,br,8);
    for(up=udata,i=0;i<8;i+=2,++up)
    {
        *up = hctoi(br[i])*16 + hctoi(br[i+1]);
    }
    
       for (i=0; i<4; i++)
	buffer[i+poz+2]=udata[i];
   
       //menjamo rc
    for(up=udata,i=0;i<8;i+=2,++up)
    {
        *up = hctoi(rc[i])*16 + hctoi(rc[i+1]);
    }
     for (i=0; i<4; i++)
	buffer[i+12]=udata[i];    
    /* menjamo i NI */
       for(up=udata,i=0;i<2;i+=2,++up)
    {
        *up = hctoi(ni[i])*16 + hctoi(ni[i+1]);
    }
    buffer[poz_pc+9]=udata[0];    

     promeni_gt(buffer, count,client, gt_number);
     return 0;
}


int izvuci_dpc(unsigned char* buffer, int count)
{
  int i;
  int poz_pc;
  char client_dpc[10];
  int dpc_dekadno=0;
  int pr=1;
  char br[5];


   for(i=0; i<count; i++)
 {
   if((buffer[i]==2) && (buffer[i+1]==16))
   {
     poz_pc=i+8;
   break;
   }
 }
 for (i=poz_pc; i<poz_pc+4; i++)
 {
   funkcija_konverzija(buffer[i], br, 2);
    if(i==poz_pc)
      strcpy(client_dpc, br);
    else
      strcat(client_dpc, br);
  }
  
  for(i=7; i>=0; i--)
 {
   if((client_dpc[i]>='0') && (client_dpc[i]<='9'))
   dpc_dekadno+=pr*(client_dpc[i]-48);
   else
   dpc_dekadno+=pr*(client_dpc[i]-'a'+10);
   pr*=16;
 }
 //printf ("------dpc paketa je %d\n", dpc_dekadno);
 return dpc_dekadno;
 
}

int izvuci_opc(unsigned char* buffer, int count)
{
  int i;
  int poz_pc;
  char client_opc[10];
  int opc_dekadno=0;
  int pr=1;
  char br[5];


   for(i=0; i<count; i++)
 {
   if((buffer[i]==2) && (buffer[i+1]==16))
   {
     poz_pc=i+4;
   break;
   }
 }
 for (i=poz_pc; i<poz_pc+4; i++)
 {
   funkcija_konverzija(buffer[i], br, 2);
    if(i==poz_pc)
      strcpy(client_opc, br);
    else
      strcat(client_opc, br);
  }
  
  for(i=7; i>=0; i--)
 {
   if((client_opc[i]>='0') && (client_opc[i]<='9'))
   opc_dekadno+=pr*(client_opc[i]-48);
   else
   opc_dekadno+=pr*(client_opc[i]-'a'+10);
   pr*=16;
 }
 printf ("-------------opc paketa je %d\n", opc_dekadno);
 return opc_dekadno;
 
}
void izvuci_podred_za_korisnika(Cvor_liste** pocetak_reda2,int dpc,Cvor_liste** korisnik_pocetak,Cvor_liste** korisnik_kraj)
{
  if(*pocetak_reda2==NULL)
  {
    *korisnik_pocetak=NULL;
    *korisnik_kraj=NULL;
    return;
  }

  Cvor_liste* tekuci_cvor=(*pocetak_reda2)->sledeci_cvor;
  Cvor_liste* prethodni_cvor=*pocetak_reda2;
  Cvor_liste* pomocni_cvor;

  if((izvuci_dpc((*pocetak_reda2)->buffer, (*pocetak_reda2)->velicinapaketa)==dpc) && (*pocetak_reda2!=NULL))
  {
   // printf("1\n");
   dodaj_u_red(korisnik_pocetak, korisnik_kraj, (*pocetak_reda2)->buffer, (*pocetak_reda2)->velicinapaketa);
      // printf("2\n");

    pomocni_cvor=*pocetak_reda2;
    if ((*pocetak_reda2)->sledeci_cvor == NULL)
    {
         // printf("3\n");
      *pocetak_reda2=NULL;
    }
    else
    {
   *pocetak_reda2=(*pocetak_reda2)->sledeci_cvor;
      // printf("4\n");
    }
    //free(pomocni_cvor); 
  }
     // printf("5\n");

      if(*pocetak_reda2==NULL)
    return; 
      while(tekuci_cvor!=NULL){
		if(izvuci_dpc((tekuci_cvor)->buffer, (tekuci_cvor)->velicinapaketa)==dpc){
			pomocni_cvor=tekuci_cvor;
			prethodni_cvor->sledeci_cvor=tekuci_cvor->sledeci_cvor;
			tekuci_cvor=tekuci_cvor->sledeci_cvor;
			free(pomocni_cvor);
		}
		else{
			prethodni_cvor=tekuci_cvor;
			tekuci_cvor=tekuci_cvor->sledeci_cvor;
		}
	
	}
//printf("usepsno izuvkli podred\n");
}

static void *opsluzi_korisnika (void *arg)
{

   int count;
   fd_set read_set;		/* Skup deskriptora sa kojih server treba da cita. */
  /* Inicijalizuje se deskriptor konekcije od klijenta. */
   int client = *((int *) arg);
   free (arg);
   char idpaketa[20];
   char buffer[500];
   char query[300];
   int provider;
   int opc, dpc, rc, ni, client_dpc;
   char gt_number[20];
  Cvor_liste* korisnik_pocetak=NULL;
  Cvor_liste* korisnik_kraj=NULL;   
   
  /* Nit se prebacuje u detached stanje. */
  if ((errno = pthread_detach (pthread_self ())) != 0)
    error_fatal ("%s pthread_detach() error: %s\n", program, strerror (errno));

    struct sockaddr adresica; /* Adresa sa informacijma o peer-u. */
    socklen_t duzinaadresice; /* Velicina prethodne strukture. */
    struct sockaddr_in *addrin; /* Pomocni pokazivac. */
    char name[16]; /* Ime adrese. */
   duzinaadresice = sizeof (struct sockaddr);
     if (getpeername (client, &adresica, &duzinaadresice) < 0)
            error_fatal ("%s getpeername() error: %s\n", "program", strerror (errno));
          /* Ukoliko je u pitanju struktura za IP adrese... */
    if (duzinaadresice == sizeof (struct sockaddr_in))
          {
             /* ...racuna se prezentacija na osnovu numericke vrednosti... */
              addrin = (struct sockaddr_in *) &adresica;
         if (inet_ntop (AF_INET, &(addrin->sin_addr), name, 16) == 0)
              error_fatal ("%s inet_ntop() error: %s\n", "program", strerror (errno));
             /* ...i ispisuju IP adresa i port. */
              printf ("Peer info %s:%d\n", name, ntohs (addrin->sin_port));
	      sprintf(query, "select count(*) from users where ip='%s' and port=%d", name, ntohs (addrin->sin_port));
	      loguj(query);
	       if (mysql_query(conn, query))
	       {
                fprintf(stderr, "%s\n", mysql_error(conn));
                close(client); 
                pthread_exit (NULL);
                 }
             res = mysql_use_result(conn);
	     row = mysql_fetch_row(res);
	     printf("------------------ u  bazi imamo %d konekciju sa navedenim parametima\n", atoi(row[0]));
	     
	     
	     if(atoi(row[0])==0)
	     {
	           mysql_free_result(res);
	       close(client);
		pthread_exit (NULL);
	     }
	  }
    mysql_free_result(res);
    sprintf(query, "select opc from users where ip='%s' and port=%d", name, ntohs (addrin->sin_port));
     if (mysql_query(conn, query))
	       {
                fprintf(stderr, "%s\n", mysql_error(conn));
		close(client);
		pthread_exit (NULL);
	      }
             res = mysql_use_result(conn);
	     row = mysql_fetch_row(res);
	     client_dpc=atoi(row[0]);
         mysql_free_result(res);


   printf("New connection sa id file MAX_DESCRIPTOR-a %d.\n", client);  
   loguj("Server krece da gradi stek\n");
   izgradi_stek2(client);
   loguj("Igradili smo stek!\n");  
  
   /* posto ova nit opsluzuje samo jednog korisnika vec sada se iz baze
    * moze videt na koga treba rutirati pakete koji stignu od ovog usera*/
   
   sprintf(query, "select id_providera from routing where id_usera in (select id from users where ip='%s' and port=%d)", name, ntohs (addrin->sin_port));
   printf("%s\n", query);
   if (mysql_query(conn, query))
	       {
                fprintf(stderr, "%s\n", mysql_error(conn));
		 close(client);
		  pthread_exit (NULL);
                 }
             res = mysql_use_result(conn);
	     row = mysql_fetch_row(res);
	     printf("------------------ korisnika treba rutirati na provajdera sa id-jem %d \n", atoi(row[0]));
             provider=atoi(row[0]);
      mysql_free_result(res);

   /* uzimamo iz baze parametre za pakovanje paketa za providera */
   
    sprintf(query, "select opc, dpc, gt_number, rc, ni from providers where id=%d", provider);
   if (mysql_query(conn, query))
	       {
                fprintf(stderr, "%s\n", mysql_error(conn));
		 close(client);
		  pthread_exit (NULL);
                 }
             res = mysql_use_result(conn);
	     row = mysql_fetch_row(res);
             opc=atoi(row[0]);
             dpc=atoi(row[1]);
             rc=atoi(row[3]);
	     ni=atoi(row[4]);
             strcpy(gt_number, row[2]);
      mysql_free_result(res);
   
   FD_ZERO (&read_set);
   
  for (;;)
  {
  FD_SET (client, &read_set);
           struct timeval tv;
           tv.tv_usec = 2;
	   tv.tv_sec = 0;
	   
      
      /* Server se blokira dok se ne pojavi aktivnost na nekom
       * od deskriptora iz radnog skupa. */
      select (MAX_DESCRIPTOR, &read_set, NULL, NULL, &tv);
	  /* Ako je tekuci socket aktivan...  */
	  if (FD_ISSET (client, &read_set))
	    {
	      /* ...pokusava se sa ucitavanjem poruke od 
	       * klijenta,...  */
	      count =   recv (client, buffer, BUFFER_SIZE, 0);
	      printf("stigao je paket od korisnika duzine %d\n", count);
	      
	      if(count>20)
	      {
		 //printf("pristigao paket od korisnika\n");
		 promeni_paket(buffer, count, 1, opc, dpc, gt_number, rc, ni);
		 pthread_mutex_lock (&mutex);
		 dodaj_u_red(&pocetak_reda, &kraj_reda, buffer, count);
		 ispisi_red(pocetak_reda);
	         pthread_mutex_unlock (&mutex);
		 //printf("paket dodat u red1\n");
	         //printf("****************************************\n");
	         FD_CLR (client, &read_set);
	    }
	    
	    if(count==0)
	    {
	       FD_CLR (client, &read_set);
	       close (client); 
	       pthread_exit (NULL);

	      
	    }
	    
	}
     
  unsigned char paket[1000];    
      int vel=0;	
      
 if(korisnik_pocetak==NULL)
      {
		pthread_mutex_lock (&mutex);
                izvuci_podred_za_korisnika(&pocetak_reda2,client_dpc,&korisnik_pocetak,&korisnik_kraj);
	        pthread_mutex_unlock (&mutex);
      }
  
        vel= procitaj_prvi_u_redu(&korisnik_pocetak, &korisnik_kraj, paket);
	if(vel>0)
	{
      	printf("procitali iz reda KORISNIKA paket velicine %d\n", vel);
        vel= uzmi_iz_reda(&korisnik_pocetak, &korisnik_kraj, paket);
	if (send(client, paket, vel, 0) != vel)
         pthread_exit (NULL);
        else
	  printf("salje se paket korisniku\n");
	}
	
   }
   
   close(client);  
   pthread_exit (NULL);

}

static void* do_work (void *arg)
{

    int server;			/* Deskriptor konekcije sa serverom. */
    unsigned char buffer[BUFFER_SIZE];	/* Bafer za smestanje poruke od servera. */
    int count;			/* Broj bajtova primljenih u jednom paketu. */
    struct sockaddr_in addressclinet;
    struct sockaddr_in address = *((struct sockaddr_in *) arg);
    int dpc;
    
    char gt_number[20];
    
    Cvor_liste* provider_pocetak=NULL;
    Cvor_liste* provider_kraj=NULL;   
    
    
    if ((errno = pthread_detach (pthread_self ())) != 0)
    error_fatal ("%s pthread_detach() error: %s\n", "server", strerror (errno));
    
    printf("kreirana nit %u :) \n", pthread_self());
    if ((server =  socket (AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
    error_fatal ("%s socket() error: %s\n", "server", strerror (errno));
    //printf("kreiran socket %u \n", pthread_self());

    int poz=-1;
    int i;
    pthread_mutex_lock (&mutex);
    for(i=0; i<count_providers;i++)
      if(tid[i]==pthread_self())
	poz=i;
    pthread_mutex_unlock (&mutex); 
    
    bzero (&addressclinet, sizeof (struct sockaddr_in));
    addressclinet.sin_family = AF_INET;
    addressclinet.sin_port = htons(2904+providers[poz].id);
    
    dpc=providers[poz].dpc;
    strcpy(gt_number, providers[poz].gt_number);

    if (inet_pton (AF_INET, "185.65.107.250", &addressclinet.sin_addr) < 0)
	error_fatal ("%s inet_pton() error: %s\n", "server", strerror (errno));
    else

    if (bind (server, (struct sockaddr *) &addressclinet, sizeof (struct sockaddr_in))< 0)
         pthread_exit (NULL);

    /* Vrsi se povezivanje sa serverom. */
    if (connect (server, (struct sockaddr *) &address, sizeof (struct sockaddr_in)) < 0)
         pthread_exit (NULL);
    printf("konekcija na server uspela na %d PORTu\n",   ntohs(addressclinet.sin_port));    
    izgradi_stek(server, poz);

    fd_set read_set;		/* Skup deskriptora sa kojih
				 * server treba da cita. */
   FD_ZERO (&read_set);
   struct timeval tv;
           tv.tv_usec = 3;
	   tv.tv_sec = 0; 
    
    while(1)
   {     
      unsigned char paket[1000];    
      int vel=0;
      char idpaketa[20];
	sleep(1);

        FD_SET (server, &read_set);

	// select za 193
	 /* Server se blokira dok se ne pojavi aktivnost na nekom
       * od deskriptora iz radnog skupa. */
      select (MAX_DESCRIPTOR, &read_set, NULL, NULL, &tv);
	  /* Ako je tekuci socket aktivan...  */
	  if (FD_ISSET (server, &read_set))
	    {
	      /* ...pokusava se sa ucitavanjem poruke od 
	       * klijenta,...  */
	      vel =   recv (server, paket, BUFFER_SIZE, 0);
	       if(vel>40)
	      { 
		printf("*************************pristigao paket od providera\n");
	        int r=promeni_paket_inverzno(paket, vel, 2, gt_number, 100, 0);
		if(r==0)
		{
		pthread_mutex_lock (&mutex);
		dodaj_u_red(&pocetak_reda2, &kraj_reda2, paket, vel);
		ispisi_red(pocetak_reda2);
		pthread_mutex_unlock (&mutex);
	        printf("paket dodat u red2\n");
		}
	      }
	       if(vel==0)
	    {
	       FD_CLR (server, &read_set);
	       close (server); 
               return;	       
	    }
	}
	
	
      if(provider_pocetak==NULL)
      {
		pthread_mutex_lock (&mutex);
                izvuci_podred_za_korisnika(&pocetak_reda,dpc,&provider_pocetak,&provider_kraj);
	        pthread_mutex_unlock (&mutex);
      }
        else
	{
        vel= procitaj_prvi_u_redu(&provider_pocetak, &provider_kraj, paket);
	if(vel>0)
	{
      	printf("procitali iz reda za Providera paket velicine %d\n", vel);
        vel= uzmi_iz_reda(&provider_pocetak, &provider_kraj, paket);
	 printf("           %d \n", sctp_sendmsg(server, paket,vel, NULL, 0,(u_int32_t)htonl(3),(u_int32_t) 0,1, 0, 0));                   
	  printf("----------------------salje se paket provideru \n");	
	}
	
   }
   }
    if (close (server) < 0)
      error_fatal ("%s close() error: %s\n", "server", strerror (errno));

}
main(int arg, char** argv) {
  
   char query[300];
   char *servers = "localhost";
   char *user = "ss7user";
   char *password = "trlababalan"; 
   char *database = "mobiclick";
 
   
   int i, server;
   unsigned char buffer[BUFFER_SIZE];	/* Bafer za slanje poruke */
   char prva[10];
   char druga[50];
   char treca[500];
   char cetvrta[500];
   
   struct sockaddr_in address[10];	/* Adresa providera. */
   struct sockaddr_in address_server;
   program=argv[0];
   
  int* client[100];			/* Pokazivac na deskriptor konekcije od klijenta. */
  int br_clienta =0;			/* redni broj klijenta*/
  fd_set read_set;		/* Skup deskriptora sa kojih server treba da cita. */
  fd_set write_set;
  int count;
  
  
   FILE* ulaz=fopen("MessageClassValues.txt","r");
	if(ulaz==NULL){
		fprintf(stderr,"Problem sa otvaranjem datoteke %s!\n", "MessageClassValues.txt");
		exit(EXIT_FAILURE);
	}
	
	while(fgets(buffer, 100, ulaz)!=0){
	strcpy(prva, "");
	strcpy(druga,"");
	strcpy(treca,"");
	strcpy(cetvrta, "");

		sscanf(buffer, "%s %s %s %s", prva, druga, treca, cetvrta);
		messageClassValues=dodaj_cvor(messageClassValues, prva, druga, treca, cetvrta);
		
	}
	fclose(ulaz);
	
	ulaz=fopen("MessageTypeValue.txt","r");
	if(ulaz==NULL){
		fprintf(stderr,"Problem sa otvaranjem datoteke %s!\n", "MessageTypeValue.txt");
		exit(EXIT_FAILURE);
	}
	while(fgets(buffer, 100, ulaz)!=0){
	strcpy(prva, "");
	strcpy(druga,"");
	strcpy(treca,"");
	strcpy(cetvrta, "");
		sscanf(buffer, "%s %s %s %s", prva, druga, treca, cetvrta);
		MessageTypeValue=dodaj_cvor(MessageTypeValue, prva, druga, treca, cetvrta);
		
	}
	fclose(ulaz);
	
	ulaz=fopen("ParametarValue.txt","r");
	if(ulaz==NULL){
		fprintf(stderr,"Problem sa otvaranjem datoteke %s!\n", "ParametarValue.txt");
		exit(EXIT_FAILURE);
	}
	while(fgets(buffer, 100, ulaz)!=0){
	strcpy(prva, "");
	strcpy(druga,"");
	strcpy(treca,"");
	strcpy(cetvrta, "");
		sscanf(buffer, "%s %s %s %s", prva, druga, treca, cetvrta);
		ParametarValue=dodaj_cvor(ParametarValue, prva, druga, treca, cetvrta);
	}
	fclose(ulaz);
	

	ulaz=fopen("ParametarOrder.txt","r");
	if(ulaz==NULL){
		fprintf(stderr,"Problem sa otvaranjem datoteke %s!\n", "ParametarValue.txt");
		exit(EXIT_FAILURE);
	}
	while(fgets(buffer, 300, ulaz)!=0){
	strcpy(prva, "");
	strcpy(druga,"");
	strcpy(treca,"");
	strcpy(cetvrta, "");
		sscanf(buffer, "%s %s %s %s", prva, druga, treca,  cetvrta);
		ParametarOrder=dodaj_cvor(ParametarOrder, prva, druga, treca, cetvrta);
	}
	fclose(ulaz);
	
 	//ispisi_stablo(messageClassValues);  
   	//ispisi_stablo(MessageTypeValue);
   	//ispisi_stablo(ParametarValue);
   	//ispisi_stablo(ParametarOrder);
	
   conn = mysql_init(NULL);
   /* Connect to database */
   if (!mysql_real_connect(conn, servers,user, password, database, 0, NULL, 0)) {
      fprintf(stderr, "%s\n", mysql_error(conn));
      exit(1);
   }
   /* send SQL query */

   if (mysql_query(conn, "select * from providers where enabled=1")) {
      fprintf(stderr, "%s\n", mysql_error(conn));
      exit(1);
   }
   res = mysql_use_result(conn);
   int numRows = mysql_num_fields(res);
   while ((row = mysql_fetch_row(res)) != NULL)
   {
       providers[count_providers].id=atoi(row[0]);;
       strcpy(providers[count_providers].ip,row[1]);
       providers[count_providers].port=atoi(row[2]);;
       providers[count_providers].opc=atoi(row[3]);;
       providers[count_providers].dpc=atoi(row[4]);;
       strcpy(providers[count_providers].gt_number,row[5]);
       providers[count_providers].rc=atoi(row[6]);;
       providers[count_providers].ni=atoi(row[7]);;
        
       count_providers++;
   }
   
  for(i=0; i<count_providers; i++)
   //if(i==0)
  {
      printf("%d %s %d %d %d %s %d %d\n", providers[i].id, providers[i].ip, providers[i].port, providers[i].opc, providers[i].dpc, providers[i].gt_number, providers[i].rc, providers[i].ni);
      bzero (&address[i], sizeof (struct sockaddr_in));
      address[i].sin_family = AF_INET;
      address[i].sin_port = htons (providers[i].port);
      inet_pton (AF_INET, providers[i].ip, &address[i].sin_addr); 
      pthread_create (&tid[i], NULL, &do_work, &address[i]);  

  }
    
    
    
      
  /*
   * Kreira se socket na kome ce server prihvatati zahteve za
   * konekcijom od klijenata.
   */
  if ((server = socket (AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
    error_fatal ("%s socket() error: %s\n", argv[0], strerror (errno));

  /* Vezuje se socket za datu adresu i port servera. */
  bzero (&address_server, sizeof (struct sockaddr_in));
  address_server.sin_family = AF_INET;
  address_server.sin_port = htons (2905);
  address_server.sin_addr.s_addr = htonl (INADDR_ANY);
  
  inet_pton (AF_INET, "185.65.107.249", &address_server.sin_addr);

  if (bind (server, (struct sockaddr *) &address_server, sizeof (struct sockaddr_in))< 0)
    error_fatal ("%s server bind() error: %s\n", argv[0], strerror (errno));
  
  FD_ZERO (&read_set);
  FD_SET (server, &read_set);
  FD_ZERO (&write_set);
  
  

  /*
   * Postavlja se maksimalni broj istovremenih konekcija koje server
   * moze da prihvati.
   */
  if (listen (server, BACKLOG) < 0)
    error_fatal ("%s listen() error: %s\n", "server", strerror (errno));

  /* Server u petlji prihvata i obradjuje zahteve od klijenata. */
  while(1)
    {
      sleep(1);
           /*
       * Server se blokira dok se ne pojavi aktivnost na nekom od
       * deskriptora iz radnog skupa.
       */
      if (select (FD_SETSIZE, &read_set, &write_set, NULL, NULL) < 0)
	error_fatal ("%s select() error: %s\n", program, strerror (errno));
       /*
       * Ako je aktiviran deskriptor preko koga server prihvata
       * konekcije,...
       */
      if (FD_ISSET (server, &read_set))
	{     
       /* Alocira se memorija za deskriptor konekcije od klijenta. */
       
      if ((client[br_clienta] = malloc (sizeof (int))) == NULL)
	error_fatal ("%s malloc() error: %s\n", "server", strerror (errno));

      /* Prihvata se zahtev za konekcijom od klijenta. */
       if ((*client[br_clienta] = accept (server, NULL, NULL)) < 0)
	    {
	      if (errno == EINTR)
		continue;
	      else
		error_fatal ("%s accept() error: %s\n", "server",    strerror (errno));
	    }
	
	
    struct sockaddr adresica; /* Adresa sa informacijma o peer-u. */
    socklen_t duzinaadresice; /* Velicina prethodne strukture. */
    struct sockaddr_in *addrin; /* Pomocni pokazivac. */
    char name[16]; /* Ime adrese. */
   duzinaadresice = sizeof (struct sockaddr);
     if (getpeername (*client[br_clienta], &adresica, &duzinaadresice) < 0)
            error_fatal ("%s getpeername() error: %s\n", "program", strerror (errno));
          /* Ukoliko je u pitanju struktura za IP adrese... */
    if (duzinaadresice == sizeof (struct sockaddr_in))
          {
             /* ...racuna se prezentacija na osnovu numericke vrednosti... */
              addrin = (struct sockaddr_in *) &adresica;
         if (inet_ntop (AF_INET, &(addrin->sin_addr), name, 16) == 0)
              error_fatal ("%s inet_ntop() error: %s\n", "program", strerror (errno));
             /* ...i ispisuju IP adresa i port. */
              printf ("Peer info %s:%d\n", name, ntohs (addrin->sin_port));
	      sprintf(query, "select count(*) from users where ip='%s' and port=%d", name, ntohs (addrin->sin_port));
	      loguj(query);
	       if (mysql_query(conn, query))
	       {
                fprintf(stderr, "%s\n", mysql_error(conn));
                exit(1);
                 }
             res = mysql_use_result(conn);
	     row = mysql_fetch_row(res);
	     printf("------------------ u  bazi imamo %d konekciju sa navedenim parametima\n", atoi(row[0]));
	     
	     
	     if(atoi(row[0])!=0)
	     {
	       mysql_free_result(res);
 if ((errno = pthread_create (&tidclient[br_clienta], NULL, &opsluzi_korisnika, client[br_clienta])) != 0)
	        error_fatal ("%s pthread_create() error: %s\n", "server",   strerror (errno));    
	   br_clienta++;
	     }
	  }
       }    
	
    }

   /* close connection */
   mysql_free_result(res);
   mysql_close(conn);
   printf("zatvorena konekcija\n");
}
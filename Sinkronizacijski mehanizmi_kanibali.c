#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

//gcc -pthread kanibali.c -o c ./c

#define MIN_BR_PUTNIKA 3
#define KAPACITET_CAMCA 7
#define BR_IZGENERIRANIH_MISIONARA 4
#define BR_IZGENERIRANIH_KANIBALA  9

typedef struct { //struktura za biljezenje stanja
   char C[BR_IZGENERIRANIH_KANIBALA + BR_IZGENERIRANIH_MISIONARA][3];
   char LO[BR_IZGENERIRANIH_KANIBALA + BR_IZGENERIRANIH_MISIONARA][3];
   char DO[BR_IZGENERIRANIH_KANIBALA + BR_IZGENERIRANIH_MISIONARA][3];
} st;

st stanje;
int ind_C = 0;
int ind_LO = 0;
int ind_DO = 0;

int br_putnika_u_camcu = 0;
int br_misionara_u_camcu = 0;
int br_kanibala_u_camcu = 0;
int br_misionara_ceka_D = 0;
int br_misionara_ceka_L = 0;
int camac_vozi = 0; // 0 - ne vozi, 1 - vozi
int strana_obale = 0; // 0 - desno, 1 lijevo
int br_ceka = 0;

void print_stanje();//deklaracija funkcije
void azuriraj_stanje();//deklaracija funkcije
void obrisi_string_DO(char *string);//deklaracija funkcije
void obrisi_string_LO(char *string);//deklaracija funkcije


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_camac_ceka = PTHREAD_COND_INITIALIZER; // red za camac - ceka dok se ne napuni
pthread_cond_t cond_camac_plovi = PTHREAD_COND_INITIALIZER;// red za putnike - camac plovi
pthread_cond_t cond_kanibal_D = PTHREAD_COND_INITIALIZER;// red za kanibale na desnoj obali - cekaju dok ne smiju uci(dok ne dodje camac ili br_misionara je ok)
pthread_cond_t cond_kanibal_L = PTHREAD_COND_INITIALIZER;// red za kanibale na lijevoj obali - cekaju dok ne smiju uci(dok ne dodje camac ili br_misionara je ok)
pthread_cond_t cond_misionar_D = PTHREAD_COND_INITIALIZER; // red za misionare na desnoj obali - cekaju dok ne smiju uci(dok ne dodje camac ili br_kanibala je ok)
pthread_cond_t cond_misionar_L = PTHREAD_COND_INITIALIZER; // red za misionare na lijevoj obali - cekaju dok ne smiju uci(dok ne dodje camac ili br_kanibala je ok)


void* camac_fja(void* arg) {
   printf("C: prazan na desnoj obali\n");
   print_stanje();

   while(1) {

      pthread_mutex_lock(&mutex);

      while(br_putnika_u_camcu < 3) pthread_cond_wait(&cond_camac_ceka, &mutex);

      printf("C: tri putnika ukrcana, polazim za jednu sekundu\n");

      pthread_mutex_unlock(&mutex);
      sleep(1);

      pthread_mutex_lock(&mutex);
      camac_vozi = 1;
      if(strana_obale == 0 ){
         printf("C: prevozim s desne na lijevu obalu:");
         int i; 
         for (i = 0; i < 7; i++) {
            if (strlen(stanje.C[i]) > 0)printf("%s ",  stanje.C[i]);
         }
         printf("\n\n");
      } else {
         printf("C: prevozim s lijeve na desnu obalu:");
         int i;
         for (i = 0; i < 7; i++) {
            if (strlen(stanje.C[i]) > 0)printf("%s ",  stanje.C[i]);
         }
         printf("\n\n");
      }

      sleep(2);//trajanje puta
      
      if(strana_obale == 0 ){
         printf("C: preveo s desne na lijevu obalu:");
         int i; 
         for (i = 0; i < 7; i++) {
            if (strlen(stanje.C[i]) > 0)printf("%s ",  stanje.C[i]);
         }
         printf("\n");
      } else {
         printf("C: preveo s lijeve na desnu obalu:");
         int i;
         for (i = 0; i < 7; i++) {
            if (strlen(stanje.C[i]) > 0)printf("%s ",  stanje.C[i]);
         }
         printf("\n");
      }
      azuriraj_stanje();

      camac_vozi = 0;
      pthread_cond_broadcast(&cond_camac_plovi);//pusti sve putnike
      if(strana_obale == 0){
         printf("C: prazan na lijevoj obali\n");
         strana_obale = 1;
         print_stanje();
         pthread_cond_broadcast(&cond_kanibal_L);
         pthread_cond_broadcast(&cond_misionar_L);
      }  else  {
         printf("C: prazan na desnoj obali\n");
         strana_obale = 0;
         print_stanje();
         pthread_cond_broadcast(&cond_kanibal_D);
         pthread_cond_broadcast(&cond_misionar_D);
      }
      
      pthread_mutex_unlock(&mutex);
   }
   
}


void* misionar_fja_D(void* arg) {
   int misionar_id;
   misionar_id = *((int *)arg) + 1;
   printf("M%d došao na desnu obalu\n", misionar_id);
   br_misionara_ceka_D++;
   sprintf(stanje.DO[ind_DO++], "M%d", misionar_id);
   print_stanje();

   pthread_mutex_lock(&mutex);
  if(br_putnika_u_camcu > 0 || strana_obale == 1) {

    while( (br_putnika_u_camcu >= 7 || strana_obale == 1 
                  || camac_vozi
                  || (br_kanibala_u_camcu > (br_misionara_u_camcu + 1 ))    )
               && ( br_misionara_ceka_D + br_misionara_u_camcu < br_kanibala_u_camcu 
                  ||  strana_obale == 1) ) pthread_cond_wait(&cond_misionar_D, &mutex);
   
  }
   
   sprintf(stanje.C[ind_C], "M%d", misionar_id);
   obrisi_string_DO(stanje.C[ind_C]);
   ind_C++;
   br_putnika_u_camcu++;
   printf("M%d ušao u čamac\n", misionar_id);
   //printf("indD0 = %d\n", ind_DO);
   br_misionara_ceka_D--;
   br_misionara_u_camcu++;
   ind_DO--;
   print_stanje();
   if(br_putnika_u_camcu >= 3) pthread_cond_signal(&cond_camac_ceka);
   pthread_cond_broadcast(&cond_kanibal_D);//povuci kanibala
   pthread_cond_wait(&cond_camac_plovi, &mutex);
   //printf("M%d stigao i umro\n", misionar_id);
   pthread_mutex_unlock(&mutex);
   return NULL;
   
}

void* misionar_fja_L(void* arg) {
   int misionar_id;
   misionar_id = *((int *)arg) + 1;
   printf("M%d došao na lijevu obalu\n", misionar_id);
   br_misionara_ceka_L++;
   sprintf(stanje.LO[ind_LO++], "M%d", misionar_id);
   print_stanje();

   pthread_mutex_lock(&mutex);
   if(br_putnika_u_camcu > 0 || strana_obale == 0)  {

      while( (br_putnika_u_camcu >= 7 || strana_obale == 0 
                  || camac_vozi 
                  || (br_kanibala_u_camcu > (br_misionara_u_camcu + 1)))
             && ( br_misionara_ceka_L + br_misionara_u_camcu < br_kanibala_u_camcu 
                  ||  strana_obale == 0)    ) pthread_cond_wait(&cond_misionar_L, &mutex);
   
   }
   
   sprintf(stanje.C[ind_C], "M%d", misionar_id);
   obrisi_string_LO(stanje.C[ind_C]);
   ind_C++;
   br_putnika_u_camcu++;
   printf("M%d ušao u čamac\n", misionar_id);
   //printf("indL0 = %d\n", ind_LO);
   br_misionara_ceka_L--;
   br_misionara_u_camcu++;
   ind_LO--;
   print_stanje();
   if(br_putnika_u_camcu >= 3) pthread_cond_signal(&cond_camac_ceka);
   pthread_cond_broadcast(&cond_kanibal_L);// povuci kanibala
   pthread_cond_wait(&cond_camac_plovi, &mutex);
   //printf("M%d stigao i umro\n", misionar_id);
   pthread_mutex_unlock(&mutex);
   return NULL;
  
}

void* kanibal_fja_D(void* arg) {
   int kanibal_id;
   kanibal_id = *((int *)arg) + 1;
   printf("K%d došao na desnu obalu\n", kanibal_id);
   sprintf(stanje.DO[ind_DO++], "K%d", kanibal_id);
   print_stanje();

   pthread_mutex_lock(&mutex);
   if(br_putnika_u_camcu > 0 || strana_obale == 1){

      while(br_putnika_u_camcu >= 7 || strana_obale == 1 
            || camac_vozi
                  || ( br_kanibala_u_camcu >= br_misionara_u_camcu && br_misionara_u_camcu != 0  )) pthread_cond_wait(&cond_kanibal_D, &mutex);
   
   }
   
   sprintf(stanje.C[ind_C], "K%d", kanibal_id);
   obrisi_string_DO(stanje.C[ind_C]);
   ind_C++;
   br_putnika_u_camcu++;
   printf("K%d ušao u čamac\n", kanibal_id);
   //printf("indD0 = %d\n", ind_DO);
   ind_DO--;
   br_kanibala_u_camcu++;
   print_stanje();
   if(br_putnika_u_camcu >= 3) pthread_cond_signal(&cond_camac_ceka);
   //pthread_cond_broadcast(&cond_kanibal_D);
   //pthread_cond_broadcast(&cond_misionar_D);
   pthread_cond_wait(&cond_camac_plovi, &mutex);
   //printf("K%d stigao i umro\n", kanibal_id);
   pthread_mutex_unlock(&mutex);
   return NULL;
   
}

void* kanibal_fja_L(void* arg) {
   int kanibal_id;
   kanibal_id = *((int *)arg) + 1;
   printf("K%d došao na lijevu obalu\n", kanibal_id);
   sprintf(stanje.LO[ind_LO++], "K%d", kanibal_id);
   print_stanje();

   pthread_mutex_lock(&mutex);
   
   if(br_putnika_u_camcu > 0 || strana_obale == 0) {

      while(br_putnika_u_camcu >= 7 
            || strana_obale == 0
                  || ( br_kanibala_u_camcu >= br_misionara_u_camcu && br_misionara_u_camcu != 0  )) pthread_cond_wait(&cond_kanibal_L, &mutex);
   
   }
   sprintf(stanje.C[ind_C], "K%d", kanibal_id);
   obrisi_string_LO(stanje.C[ind_C]);
   ind_C++;
   br_putnika_u_camcu++;
   printf("K%d ušao u čamac\n", kanibal_id);
   //printf("indL0 = %d\n", ind_LO);
   ind_LO--;
   br_kanibala_u_camcu++;
   print_stanje();
   if(br_putnika_u_camcu >= 3) pthread_cond_signal(&cond_camac_ceka);
   //pthread_cond_broadcast(&cond_kanibal_L);
   //pthread_cond_broadcast(&cond_misionar_L);
   pthread_cond_wait(&cond_camac_plovi, &mutex);
   //printf("K%d stigao i umro\n", kanibal_id);
   pthread_mutex_unlock(&mutex);
   return NULL;
   
}

void* gen_misionar_fja(void* arg) {
   
   pthread_t dretve[BR_IZGENERIRANIH_MISIONARA];

   int i;
   for( i = 0; i < BR_IZGENERIRANIH_MISIONARA; i++) {
      int random_strana = rand() % 2;
      
      sleep(2);
      if(random_strana == 0) { // random odabrana desna strana

         if(pthread_create(&dretve[i], NULL, misionar_fja_D, &i)) {
            printf("Neuspješno generiranje misionara M%d", i+1);
         }

      } else { //random odabrana lijeva strana

         if(pthread_create(&dretve[i], NULL, misionar_fja_L, &i)) {
            printf("Neuspješno generiranje misionara M%d", i+1);
         }
      }
   }
   for( i = 0; i < BR_IZGENERIRANIH_MISIONARA; i++) {//cekanje da dretve zavrse
      pthread_join(dretve[i], NULL);
   }
    
}


void* gen_kanibal_fja(void* arg) {

   pthread_t dretve[BR_IZGENERIRANIH_KANIBALA];

   int i;
   for( i = 0; i < BR_IZGENERIRANIH_KANIBALA; i++) {
      int random_strana = rand() % 2;

      sleep(1);

      if(random_strana == 0) { // random odabrana desna strana

         if(pthread_create(&dretve[i], NULL, kanibal_fja_D, &i)) {
            printf("Neuspješno generiranje misionara M%d", i+1);
         }
         
      } else { //random odabrana lijeva strana

         if(pthread_create(&dretve[i], NULL, kanibal_fja_L, &i)) {
            printf("Neuspješno generiranje kanibala M%d", i+1);
         }
      }
   }

   for( i = 0; i < BR_IZGENERIRANIH_KANIBALA; i++) {//cekanje da dretve zavrse
      pthread_join(dretve[i], NULL);
   }
}

void print_stanje() {

   int i;

   if(strana_obale == 0) printf("C[D]={"); else printf("C[L]={");
   for (i = 0; i < 7; i++) {
        if (strlen(stanje.C[i]) > 0)printf("%s ",  stanje.C[i]);
   }
    printf("} ");
   
   printf("LO={");
   for (i = 0; i < BR_IZGENERIRANIH_KANIBALA + BR_IZGENERIRANIH_MISIONARA; i++) {
        if (strlen(stanje.LO[i]) > 0)printf("%s ",  stanje.LO[i]);
    }
    printf("} ");

    printf("DO={");
    for (i = 0; i < BR_IZGENERIRANIH_KANIBALA + BR_IZGENERIRANIH_MISIONARA; i++) {
        if (strlen(stanje.DO[i]) > 0)printf("%s ",  stanje.DO[i]);
    }
    printf("} \n\n");
   
}

void azuriraj_stanje() {
 
   // Praznjenje camca
   for (int i = 0; i < BR_IZGENERIRANIH_KANIBALA + BR_IZGENERIRANIH_MISIONARA; i++) {
      strcpy(stanje.C[i], "");
   }
   //azuriranje brojaca putnika
   br_putnika_u_camcu = 0;
   br_kanibala_u_camcu = 0;
   br_misionara_u_camcu = 0;
   //azuriranje indeksa za polje camac
   ind_C = 0;
   
}

void obrisi_string_DO(char *string) {
    for (int i = 0; i < BR_IZGENERIRANIH_MISIONARA + BR_IZGENERIRANIH_KANIBALA; i++) {
        if (strcmp(string, stanje.DO[i]) == 0) {  // provjerava se jednakost stringova
            stanje.DO[i][0] = '\0';  // prvi karakter stringa se postavlja na nulu
            return;
        }
    }
}

void obrisi_string_LO(char *string) {
    for (int i = 0; i < BR_IZGENERIRANIH_MISIONARA + BR_IZGENERIRANIH_KANIBALA; i++) {
        if (strcmp(string, stanje.LO[i]) == 0) {  // provjerava se jednakost stringova
            stanje.LO[i][0] = '\0';  // prvi karakter stringa se postavlja na nulu
            return;
        }
    }
}


int main(void) {

   srand(time(NULL));

   printf("Legenda: M-misionar, K-kanibal, C-čamac,\n");
   printf("         LO-lijeva obala, DO-desna obala\n");
   printf("         L-lijevo, D-desno\n\n");
                    

   pthread_t camac, gen_misionar, gen_kanibal;

   // Stvaranje dretve camac
    pthread_create(&camac, NULL, camac_fja, NULL);

   // Stvaranje pomocne dretve za generiranje misionara 
    pthread_create(&gen_misionar, NULL, gen_misionar_fja, NULL);
    
    // Stvaranje pomocne dretve za generiranje kanibala
    pthread_create(&gen_kanibal, NULL, gen_kanibal_fja, NULL);

    // Čekanje da se sve dretve završe
    pthread_join(camac, NULL);
    pthread_join(gen_misionar, NULL);
    pthread_join(gen_kanibal, NULL);


   return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <stack>
#include <iostream>

using namespace std;

//program za trenutno vrijeme
struct timespec t0; 
void postavi_pocetno_vrijeme(){
	clock_gettime(CLOCK_REALTIME, &t0);
}

/* dohvaca vrijeme proteklo od pokretanja programa */
void vrijeme(void){
	struct timespec t;
	clock_gettime(CLOCK_REALTIME, &t);
	t.tv_sec -= t0.tv_sec;
	t.tv_nsec -= t0.tv_nsec;
	if (t.tv_nsec < 0) {
		t.tv_nsec += 1000000000;
		t.tv_sec--;
	}
	printf("%03ld.%03ld:\t", t.tv_sec, t.tv_nsec/1000000);
}

//deklaracija funkcije koja ce obraditi prekide -> poslane signale
void obradi_signal(int sig);
//deklaracija fje za ispis
void ispis_stanja();
//pomocne podatkovne strukture kao globalne varijable
int K_Z[3] = {0,0,0};
int T_P = 0;

void spavaj(time_t sekundi)
{
	struct timespec koliko;
	koliko.tv_sec = sekundi;
	koliko.tv_nsec = 0;

	while (nanosleep(&koliko, &koliko) == -1 && errno == EINTR) {
         //PRINTF("Bio prekinut, nastavljam\n");
   }
		
}

//stog na koji cu stavljati stanja podatkovnih struktura
stack<int> stog;
stack<int> stog_pom;


int main(void) {

   struct sigaction act;

    act.sa_handler = obradi_signal;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGUSR1, &act, NULL);

    act.sa_handler = obradi_signal;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGINT, &act, NULL);

    act.sa_handler = obradi_signal;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGTERM, &act, NULL);

   postavi_pocetno_vrijeme();
   
   vrijeme();//ispis vremena
   printf("Program s PID=%ld krenuo s radom\n", (long) getpid());
   vrijeme();//ispis vremena
   //ispis stanja pomocnih struktura i stoga
   ispis_stanja();


   int radi = 1;
   while(radi) {
      
   }

   return 0;
}

/*
Koristim signale:
SIGINT - zahtjev 3 ----> broj signala = 2
SIGUSR1 - zahtjev 2 ----> broj signala = 10
SIGTERM - zahtjev 1 ----> broj signala = 15
*visa zahtjev - visi prioritet
*/

void obradi_signal(int sig){

   int razina_signala;
   if(sig ==  2) razina_signala = 3;
   else if(sig == 10) razina_signala = 2;
   else if(sig == 15) razina_signala = 1;

   K_Z[razina_signala - 1] = 1;


   while((K_Z[0] != 0 || K_Z[1] != 0 || K_Z[2] != 0)) {

      if(razina_signala < T_P) {
         printf("\n");
         vrijeme();
         printf("SKLOP: Dogodio se prekid razine %d  ali se on pamti i ne prosljeđuje procesoru\n", razina_signala);
         vrijeme();
         ispis_stanja();
         razina_signala = 5;
         break;
      } else {
         int zastavica;
         if(K_Z[0] == 1) zastavica = 1;
         if(K_Z[1] == 1) zastavica = 2;
         if(K_Z[2] == 1) zastavica = 3;

         if(zastavica > T_P) {

            if(zastavica == razina_signala) {
               printf("\n");
               vrijeme();
               printf("SKLOP: Dogodio se prekid razine %d i prosljeđuje se procesoru\n", zastavica);
               vrijeme();
            } else {
               printf("\n");
               vrijeme();
               printf("SKLOP: promijenio se T_P, i prosljeđuje prekid razine %d procesoru", zastavica);
            }
            
            K_Z[zastavica - 1] = 0;
            stog.push(T_P);
            T_P = zastavica;
            printf("\n");
            vrijeme();
            printf("Počela obrada prekida razine %d\n", zastavica);
            vrijeme();
            ispis_stanja();
            spavaj(10);

            
         }

         printf("\n");
         vrijeme();
         printf("Završila obrada prekida razine %d\n", T_P);
         T_P = stog.top();
         stog.pop();

         if(T_P == 0) {
            printf("\n");
            vrijeme();
            printf("Nastavlja se izvođenje glavnog programa\n");
            vrijeme();
            ispis_stanja();

         } else {
            printf("\n");
            vrijeme();
            printf("Nastavlja se obrada prekida razine %d\n", T_P);
            vrijeme();
            ispis_stanja();
         }


      }
   }      
}


void ispis_stanja() {
   printf("K_Z=");
   for(int i = 0; i < 3;i++) {
      printf("%d", K_Z[i]);
   }
   printf(", T_P=%d, stog: ", T_P);

   if(stog.empty()) {
      printf("-");
   } else {
      string s;
      int i = 0;
      while(!stog.empty() ) {
         if(i == 0) {
            cout << stog.top() << ", ";
            cout << "reg[" << stog.top() << "] ";
         } else {
            cout << ", " << stog.top() << ", ";
            cout << "reg[" << stog.top() << "] ";
         }
         i++;
         stog_pom.push(stog.top()); 
         stog.pop();
      }
      //vracanje na originalni stog
      while(!stog_pom.empty()) {
         stog.push(stog_pom.top());
         stog_pom.pop();
      }
   }
   printf("\n");

}
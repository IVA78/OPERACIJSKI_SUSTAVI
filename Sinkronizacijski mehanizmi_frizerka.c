#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <sys/sem.h>
#include <time.h>

#define N 4 //broj stolica u cekaonici je N - 1
#define BR_KLIJENATA 5 //broj klijenata koji dolaze

/*
To lock a semaphore or wait we can use the sem_wait function:
int sem_wait(sem_t *sem);

To release or signal a semaphore, we use the sem_post function:
int sem_post(sem_t *sem);

A semaphore is initialised by using sem_init(for processes or threads) or sem_open (for IPC).
sem_init(sem_t *sem, int pshared, unsigned int value);
Where,
sem : Specifies the semaphore to be initialized.
pshared : This argument specifies whether or not the newly initialized semaphore is
          shared between processes or between threads. A non-zero value means the semaphore
          is shared between processes and a value of zero means it is shared between threads.
value : Specifies the value to assign to the newly initialized semaphore.

To destroy a semaphore, we can use sem_destroy.
sem_destroy(sem_t *mutex);

To declare a semaphore, the data type is sem_t

***
Nakon deklaracije, semafor se inicijalizira 
pomoću funkcije sem_init(), koja prima tri argumenta: 
adresu semafora, vrijednost koju se želi inicijalizirati 
(0 za binarni, veću od 0 za opći semafor),
 i broj procesa koji mogu koristiti semafor istovremeno.
*/

typedef struct {

   //semafor za frizerku - dosao klijent (obavijest)
   sem_t semafor_frizerka;
   //semafor za klijenta - frizerka uzima klijenta za sisanje
   sem_t semafor_klijent;
   //semafor za stolac - frizerka radi frizuru
   sem_t semafor_stolac;
   //semafor za medjusobno iskljucivanje procesa - pristup zajednickim varijablama
   sem_t semafor_pristup;

    
   //polje za biljezenje klijenata
   int klijenti_u_cekaonici[N];
   //indeks prvog klijenta
   int prvi;
   //indeks zadnjeg klijenta
   int zadnji;
   //otvoreno
   bool otvoreno;
   //kraj radnog vremena
   bool kraj_radnog_vremena;
   //frizerka spava
   int ispisano;
   //redni broj klijenta u cekaoni
   int red_br;

} zajednicki_spremnik;

zajednicki_spremnik *spremnik;

int shm_ID;



void frizerka() {

    printf("Frizerka: Otvaram salon\n");
    spremnik->otvoreno = true;
    printf("Frizerka: Postavljam znak OTVORENO\n");

    while (1) {

        //inicijalno 1
        sem_wait(&spremnik->semafor_pristup);

        //ako je kraj radnog vremena, stavi da je zatvoreno
        if(spremnik->kraj_radnog_vremena) {
            spremnik->otvoreno = false;
            printf("Frizerka: Postavljam znak ZATVORENO\n");
        }
        
        //ako ima klijenata u cekaonici
        if (spremnik->prvi != spremnik->zadnji) {
            
            spremnik->ispisano = 0;
            int customer = spremnik->klijenti_u_cekaonici[spremnik->prvi];
            spremnik->prvi = (spremnik->prvi + 1) % N;
            printf("Frizerka: Idem raditi na klijentu %d\n", customer);

            sem_post(&spremnik->semafor_stolac);
            
            sem_post(&spremnik->semafor_pristup);
            sleep(rand() % 5 + 1);
            printf("Frizerka: Klijent %d gotov\n", customer);
            sem_post(&spremnik->semafor_klijent);
            
        } 
        //ako nema klijenata u cekaonici, a nije kraj radnog vremena
         else if(!spremnik->kraj_radnog_vremena){
            sem_post(&spremnik->semafor_pristup);

            if(!spremnik->ispisano){
                printf("Frizerka: Spavam dok klijenti ne dođu\n");
                spremnik->ispisano = 1;
            }
            

            sem_wait(&spremnik->semafor_frizerka);
            
        } //ako nema klijenata i kraj je radnog vremena
         else {
            printf("Frizerka: Zatvaram salon\n");
            exit(0);
        }
    }
}

void klijent(int x) {
      
    //sleep(rand() % 5 + 1);
    printf("          Klijent(%d): Želim na frizuru\n", x);
    sem_wait(&spremnik->semafor_pristup);
    if (((spremnik->zadnji + 1) % N != spremnik->prvi) && spremnik->otvoreno) {
        spremnik->klijenti_u_cekaonici[spremnik->zadnji] = x;
        spremnik->zadnji = (spremnik->zadnji + 1) % N;
        spremnik->red_br++;
        printf("          Klijent(%d): Ulazim u čekaonicu (%d)\n", x, spremnik->red_br);
        sem_post(&spremnik->semafor_pristup);
        sem_post(&spremnik->semafor_frizerka);
        sem_wait(&spremnik->semafor_stolac);
        spremnik->red_br--;
        printf("          Klijent(%d):  frizerka mi radi frizuru\n", x);
        sem_wait(&spremnik->semafor_klijent);
        
    } else {
        printf("          Klijent(%d): Nema mjesta u čekaoni, vratit ću se sutra\n", x);
        sem_post(&spremnik->semafor_pristup);
        exit(0);
    }
}

int main() {

    srand(time(NULL));

    shm_ID = shmget(IPC_PRIVATE, sizeof(zajednicki_spremnik), IPC_CREAT | IPC_EXCL |  S_IRUSR | S_IWUSR);


   if(shm_ID == -1) {
      exit(0);
   }

   //shared memory attach
   spremnik = (zajednicki_spremnik *) shmat(shm_ID, NULL, 0);

   //inicijalizacija zajednickih varijabli
    sem_init(&spremnik->semafor_frizerka, N +1, 0);
    sem_init(&spremnik->semafor_klijent, N +1, 0);
    sem_init(&spremnik->semafor_pristup, N +1, 1);
    sem_init(&spremnik->semafor_stolac, N + 1, 0);
    spremnik->prvi = 0;
    spremnik->zadnji = 0;
    spremnik->otvoreno = false;
    spremnik->kraj_radnog_vremena = false;
    spremnik->ispisano = 0;
    spremnik->red_br = 0;


    //stvaranje procesa frizerka
    if(fork() == 0) {
      //posao frizerke - napisan u funkciji frizerka
      frizerka();
      exit(0);
    }
    
    //stvaranje procesa klijenti
    int i;
    for(i = 0; i < BR_KLIJENATA; i++) {
      sleep(rand() % 2 + 1);
      if(fork() == 0) {
         klijent(i+1);
         exit(0);
      }
    }   
    
    
    //stvaranje jos nekoliko nakon prvih 5
    for(int j = i; j < BR_KLIJENATA + i - 3; j++) {
      sleep(rand() % 2 + 1);
      if(fork() == 0) {
         klijent(j+1);
         exit(0);
      }
    }

   sleep(20);
   spremnik->kraj_radnog_vremena = true;
   sem_post(&spremnik->semafor_frizerka);



    //pricekaj sve procese da zavrse
   for(int i = 0; i < BR_KLIJENATA + 1; i++) {
      (void)wait(NULL);
   }

   //oslobađanje zajedničke memorije
   //shared memory detach
   (void) shmdt((zajednicki_spremnik *) spremnik);
   //Shared memory control operations - Remove the shared 
   //memory identified specified by shmid from the system 
   //and destroy the shared memory segment and shmid_ds data structure associated with shmid. 
   (void) shmctl (shm_ID, IPC_RMID, NULL);

    

    return 0;
}

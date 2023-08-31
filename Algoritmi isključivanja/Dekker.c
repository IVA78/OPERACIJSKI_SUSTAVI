/*Zadatak 2a: Napisati program koji stvara dva procesa.
 Svaki proces povećava zajedničku varijablu A za 1 u petlji M  puta.
 Parametar M zadati kao argument iz komandne linije. 
 Sinkronizirati ta dva procesa Dekkerovim algoritmom
*/

#include <stdio.h>
#include  <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>


typedef struct {
   int pravo;
   int Z[2];
   long long int A;

} zajednicki_spremnik;
zajednicki_spremnik *spremnik;

//identifikacijski broj segmenta
int ID;

void proces0(int M) {
   printf("Evo me u procesu %d\n", 0);

   spremnik->Z[0] = 1;//dizem zastavicu procesa 0
   while(spremnik->Z[1] == 1) {
      if(spremnik->pravo != 0) {
         spremnik->Z[0] = 0;
         while(spremnik->pravo != 0);//radno cekanje sve dok ne dobije pravo
         spremnik->Z[0] = 1;

      }
   }

   //kriticni odsijek
   for(int i = 0; i < M; i++) {
      spremnik->A = spremnik->A + 1;
   }
   printf("Evo me u procesu %d, odradio posao, A = %lld\n", 0, spremnik->A);

   spremnik->pravo = 1;//dajem pravo procesu 1
   spremnik->Z[0] = 0;
}

void proces1(int M) {
   printf("Evo me u procesu %d\n", 1);
   spremnik->Z[1] = 1;//dizem zastavicu procesa 1
   while(spremnik->Z[0] == 1 ){
      if(spremnik->pravo != 1) {
         spremnik->Z[1] = 0;
         while(spremnik->pravo != 1);//radno cekanje
         spremnik->Z[1] = 1;
      }
   }

   //kriticni odsjecak
   for(int i = 0; i < M; i++) {
      spremnik->A = spremnik->A + 1;
   };
   printf("Evo me u procesu %d, odradio posao, A = %lld\n", 1, spremnik->A);

   spremnik->pravo = 0;//dajem pravo procesu 0
   spremnik->Z[1] = 0;
}

int main(void) {

   /*
   ZAUZIMANJE ZAJEDNICKE MEMORIJE
   -shmget vraca identifikator dijeljenje memorije
   -int shmget(key_t key, size_t size, int shmflg);

   IPC_CREAT
   Create a shared memory segment if the key specified does not 
   already have an associated ID. IPC_CREAT is ignored when 
   IPC_PRIVATE is specified.

   IPC_EXCL
   Causes the shmget() function to fail if the key specified 
   has an associated ID. IPC_EXCL is ignored when IPC_CREAT 
   is not specified or IPC_PRIVATE is specified.

   S_IRUSR
   Permits read access when the effective user ID of the caller
   matches either shm_perm.cuid or shm_perm.uid.

   S_IWUSR
   Permits write access when the effective user ID of the caller 
   matches either shm_perm.cuid or shm_perm.uid.
   */
   
   ID = shmget(IPC_PRIVATE, sizeof(zajednicki_spremnik), IPC_CREAT | IPC_EXCL |  S_IRUSR | S_IWUSR);

   //ako dijeljena memorija nije uspjesno zauzeta, good bye
   if(ID == -1) {
      exit(0);
   }

   //shared memory attach
   spremnik = (zajednicki_spremnik *) shmat(ID, NULL, 0);
   //inicijalizacija zajednickih varijabli
   spremnik->A = 0;
   spremnik->pravo = 0;
   spremnik->Z[0] = 0;
   spremnik->Z[1] = 0;

   int M;
   printf("Upišite vrijednost varijable M > \n");
   scanf("%d", &M);

   //stvaranje paralelnih procesa

   if(fork() == 0) {
      proces0(M);
      exit(0);
   }

   if(fork() == 0) {
      proces1(M);
      exit(0);
   }
   
   (void) wait(NULL);//ceka dijete koje prvo zavrsi
   (void) wait(NULL);//ceka dijete koje drugo zavrsi

   printf("Konacna vrijednost zajedničke varijable A je %lld\n", spremnik->A);

   //oslobađanje zajedničke memorije
   //shared memory detach
   (void) shmdt((zajednicki_spremnik *) spremnik);
   //Shared memory control operations - Remove the shared 
   //memory identified specified by shmid from the system 
   //and destroy the shared memory segment and shmid_ds data structure associated with shmid. 
   (void) shmctl (ID, IPC_RMID, NULL);



   return 0;
}
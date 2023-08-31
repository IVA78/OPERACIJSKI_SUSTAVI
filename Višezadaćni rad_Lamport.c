/* Napisati program koji stvara N dretvi.
   Svaka dretva povećava zajedničku varijablu A za 1 u petlji M  puta. 
   Parametre N i M zadati kao argument iz komandne linije.
   Sinkronizirati dretve Lamportovim algoritmom 
*/

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

long long int A;
int M, N;//zajednicka varijabla svim dretvama
int *ulaz_p;//pointer na polje ulaz
int *broj_p;//pointer na polje broj


void *pocetna_fja(void *x) {

   //printf("Evo me u dretvi\n");

   int id = *((int *)x);
   //printf("moj id: %d\n", id);
   
      *(ulaz_p + id) = 1;

      //pronadji najveci
      long long int max = 0;
      for(int i = 0; i < N; i++) {
         if(*(broj_p + i) > max) {
            max = *(broj_p + i);
         }
      }
      *(broj_p + id) = max + 1;
      *(ulaz_p + id) = 0;

      for(int i = 0; i < N; i++) {
         while(*(ulaz_p + i) == 1);
         while(*(broj_p + i) != 0 && ( (*(broj_p + i) < *(broj_p + id)) || ( (*(broj_p + i) == *(broj_p + id)) && (i < id) ) ));
      }

      //pocetak kriticnog odsjecka
      for(int i = 0; i < M; i++) {
         A++;
      }
      //printf("Radim svoj posao\n");

      *(broj_p + id) = 0;
   

    //pthread_exit(NULL);

}

int main(void) {

   printf("Unesite vrijednost varijable N > \n");
   scanf("%d", &N);
   printf("Unesite vrijednost varijable M > \n");
   scanf("%d", &M);


   pthread_t thread_id[N];//stvarat cu N dretvi, treba mi N id-jeva
   A = 0;

   int ulaz[N];
   ulaz_p = &ulaz[0];
   int broj[N];
   broj_p = &broj[0];

   for(int i = 0; i < N; i++) {//inicijalne vrijednosti  su nula
      ulaz[i] = 0;
      broj[i] = 0;
   }

   for(int i = 0; i < N; i++) {
      //printf("Stvaram dretvu %d\n", i);
      thread_id[i] = i;

      /*int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                   void *(*start_routine) (void *), void *arg);

        1. A pointer to a pthread_t variable that will be used to store the ID of the new thread.
        2. A pointer to a pthread_attr_t structure that specifies the attributes of the new thread, 
           such as its scheduling policy, stack size, and so on. If you want to use the default attributes, 
           you can pass NULL as this argument.
        3. A pointer to the function that will be executed in the new thread.
        4. A void pointer that can be used to pass arguments to the function that will be executed in the new thread.
      */
      if (pthread_create(&thread_id[i], NULL, pocetna_fja, &i) != 0) {
         printf("Nisam uspio stvoriti dretvu\n");
         exit(1);
      }
      //sleep(5);
   }

   for(int i = 0; i < N; i++) {
      pthread_join(thread_id[i], NULL);
   }

   printf("Konacna vrijednost zajedničke varijable A je %lld\n", A);

   return 0;
}
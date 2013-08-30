/**
 * shm.c
 *
 * \brief Benchmark de la mémoire partagée face au heap
 *
 * On calcul les temps d'accès en shm et en heap pour voir si l'une des deux
 * méthodes de communication est plus rapide que l'autre
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <pthread.h>
#include <err.h>
#include <unistd.h>

#include "benchmark.h"

#define ARRAY_LEN 60000
#define KEY 1252
#define DO 800

/**
 * \brief Sert à stocker les arguments passés aux threads
 *
 * `args` est la structure utilisé pour faire passer les arguments aux threads.
 * `t` et `r` sont le timer et le recorder pour les mesures du temps
 * `array` est le tableau et `size` est sa taille
 */

typedef struct args {
	timer* t;
	recorder* r;
	int* array;
	int size;
} args;

/**
 * \brief Fonction à réaliser par le thread: addition d'élements d'un tableau
 *
 * \param `param` Structure args contenant tous les élements nécessaires au thread
 */

void* work (void* param) {
	args* a = (args*) param;
	start_timer(a->t);
	int* array = a->array;
	int size = a->size;
	int k;
	for (k=0; k<DO; k++) {
		// on refait la même opération plusieurs fois pour avoir des résultats consistents
		int i;
		int tot = 0;
		for (i = 0; i<size-1; i++) {
			tot += array[i];
		}
		array[size-1] = tot;
	}
	//printf("%d\n",tot);
	write_record_n(a->r,size,stop_timer(a->t),ARRAY_LEN);
	return NULL;
}

/**
 * \brief Benchmark des temps d'accées en heap et en shm
 *
 * On compare ici les temps d'accès en shm et en heap. Pour cela on va additionner
 * entre eux les élements d'une matrice de plus en plus grande, dans le heap et
 * dans le shm, et enregistrer le temps que cela prend.
 */

int main (int argc, char *argv[])  {
	timer *t = timer_alloc();
	recorder *heap_rec = recorder_alloc("heap.csv");
	recorder *shm_rec = recorder_alloc("shm.csv");
	pthread_t thread;
	
	int i;
	for (i=100; i<=ARRAY_LEN; i+=1000) {
		//start_timer(t);
		int* array = (int*)malloc(sizeof(int)*i);
		
		int j;
		for (j=0; j<i; j++) {
			array[j] = j+1;
		}
		
		args* arg = (args*)malloc(sizeof(arg));
		arg->array = array;
		arg->t = t;
		arg->r = heap_rec;
		arg->size = i;
		
		pthread_create(&thread,NULL,work,(void*)arg);
		pthread_join(thread,NULL);
		free(array);
		free(arg);
		}
		//write_record_n(heap_rec,i,stop_timer(t),ARRAY_LEN);
		//free(array);
		
	
	for (i=100; i<=ARRAY_LEN; i+=1000) {
		
		int shm_id = shmget(KEY, sizeof(int)*i, IPC_CREAT | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );
		if (shm_id < 0) err(-1, "erreur lors de shmget");
		int* array = shmat(shm_id,NULL,0);
		//if (array = -1) err(-1, "erreur lors de shmat");
		
		//start_timer(t);
		
		int j;
		for (j=0; j<i; j++) {
			array[j] = j+1;
		}
		array[0] = i;
	
		pid_t pid = fork();
	
		if (pid < 0) {
			err(pid,"erreur de fork");
		} else if (pid == 0) {
			/*
			int shm_id = shmget(KEY, sizeof(int)*i, S_IRUSR | S_IWUSR | S_IRGRP |S_IWGRP );
			if (shm_id < 0) err(-1, "erreur lors de shmget dans le fils");
			int* array = shmat(shm_id,NULL,0);
			//if (array = (void*)-1) err(-1, "erreur lors de shmat dans le fils");
			*/
			start_timer(t);
			int k;
			for (k=0; k<DO; k++) {
		
				int size = i;
				int l;
				int tot;
				for (l = 0; l<size-1; l++) {
					tot += array[l];
				}
				array[size-1] = tot;
			}
			write_record_n(shm_rec,i,stop_timer(t),ARRAY_LEN);
			//printf("%d\n",tot);
			int dt = shmdt(array);
			if (dt < 0) err(dt,"erreur lors de shmdt dans le fils");
			exit(0);
		} else {
			waitpid(pid,NULL,0);
		}
		
		//write_record_n(shm_rec,i,stop_timer(t),ARRAY_LEN);
		int ctl = shmctl(shm_id, IPC_RMID, NULL);
		if (ctl < 0) err(ctl,"erreur lors de shmctl");
		int dt = shmdt(array);
		if (dt < 0) err(dt,"erreur lors de shmdt");
		shmctl(shm_id, IPC_RMID, NULL);
				
	}

	recorder_free(heap_rec);
	recorder_free(shm_rec);
	timer_free(t);

	return EXIT_SUCCESS;
}

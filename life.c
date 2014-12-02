/*****************************************************************************
 * life.c
 * Parallelized and optimized implementation of the game of life resides here
 ****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "life.h"
#include "util.h"

/*****************************************************************************
 * Helper function definitions
 ****************************************************************************/
#define SWAP_BOARDS( b1, b2 )  do { \
  char* temp = b1; \
  b1 = b2; \
  b2 = temp; \
} while(0)

#define BOARD( __board, __i, __j )  (__board[(__i) + LDA*(__j)])

#define NUMBER_OF_THREADS 4

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_barrier_t barrier;
//pthread_barrierattr_t attr;


//static inline void barrier() {
//	static int arrived = 0;
//	pthread_mutex_lock(&lock);
//	arrived++;
//	if (arrived<NUMBER_OF_THREADS) {
//		pthread_cond_wait(&cond, &lock);
//	}
//	else {
//		pthread_cond_broadcast(&cond);
//		arrived=0; /* be prepared for next barrier */
//	}
//	pthread_mutex_unlock(&lock);
//}

void*
parallel_game_of_life (void* arg)
{
    /* HINT: in the parallel decomposition, LDA may not be equal to
       nrows! */
    thread_struct *thread = (thread_struct *)arg;

    char* outboard = thread->outboard;
	char* inboard = thread->inboard;
	const int nrows = thread->nrows;
	const int ncols = thread->ncols;
	const int gens_max = thread->gens_max;

	const int LDA = nrows;

	int from = (thread->thread_num * nrows) / NUMBER_OF_THREADS;
	int to_row = ((thread->thread_num + 1) * nrows) / NUMBER_OF_THREADS;

	int curgen, i, j;

	/* HINT: you'll be parallelizing these loop(s) by doing a
	   geometric decomposition of the output */
	for (curgen = 0; curgen < gens_max; curgen++)
	{
		for (i = from; i < to_row; i++)
		{
			const int inorth = mod (i-1, nrows);
			const int isouth = mod (i+1, nrows);
			for (j = 0; j < ncols; j++)
			{
				const int jwest = mod (j-1, ncols);
				const int jeast = mod (j+1, ncols);
				const char neighbor_count =
					BOARD (inboard, inorth, jwest) +
					BOARD (inboard, inorth, j) +
					BOARD (inboard, inorth, jeast) +
					BOARD (inboard, i, jwest) +
					BOARD (inboard, i, jeast) +
					BOARD (inboard, isouth, jwest) +
					BOARD (inboard, isouth, j) +
					BOARD (inboard, isouth, jeast);

				BOARD(outboard, i, j) = alivep (neighbor_count, BOARD (inboard, i, j));

			}
		}

		pthread_barrier_wait(&barrier);
		SWAP_BOARDS( outboard, inboard );
	}

    /*
     * We return the output board, so that we know which one contains
     * the final result (because we've been swapping boards around).
     * Just be careful when you free() the two boards, so that you don't
     * free the same one twice!!!
     */
    return NULL;
}

/*****************************************************************************
 * Game of life implementation
 ****************************************************************************/
char*
game_of_life (char* outboard, 
	      char* inboard,
	      const int nrows,
	      const int ncols,
	      const int gens_max)
{
	printf("got here\n\n");


	int num_threads = NUMBER_OF_THREADS;
	pthread_barrier_init(&barrier, NULL, num_threads);
	pthread_t tid[num_threads];
	thread_struct threads[num_threads];
	int err;

	int i;

	printf("before the thread for loop\n\n");

	for (i=0; i < num_threads; i++) {
		threads[i].thread_num = i;
		threads[i].outboard = outboard;
		threads[i].inboard = inboard;
		threads[i].nrows = nrows;
		threads[i].ncols = ncols;
		threads[i].gens_max = gens_max;

		err = pthread_create(&(tid[i]), NULL, parallel_game_of_life, (void*)&threads[i]);
		if (err) {
			printf("\ncan't create thread :[%d]\n", err);
			exit(EXIT_FAILURE);
		}
	}
	for (i=0; i < num_threads; i++) {
		pthread_join(tid[i], NULL);
	}

	if (gens_max % 2 == 0) {
		return inboard;
	} else {
		return outboard;
	}
  //return sequential_game_of_life (outboard, inboard, nrows, ncols, gens_max);
}

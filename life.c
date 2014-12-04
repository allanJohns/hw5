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

#define BOARD( __board, __i, __j )  (__board[(__j) + LDA*(__i)])

#define NUMBER_OF_THREADS 4

void*
parallel_game_of_life (void* arg)
{
    /* HINT: in the parallel decomposition, LDA may not be equal to
       nrows! */

	int inorth;
	int isouth;
	int jwest;
	int jeast;

    thread_struct *thread = (thread_struct *)arg;

    char* outboard = thread->outboard;
	char* inboard = thread->inboard;
	const int nrows = thread->nrows;
	const int ncols = thread->ncols;
	const int gens_max = thread->gens_max;
	pthread_barrier_t *bar = thread->bar;

	const int LDA = nrows;

	/**
	 * dividing up the number of rows between the 4 threads
	 */
	int from = (thread->thread_num * nrows) / NUMBER_OF_THREADS;
	int to_row = ((thread->thread_num + 1) * nrows) / NUMBER_OF_THREADS;

	int curgen, i, j;

	/* HINT: you'll be parallelizing these loop(s) by doing a
	   geometric decomposition of the output */
	for (curgen = 0; curgen < gens_max; curgen++)
	{
		for (i = from; i < to_row; i++)
		{
			//Only use mod to calculate inorth and isouth if we're at the boundary
			if (i == 0 || i == nrows - 1) {
				inorth = mod (i-1, nrows);
				isouth = mod (i+1, nrows);
			} else {
				inorth = i-1;
				isouth = i+1;
			}
			for (j = 0; j < ncols; j++)
			{
				//Only use mod to calculate jwest and jeast if we're at the boundary
				if (j == 0 || j == ncols - 1) {
					jwest = mod (j-1, ncols);
					jeast = mod (j+1, ncols);
				} else {
					jwest = j-1;
					jeast = j+1;
				}

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

		pthread_barrier_wait(bar);
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
	pthread_barrier_t barrier;
	pthread_barrier_init(&barrier, NULL, NUMBER_OF_THREADS);
	pthread_t tid[NUMBER_OF_THREADS];
	thread_struct threads[NUMBER_OF_THREADS];
	int err, i;

	for (i=0; i < NUMBER_OF_THREADS; i++) {
		threads[i].thread_num = i;
		threads[i].outboard = outboard;
		threads[i].inboard = inboard;
		threads[i].nrows = nrows;
		threads[i].ncols = ncols;
		threads[i].gens_max = gens_max;
		threads[i].bar = &barrier;

		err = pthread_create(&(tid[i]), NULL, parallel_game_of_life, (void*)&threads[i]);
		if (err) {
			printf("\ncan't create thread :[%d]\n", err);
			exit(EXIT_FAILURE);
		}
	}
	for (i=0; i < NUMBER_OF_THREADS; i++) {
		pthread_join(tid[i], NULL);
	}
	
	return inboard;
}

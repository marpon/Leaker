
/*
 * 
 * to use leaker utilities when degug
 * 
*/
#ifdef USE_LEAKER
# include "leaker.h"
#else
# include <stdlib.h>
#endif


#include<stdio.h>

/* Function with memory leak */
void not_free(void)
{
   int *ptr = (int *) malloc(sizeof(int));
 
   /* Do some work */
	*ptr = 0;
   printf("\tReturn without freeing ptr\n\n");
   return; 
}

/* Function with memory leak */
void two_free(void)
{
   int *ptr = (int *) malloc(sizeof(int));
 
   /* Do some work */
   
   free(ptr);
   printf("\tdouble free \n\n");
   free(ptr);
 
   return;
}


/* Function with memory leak */
void double_alloc(void)
{
   int *ptr = (int *) malloc(sizeof(int));
 
   /* Do some work */
   
   printf("\tmalloc same pointer without freeing it before\n\n");
   ptr = (int *) malloc(sizeof(int) * 10);
   free(ptr);
 
   return; 
}


int main(void)
{
	printf("Create malloc without free\n");
	not_free();
	
	printf("Create double_alloc memory\n");
	double_alloc();
	
	printf("Create double_free memory\n");
	two_free();
	
	return 0;
}

/*
 * Module: signature.c
 * Purpose: The main signature algorithm
 * Routines;
 * pcover signature():
 *	Entry point for the signature cubes algorithm.	
 * pcover generate_primes():
 *	Generates the set of primes corresponding to 
 *	the Essential Signature Cubes
 */

#include <stdio.h>
#include <math.h>
#include <signal.h>
#include "espresso.h"
#include "signature.h"

static long start_time; /* yuk */


pcover
generate_primes(pset_family F, pset_family R)
{
	pcube c,r,lastc,b,lastb;
	pcover BB,PRIMES;
	pcube odd,even,out_part_r;
	register int i;
    	register int w, last;
	register unsigned int x;
	int count;

	out_part_r = new_cube();
	odd = new_cube();
	even = new_cube();

	count = 0;
	PRIMES = new_cover(F->count);
	foreach_set(F,lastc,c){
		BB = new_cover(R->count);
		BB->count = R->count;
		/* BB = get_blocking_matrix(R,c); */
		foreachi_set(R,i,r){
			b = GETSET(BB,i);
			if ((last = cube.inword) != -1) {
				/* Check the partial word of binary variables */
				x = r[last] & c[last];
				x = ~(x | x >> 1) & cube.inmask;
				b[last] = r[last] & (x | x << 1);
				/* Check the full words of binary variables */
				for(w = 1; w < last; w++) {
		    			x = r[w] & c[w];
		    			x = ~(x | x >> 1) & DISJOINT;
					b[w] = r[w] & (x | x << 1);
				}
	    		}
			PUTLOOP(b,LOOP(r));
			INLINEset_and(b,b,cube.binary_mask);
			INLINEset_and(out_part_r,cube.mv_mask,r);
			if(!setp_implies(out_part_r,c)){
				INLINEset_or(b,b,out_part_r);
			}
		}
		BB = unate_compl(BB);
		if(BB != NULL){
			foreach_set(BB,lastb,b){
				set_not(b);
			}
			sf_append(PRIMES,BB);
		}
		count++;
		if(count % 100 == 0){
			PRIMES = sf_contain(PRIMES);
		}
	}
	PRIMES = sf_contain(PRIMES);
	free_cube(out_part_r);
	free_cube(odd);
	free_cube(even);
	return PRIMES;
}

void
cleanup(void)
{
	s_runtime(ptime() - start_time);	
	printf("CPU Limit Exceeded\n");
	exit(1);
}

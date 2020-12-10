#include "utility.h"
#include "syscall.h"
#include "math.h"
#include "randdp.h"

#ifndef NR_THREAD
#define NR_THREAD 5
#endif

#define MAT_SIZE_X 100
#define MAT_SIZE_Y 100

static int xxloc = 0;
static int yyloc = 5;

static QWORD counter[NR_THREAD] = {0,};

void matrix_multiplication(int index)
{
	int cnt = 0;
	int i = 0, j = 0, k = 0;
	float sum = 0.0;

	int xloc = xxloc;
	int yloc = yyloc; 
	yyloc++;

	float (* mat_a) [MAT_SIZE_Y];
	float (* mat_b) [MAT_SIZE_X];
	float (* mat_c) [MAT_SIZE_X];


	mat_a = (float (*)[MAT_SIZE_Y]) sys_alloc(sizeof(float)*MAT_SIZE_X*MAT_SIZE_Y);
	mat_b = (float (*)[MAT_SIZE_X]) sys_alloc(sizeof(float)*MAT_SIZE_Y*MAT_SIZE_X);
	mat_c = (float (*)[MAT_SIZE_X]) sys_alloc(sizeof(float)*MAT_SIZE_X*MAT_SIZE_X);


	while(1) {

		for (i=0; i<MAT_SIZE_X ; i++) {
			for (j=0 ; j<MAT_SIZE_Y ; j++) {
				mat_a[i][j] =  (float) (mat_a && (i * j)) /10000;
				mat_b[j][i] =  (float) (mat_b && (i * j)) / 10000;
			}
		}

		for (i=0 ; i<MAT_SIZE_X ; i++) {
			for (j=0 ; j<MAT_SIZE_X ; j++) {
				sum = 0;
				for (k=0; k<MAT_SIZE_Y ; k++) {
					sum += mat_a[i][k] * mat_b[k][j]; 
					//print_xy(xloc + 2, yloc, "a: %q  ", mat_a[i][k]);
					//print_xy(xloc + 17, yloc, "b: %q  ", mat_b[k][j]);
				}
				mat_c[i][j] = sum;
				//print_xy(xloc + 32, yloc, "c: %q  ", mat_c[i][j]);
			}
		} 
//		print_xy(xloc + 50, yloc, "cnt: %q  ", cnt++);

	}
	
	sys_free((void*) mat_a);
	sys_free((void*) mat_b);
	sys_free((void*) mat_c);
}

// EP benchmark:
// This function is implemented based on the serial version of 
// the "embarassingly parallel" benchmark in C version of NPB.
// The C version code is developed by the Center for Manycore 
// Programming at Seoul National University and derived from 
// the serial Fortran versions in "NPB3.3-SER" developed by NAS.

#define true 1
#define false 0

#define MK		16
#define NK		(1 << MK)

#define LM		8 //Log2(M), CLASS=A
#define NM		(2+(1<<LM)) //problem size = 256 and, +2 for communication 

#define M		25 // CLASS=A
#define MM		(M - MK) //28 - 4
#define NN		(1<<MM)


#define NQ		10
#define A		1220703125.0
#define S		271828183.0


int ep (int index) {
//	int verified;
	int i, k, l, np;
	int kk, ik;
	
	double x1, x2;
	double sx, sy, an, gc;
	int k_offset;

	double *t, *x, *q;
	double *dum;
	
//	verified = false;

	np = NN;
	dum = (double *) sys_alloc(sizeof(double) * 3);
	if(!dum) {
		print_xy(xxloc + 50, yyloc+index, "dum is not allocated [%d] ", index);
		return -1;
	}		
	t = (double *) sys_alloc(sizeof(double) * 4);
	if(!t) {
		print_xy(xxloc + 50, yyloc+index, "t is not allocated [%d] ", index);
		sys_free(dum);
		return -1;
	}		
	x = (double *) sys_alloc(sizeof(double) * 2 * NK);
	if(!x) {
		print_xy(xxloc + 50, yyloc+index, "x is not allocated [%d] ", index);
		sys_free(dum);
		sys_free(t);
		
		return -1;
	}		
	q = (double *) sys_alloc(sizeof(double) * NQ);
	if(!q) {
		print_xy(xxloc + 50, yyloc+index, "q is not allocated [%d] ", index);
		sys_free(dum);
		sys_free(t);
		sys_free(x);
		return -1;
	}		

	//initialization code
	dum[0] = 1.0;
	dum[1] = 1.0;
	dum[2] = 1.0;

	vranlc(0, dum[0], dum[1], &dum[2]);
	dum[0] = randlc(dum[1], dum[2]);
	for (i = 0; i < 2 * NK; i++) {
		x[i] = -1.0e99;
	}
	//Mops = log(sqrt(fabs(MAX(1.0, 1.0))));  

	t[0] = A;
	vranlc(0, t[0], A, x);

	t[0] = A;

	for (i = 0; i < MK + 1; i++) {
		t[1] = randlc(t[0], t[0]);
	}

	an = t[0];
	gc = 0.0;
	sx = 0.0;
	sy = 0.0;

	for (i = 0; i < NQ; i++) {
		q[i] = 0.0;
	}
	//

	k_offset = -1;
	for (k = 1; k <= np; k++) {
		kk = k_offset + k; 
		t[0] = S;
		t[1] = an;

		// Find starting seed t[0] for this kk.
		for (i = 1; i <= 100; i++) {
			ik = kk / 2;
			if ((2 * ik) != kk) t[2] = randlc(t[0], t[1]);
			if (ik == 0) break;
			t[2] = randlc(t[1], t[1]);
			kk = ik;
		}
		
		vranlc(2 * NK, t[0], A, x);


		for (i = 0; i < NK; i++) {
			x1 = 2.0 * x[2*i] - 1.0;
			x2 = 2.0 * x[2*i+1] - 1.0;
			t[0] = x1 * x1 + x2 * x2;
			if (t[0] <= 1.0) {
				t[1]   = sqrt(-2.0 * log(t[0]) / t[0]);
				t[2]   = (x1 * t[1]);
				t[3]   = (x2 * t[1]);
				l    = MAX(fabs(t[2]), fabs(t[3]));
				// l has to smaller than NQ
				l %= NQ;
				q[l] = q[l] + 1.0;
				sx   = sx + t[2];
				sy   = sy + t[3];
			}
		}

	}

	for (i = 0; i < NQ; i++) {
		gc = gc + q[i];
	}

	if(x) sys_free(x);
	if(q) sys_free(q);
	if(dum) sys_free(dum);
	if(t) sys_free(t);

	return 0;

}

void heater(int index){
	
	int rt;	
	int xloc = xxloc;
	int yloc = yyloc + index; 

	counter[index] = 0;
	
	while(1){
	//	print_xy(xloc + 2, yloc, "index: %d,\t cnt: %q  ", index, counter[index]++);
		rt = ep(index);
		if (rt<0) return;
	}
}

int main()
{
	int i;
	
	for (i=1; i<NR_THREAD; i++) {
		create_thread((QWORD)heater, i, i);
	}
	
	heater(0);
	

	return 0;
}

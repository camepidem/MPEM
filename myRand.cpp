//myRand.cpp

#include "stdafx.h"
#include "myRand.h"
#include "mt19937ar.h"

/*
	generate a random number between 0 and 1
*/
double	dUniformRandom() {
	return genrand_real2();
}

int nUniformRandom(int min, int max) {
	return int(min + dUniformRandom()*(max-min+1));
}

/*
	generate a normal variate with given mean and standard deviation
*/
double dNormalRandom(double m, double s) {
	/* normal random variate generator*/
	/* mean m, standard deviation s */
	double x1, x2, w, y1;
	static double y2;
	static int use_last = 0;

	if (use_last) {
		/* use value from previous call */
		y1 = y2;
		use_last = 0;
	} else {
		do {
			x1 = 2.0 * dUniformRandom() - 1.0;
			x2 = 2.0 * dUniformRandom() - 1.0;
			w = x1 * x1 + x2 * x2;
		} while ( w >= 1.0 );

		w = sqrt( (-2.0 * log( w ) ) / w );
		y1 = x1 * w;
		y2 = x2 * w;
		use_last = 1;
	}
	return( m + y1 * s );
}

/*
	seed RNG with given seed or with system clock if ulnSeed is zero
*/
unsigned long vSeedRNG(unsigned long ulnSeed,int RunNo)
{
	if(ulnSeed == 0) {
		/* seed with system clock */
		ulnSeed = (unsigned long) time(NULL);
		ulnSeed += RunNo*71;
	}
	init_genrand(ulnSeed);
	return ulnSeed;
}

/*
	generate a random long
*/
unsigned long ulnRandom()
{
	return genrand_int32();
}

/*
	produce a poisson variate of given mean
  	uses normal approximation for large lambda
*/
int nPoissonVariate(double dLambda)
{
	double 		dLambdaExp;
	int	   	nRet;
	double 		dT;

	if(dLambda < _POISSON_APPROX_THRESH) {
		dLambdaExp = exp(-dLambda);
		nRet = -1;
		dT=1.0;
		do {
			nRet++;
			dT *= dUniformRandom();
		} while (dT > dLambdaExp);
	} else {
		nRet = -1;

		while(nRet < 0)	/* make sure always have a +ve value */
		{
			nRet = (int)(0.5 + dNormalRandom(dLambda, sqrt(dLambda)));
		}
	}
	return nRet;
}

/*
	produce a exponential variate with given rate parameter
*/
double dExponentialVariate(double dParam)
{
	double dRand;

	do {
		dRand = dUniformRandom();
	} while(dRand == 0.0); /* make sure don't have to divide by zero */
	return -log(dRand)/dParam;
}


/*
	simple minded way of doing this is to just do n bernoulli trials
*/
int nSlowBinomial(int n, double p)
{
	int i,x;

	x=0;
	for(i=0;i<n;i++) {
		if(dUniformRandom()<=p) {
			/* less than or equals to handle equality */
			x++;
		}
	}
	return x;
}

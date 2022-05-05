#ifndef _MYRAND_H_
#define _MYRAND_H_

#define	_POISSON_APPROX_THRESH	10.0

/*
	seed RNG with given seed or with system clock if ulnSeed is zero
*/
unsigned long vSeedRNG(unsigned long ulnSeed, int RunNo);

/*
	generate a random number between 0 and 1
*/
double	dUniformRandom(void);

/* 
	generate a random integer in [min, max]
*/
int nUniformRandom(int min, int max);

/*
	produce a poisson variate of given mean
	this is a pretty slow implementation of this - should really speed up for
large dLambda
*/
int nPoissonVariate(double dLambda);

/*
	produce a exponential variate with given rate parameter
*/
double dExponentialVariate(double dParam);

/*
	generate a random long
*/
unsigned long ulnRandom(void);

/*
	slow binomial variate
*/
int nSlowBinomial(int n, double p);

/*
	generate a normal variate with given mean and standard deviation
*/
double dNormalRandom(double m, double s);


#endif /* _MYRAND_H_ */

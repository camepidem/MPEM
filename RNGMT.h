// RNGMT.h
//
// C++ Class wrappering the MT RNG

#pragma once

class RNGMT {

public:

	RNGMT();
	~RNGMT();

	/*
	seed RNG with given seed or with system clock if ulnSeed is zero
	*/
	unsigned long vSeedRNG(unsigned long ulnSeed, int RunNo);

	/*
	generate a random number between 0 and 1
	*/
	double	dUniformRandom(void);

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

protected:

	/* initializes mt[N] with a seed */
	void init_genrand(unsigned long s);
	/* initialize by an array with array-length */
	/* init_key is the array for initializing keys */
	/* key_length is its length */
	/* slight change for C++, 2004/2/26 */
	void init_by_array(unsigned long init_key[], int key_length);
	/* generates a random number on [0,0xffffffff]-interval */
	unsigned long genrand_int32(void);
	/* generates a random number on [0,0x7fffffff]-interval */
	long genrand_int31(void);
	/* generates a random number on [0,1]-real-interval */
	double genrand_real1(void);
	/* generates a random number on [0,1)-real-interval */
	double genrand_real2(void);
	/* generates a random number on (0,1)-real-interval */
	double genrand_real3(void);
	/* generates a random number on [0,1) with 53-bit resolution*/
	double genrand_res53(void);

	/* Period parameters */
	static const int N = 624;
	static const int M = 397;
	static const unsigned long MATRIX_A = 0x9908b0dfUL;   /* constant vector a */
	static const unsigned long UPPER_MASK = 0x80000000UL; /* most significant w-r bits */
	static const unsigned long LOWER_MASK = 0x7fffffffUL; /* least significant r bits */

	unsigned long mt[N]; /* the array for the state vector  */
	int mti; /* mti==N+1 means mt[N] is not initialized */


	static double POISSON_APPROX_THRESH;

};

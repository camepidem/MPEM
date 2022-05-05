// RNGMT.cpp
//
// 

#include "stdafx.h"
#include "RNGMT.h"

double RNGMT::POISSON_APPROX_THRESH = 10.0;

RNGMT::RNGMT() {
	mti = N + 1;
}

RNGMT::~RNGMT() {
	//Nothing to do...
}

//Exposed functions:

/*
	generate a random number between 0 and 1
*/
double RNGMT::dUniformRandom() {
	return genrand_real2();
}

/*
	generate a normal variate with given mean and standard deviation
*/
double RNGMT::dNormalRandom(double m, double s) {
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
unsigned long RNGMT::vSeedRNG(unsigned long ulnSeed,int RunNo)
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
unsigned long RNGMT::ulnRandom()
{
	return genrand_int32();
}

/*
	produce a poisson variate of given mean
  	uses normal approximation for large lambda
*/
int RNGMT::nPoissonVariate(double dLambda)
{
	double 		dLambdaExp;
	int	   	nRet;
	double 		dT;

	if(dLambda < POISSON_APPROX_THRESH) {
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
double RNGMT::dExponentialVariate(double dParam)
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
int RNGMT::nSlowBinomial(int n, double p)
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


//Internal workings of mt19937ar:

void RNGMT::init_genrand(unsigned long s)
{
    mt[0]= s & 0xffffffffUL;
    for (mti=1; mti<N; mti++) {
        mt[mti] =
	    (1812433253UL * (mt[mti-1] ^ (mt[mti-1] >> 30)) + mti);
        /* See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier. */
        /* In the previous versions, MSBs of the seed affect   */
        /* only MSBs of the array mt[].                        */
        /* 2002/01/09 modified by Makoto Matsumoto             */
        mt[mti] &= 0xffffffffUL;
        /* for >32 bit machines */
    }
}

/* initialize by an array with array-length */
/* init_key is the array for initializing keys */
/* key_length is its length */
/* slight change for C++, 2004/2/26 */
void RNGMT::init_by_array(unsigned long init_key[], int key_length)
{
    int i, j, k;
    init_genrand(19650218UL);
    i=1; j=0;
    k = (N>key_length ? N : key_length);
    for (; k; k--) {
        mt[i] = (mt[i] ^ ((mt[i-1] ^ (mt[i-1] >> 30)) * 1664525UL))
          + init_key[j] + j; /* non linear */
        mt[i] &= 0xffffffffUL; /* for WORDSIZE > 32 machines */
        i++; j++;
        if (i>=N) { mt[0] = mt[N-1]; i=1; }
        if (j>=key_length) j=0;
    }
    for (k=N-1; k; k--) {
        mt[i] = (mt[i] ^ ((mt[i-1] ^ (mt[i-1] >> 30)) * 1566083941UL))
          - i; /* non linear */
        mt[i] &= 0xffffffffUL; /* for WORDSIZE > 32 machines */
        i++;
        if (i>=N) { mt[0] = mt[N-1]; i=1; }
    }

    mt[0] = 0x80000000UL; /* MSB is 1; assuring non-zero initial array */
}

/* generates a random number on [0,0xffffffff]-interval */
unsigned long RNGMT::genrand_int32(void)
{
    unsigned long y;
    static unsigned long mag01[2]={0x0UL, MATRIX_A};
    /* mag01[x] = x * MATRIX_A  for x=0,1 */

    if (mti >= N) { /* generate N words at one time */
        int kk;

        if (mti == N+1)   /* if init_genrand() has not been called, */
            init_genrand(5489UL); /* a default initial seed is used */

        for (kk=0;kk<N-M;kk++) {
            y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
            mt[kk] = mt[kk+M] ^ (y >> 1) ^ mag01[y & 0x1UL];
        }
        for (;kk<N-1;kk++) {
            y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
            mt[kk] = mt[kk+(M-N)] ^ (y >> 1) ^ mag01[y & 0x1UL];
        }
        y = (mt[N-1]&UPPER_MASK)|(mt[0]&LOWER_MASK);
        mt[N-1] = mt[M-1] ^ (y >> 1) ^ mag01[y & 0x1UL];

        mti = 0;
    }

    y = mt[mti++];

    /* Tempering */
    y ^= (y >> 11);
    y ^= (y << 7) & 0x9d2c5680UL;
    y ^= (y << 15) & 0xefc60000UL;
    y ^= (y >> 18);

	return y;
}

/* generates a random number on [0,0x7fffffff]-interval */
long RNGMT::genrand_int31(void)
{
    return (long)(genrand_int32()>>1);
}

/* generates a random number on [0,1]-real-interval */
double RNGMT::genrand_real1(void)
{
    return genrand_int32()*(1.0/4294967295.0);
    /* divided by 2^32-1 */
}

/* generates a random number on [0,1)-real-interval */
double RNGMT::genrand_real2(void)
{
    return genrand_int32()*(1.0/4294967296.0);
    /* divided by 2^32 */
}

/* generates a random number on (0,1)-real-interval */
double RNGMT::genrand_real3(void)
{
    return (((double)genrand_int32()) + 0.5)*(1.0/4294967296.0);
    /* divided by 2^32 */
}

/* generates a random number on [0,1) with 53-bit resolution*/
double RNGMT::genrand_res53(void)
{
    unsigned long a=genrand_int32()>>5, b=genrand_int32()>>6;
    return(a*67108864.0+b)*(1.0/9007199254740992.0);
}
#ifndef lint
static char Rcs_Id[] =
    "$Id: randistrs.c,v 1.8 2008/07/25 23:34:01 geoff Exp $";
#endif

/*
 * C library functions for generating various random distributions
 * using the Mersenne Twist PRNG.  See the header file for full
 * documentation.
 *
 * These functions were written by Geoffrey H. Kuenning, Claremont, CA.
 *
 * Unless otherwise specified, these algorithms are taken from Averill
 * M. Law and W. David Kelton, "Simulation Modeling and Analysis",
 * McGraw-Hill, 1991.
 *
 * Copyright 2001, 2002, Geoffrey H. Kuenning, Claremont, CA.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All modifications to the source code must be clearly marked as
 *    such.  Binary redistributions based on modified source code
 *    must be clearly marked as modified versions in the documentation
 *    and/or other materials provided with the distribution.
 * 4. The name of Geoff Kuenning may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GEOFF KUENNING AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL GEOFF KUENNING OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Log: randistrs.c,v $
 * Revision 1.8  2008/07/25 23:34:01  geoff
 * Fix notation for intervals in commentary.
 *
 * Revision 1.7  2005/05/17 21:40:10  geoff
 * Fix a bug that caused rds_iuniform to generate off-by-one values if the
 * lower bound was negative.
 *
 * Revision 1.6  2002/10/30 00:50:44  geoff
 * Add a (BSD-style) license.  Fix all places where logs are taken so
 * that there is no risk of unintentionally taking the log of zero.  This
 * is a very low-probability occurrence, but it's better to have robust
 * code.
 *
 * Revision 1.5  2001/06/20 09:07:57  geoff
 * Fix a place where long long wasn't conditionalized.
 *
 * Revision 1.4  2001/06/19 00:41:17  geoff
 * Add the "l" versions of all functions.  Add the MT_NO_CACHING option.
 *
 * Revision 1.3  2001/06/18 10:09:24  geoff
 * Add the iuniform functions to generate unbiased uniformly distributed
 * integers.
 *
 * Revision 1.2  2001/04/10 09:11:38  geoff
 * Make sure the Erlang distribution has a p of 1 or more.  Fix a serious
 * bug in the Erlang calculation (the value returned was completely
 * wrong).
 *
 * Revision 1.1  2001/04/09 08:39:54  geoff
 * Initial revision
 *
 */

#include "mtwist.h"
#include "randistrs.h"
#include <math.h>

/*
 * Table of contents:
 */
long			rds_iuniform(mt_state * state, long lower, long upper);
					/* (Integer) uniform distribution */
#ifndef MT_NO_LONGLONG
long long		rds_liuniform(mt_state * state, long long lower,
			  long long upper);
					/* (Integer) uniform distribution */
#endif /* MT_NO_LONGLONG */
double			rds_uniform(mt_state * state,
			  double lower, double upper);
					/* (Floating) uniform distribution */
double			rds_luniform(mt_state * state,
			  double lower, double upper);
					/* (Floating) uniform distribution */
double			rds_exponential(mt_state * state, double mean);
					/* Exponential distribution */
double			rds_lexponential(mt_state * state, double mean);
					/* Exponential distribution */
double			rds_erlang(mt_state * state, int p, double mean);
					/* p-Erlang distribution */
double			rds_lerlang(mt_state * state, int p, double mean);
					/* p-Erlang distribution */
double			rds_weibull(mt_state * state,
			  double shape, double scale);
					/* Weibull distribution */
double			rds_lweibull(mt_state * state,
			  double shape, double scale);
					/* Weibull distribution */
double			rds_normal(mt_state * state,
			  double mean, double sigma);
					/* Normal distribution */
double			rds_lnormal(mt_state * state,
			  double mean, double sigma);
					/* Normal distribution */
double			rds_lognormal(mt_state * state,
			  double shape, double scale);
					/* Lognormal distribution */
double			rds_llognormal(mt_state * state,
			  double shape, double scale);
					/* Lognormal distribution */
double			rds_triangular(mt_state * state,
			  double lower, double upper, double mode);
					/* Triangular distribution */
double			rds_ltriangular(mt_state * state,
			  double lower, double upper, double mode);
					/* Triangular distribution */
double			rds_empirical(mt_state * state,
			  int n_probs, double * values, double * probs);
					/* Empirical distribution */
double			rds_lempirical(mt_state * state,
			  int n_probs, double * values, double * probs);
					/* Empirical distribution */
long			rd_iuniform(long lower, long upper);
					/* (Integer) uniform distribution */
#ifndef MT_NO_LONGLONG
long long		rd_liuniform(long long lower, long long upper);
					/* (Integer) uniform distribution */
#endif /* MT_NO_LONGLONG */
double			rd_uniform(double lower, double upper);
					/* (Floating) uniform distribution */
double			rd_luniform(double lower, double upper);
					/* (Floating) uniform distribution */
double			rd_exponential(double mean);
					/* Exponential distribution */
double			rd_lexponential(double mean);
					/* Exponential distribution */
double			rd_erlang(int p, double mean);
					/* p-Erlang distribution */
double			rd_lerlang(int p, double mean);
					/* p-Erlang distribution */
double			rd_weibull(double shape, double scale);
					/* Weibull distribution */
double			rd_lweibull(double shape, double scale);
					/* Weibull distribution */
double			rd_normal(double mean, double sigma);
					/* Normal distribution */
double			rd_lnormal(double mean, double sigma);
					/* Normal distribution */
double			rd_lognormal(double shape, double scale);
					/* Lognormal distribution */
double			rd_llognormal(double shape, double scale);
					/* Lognormal distribution */
double			rd_triangular(double lower, double upper, double mode);
					/* Triangular distribution */
double			rd_ltriangular(double lower, double upper, double mode);
					/* Triangular distribution */
double			rd_empirical(int n_probs,
			  double * values, double * probs);
					/* Empirical distribution */
double			rd_lempirical(int n_probs,
			  double * values, double * probs);
					/* Empirical distribution */

/*
 * The Mersenne Twist PRNG makes it default state available as an
 * external variable.  This feature is undocumented, but is useful to
 * use because it allows us to avoid implementing every function
 * twice.  (In fact, the feature was added to enable this file to be
 * written.  It would be better to write in C++, where I could control
 * the access to the state.)
 */
extern mt_state		mt_default_state;

/*
 * Threshold below which it is OK for uniform integer distributions to make
 * use of the double-precision code as a crutch.  For ranges below
 * this value, a double-precision random value is generated and then
 * mapped to the given range.  For a lower bound of zero, this is
 * equivalent to mapping a 32-bit integer into the range by using the
 * following formula:
 *
 *	final = upper * mt_lrand() / (1 << 32);
 *
 * That formula can't be computed using integer arithmetic, since the
 * multiplication must precede the division and would cause overflow.
 * Double-precision calculations solve that problem.  However the
 * formula will also produce biased results unless the range ("upper")
 * is exactly a power of 2.  To see this, suppose mt_lrand produced
 * values from 0 to 7 (i.e., 8 values), and we asked for numbers in
 * the range [0, 7).  The 8 values uniformly generated by mt_lrand
 * would be mapped into the 7 output values.  Clearly, one output
 * value (in this case, 4) would occur twice as often as the others
 *
 * The amount of bias introduced by this approximation depends on the
 * relative sizes of the requested range and the range of values
 * produced by mt_lrand.  If the ranges are almost equal, some values
 * will occur almost twice as often as they should.  At the other
 * extreme, consider a requested range of 3 values (0 to 2,
 * inclusive).  If the PRNG cycles through all 2^32 possible values,
 * two of the output values will be generated 1431655765 times and the
 * third will appear 1431655766 times.  Clearly, the bias here is
 * within the expected limits of randomness.
 *
 * The exact amount of bias depends on the relative size of the range
 * compared to the width of the PRNG output.  In general, for an
 * output range of r, no value will appear more than r/(2^32) extra
 * times using the simple integer algorithm.
 *
 * The threshold given below will produce a bias of under 0.01%.  For
 * values above this threshold, a slower but 100% accurate algorithm
 * will be used.
 */
#ifndef RD_MAX_BIAS
#define RD_MAX_BIAS		0.0001
#endif /* RD_MAX_BIAS */
#ifndef RD_UNIFORM_THRESHOLD
#define RD_UNIFORM_THRESHOLD	((int)((double)(1u << 31) * 2.0 * RD_MAX_BIAS))
#endif /* RD_UNIFORM_THRESHOLD */

/*
 * Generate a uniform integer distribution on the open interval
 * [lower, upper).  See comments above about RD_UNIFORM_THRESHOLD.  If
 * we are above the threshold, this function is relatively expensive
 * because we may have to repeatedly draw random numbers to get a
 * one that works.
 */
long rds_iuniform(
    mt_state *		state,		/* State of the MT PRNG to use */
    long		lower,		/* Lower limit of distribution */
    long		upper)		/* Upper limit of distribution */
    {
    unsigned long	range = upper - lower;
					/* Range of requested distribution */

    if (range <= RD_UNIFORM_THRESHOLD)
	return lower + (long)(mts_ldrand(state) * range);
    else
	{
	/*
	 * Using the simple formula would produce too much bias.
	 * Instead, draw numbers until we get one within the range.
	 * To save time, we first calculate a mask so that we only
	 * look at the number of bits we actually need.  Since finding
	 * the mask is expensive, we do a bit of caching here (note
	 * that the caching makes the code non-reentrant; set
	 * MT_NO_CACHING to achieve reentrancy).
	 *
	 * Incidentally, the astute reader will note that we use the
	 * low-order bits of the PRNG output.  If the PRNG were linear
	 * congruential, using the low-order bits wouuld be a major
	 * no-no.  However, the Mersenne Twist PRNG doesn't have that
	 * drawback.
	 */
#ifdef MT_NO_CACHING
	unsigned long	rangemask = 0;	/* Mask for range */
#else /* MT_NO_CACHING */
	static unsigned long
			lastrange = 0;	/* Range used last time */
	static unsigned long
			rangemask = 0;	/* Mask for range */
#endif /* MT_NO_CACHING */
	register unsigned long
			ranval;		/* Random value from mts_lrand */

#ifndef MT_NO_CACHING
	if (range != lastrange)
#endif /* MT_NO_CACHING */
	    {
	    /*
	     * Range is different from last time, recalculate mask.
	     *
	     * A few iterations could be trimmed off of the loop if we
	     * started rangemask at the next power of 2 above
	     * RD_UNIFORM_THRESHOLD.  However, I don't currently know
	     * a formula for generating that value (though there is
	     * probably one in HAKMEM).
	     */
#ifndef MT_NO_CACHING
	    lastrange = range;
#endif /* MT_NO_CACHING */
	    for (rangemask = 1;
	      rangemask < range  &&  rangemask != 0;
	      rangemask <<= 1)
		;

	    /*
	     * If rangemask became zero, the range is over 2^31.  In
	     * that case, subtracting 1 from rangemask will produce a
	     * full-word mask, which is what we need.
	     */
	    rangemask -= 1;
	    }

	/*
	 * Draw random numbers until we get one in the requested range.
	 */
	do
	    {
	    ranval = mts_lrand(state) & rangemask;
	    }
	    while (ranval >= range);
	return lower + ranval;
	}
    }

#ifndef MT_NO_LONGLONG
/*
 * Generate a uniform integer distribution on the half-open interval
 * [lower, upper).
 */
long long rds_liuniform(
    mt_state *		state,		/* State of the MT PRNG to use */
    long long		lower,		/* Lower limit of distribution */
    long long		upper)		/* Upper limit of distribution */
    {
    unsigned long long	range = upper - lower;
					/* Range of requested distribution */

    /*
     * Draw numbers until we get one within the range.  To save time,
     * we first calculate a mask so that we only look at the number of
     * bits we actually need.  Since finding the mask is expensive, we
     * do a bit of caching here.  See rds_iuniform for more information.
     */
#ifdef MT_NO_CACHING
    unsigned long	rangemask = 0;	/* Mask for range */
#else /* MT_NO_CACHING */
    static unsigned long
		    lastrange = 0;	/* Range used last time */
    static unsigned long
		    rangemask = 0;	/* Mask for range */
#endif /* MT_NO_CACHING */
    register unsigned long
		    ranval;		/* Random value from mts_lrand */

#ifndef MT_NO_CACHING
    if (range != lastrange)
#endif /* MT_NO_CACHING */
	{
	/*
	 * Range is different from last time, recalculate mask.
	 */
#ifndef MT_NO_CACHING
	lastrange = range;
#endif /* MT_NO_CACHING */
	for (rangemask = 1;
	  rangemask < range  &&  rangemask != 0;
	  rangemask <<= 1)
	    ;

	/*
	 * If rangemask became zero, the range is over 2^31.  In
	 * that case, subtracting 1 from rangemask will produce a
	 * full-word mask, which is what we need.
	 */
	rangemask -= 1;
	}

    /*
     * Draw random numbers until we get one in the requested range.
     */
    do
	{
	ranval = mts_llrand(state) & rangemask;
	}
	while (ranval >= range);
    return lower + ranval;
    }
#endif /* MT_NO_LONGLONG */

/*
 * Generate a uniform distribution on the half-open interval [lower, upper).
 */
double rds_uniform(
    mt_state *		state,		/* State of the MT PRNG to use */
    double		lower,		/* Lower limit of distribution */
    double		upper)		/* Upper limit of distribution */
    {
    return lower + mts_drand(state) * (upper - lower);
    }

/*
 * Generate a uniform distribution on the half-open interval [lower, upper).
 */
double rds_luniform(
    mt_state *		state,		/* State of the MT PRNG to use */
    double		lower,		/* Lower limit of distribution */
    double		upper)		/* Upper limit of distribution */
    {
    return lower + mts_ldrand(state) * (upper - lower);
    }

/*
 * Generate an exponential distribution with the given mean.
 */
double rds_exponential(
    mt_state *		state,		/* State of the MT PRNG to use */
    double		mean)		/* Mean of generated distribution */
    {
    double		random_value;	/* Random sample on [0,1) */

    do
	random_value = mts_drand(state);
    while (random_value == 0.0);
    return -mean * log(random_value);
    }

/*
 * Generate an exponential distribution with the given mean.
 */
double rds_lexponential(
    mt_state *		state,		/* State of the MT PRNG to use */
    double		mean)		/* Mean of generated distribution */
    {
    double		random_value;	/* Random sample on [0,1) */

    do
	random_value = mts_ldrand(state);
    while (random_value == 0.0);
    return -mean * log(random_value);
    }

/*
 * Generate a p-Erlang distribution with the given mean.
 */
double rds_erlang(
    mt_state *		state,		/* State of the MT PRNG to use */
    int			p,		/* Order of distribution to generate */
    double		mean)		/* Mean of generated distribution */
    {
    int			order;		/* Order generated so far */
    double		random_value;	/* Value generated so far */

    do
	{
	if (p <= 1)
	    p = 1;
	random_value = mts_drand(state);
	for (order = 1;  order < p;  order++)
	    random_value *= mts_drand(state);
	}
    while (random_value == 0.0);
    return -mean * log(random_value) / p;
    }

/*
 * Generate a p-Erlang distribution with the given mean.
 */
double rds_lerlang(
    mt_state *		state,		/* State of the MT PRNG to use */
    int			p,		/* Order of distribution to generate */
    double		mean)		/* Mean of generated distribution */
    {
    int			order;		/* Order generated so far */
    double		random_value;	/* Value generated so far */

    do
	{
	if (p <= 1)
	    p = 1;
	random_value = mts_ldrand(state);
	for (order = 1;  order < p;  order++)
	    random_value *= mts_ldrand(state);
	}
    while (random_value == 0.0);
    return -mean * log(random_value) / p;
    }

/*
 * Generate a Weibull distribution with the given shape and scale parameters.
 */
double rds_weibull(
    mt_state *		state,		/* State of the MT PRNG to use */
    double		shape,		/* Shape of the distribution */
    double		scale)		/* Scale of the distribution */
    {
    double		random_value;	/* Random sample on [0,1) */

    do
	random_value = mts_drand(state);
    while (random_value == 0.0);
    return scale * exp(log(-log(random_value)) / shape);
    }
					/* Weibull distribution */
/*
 * Generate a Weibull distribution with the given shape and scale parameters.
 */
double rds_lweibull(
    mt_state *		state,		/* State of the MT PRNG to use */
    double		shape,		/* Shape of the distribution */
    double		scale)		/* Scale of the distribution */
    {
    double		random_value;	/* Random sample on [0,1) */

    do
	random_value = mts_ldrand(state);
    while (random_value == 0.0);
    return scale * exp(log(-log(random_value)) / shape);
    }
					/* Weibull distribution */
/*
 * Generate a normal distribution with the given mean and standard
 * deviation.  See Law and Kelton, p. 491.
 */
double rds_normal(
    mt_state *		state,		/* State of the MT PRNG to use */
    double		mean,		/* Mean of generated distribution */
    double		sigma)		/* Standard deviation to generate */
    {
    double		mag;		/* Magnitude of (x,y) point */
    double		offset;		/* Unscaled offset from mean */
    double		xranval;	/* First random value on [-1,1) */
    double		yranval;	/* Second random value on [-1,1) */

    /*
     * Generating a normal distribution is a bit tricky.  We may need
     * to make several attempts before we get a valid result.  When we
     * are done, we will have two normally distributed values, one of
     * which we discard.
     */
    do
	{
	xranval = 2.0 * mts_drand(state) - 1.0;
	yranval = 2.0 * mts_drand(state) - 1.0;
	mag = xranval * xranval + yranval * yranval;
	}
    while (mag > 1.0  ||  mag == 0.0);

    offset = sqrt((-2.0 * log(mag)) / mag);
    return mean + sigma * xranval * offset;

    /*
     * The second random variate is given by:
     *
     *     mean + sigma * yranval * offset;
     *
     * If this were a C++ function, it could probably save that value
     * somewhere and return it in the next subsequent call.  But
     * that's too hard to make bulletproof (and reentrant) in C.
     */
    }

/*
 * Generate a normal distribution with the given mean and standard
 * deviation.  See Law and Kelton, p. 491.
 */
double rds_lnormal(
    mt_state *		state,		/* State of the MT PRNG to use */
    double		mean,		/* Mean of generated distribution */
    double		sigma)		/* Standard deviation to generate */
    {
    double		mag;		/* Magnitude of (x,y) point */
    double		offset;		/* Unscaled offset from mean */
    double		xranval;	/* First random value on [-1,1) */
    double		yranval;	/* Second random value on [-1,1) */

    /*
     * Generating a normal distribution is a bit tricky.  We may need
     * to make several attempts before we get a valid result.  When we
     * are done, we will have two normally distributed values, one of
     * which we discard.
     */
    do
	{
	xranval = 2.0 * mts_ldrand(state) - 1.0;
	yranval = 2.0 * mts_ldrand(state) - 1.0;
	mag = xranval * xranval + yranval * yranval;
	}
    while (mag > 1.0  ||  mag == 0.0);

    offset = sqrt((-2.0 * log(mag)) / mag);
    return mean + sigma * xranval * offset;

    /*
     * The second random variate is given by:
     *
     *     mean + sigma * yranval * offset;
     *
     * If this were a C++ function, it could probably save that value
     * somewhere and return it in the next subsequent call.  But
     * that's too hard to make bulletproof (and reentrant) in C.
     */
    }

/*
 * Generate a lognormal distribution with the given shape and scale
 * parameters.
 */
double rds_lognormal(
    mt_state *		state,		/* State of the MT PRNG to use */
    double		shape,		/* Shape of the distribution */
    double		scale)		/* Scale of the distribution */
    {
    return exp(rds_normal(state, scale, shape));
    }

/*
 * Generate a lognormal distribution with the given shape and scale
 * parameters.
 */
double rds_llognormal(
    mt_state *		state,		/* State of the MT PRNG to use */
    double		shape,		/* Shape of the distribution */
    double		scale)		/* Scale of the distribution */
    {
    return exp(rds_lnormal(state, scale, shape));
    }

/*
 * Generate a triangular distibution between given limits, with a
 * given mode.
 */
double rds_triangular(
    mt_state *		state,		/* State of the MT PRNG to use */
    double		lower,		/* Lower limit of distribution */
    double		upper,		/* Upper limit of distribution */
    double		mode)		/* Highest point of distribution */
    {
    double		ran_value;	/* Value generated by PRNG */
    double		scaled_mode;	/* Scaled version of mode */

    scaled_mode = (mode - lower) / (upper - lower);
    ran_value = mts_drand(state);
    if (ran_value <= scaled_mode)
	ran_value = sqrt(scaled_mode * ran_value);
    else
	ran_value = 1.0 - sqrt((1.0 - scaled_mode) * (1.0 - ran_value));
    return lower + (upper - lower) * ran_value;
    }

/*
 * Generate a triangular distibution between given limits, with a
 * given mode.
 */
double rds_ltriangular(
    mt_state *		state,		/* State of the MT PRNG to use */
    double		lower,		/* Lower limit of distribution */
    double		upper,		/* Upper limit of distribution */
    double		mode)		/* Highest point of distribution */
    {
    double		ran_value;	/* Value generated by PRNG */
    double		scaled_mode;	/* Scaled version of mode */

    scaled_mode = (mode - lower) / (upper - lower);
    ran_value = mts_ldrand(state);
    if (ran_value <= scaled_mode)
	ran_value = sqrt(scaled_mode * ran_value);
    else
	ran_value = 1.0 - sqrt((1.0 - scaled_mode) * (1.0 - ran_value));
    return lower + (upper - lower) * ran_value;
    }

/*
 * Generate an empirical distribution given a set of values and their
 * probabilities.
 */
double rds_empirical(
    mt_state *		state,		/* State of the MT PRNG to use */
    int			n_probs,	/* Number of probabilities given */
    double *		values,		/* Vals returned with various probs */
    double *		probs)		/* Probs of various values */
    {
    int			i;		/* Index into both arrays */
    double		ran_value;	/* Value generated by PRNG */

    ran_value = mts_drand(state);
    /*
     * NEEDSWORK: This should be a binary search if n_probs is
     * moderately large (e.g., more than about 5-7).
     */
    for (i = 0;  i < n_probs;  i++)
	{
	if (ran_value <= probs[i])
	    return values[i];
	}
    return values[n_probs];
    }

/*
 * Generate an empirical distribution given a set of values and their
 * probabilities.
 */
double rds_lempirical(
    mt_state *		state,		/* State of the MT PRNG to use */
    int			n_probs,	/* Number of probabilities given */
    double *		values,		/* Vals returned with various probs */
    double *		probs)		/* Probs of various values */
    {
    int			i;		/* Index into both arrays */
    double		ran_value;	/* Value generated by PRNG */

    ran_value = mts_ldrand(state);
    /*
     * NEEDSWORK: This should be a binary search if n_probs is
     * moderately large (e.g., more than about 5-7).
     */
    for (i = 0;  i < n_probs;  i++)
	{
	if (ran_value <= probs[i])
	    return values[i];
	}
    return values[n_probs];
    }

/*
 * Generate a uniform integer distribution on the half-open interval
 * [lower, upper).  See comments on rds_iuniform.
 */
long rd_iuniform(
    long		lower,		/* Lower limit of distribution */
    long		upper)		/* Upper limit of distribution */
    {
    return rds_iuniform(&mt_default_state, lower, upper);
    }

#ifndef MT_NO_LONGLONG
/*
 * Generate a uniform integer distribution on the open interval
 * [lower, upper).  See comments on rds_iuniform.
 */
long long rd_liuniform(
    long long		lower,		/* Lower limit of distribution */
    long long		upper)		/* Upper limit of distribution */
    {
    return rds_liuniform(&mt_default_state, lower, upper);
    }
#endif /* MT_NO_LONGLONG */

/*
 * Generate a uniform distribution on the open interval [lower, upper).
 */
double rd_uniform(
    double		lower,		/* Lower limit of distribution */
    double		upper)		/* Upper limit of distribution */
    {
    return rds_uniform (&mt_default_state, lower, upper);
    }

/*
 * Generate a uniform distribution on the open interval [lower, upper).
 */
double rd_luniform(
    double		lower,		/* Lower limit of distribution */
    double		upper)		/* Upper limit of distribution */
    {
    return rds_luniform (&mt_default_state, lower, upper);
    }

/*
 * Generate an exponential distribution with the given mean.
 */
double rd_exponential(
    double		mean)		/* Mean of generated distribution */
    {
    return rds_exponential (&mt_default_state, mean);
    }

/*
 * Generate an exponential distribution with the given mean.
 */
double rd_lexponential(
    double		mean)		/* Mean of generated distribution */
    {
    return rds_lexponential (&mt_default_state, mean);
    }

/*
 * Generate a p-Erlang distribution with the given mean.
 */
double rd_erlang(
    int			p,		/* Order of distribution to generate */
    double		mean)		/* Mean of generated distribution */
    {
    return rds_erlang (&mt_default_state, p, mean);
    }

/*
 * Generate a p-Erlang distribution with the given mean.
 */
double rd_lerlang(
    int			p,		/* Order of distribution to generate */
    double		mean)		/* Mean of generated distribution */
    {
    return rds_lerlang (&mt_default_state, p, mean);
    }

/*
 * Generate a Weibull distribution with the given shape and scale parameters.
 */
double rd_weibull(
    double		shape,		/* Shape of the distribution */
    double		scale)		/* Scale of the distribution */
    {
    return rds_weibull (&mt_default_state, shape, scale);
    }

/*
 * Generate a Weibull distribution with the given shape and scale parameters.
 */
double rd_lweibull(
    double		shape,		/* Shape of the distribution */
    double		scale)		/* Scale of the distribution */
    {
    return rds_lweibull (&mt_default_state, shape, scale);
    }

/*
 * Generate a normal distribution with the given mean and standard
 * deviation.  See Law and Kelton, p. 491.
 */
double rd_normal(
    double		mean,		/* Mean of generated distribution */
    double		sigma)		/* Standard deviation to generate */
    {
    return rds_normal (&mt_default_state, mean, sigma);
    }

/*
 * Generate a normal distribution with the given mean and standard
 * deviation.  See Law and Kelton, p. 491.
 */
double rd_lnormal(
    double		mean,		/* Mean of generated distribution */
    double		sigma)		/* Standard deviation to generate */
    {
    return rds_lnormal (&mt_default_state, mean, sigma);
    }

/*
 * Generate a lognormal distribution with the given shape and scale
 * parameters.
 */
double rd_lognormal(
    double		shape,		/* Shape of the distribution */
    double		scale)		/* Scale of the distribution */
    {
    return rds_lognormal (&mt_default_state, shape, scale);
    }

/*
 * Generate a lognormal distribution with the given shape and scale
 * parameters.
 */
double rd_llognormal(
    double		shape,		/* Shape of the distribution */
    double		scale)		/* Scale of the distribution */
    {
    return rds_llognormal (&mt_default_state, shape, scale);
    }

/*
 * Generate a triangular distibution between given limits, with a
 * given mode.
 */
double rd_triangular(
    double		lower,		/* Lower limit of distribution */
    double		upper,		/* Upper limit of distribution */
    double		mode)
    {
    return rds_triangular (&mt_default_state, lower, upper, mode);
    }

/*
 * Generate a triangular distibution between given limits, with a
 * given mode.
 */
double rd_ltriangular(
    double		lower,		/* Lower limit of distribution */
    double		upper,		/* Upper limit of distribution */
    double		mode)
    {
    return rds_ltriangular (&mt_default_state, lower, upper, mode);
    }

/*
 * Generate an empirical distribution given a set of values and their
 * probabilities.
 */
double rd_empirical(
    int			n_probs,	/* Number of probabilities given */
    double *		values,		/* Vals returned with various probs */
    double *		probs)		/* Probs of various values */
    {
    return rds_empirical (&mt_default_state, n_probs, values, probs);
    }

/*
 * Generate an empirical distribution given a set of values and their
 * probabilities.
 */
double rd_lempirical(
    int			n_probs,	/* Number of probabilities given */
    double *		values,		/* Vals returned with various probs */
    double *		probs)		/* Probs of various values */
    {
    return rds_lempirical (&mt_default_state, n_probs, values, probs);
    }

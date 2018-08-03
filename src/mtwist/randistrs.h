#ifndef RANDISTRS_H
#define RANDISTRS_H

/*
 * $Id: randistrs.h,v 1.5 2008/07/25 23:34:01 geoff Exp $
 *
 * Header file for C/C++ use of a generalized package that generates
 * random numbers in various distributions, using the Mersenne-Twist
 * pseudo-RNG.  See mtwist.h and mtwist.c for documentation on the PRNG.
 *
 * Author of this header file: Geoffrey H. Kuenning, April 7, 2001.
 *
 * All of the functions provided by this package have three variants.
 * The rd_xxx versions use the default state vector provided by the MT
 * package.  The rds_xxx versions use a state vector provided by the
 * caller.  In general, the rds_xxx versions are preferred for serious
 * applications, since they allow random numbers used for different
 * purposes to be drawn from independent, uncorrelated streams.
 * Finally, the C++ interface provides a class "mt_distribution",
 * derived from mt_prng, with no-prefix ("xxx") versions of each
 * function.
 *
 * The summary below will describe only the rds_xxx functions.  The
 * rd_xxx functions have identical specifications, except that the
 * "state" argument is omitted.  In all cases, the "state" argument
 * has type mt_state, and must have been initialized either by calling
 * one of the Mersenne Twist seeding functions, or by being set to all
 * zeros.
 *
 * The "l" version of each function calls the 64-bit version of the
 * PRNG instead of the 32-bit version.  In general, you shouldn't use
 * those functions unless your application is *very* sensitive to tiny
 * variations in the probability distribution.  This is especially
 * true of the uniform and empirical distributions.
 *
 * Random-distribution functions:
 *
 * rds_iuniform(mt_state* state, long lower, long upper)
 *		(Integer) uniform on the half-open interval [lower, upper).
 * rds_liuniform(mt_state* state, long long lower, long long upper)
 *		(Integer) uniform on the half-open interval [lower, upper).
 *		Don't use unless you need numbers bigger than a long!
 * rds_uniform(mt_state* state, double lower, double upper)
 *		(Floating) uniform on the half-open interval [lower, upper).
 * rds_luniform(mt_state* state, double lower, double upper)
 *		(Floating) uniform on the half-open interval [lower, upper).
 *		Higher precision but slower than rds_uniform.
 * rds_exponential(mt_state* state, double mean)
 *		Exponential with the given mean.
 * rds_lexponential(mt_state* state, double mean)
 *		Exponential with the given mean.
 *		Higher precision but slower than rds_exponential.
 * rds_erlang(mt_state* state, int p, double mean)
 *		p-Erlang with the given mean.
 * rds_lerlang(mt_state* state, int p, double mean)
 *		p-Erlang with the given mean.
 *		Higher precision but slower than rds_erlang.
 * rds_weibull(mt_state* state, double shape, double scale)
 *		Weibull with the given shape and scale parameters.
 * rds_lweibull(mt_state* state, double shape, double scale)
 *		Weibull with the given shape and scale parameters.
 *		Higher precision but slower than rds_weibull.
 * rds_normal(mt_state* state, double mean, double sigma)
 *		Normal with the  given mean and standard deviation.
 * rds_lnormal(mt_state* state, double mean, double sigma)
 *		Normal with the  given mean and standard deviation.
 *		Higher precision but slower than rds_normal.
 * rds_lognormal(mt_state* state, double shape, double scale)
 *		Lognormal with the given shape and scale parameters.
 * rds_llognormal(mt_state* state, double shape, double scale)
 *		Lognormal with the given shape and scale parameters.
 *		Higher precision but slower than rds_lognormal.
 * rds_triangular(mt_state* state, double lower, double upper, double mode)
 *		Triangular on the closed interval (lower, upper) with
 *		the given mode.
 * rds_ltriangular(mt_state* state, double lower, double upper, double mode)
 *		Triangular on the closed interval (lower, upper) with
 *		the given mode.
 *		Higher precision but slower than rds_triangular.
 * rds_empirical(mt_state* state, int n_probs, double* values, double* probs)
 *		values[0] with probability probs[0], values[1] with
 *		probability probs[1] - probs[0], etc.; values[n_probs]
 *		with probability 1-probs[n_probs-1].  Note that there
 *		is one more value than there are probabilities.  It is
 *		the caller's responsibility to make sure that the
 *		probabilities are monotonically increasing and that
 *		their sum is less than or equal to 1; if this
 *		condition is violated, some of the values will never
 *		be generated.
 * rds_lempirical(mt_state* state, int n_probs, double* values, double* probs)
 *		Empirical distribution.  Higher precision but slower than
 *		rds_empirical.
 * rd_iuniform(long lower, long upper)
 * rd_liuniform(long long lower, long long upper)
 *		As above, using the default MT-PRNG.
 * rd_uniform(double lower, double upper)
 * rd_luniform(double lower, double upper)
 *		As above, using the default MT-PRNG.
 * rd_exponential(double mean)
 * rd_lexponential(double mean)
 *		As above, using the default MT-PRNG.
 * rd_erlang(int p, double mean)
 * rd_lerlang(int p, double mean)
 *		As above, using the default MT-PRNG.
 * rd_weibull(double shape, double scale)
 * rd_lweibull(double shape, double scale)
 *		As above, using the default MT-PRNG.
 * rd_normal(double mean, double sigma)
 * rd_lnormal(double mean, double sigma)
 *		As above, using the default MT-PRNG.
 * rd_lognormal(double shape, double scale)
 * rd_llognormal(double shape, double scale)
 *		As above, using the default MT-PRNG.
 * rd_triangular(double lower, double upper, double mode)
 * rd_ltriangular(double lower, double upper, double mode)
 *		As above, using the default MT-PRNG.
 * rd_empirical(int n_probs, double* values, double* probs)
 * rd_lempirical(int n_probs, double* values, double* probs)
 *		As above, using the default MT-PRNG.
 *
 * $Log: randistrs.h,v $
 * Revision 1.5  2008/07/25 23:34:01  geoff
 * Fix notation for intervals in commentary.
 *
 * Revision 1.4  2001/06/20 09:07:58  geoff
 * Fix a place where long long wasn't conditionalized.
 *
 * Revision 1.3  2001/06/19 00:41:17  geoff
 * Add the "l" versions of all functions.
 *
 * Revision 1.2  2001/06/18 10:09:24  geoff
 * Add the iuniform functions.  Improve the header comments.  Add a C++
 * interface.  Clean up some stylistic inconsistencies.
 *
 * Revision 1.1  2001/04/09 08:39:54  geoff
 * Initial revision
 *
 */

#include "mtwist.h"

#ifdef __cplusplus
extern "C"
    {
#endif

/*
 * Functions that use a provided state.
 */
extern long		rds_iuniform(mt_state* state, long lower, long upper);
					/* (Integer) uniform distribution */
#ifndef MT_NO_LONGLONG
extern long long	rds_liuniform(mt_state* state, long long lower,
			  long long upper);
					/* (Integer) uniform distribution */
#endif /* MT_NO_LONGLONG */
extern double		rds_uniform(mt_state* state,
			  double lower, double upper);
					/* (Floating) uniform distribution */
extern double		rds_luniform(mt_state* state,
			  double lower, double upper);
					/* (Floating) uniform distribution */
extern double		rds_exponential(mt_state* state, double mean);
					/* Exponential distribution */
extern double		rds_lexponential(mt_state* state, double mean);
					/* Exponential distribution */
extern double		rds_erlang(mt_state* state, int p, double mean);
					/* p-Erlang distribution */
extern double		rds_lerlang(mt_state* state, int p, double mean);
					/* p-Erlang distribution */
extern double		rds_weibull(mt_state* state,
			  double shape, double scale);
					/* Weibull distribution */
extern double		rds_lweibull(mt_state* state,
			  double shape, double scale);
					/* Weibull distribution */
extern double		rds_normal(mt_state* state,
			  double mean, double sigma);
					/* Normal distribution */
extern double		rds_lnormal(mt_state* state,
			  double mean, double sigma);
					/* Normal distribution */
extern double		rds_lognormal(mt_state* state,
			  double shape, double scale);
					/* Lognormal distribution */
extern double		rds_llognormal(mt_state* state,
			  double shape, double scale);
					/* Lognormal distribution */
extern double		rds_triangular(mt_state* state,
			  double lower, double upper, double mode);
					/* Triangular distribution */
extern double		rds_ltriangular(mt_state* state,
			  double lower, double upper, double mode);
					/* Triangular distribution */
extern double		rds_empirical(mt_state* state,
			  int n_probs, double* values, double* probs);
					/* Empirical distribution */
extern double		rds_lempirical(mt_state* state,
			  int n_probs, double* values, double* probs);
					/* Empirical distribution */

/*
 * Functions that use the default state of the PRNG.
 */
extern long		rd_iuniform(long lower, long upper);
					/* (Integer) uniform distribution */
#ifndef MT_NO_LONGLONG
extern long long	rd_liuniform(long long lower, long long upper);
					/* (Integer) uniform distribution */
#endif /* MT_NO_LONGLONG */
extern double		rd_uniform(double lower, double upper);
					/* (Floating) uniform distribution */
extern double		rd_luniform(double lower, double upper);
					/* (Floating) uniform distribution */
extern double		rd_exponential(double mean);
					/* Exponential distribution */
extern double		rd_lexponential(double mean);
					/* Exponential distribution */
extern double		rd_erlang(int p, double mean);
					/* p-Erlang distribution */
extern double		rd_lerlang(int p, double mean);
					/* p-Erlang distribution */
extern double		rd_weibull(double shape, double scale);
					/* Weibull distribution */
extern double		rd_lweibull(double shape, double scale);
					/* Weibull distribution */
extern double		rd_normal(double mean, double sigma);
					/* Normal distribution */
extern double		rd_lnormal(double mean, double sigma);
					/* Normal distribution */
extern double		rd_lognormal(double shape, double scale);
					/* Lognormal distribution */
extern double		rd_llognormal(double shape, double scale);
					/* Lognormal distribution */
extern double		rd_triangular(double lower, double upper, double mode);
					/* Triangular distribution */
extern double		rd_ltriangular(double lower, double upper,
			  double mode);	/* Triangular distribution */
extern double		rd_empirical(int n_probs,
			  double* values, double* probs);
					/* Empirical distribution */
extern double		rd_lempirical(int n_probs,
			  double* values, double* probs);
					/* Empirical distribution */

#ifdef __cplusplus
    }
#endif

#ifdef __cplusplus
/*
 * C++ interface to the random-distribution generators.  This class is
 * little more than a wrapper for the C functions, but it fits a bit
 * more nicely with the mt_prng class.
 */
class mt_distribution : public mt_prng
    {
    public:
	/*
	 * Constructors and destructors.  All constructors and
	 * destructors are the same as for mt_prng.
	 */
			mt_distribution(
					// Default constructor
			    bool pickSeed = false)
					// True to get seed from /dev/urandom
					// ..or time
			    : mt_prng(pickSeed)
			    {
			    }
			mt_distribution(unsigned long seed)
					// Construct with 32-bit seeding
			    : mt_prng(seed)
			    {
			    }
			mt_distribution(unsigned long seeds[MT_STATE_SIZE])
					// Construct with full seeding
			    : mt_prng(seeds)
			    {
			    }
			~mt_distribution() { }

	/*
	 * Functions for generating distributions.  These simply
	 * invoke the C functions above.
	 */
	long		iuniform(long lower, long upper)
					/* Uniform distribution */
			    {
			    return rds_iuniform(&state, lower, upper);
			    }
#ifndef MT_NO_LONGLONG
	long long	liuniform(long long lower, long long upper)
					/* Uniform distribution */
			    {
			    return rds_liuniform(&state, lower, upper);
			    }
#endif /* MT_NO_LONGLONG */
	double		uniform(double lower, double upper)
					/* Uniform distribution */
			    {
			    return rds_uniform(&state, lower, upper);
			    }
	double		luniform(double lower, double upper)
					/* Uniform distribution */
			    {
			    return rds_luniform(&state, lower, upper);
			    }
	double		exponential(double mean)
					/* Exponential distribution */
			    {
			    return rds_exponential(&state, mean);
			    }
	double		lexponential(double mean)
					/* Exponential distribution */
			    {
			    return rds_lexponential(&state, mean);
			    }
	double		erlang(int p, double mean)
					/* p-Erlang distribution */
			    {
			    return rds_erlang(&state, p, mean);
			    }
	double		lerlang(int p, double mean)
					/* p-Erlang distribution */
			    {
			    return rds_lerlang(&state, p, mean);
			    }
	double		weibull(double shape, double scale)
					/* Weibull distribution */
			    {
			    return rds_weibull(&state, shape, scale);
			    }
	double		lweibull(double shape, double scale)
					/* Weibull distribution */
			    {
			    return rds_lweibull(&state, shape, scale);
			    }
	double		normal(double mean, double sigma)
					/* Normal distribution */
			    {
			    return rds_normal(&state, mean, sigma);
			    }
	double		lnormal(double mean, double sigma)
					/* Normal distribution */
			    {
			    return rds_lnormal(&state, mean, sigma);
			    }
	double		lognormal(double shape, double scale)
					/* Lognormal distribution */
			    {
			    return rds_lognormal(&state, shape, scale);
			    }
	double		llognormal(double shape, double scale)
					/* Lognormal distribution */
			    {
			    return rds_llognormal(&state, shape, scale);
			    }
	double		triangular(double lower, double upper, double mode)
					/* Triangular distribution */
			    {
			    return rds_triangular(&state, lower, upper, mode);
			    }
	double		ltriangular(double lower, double upper, double mode)
					/* Triangular distribution */
			    {
			    return rds_ltriangular(&state, lower, upper, mode);
			    }
	double		empirical(int n_probs, double* values, double* probs)
					/* Empirical distribution */
			    {
			    return
			      rds_empirical(&state, n_probs, values, probs);
			    }
	double		lempirical(int n_probs, double* values, double* probs)
					/* Empirical distribution */
			    {
			    return
			      rds_lempirical(&state, n_probs, values, probs);
			    }
    };
#endif

#endif /* RANDISTRS_H */

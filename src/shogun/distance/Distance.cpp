/*
 * this program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Written (W) 2006-2009 Christian Gehl
 * Written (W) 2006-2009 Soeren Sonnenburg
 * Copyright (C) 2006-2009 Fraunhofer Institute FIRST and Max-Planck-Society
 */

#include <shogun/base/Parallel.h>
#include <shogun/base/Parameter.h>
#include <shogun/base/progress.h>
#include <shogun/io/File.h>
#include <shogun/io/SGIO.h>
#include <shogun/lib/Signal.h>
#include <shogun/lib/Time.h>
#include <shogun/lib/common.h>
#include <shogun/lib/config.h>

#include <shogun/distance/Distance.h>
#include <shogun/features/Features.h>

#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#endif

#ifdef HAVE_OPENMP
#include <omp.h>

#endif

using namespace shogun;

CDistance::CDistance() : CSGObject()
{
	init();
}


CDistance::CDistance(CFeatures* p_lhs, CFeatures* p_rhs) : CSGObject()
{
	init();
	init(p_lhs, p_rhs);
}

CDistance::~CDistance()
{
	SG_FREE(precomputed_matrix);
	precomputed_matrix=NULL;

	remove_lhs_and_rhs();
}

bool CDistance::init(CFeatures* l, CFeatures* r)
{
	REQUIRE(check_compatibility(l, r), "Features are not compatible!\n");

	//increase reference counts
	SG_REF(l);
	SG_REF(r);

	//remove references to previous features
	remove_lhs_and_rhs();

	lhs=l;
	rhs=r;

	num_lhs=l->get_num_vectors();
	num_rhs=r->get_num_vectors();

	SG_FREE(precomputed_matrix);
	precomputed_matrix=NULL ;

	return true;
}

bool CDistance::check_compatibility(CFeatures* l, CFeatures* r)
{
	REQUIRE(l, "Left hand side features must be set!\n");
	REQUIRE(r, "Right hand side features must be set!\n");

	REQUIRE(l->get_feature_type()==r->get_feature_type(),
		"Right hand side of features (%s) must be of same type with left hand side features (%s)\n",
		r->get_name(), l->get_name());

	if (l->support_compatible_class())
	{
		REQUIRE(l->get_feature_class_compatibility(r->get_feature_class()),
			"Right hand side of features (%s) must be compatible with left hand side features (%s)\n",
			r->get_name(), l->get_name());
	}
	else if (r->support_compatible_class())
	{
		REQUIRE(r->get_feature_class_compatibility(l->get_feature_class()),
			"Right hand side of features (%s) must be compatible with left hand side features (%s)\n",
			r->get_name(), l->get_name());
	}
	else
	{
		REQUIRE(l->get_feature_class()==r->get_feature_class(),
			"Right hand side of features (%s) must be compatible with left hand side features (%s)\n",
			r->get_name(), l->get_name());
	}

	return true;
}

void CDistance::load(CFile* loader)
{
	SG_SET_LOCALE_C;
	SG_RESET_LOCALE;
}

void CDistance::save(CFile* writer)
{
	SG_SET_LOCALE_C;
	SG_RESET_LOCALE;
}

void CDistance::remove_lhs_and_rhs()
{
	SG_UNREF(rhs);
	rhs = NULL;
	num_rhs=0;

	SG_UNREF(lhs);
	lhs = NULL;
	num_lhs=0;
}

void CDistance::remove_lhs()
{
	SG_UNREF(lhs);
	lhs = NULL;
	num_lhs=0;
}

/// takes all necessary steps if the rhs is removed from distance
void CDistance::remove_rhs()
{
	SG_UNREF(rhs);
	rhs = NULL;
	num_rhs=0;
}

CFeatures* CDistance::replace_rhs(CFeatures* r)
{
	//make sure features are compatible
	REQUIRE(check_compatibility(lhs, r), "Features are not compatible!\n");

	//remove references to previous rhs features
	CFeatures* tmp=rhs;

	rhs=r;
	num_rhs=r->get_num_vectors();

	SG_FREE(precomputed_matrix);
	precomputed_matrix=NULL ;

	// return old features including reference count
	return tmp;
}

CFeatures* CDistance::replace_lhs(CFeatures* l)
{
	//make sure features are compatible
	REQUIRE(check_compatibility(l, rhs), "Features are not compatible!\n");

	//remove references to previous rhs features
	CFeatures* tmp=lhs;

	lhs=l;
	num_lhs=l->get_num_vectors();

	SG_FREE(precomputed_matrix);
	precomputed_matrix=NULL ;

	// return old features including reference count
	return tmp;
}

float64_t CDistance::distance(int32_t idx_a, int32_t idx_b)
{
	REQUIRE(idx_a < lhs->get_num_vectors() && idx_b < rhs->get_num_vectors() && \
			idx_a >= 0 && idx_b >= 0,
			"idx_a (%d) must be in [0,%d] and idx_b (%d) must be in [0,%d]\n",
			idx_a, lhs->get_num_vectors()-1, idx_b, rhs->get_num_vectors()-1)

	ASSERT(lhs)
	ASSERT(rhs)

	if (lhs==rhs)
	{
		int32_t num_vectors = lhs->get_num_vectors();

		if (idx_a>=num_vectors)
			idx_a=2*num_vectors-1-idx_a;

		if (idx_b>=num_vectors)
			idx_b=2*num_vectors-1-idx_b;
	}


	if (precompute_matrix && (precomputed_matrix==NULL) && (lhs==rhs))
		do_precompute_matrix() ;

	if (precompute_matrix && (precomputed_matrix!=NULL))
	{
		if (idx_a>=idx_b)
			return precomputed_matrix[idx_a*(idx_a+1)/2+idx_b] ;
		else
			return precomputed_matrix[idx_b*(idx_b+1)/2+idx_a] ;
	}

	return compute(idx_a, idx_b);
}

void CDistance::run_distance_rhs(SGVector<float64_t>& result, const index_t idx_r_start, index_t idx_start, const index_t idx_stop, const index_t idx_a)
{
	for(index_t i=idx_r_start; idx_start < idx_stop; ++i,++idx_start)
        result.vector[i] = this->distance(idx_a,idx_start);
}

void CDistance::run_distance_lhs(SGVector<float64_t>& result, const index_t idx_r_start, index_t idx_start, const index_t idx_stop, const index_t idx_b)
{
	for(index_t i=idx_r_start; idx_start < idx_stop; ++i,++idx_start)
        result.vector[i] = this->distance(idx_start,idx_b);
}

void CDistance::do_precompute_matrix()
{
	int32_t num_left=lhs->get_num_vectors();
	int32_t num_right=rhs->get_num_vectors();
	SG_INFO("precomputing distance matrix (%ix%i)\n", num_left, num_right)

	ASSERT(num_left==num_right)
	ASSERT(lhs==rhs)
	int32_t num=num_left;

	SG_FREE(precomputed_matrix);
	precomputed_matrix=SG_MALLOC(float32_t, num*(num+1)/2);

	for (auto i : SG_PROGRESS(range(num)))
	{
		for (int32_t j=0; j<=i; j++)
			precomputed_matrix[i*(i+1)/2+j] = compute(i,j) ;
	}
}

void CDistance::init()
{
	precomputed_matrix = NULL;
	precompute_matrix = false;
	lhs = NULL;
	rhs = NULL;
	num_lhs=0;
	num_rhs=0;

	SG_ADD(&lhs, "lhs", "Left hand side features.");
	SG_ADD(&rhs, "rhs", "Right hand side features.");
}

template <class T>
SGMatrix<T> CDistance::get_distance_matrix()
{
	T* result = NULL;

	REQUIRE(has_features(), "no features assigned to distance\n")
	init(lhs, rhs);

	int32_t m=get_num_vec_lhs();
	int32_t n=get_num_vec_rhs();

	int64_t total_num = int64_t(m)*n;
	int64_t total=0;

	// if lhs == rhs and sizes match assume k(i,j)=k(j,i)
	bool symmetric= (lhs && lhs==rhs && m==n);

	SG_DEBUG("returning distance matrix of size %dx%d\n", m, n)

	result=SG_MALLOC(T, total_num);

	PRange<int64_t> pb = PRange<int64_t>(
	    range(total_num), *this->io, "PROGRESS: ", UTF8, []() { return true; });
	int32_t num_threads;
	int64_t step;
	#pragma omp parallel shared(num_threads, step)
	{
#ifdef HAVE_OPENMP
		#pragma omp single
		{
			num_threads=omp_get_num_threads();
			step=total_num/num_threads;
			num_threads--;
		}
		int32_t thread_num=omp_get_thread_num();
#else
		num_threads=0;
		step=total_num;
		int32_t thread_num=0;
#endif
		bool verbose=(thread_num == 0);

		int32_t start=compute_row_start(thread_num*step, n, symmetric);
		int32_t end=(thread_num==num_threads) ? m : compute_row_start((thread_num+1)*step, n, symmetric);

		for (int32_t i=start; i<end; i++)
		{
			int32_t j_start=0;

			if (symmetric)
				j_start=i;

			for (int32_t j=j_start; j<n; j++)
			{
				float64_t v=this->distance(i,j);
				result[i+j*m]=v;

				if (symmetric && i!=j)
					result[j+i*m]=v;

				if (verbose)
				{
					total++;

					if (symmetric && i!=j)
						total++;

					pb.print_progress();

					// TODO: replace with new signal
					// if (CSignal::cancel_computations())
					//	break;
				}
			}
		}
	}
	pb.complete();

	return SGMatrix<T>(result,m,n,true);
}

template SGMatrix<float64_t> CDistance::get_distance_matrix<float64_t>();
template SGMatrix<float32_t> CDistance::get_distance_matrix<float32_t>();

/**
 * Copyright 1993-2015 NVIDIA Corporation.  All rights reserved.
 *
 * Please refer to the NVIDIA end user license agreement (EULA) associated
 * with this source code for terms and conditions that govern your use of
 * this software. Any use, reproduction, disclosure, or distribution of
 * this software and related documentation outside the terms of the EULA
 * is strictly prohibited.
 *
 */

// System includes
#include <stdio.h>
#include <assert.h>

// CUDA runtime
#include <cuda_runtime.h>

// Helper functions and utilities to work with CUDA


__global__ void cudaSetBitArray(unsigned char *filter, long long *bitArray);
__global__ void cudaLookBitArray(unsigned char *filter, long long *bitArray, int *res);
__global__ void cudaLookFilters(unsigned char **filters, long long *bitArray, unsigned char *ans);
__global__ void cudaMergeFilter(unsigned char *dstFilter, unsigned char *srcFilter);

class Bloomfilter
{
public:
    Bloomfilter(long long size, int numHash,long long (**hash)(const char *));
    ~Bloomfilter();

    void insert(const char *data);
    bool lookup(const char *data);

    unsigned char *getFilter();
    void copyFilter(unsigned char *hostFilter);
    void setFilter(unsigned char *hostFilter);
	void getBitArray(long long *&bitArray, const char *data);
	bool lookBitArray(long long *bitArray);

    void mergeFilter(unsigned char *filter);
    void zeroFilter();

    static void initFilters(unsigned char ***filters, unsigned int size);
    static void initAnswer(unsigned char **ans, unsigned int size);
    static void setAnswer(unsigned char *ans, unsigned int size);
    static void lookFilters(unsigned char **filters, unsigned char *cuda_ans, long long *bitArray, unsigned char *ans, unsigned int size);
    void insertFilters(unsigned char **filters, unsigned int idx);
	static void error_handling(cudaError_t n);

    unsigned char *filter;
private:
    long long *cudaBitArray;
    int *cudaRes;

	long long size;
	int numHash;
	long long (**hash)(const char *);
};

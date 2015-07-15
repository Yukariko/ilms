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

class Bloomfilter
{
public:
    Bloomfilter(long long size, int numHash,long long (**hash)(char *));
    ~Bloomfilter();

    void insert(char *data);
    bool lookup(char *data);

    unsigned char *getFilter();
    void copyFilter(unsigned char *hostFilter);
	void getBitArray(long long *&bitArray, char *data);
	bool lookBitArray(long long *bitArray);

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
	long long (**hash)(char *);
};

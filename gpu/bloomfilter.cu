/**
 * Copyright 1993-2015 NVIDIA Corporation.	All rights reserved.
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
#include <iostream>

// CUDA runtime
#include <cuda_runtime.h>

#include "bloomfilter.h"

//const int bf_size = 2 * 1024 * 1024;

__global__ void cudaSetBitArray(unsigned char *filter, long long *bitArray)
{
	int tid = blockIdx.x;
	filter[bitArray[tid] >> 3] |= (1 << (bitArray[tid] & 7));
}

__global__ void cudaLookBitArray(unsigned char *filter, long long *bitArray, int *res)
{
	int tid = blockIdx.x;
	if(!(filter[bitArray[tid] >> 3] & (1 << (bitArray[tid] & 7))))
		*res = 0;
}

__global__ void cudaLookFilters(unsigned char **filters, long long *bitArray, unsigned char *ans)
{
	int nFilter = blockIdx.x * threadIdx.x;
	int nHash = threadIdx.y;

	if(!(filters[nFilter][bitArray[nHash] >> 3] & (1 << (bitArray[nHash] & 7))))
		ans[nFilter] = 0;
}

Bloomfilter::Bloomfilter(long long size, int numHash,long long (**hash)(char *))
{
	this->size = size;
	this->numHash = numHash;

	this->hash = (long long (**)(char *))malloc(sizeof(long long (*)(char *)) * numHash);

	for(int i=0;i<numHash;i++)
		this->hash[i] = hash[i];

	error_handling( cudaMalloc((void **)&filter, size / 8 + 1) );
	error_handling( cudaMemset((void *)filter, 0, size / 8 + 1) );
	error_handling( cudaMalloc((void **)&cudaBitArray, sizeof(long long) * numHash) );
	error_handling( cudaMalloc((void **)&cudaRes, sizeof(int)) );
}

Bloomfilter::~Bloomfilter()
{
		cudaFree(cudaBitArray);
		cudaFree(filter);
		cudaFree(cudaRes);
}

void Bloomfilter::insert(char *data)
{
	long long *bitArray;
	getBitArray(bitArray, data);
	cudaSetBitArray<<<numHash,1>>>(filter, cudaBitArray);
}

bool Bloomfilter::lookup(char *data)
{
	long long *bitArray;
	getBitArray(bitArray, data);
	return lookBitArray(bitArray);
}

unsigned char *Bloomfilter::getFilter()
{
	return filter;
}

void Bloomfilter::copyFilter(unsigned char *hostFilter)
{
	return error_handling( cudaMemcpy((void *)hostFilter, (const void *)filter, size / 8 + 1, cudaMemcpyDeviceToHost) );
}

void Bloomfilter::getBitArray(long long *&bitArray, char *data)
{
	long long array[20];
	for(int i=0;i<numHash;i++)
	{
		array[i] = hash[i](data) % size;
	}
	error_handling( cudaMemcpy((void *)cudaBitArray, (const void *)array, sizeof(long long) * numHash, cudaMemcpyHostToDevice) );
	//error_handling( cudaMemcpy((void *)bitArray, (const void *)cudaBitArray, sizeof(long long) * numHash, cudaMemcpyHostToDevice) );
	bitArray = cudaBitArray;
}

bool Bloomfilter::lookBitArray(long long *bitArray)
{
	int res = 1;
	error_handling( cudaMemcpy((void *)cudaRes, (const void *)&res, sizeof(int), cudaMemcpyHostToDevice) );

	cudaLookBitArray<<<numHash,1>>>(filter, bitArray, cudaRes);

	error_handling( cudaMemcpy((void *)&res, (const void *)cudaRes,sizeof(int), cudaMemcpyDeviceToHost) );
	return !!res;
}

void Bloomfilter::initFilters(unsigned char ***filters, unsigned int size)
{
	error_handling( cudaMalloc((void **)filters, size * sizeof(unsigned char *)) );
}

void Bloomfilter::insertFilters(unsigned char **filters, unsigned int idx)
{
	error_handling( cudaMemcpy(&filters[idx], (const void *)&filter, sizeof(unsigned char *), cudaMemcpyHostToDevice) );
}

void Bloomfilter::initAnswer(unsigned char **ans, unsigned int size)
{
	error_handling( cudaMalloc((void **)ans, size) );
}

void Bloomfilter::setAnswer(unsigned char *ans, unsigned int size)
{
	error_handling( cudaMemset((void *)ans, 1, size) );
}

void Bloomfilter::lookFilters(unsigned char **filters, unsigned char *cuda_ans, long long *bitArray, unsigned char *ans, unsigned int size)
{
	setAnswer(cuda_ans, size);

	dim3 block(32);
	dim3 thread(size / 32, 11);

	cudaLookFilters<<<block, thread>>>(filters, bitArray, cuda_ans);
	error_handling( cudaMemcpy((void *)ans, (const void *)cuda_ans, size, cudaMemcpyDeviceToHost) );
}


void Bloomfilter::error_handling(cudaError_t n)
{
	if(n)
	{
		printf("Error! %d\n",n);
		exit(0);
	}
}

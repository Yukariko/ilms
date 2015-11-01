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

// 주어진 비트번호에 비트 set
__global__ void cudaSetBitArray(unsigned char *filter, long long *bitArray);
// 주어진 비트번호로 검색
__global__ void cudaLookBitArray(unsigned char *filter, long long *bitArray, int *res);
// 여러 필터에 대해 데이터 검색
__global__ void cudaLookFilters(unsigned char **filters, long long *bitArray, unsigned char *ans);
// 필터 병합
__global__ void cudaMergeFilter(unsigned char *dstFilter, unsigned char *srcFilter);

class Bloomfilter
{
public:
	Bloomfilter(long long size, int numHash,long long (**hash)(const char *));
	~Bloomfilter();

	// 데이터 등록
	void insert(const char *data);

	// 데이터 검색
	bool lookup(const char *data);

	unsigned char *getFilter();

	// 현재 핕터를 hostFilter로 복사
	void copyFilter(unsigned char *hostFilter);

	// 다른 필터로 현재 핕터를 교체
	void setFilter(unsigned char *hostFilter);

	// 해당 데이터의 비트번호들을 구함
	void getBitArray(long long *&bitArray, const char *data);

	// 미리 구해놓은 비트번호들로 데이터 검색
	bool lookBitArray(long long *bitArray);

	// 다른 필터와 병합
	void mergeFilter(unsigned char *filter);

	// fileter를 0으로 초기화
	void zeroFilter();

	// 여러 필터들을 한 배열에 나열하기 위한 filters 배열 초기화
	static void initFilters(unsigned char ***filters, unsigned int size);

	// 검색결과 배열 생성
	static void initAnswer(unsigned char **ans, unsigned int size);

	// 검색결과 초기화
	static void setAnswer(unsigned char *ans, unsigned int size);

	// 여러 필터에 대해 검색
	static void lookFilters(unsigned char **filters, unsigned char *cuda_ans, long long *bitArray, unsigned char *ans, unsigned int size);

	// 필터배열에 필터 추가
	void insertFilters(unsigned char **filters, unsigned int idx);
	static void error_handling(cudaError_t n);

	unsigned char *filter;
private:
	// cpu로 구한 비트번호들을 gpu로 담을 배열
	long long *cudaBitArray;
	// 검색결과를 표시하는 배열
	int *cudaRes;

	// 필터 크기 (bit)
	long long size;
	// 해시함수 개수
	int numHash;
	// 해시함수
	long long (**hash)(const char *);
};

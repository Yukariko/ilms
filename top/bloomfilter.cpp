#include <cstring>
#include "bloomfilter.h"

/*
 * 블룸필터의 생성자
 * 비트필드를 초기화 함
 * size : 비트필드의 크기(비트단위)
 * numHash : 해시 함수 개수
 * hash : 해시 함수 배열
 */

Bloomfilter::Bloomfilter(long long size, int numHash,long long (**hash)(const char *))
{
	this->size = size;
	this->numHash = numHash;

	this->hash = new(long long (*[numHash])(const char *));

	for(int i=0;i<numHash;i++)
		this->hash[i] = hash[i];

	filter = new unsigned char[size/8+1];
	memset(filter,0,size/8+1);
}

/*
 * 블룸필터 소멸자
 * 필드 맵 삭제
 */

Bloomfilter::~Bloomfilter()
{
	delete[] filter;
	delete[] hash;
}

void Bloomfilter::insert(const char *data)
{
	for(int i=0;i<numHash;i++)
	{
		const long long res = hash[i](data) % size;
		filter[res>>3] |= (1 << (res&7));
	}
}

bool Bloomfilter::lookup(const char *data)
{
	for(int i=0;i<numHash;i++)
	{
		const long long res = hash[i](data) % size;
		if(!(filter[res>>3] & (1 << (res&7))))
			return false;
	}
	return true;
}

/*
 * bitArray : 비트번호들의 배열, numHash만큼 들어있어야 함
 */

bool Bloomfilter::lookBitArray(long long *bitArray)
{
	for(int i=0;i<numHash;i++)
	{
		const long long res = bitArray[i];
		if(!(filter[res>>3] & (1 << (res&7))))
			return false;
	}
	return true;
}

/*
 * bitArray : 구한 비트번호들을 저장할 변수
 * data : 비트번호를 구할 데이터
 */

void Bloomfilter::getBitArray(long long *bitArray, const char *data)
{
	for(int i=0;i<numHash;i++)
	{
		bitArray[i] = hash[i](data) % size;
	}
}

void Bloomfilter::zeroFilter()
{
	memset((void *)filter, 0, size / 8 + 1);
}

/*
 * hostFilter : 다른 블룸필터의 필터 배열, 사이즈가 같아야함
 */

void Bloomfilter::mergeFilter(unsigned char *hostFilter)
{
	for(int i=0, e = size / 8 + 1; i < e; i++)
		filter[i] |= hostFilter[i];
}

void Bloomfilter::setFilter(unsigned char *hostFilter)
{
	memcpy((void *)filter, (const void *)hostFilter, size / 8 + 1);
}
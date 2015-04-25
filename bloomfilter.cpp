#include <cstring>

#include "bloomfilter.h"

#define SETBIT(a,n) (a[n>>3] |= (1 << (n&7)))
#define GETBIT(a,n) (a[n>>3] & (1 << (n&7)))

/*
 * 블룸필터의 생성자
 * 비트필드를 초기화 함
 * size : 비트필드의 크기(비트단위)
 * numHash : 해시 함수 개수
 * hash : 해시 함수 배열
 */

Bloomfilter::Bloomfilter(long long size, int numHash,long long (**hash)(int)))
{
	this->size = size;
	this->numHash = numHash;
	this->hash = hash;

	field = new unsigned char[size/8+1];
	memset(filed,0,size/8+1);
}

/*
 * 블룸필터 소멸자
 * 필드 맵 삭제
 */

Bloomfilter::~Bloomfilter()
{
	delete[] field;
}

/*
 * 데이터 등록
 */

void Bloomfilter::insert(int data)
{
	for(int i=0;i<numHash;i++)
	{
		const long long res = hash[i](data) % size;
		SETBIT(filed,res);
	}
}

/*
 * 데이터 검색
 */

bool Bloomfilter::lookup(int data)
{
	for(int i=0;i<numHash;i++)
	{
		const long long res = hash[i](data) % size;
		if(!GETBIT(field,res)) return false;
	}
	return true;
}
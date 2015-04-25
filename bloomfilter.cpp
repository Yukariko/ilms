#include <cstring>
#include "bloomfilter.h"

/*
 * 블룸필터의 생성자
 * 비트필드를 초기화 함
 * size : 비트필드의 크기(비트단위)
 * numHash : 해시 함수 개수
 * hash : 해시 함수 배열
 */

Bloomfilter::Bloomfilter(long long size, int numHash,long long (**hash)(long long))
{
	this->size = size;
	this->numHash = numHash;
	this->hash = hash;

	field = new unsigned char[size/8+1];
	memset(field,0,size/8+1);
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

void Bloomfilter::insert(long long data)
{
	for(int i=0;i<numHash;i++)
	{
		const long long res = hash[i](data) % size;
		field[res>>3] |= (1 << (res&7));
	}
}

/*
 * 데이터 검색
 */

bool Bloomfilter::lookup(long long data)
{
	for(int i=0;i<numHash;i++)
	{
		const long long res = hash[i](data) % size;
		if(!(field[res>>3] & (1 << (res&7))))
			return false;
	}
	return true;
}
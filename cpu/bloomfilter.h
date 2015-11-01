#ifndef BLOOMFILTER_H
#define BLOOMFILTER_H

/*
 * 블룸필터 클래스
 * CPU 버전
 */

class Bloomfilter
{
public:
	Bloomfilter(long long size, int numHash,long long (**hash)(const char *));
	~Bloomfilter();

	// 데이터 등록
	void insert(const char *data);

	// 데이터 검색
	bool lookup(const char *data);

	// 다른 필터로 현재 핕터를 교체
	void setFilter(unsigned char *hostFilter);

	// 다른 필터와 병합
	void mergeFilter(unsigned char *hostFilter);

	// fileter를 0으로 초기화
	void zeroFilter();

	// 미리 구해놓은 비트번호들로 데이터 검색
	bool lookBitArray(long long *bitArray);

	// 해당 데이터의 비트번호들을 구함
	void getBitArray(long long *bitArray, const char *data);

	// 필터 배열
	unsigned char *filter;
private:
	// 필터 크기 (bit)
	long long size;
	// 해시함수 개수
	int numHash;
	// 해시함수
	long long (**hash)(const char *);
};

#endif
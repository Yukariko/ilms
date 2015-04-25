#include "ilms.h"

/*
 * Ilms 생성자
 * 설정들을 불러오고
 * 블룸필터를 초기화 해줌
 * 해시 함수 정의 필요
 */

const long long defaultSize = 8 * 1024 * 1024;

Ilms::Ilms()
{
	long long (*hash[])(int) = {test,test2,test3,test4,test5,test6,test7,test8,test9,test10,test11};
	myFilter = new Bloomfilter(defaultSize, 11, hash);
	childFilter = new Bloomfilter(defaultSize, 11, hash);
}

int Ilms::start()
{


	return 1;
}
#include "ilms.h"

/*
 * 테스트 해시 함수
 */

long long test(long long data){return data*997*1499;}
long long test2(long long data){return data*1009*1361;}
long long test3(long long data){return data*1013*1327;}
long long test4(long long data){return data*1013*1327;}
long long test5(long long data){return data*1013*1327;}
long long test6(long long data){return data*1013*1327;}
long long test7(long long data){return data*1013*1327;}
long long test8(long long data){return data*1013*1327;}
long long test9(long long data){return data*1013*1327;}
long long test10(long long data){return data*1013*1327;}
long long test11(long long data){return data*1013*1327;}


/*
 * Ilms 생성자
 * 설정들을 불러오고
 * 블룸필터를 초기화 해줌
 * 해시 함수 정의 필요
 */

const long long defaultSize = 8 * 1024 * 1024;

Ilms::Ilms()
{
	long long (*hash[11])(long long) = {test,test2,test3,test4,test5,test6,test7,test8,test9,test10,test11};
	myFilter = new Bloomfilter(defaultSize, 11, hash);
	childFilter = new Bloomfilter(defaultSize, 11, hash);
}

int Ilms::start()
{


	return 1;
}
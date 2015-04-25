#ifndef BLOOMFILTER_H
#define BLOOMFILTER_H

class Bloomfilter
{
public:
	Bloomfilter(long long size, int numHash,long long (**hash)(long long));
	~Bloomfilter();
	void insert(long long data);
	bool lookup(long long data);

private:
	unsigned char *field;
	long long size;
	int numHash;
	long long (**hash)(long long);
};


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

#endif
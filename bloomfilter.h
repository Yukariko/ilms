#ifndef BLOOMFILTER_H
#define BLOOMFILTER_H

class Bloomfilter
{
public:
	Bloomfilter(long long size, int numHash,long long (**hash)(int)));
	~Bloomfilter();
	void insert(int data);
	bool lookup(int data);

private:
	unsigned char *field;
	long long size;
	int numHash;
	long long (**hash)(int));
};


/*
 * 테스트 해시 함수
 */

long long test(int data){return a*997*1499;}
long long test2(int data){return a*1009*1361;}
long long test3(int data){return a*1013*1327;}
long long test4(int data){return a*1013*1327;}
long long test5(int data){return a*1013*1327;}
long long test6(int data){return a*1013*1327;}
long long test7(int data){return a*1013*1327;}
long long test8(int data){return a*1013*1327;}
long long test9(int data){return a*1013*1327;}
long long test10(int data){return a*1013*1327;}
long long test11(int data){return a*1013*1327;}

#endif
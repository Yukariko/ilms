#include "scanner.h"

/*
 * 스캐너 기본 생성자
 * 사실상 안쓰임
 */

Scanner::Scanner()
{
	buf = 0;
	cur = 0;
	end = 0;
	len = 0;
}

/*
 * 스캐너 생성자
 * 버퍼를 받고 설정들 초기화
 */

Scanner::Scanner(char *buf, int len)
{
	this->buf = buf;
	this->end = buf+len;
	this->len = len;
	cur = buf;
}

/*
 * 스캐너 변수 읽기
 * 1바이트를 읽어 char형 변수 반환
 */

bool Scanner::next_value(char &val)
{
	if(isEnd(1)) return false;
	val = *cur;
	cur += 1;
	return true;
}

/*
 * 스캐너 변수 읽기
 * 1바이트를 읽어 unsigned char형 변수 반환
 */

bool Scanner::next_value(unsigned char &val)
{
	if(isEnd(1)) return false;
	val = *(unsigned char *)cur;
	cur += 1;
	return true;	
}

/*
 * 스캐너 변수 읽기
 * 4바이트를 읽어 int형 변수 반환
 */

bool Scanner::next_value(int &val)
{
	if(isEnd(4)) return false;
	val = *(int *)cur;
	cur += 4;
	return true;
}

/*
 * 스캐너 변수 읽기
 * 8바이트를 읽어 long long형 변수 반환
 */

bool Scanner::next_value(long long &val)
{
	if(isEnd(8)) return false;
	val = *(long long *)cur;
	cur += 8;
	return true;
}

/*
 * 스캐너 변수 읽기
 * char형 포인터 변수 반환
 */

bool Scanner::next_value(char *&val)
{
	unsigned char len;
	if(!next_value(len)) return false;
	if(isEnd(len)) return false;

	val = cur;
	cur += len;

	return true;
}

/*
 * 스캐너 변수 읽기
 * char형 포인터 변수 반환
 */

bool Scanner::next_value(char *&val, int len)
{
	if(isEnd(len)) return false;

	val = cur;
	cur += len;

	return true;
}

/*
 * 스캐너 현재 위치 반환
 */

char *Scanner::get_cur()
{
	return cur;
}

/*
 * 스캐너의 끝범위 반환
 */

bool Scanner::isEnd(int pos)
{
	return cur + pos > end;
}
#include "scanner.h"


Scanner::Scanner()
{
	buf = 0;
	cur = 0;
	end = 0;
	len = 0;
}

Scanner::Scanner(char *buf, int len)
{
	this->buf = buf;
	this->end = buf+len;
	this->len = 0;
	cur = buf;
}

bool Scanner::next_value(char &val)
{
	if(isEnd(1)) return false;
	val = *cur;
	cur += 1;
	return true;
}

bool Scanner::next_value(unsigned char &val)
{
	if(isEnd(1)) return false;
	val = *(unsigned char *)cur;
	cur += 1;
	return true;	
}

bool Scanner::next_value(int &val)
{
	if(isEnd(4)) return false;
	val = *(int *)cur;
	cur += 4;
	return true;
}

bool Scanner::next_value(long long &val)
{
	if(isEnd(8)) return false;
	val = *(long long *)cur;
	cur += 4;
	return true;
}

bool Scanner::next_value(char *&val)
{
	unsigned char len;
	if(!next_value(len)) return false;
	if(isEnd(len)) return false;

	val = cur;
	cur += len;

	return true;
}

char *get_cur()
{
	return cur;
}

bool Scanner::isEnd(int pos)
{
	return cur + pos > end;
}
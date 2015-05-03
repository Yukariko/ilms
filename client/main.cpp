#include <iostream>

#include "ilmscli.h"

int main()
{
	IlmsCli ilms("210.117.184.166");

	char buf[BUF_SIZE];

	int len;

	ilms->req_data_add(1234LL,"210.117.184.166");

	len = ilms->req_data_search(1234LL, buf);

	if(len > 0)
		std::cout << buf << std::endl;

	return 0;
}
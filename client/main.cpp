#include <iostream>
#include <cstdio>

#include "ilmscli.h"

int main()
{
	char cmd[BUF_SIZE];
	IlmsCli ilms("0");
	while(scanf("%s",cmd) == 1)
	{
		char value[BUF_SIZE];

		scanf("%s",value);

		if(strcmp(cmd, "IP") == 0)
		{
			ilms.setIp(value);
		}

		else if(strcmp(cmd, "SET") == 0)
		{
			long long data = *(long long *)value;
			scanf("%s",value);
			ilms.req_data_add(data, value);
		}

		else if(strcmp(cmd, "GET") == 0)
		{
			char buf[BUF_SIZE];
			long long data = *(long long *)value;
			int len = ilms.req_data_search(data, buf);

			if(len > 0)
				std::cout << buf << std::endl;
		}

		else if(strcmp(cmd, "DELETE") == 0)
		{
			long long data = *(long long *)value;
			ilms.req_data_delete;
		}

		else if(strcmp(cmd, "EXIT") == 0)
		{
			break;
		}

		else
		{
			std::cout << "ERROR" << std::endl;
		}
	}
	return 0;
}
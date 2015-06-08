#include <iostream>
#include <cstdio>
#include <cstring>

#include "ilmscli.h"

int main()
{
	char cmd[BUF_SIZE];
	IlmsCli ilms("0");
	while(scanf("%s",cmd) == 1)
	{
		char value[BUF_SIZE] = {};
		char data[BUF_SIZE] = {};
		if(strcmp(cmd, "IP") == 0)
		{	
			if(scanf("%s",value) != 1)
			{
				std::cout << "ERROR" << std::endl;
				exit(1);
			}
			ilms.setIp(value);
		}

		else if(strcmp(cmd, "REG") == 0)
		{
			if(scanf("%s",data) != 2)
			{
				std::cout << "ERROR" << std::endl;
				exit(1);
			}

			ilms.req_data_register(data, value);
		}

		else if(strcmp(cmd, "SET") == 0)
		{
			if(scanf("%s%s",data,value) != 2)
			{
				std::cout << "ERROR" << std::endl;
				exit(1);
			}

			ilms.req_data_update(0, data, value);
		}

		else if(strcmp(cmd, "GET") == 0)
		{
			char buf[BUF_SIZE];
			if(scanf("%s",data) != 1)
			{
				std::cout << "ERROR" << std::endl;
				exit(1);
			}
			int len = ilms.req_data_search(data, buf);

			if(len > 0)
				std::cout << buf << std::endl;
		}

		else if(strcmp(cmd, "SUB") == 0)
		{
			if(scanf("%s%s",data,value) != 2)
			{
				std::cout << "ERROR" << std::endl;
				exit(1);
			}

			ilms.req_data_update(1, data, value);
		}

		else if(strcmp(cmd, "DELETE") == 0)
		{
			if(scanf("%s",data) != 1)
			{
				std::cout << "ERROR" << std::endl;
				exit(1);
			}
			ilms.req_data_delete(data);
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
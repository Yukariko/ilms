#ifndef ILMSCLI_H
#define ILMSCLI_H

#include <iostream>

#define PORT 7979

class IlmsCli 
{
public:
	IlmsCli(std::string ip);

	void setIp(std::string ip);


	void req_data_add(long long data,std::string ip);
	void req_data_delete(long long data);
	void req_data_search(long long data);


	void send(const char *buf,int len);
	int recieve(char *buf, int len);

private:
	std::string ip;
};


#endif
#ifndef ILMSCLI_H
#define ILMSCLI_H

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <iostream>

#define PORT 7979
#define BUF_SIZE 256

class IlmsCli 
{
public:
	IlmsCli(std::string ip);

	void setIp(std::string ip);


	void req_data_add(long long data,std::string ip);
	void req_data_delete(long long data);
	int req_data_search(long long data,char *buf);


	void send(const char *buf,int len);
	int recieve(char *buf);

	void error_handling(const char *message);

private:
	int sock;
	struct sockaddr_in serv_adr;
	std::string ip;
};


#endif
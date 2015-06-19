#ifndef ILMSCLI_H
#define ILMSCLI_H

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <iostream>

#define PORT 7979
#define MYPORT 7979
#define BUF_SIZE 256

class IlmsCli 
{
public:
	IlmsCli(std::string ip);

	void setIp(std::string ip);


	void req_id_register(char *data,std::string ip);
	void req_loc_update(char mode, char *data,std::string ip);
	void req_id_deregister(char *data);
	int req_lookup(char *data,char *buf);


	void send(const char *buf,int len);
	int recieve(char *buf);

	void error_handling(const char *message);

private:
	int sock;
	struct sockaddr_in serv_adr;
	std::string ip;
};


#endif
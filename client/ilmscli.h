#ifndef ILMSCLI_H
#define ILMSCLI_H

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <iostream>

#define PORT 7979
#define MYPORT 7979
#define BUF_SIZE 256
#define ID_SIZE 24

using namespace std;

class IlmsCli 
{
public:
	IlmsCli(string ip);

	void set_ip(const string& ip);
	bool req_id_register(const string& id, const string& loc);
	bool req_loc_update(char mode, const string& id, const string& loc);
	bool req_id_deregister(const string& id);
	int req_lookup(const string& id, string& buf);

	void send(const char *buf,int len);
	int recieve(char *buf);
	void error_handling(const char *message);

private:
	int sock;
	struct sockaddr_in serv_adr;
	string ip;
};


#endif
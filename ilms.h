#ifndef ILMS_H
#define ILMS_H

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "bloomfilter.h"
#include "topology.h"

#define BUF_SIZE 256;
#define PORT 7979

class Ilms : public tree
{
public:
	Ilms();
	~Ilms();

	void start();
	void send(const char *ip,char *buf,int len);

	//data
	void insert(long long data);
	void search(long long data);

	//process
	void proc_bf_add(char *buf, int len);
	void proc_data_add(char *buf, int len);
	void proc_data_search(char *buf, int len);

private:
	void error_handling(char *message);
	
	Bloomfilter *myFilter;
	Bloomfilter *childFilter;

	int sock;
	struct sockaddr_in serv_adr;
};

#endif
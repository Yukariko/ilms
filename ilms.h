#ifndef ILMS_H
#define ILMS_H

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "bloomfilter.h"
#include "topology.h"
#include "scanner.h"

#define BUF_SIZE 256
#define PORT 7979

class Ilms : public Tree
{
public:
	Ilms();
	~Ilms();

	void start();
	void send(const char *ip,char *buf,int len);

	//data
	void insert(long long key, long long value);
	int search(long long key,char *buf, int len);
	bool remove(long long key);


	//process
	void proc_bf_add();
	void proc_data_add();
	void proc_data_search();
	void proc_data_search_fail();
	void proc_data_delete();

private:
	void error_handling(const char *message);

	Bloomfilter *myFilter;
	Bloomfilter *childFilter;

	Scanner sc;

	int sock;
	struct sockaddr_in serv_adr;
};

#endif
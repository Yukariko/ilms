#ifndef ILMS_H
#define ILMS_H

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <iostream>

#include <leveldb/db.h>
#include <leveldb/write_batch.h>

#include "bloomfilter.h"
#include "topology.h"
#include "scanner.h"

#define BUF_SIZE 256
#define PORT 7979

#define DB_PATH "./db"

class Ilms : public Tree
{
public:
	Ilms();
	~Ilms();

	void start();
	void send(const char *ip,const char *buf,int len);

	//data
	void insert(char *key,int klen, char *val,int vlen);
	bool search(char *key,int klen,std::string &val);
	bool remove(char *key, int klen);


	//process
	void proc_bf_add();
	void proc_data_add();
	void proc_data_search();
	void proc_data_search_fail();
	void proc_data_delete();

	//request
	void req_data_add();
	void req_data_search(char *ip_org);
	void req_data_delete(char *ip_org);

private:

	leveldb::DB* db;
	leveldb::Options options;


	Bloomfilter *myFilter;
	Bloomfilter *childFilter;

	Scanner sc;

	int sock;
	struct sockaddr_in serv_adr;
};

#endif
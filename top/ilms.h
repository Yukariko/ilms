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

#define DATA_SIZE 8

#define DB_PATH "./db"

class Ilms : public Tree
{
public:
	Ilms();
	~Ilms();

	void start();
	void send(unsigned long ip_num,const char *buf,int len);
	int send_child(char *data);
	int send_child(unsigned long ip_num, char *data);
	int send_peer(char *data);
	void send_top(char *data);

	//data
	void insert(char *key,int klen,char *val,int vlen);
	bool search(char *key,int klen,std::string &val);
	bool remove(char *key,int klen);


	//process
	void proc_bf_add(unsigned long ip_num);
	void proc_data_add();
	void proc_data_search(unsigned long ip_num);
	void proc_data_search_fail();
	void proc_data_delete(unsigned long ip_num);

	//request
	void req_data_add();
	void req_data_search(unsigned long ip_num);
	void req_data_delete(unsigned long ip_num);

	//peer
	void peer_bf_add(unsigned long ip_num);
	void peer_data_search(unsigned long ip_num);
	void peer_data_delete(unsigned long ip_num);

	//top
	void top_bf_add(unsigned long ip_num);
	void top_data_search(unsigned long ip_num);

private:

	leveldb::DB* db;
	leveldb::Options options;


	Bloomfilter *my_filter;
	Bloomfilter **child_filter;
	Bloomfilter **top_filter;
	Bloomfilter **peer_filter;

	Scanner sc;

	int sock;
	struct sockaddr_in serv_adr;
};

#endif
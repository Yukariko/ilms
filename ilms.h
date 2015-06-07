#ifndef ILMS_H
#define ILMS_H

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <iostream>
#include <algorithm>
#include <thread>
#include <atomic>

#include <leveldb/db.h>
#include <leveldb/write_batch.h>

#include "bloomfilter.h"
#include "topology.h"
#include "scanner.h"

#define BUF_SIZE 256
#define PORT 7979
#define DATA_SIZE 24
#define NTHREAD 64U
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

	//data
	void insert(char *key,int klen,char *val,int vlen);
	bool search(char *key,int klen,std::string &val);
	bool remove(char *key,int klen);


	//process
	void proc_bf_add(unsigned long ip_num);
	void proc_data_update();
	void proc_data_search(unsigned long ip_num);
	void proc_data_search_fail();
	void proc_data_search_down();

	//request
	void req_data_update();
	void req_data_search(unsigned long ip_num);
	void req_data_delete(unsigned long ip_num);

	//peer
	void peer_bf_add(unsigned long ip_num);
	void peer_data_search(unsigned long ip_num);
	void peer_data_search_down();
	
	//thread
	void child_run(unsigned int i);
	void peer_run(unsigned int i);

private:
	leveldb::DB* db;
	leveldb::Options options;


	static Bloomfilter *my_filter;
	static Bloomfilter **child_filter;
	static Bloomfilter **peer_filter;

	static Scanner sc;

	std::atomic<int> global_counter;
	std::thread task[NTHREAD];
	long long bitArray[12];
	static int sock;
	static struct sockaddr_in serv_adr;
};

#endif
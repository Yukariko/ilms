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
	static void send(unsigned long ip_num,const char *buf,int len);
	int send_child(char *data);
	int send_child(unsigned long ip_num, char *data);
	int send_peer(char *data);

	//data
	void insert(char *key,int klen,const char *val,int vlen);
	bool search(char *key,int klen,std::string &val);
	bool remove(char *key,int klen);


	//process
	void proc_bf_update(unsigned long ip_num);
	void proc_lookup(unsigned long ip_num);
	void proc_lookup_nack();
	void proc_lookup_down();

	//request
	void req_id_register();
	void req_loc_update();
	void req_lookup(unsigned long ip_num);
	void req_id_deregister(unsigned long ip_num);

	//peer
	void peer_bf_update(unsigned long ip_num);
	void peer_lookup(unsigned long ip_num);
	void peer_lookup_down();
	
	//thread
	void child_run(unsigned int i);
	void peer_run(unsigned int i);

	//test
	void test_process();

private:
	leveldb::DB* db;
	leveldb::Options options;


	static Bloomfilter *my_filter;
	static Bloomfilter **child_filter;
	static Bloomfilter **peer_filter;

	static Scanner sc;

	static std::atomic<int> global_counter;
	std::thread task[NTHREAD];
	static long long bitArray[12];
	static int sock;
	static struct sockaddr_in serv_adr;
};

#endif
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
#define REFRESH_PORT 7980
#define DATA_SIZE 24
#define NTHREAD 64U
#define DB_PATH "./db"
#define REFRESH_FREQUENCY 60
#define VIRTUAL_FILTER_ID_NUM 1000
#define STAT_FREQUENCY 60

class Ilms : public Tree
{
public:
	Ilms();
	~Ilms();

	void start();
	static void send_node(unsigned long ip_num,const char *buf,int len);
	void send_refresh(unsigned long ip_num, unsigned char *filter);
	int send_child(char *data);
	int send_child(unsigned long ip_num, char *data);
	int send_peer(char *data);
	void send_id(unsigned long ip_num, char *id, const char *buf, int len);
	void loc_process(unsigned long ip_num, char *id, char mode, unsigned char vlen, char *value, std::string& ret);

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
	void req_id_register(unsigned long ip_num);
	void req_lookup(unsigned long ip_num);
	void req_id_deregister(unsigned long ip_num);

	//peer
	void peer_bf_update(unsigned long ip_num);
	void peer_lookup(unsigned long ip_num);
	void peer_lookup_down();

	//thread
	void child_run(unsigned int i);
	void peer_run(unsigned int i);
	void stat_run();
	void refresh_run();
	void cmd_run();

	//test
	void test_process();

private:
	static leveldb::DB* db;
	static leveldb::Options options;


	static Bloomfilter *my_filter;
	static Bloomfilter **child_filter;
	static Bloomfilter **peer_filter;
	static Bloomfilter *shadow_filter;

	unsigned char **cuda_child_filter;
	unsigned char **cuda_peer_filter;
	unsigned char *cuda_ans;
	unsigned char *ans;

	static Scanner sc;

	static std::atomic<int> global_counter;
	static std::atomic<int> global_switch;
	std::thread stat;
	std::thread refresh;
	std::thread cmd;

	static std::atomic<int> protocol[100];

	static long long *bitArray;
	static int sock;
	static struct sockaddr_in serv_adr, ref_adr;
};

#endif
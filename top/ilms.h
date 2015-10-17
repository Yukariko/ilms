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
#define ID_SIZE 24
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
	static void send_node(unsigned int ip_num,const char *buf,int len);
	void send_refresh(unsigned int ip_num, unsigned char *filter);
	int send_child(char *id);
	int send_child(unsigned int ip_num, char *id);
	int send_peer(char *id);
	int send_top(char *id);
	void send_id(unsigned int ip_num,char *id, const char *buf, int len);

	void loc_process(unsigned int ip_num, char *id, char mode, unsigned char vlen, char *value, std::string& ret);

	//id
	void reset(Bloomfilter *filter);
	void insert(char *key,int klen,const char *val,int vlen);
	bool search(const char *key,int klen,std::string &val);
	bool remove(char *key,int klen);

	//process
	void proc_bf_update(unsigned int ip_num);
	void proc_lookup(unsigned int ip_num);
	void proc_lookup_nack();

	//request
	void req_id_register(unsigned int ip_num);
	void req_lookup(unsigned int ip_num);
	void req_id_deregister(unsigned int ip_num);

	//peer
	void peer_bf_update(unsigned int ip_num);
	void peer_lookup(unsigned int ip_num);

	//top
	void top_bf_update(unsigned int ip_num);
	void top_lookup();

	//thread
	void stat_run();
	void refresh_run();
	void cmd_run();

	//test
	void test_process();
	void print_log(const char *id, const char *mode, const char *state, unsigned char vlen = 0, const char *value = NULL);
	
private:

	static leveldb::DB* db;
	static leveldb::Options options;


	static Bloomfilter *my_filter;
	static Bloomfilter **child_filter;
	static Bloomfilter **top_filter;
	static Bloomfilter **peer_filter;
	static Bloomfilter *shadow_filter;

	static Scanner sc;

	static std::atomic<int> global_counter;
	static std::atomic<int> global_switch;
	std::thread stat;
	std::thread refresh;
	std::thread cmd;

	static std::atomic<int> protocol[100];

	static long long bitArray[12];
	static int sock;
	static struct sockaddr_in serv_adr, ref_adr;
};

#endif
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

#include "address.h"
#include "bloomfilter.h"
#include "topology.h"
#include "scanner.h"

// 설정 값

#define BUF_SIZE 256

// ilms 노드가 사용할 포트 (UDP)
#define PORT 7979

// filter refresh에 사용할 포트 (TCP)
#define REFRESH_PORT 7980

// CPU 쓰레드 수 (지금은 사용되지 않음)
#define NTHREAD 64U

// ID binary의 길이
#define ID_SIZE 24

// mdb 데이터 저장 장소
#define DB_PATH "./db"

// refresh 주기 (초)
#define REFRESH_FREQUENCY 60

// 가상필터에 등록할 아이디 수
#define VIRTUAL_FILTER_ID_NUM 1000

// 프로토콜 통계 표시 주기 (초)
#define STAT_FREQUENCY 60

class Ilms : public Tree
{
public:
	Ilms();
	~Ilms();

	// ILMS의 통신을 담당
	void start();

	// 해당 노드에게 버퍼 전송
	static void send_node(unsigned int ip_num,const char *buf,int len);

	// 해당 노드에게 필터 전송
	void send_refresh(unsigned int ip_num, unsigned char *filter);

	// 블룸필터에 ID가 등록되어 있는 child노드에게 버퍼 전송
	// 전송한 child노드 수 반환
	int send_child(char *data);

	// 블룸필터에 ID가 등록되어 있는 child노드 중, ip가 주어진것과 다른 노드에게 버퍼 전송
	// 전송한 child노드 수 반환
	int send_child(unsigned int ip_num, char *data);

	// 블룸필터에 ID가 등록되어 있는 peer노드에게 버퍼 전송
	// 전송한 peer노드 수 반환
	int send_peer(char *data);

	// 클라이언트에게 버퍼 전송
	void send_id(unsigned int ip_num, char *id, const char *buf, int len);

	// location 요청에 따른 처리
	void loc_process(unsigned int ip_num, char *id, char mode, unsigned char vlen, char *value, std::string& ret);

	// mdb에 존재하는 ID를 필터에 다시 등록
	void reset(Bloomfilter *filter);

	// mdb에 데이터 등록
	void insert(const char *key,int klen,const char *val,int vlen);

	// mdb에 데이터 검색
	// 검색 성공시 true, 실패시 false
	bool search(const char *key,int klen,std::string &val);

	// mdb에 데이터 삭제
	// 검색 성공시 true, 실패시 false
	bool remove(const char *key,int klen);

	//프로토콜 별 처리

	//process
	void proc_bf_update(unsigned int ip_num);
	void proc_lookup(unsigned int ip_num);
	void proc_lookup_nack();
	void proc_lookup_down();

	//request
	void req_id_register(unsigned int ip_num);
	void req_lookup(unsigned int ip_num);
	void req_id_deregister(unsigned int ip_num);

	//peer
	void peer_bf_update(unsigned int ip_num);
	void peer_lookup(unsigned int ip_num);
	void peer_lookup_down();

	//thread
	void child_run(unsigned int i);
	void peer_run(unsigned int i);

	// 통계 출력 thread
	void stat_run();
	// refresh thread
	void refresh_run();
	// 터미널 명령처리 thread
	void cmd_run();

	//test
	
	// 가상 필터에 임의의 데이터 등록
	void test_process();
	// 프로토콜에 따른 메세지 출력
	void print_log(const char *id, const char *mode, const char *state, unsigned char vlen = 0, const char *value = NULL);

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

	static IDPAddress eid;
};

#endif
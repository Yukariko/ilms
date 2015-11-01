#ifndef ILMSCLI_H
#define ILMSCLI_H

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <iostream>
#include "address.h"

// 노드에 연결할 포트
#define PORT 7979
// 클라이언트에 사용할 포트
#define MYPORT 7979
#define BUF_SIZE 256
// ID binary 길이
#define ID_SIZE 24
// 응답을 기다릴 시간제한 (초)
#define TIME_LIMIT	5
// 시간초과시 재전송 횟수
#define RETRANSMISSION_FREQUENCY 5

#define LOC_LOOKUP						0x00
#define LOC_SET								0x01
#define LOC_SUB								0x02
#define LOC_REP								0X03


using namespace std;

class IlmsCli 
{
public:
	IlmsCli(string ip);

	// ip 설정
	void set_ip(const string& ip);

	// 각 프로토콜에 대한 처리
	bool req_id_register(const string& id, const string& loc);
	int req_loc_update(char mode, const string& id, const string& loc);
	bool req_id_deregister(const string& id);
	int req_lookup(const string& id);

	// 패킷 전달
	void send(const char *buf,int len);

	// 응답 대기
	int recieve(char *buf);

	// 패킷에 대한 메세지 출력
	int print_response(char *buf, int len);
	void error_handling(const char *message);

private:
	int sock;
	struct sockaddr_in serv_adr;
	string ip;
	IDPAddress eid; 
};


#endif
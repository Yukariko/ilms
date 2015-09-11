#include <cstring>

#include "ilmscli.h"

#define REQ_ID_REGISTER				0x20
#define REQ_LOC_UPDATE				0x21
#define REQ_LOOKUP						0x22
#define REQ_ID_DEREGISTER			0x23
#define REQ_SUCCESS						0x24
#define REQ_FAIL							0x25

/*
 * Ilms 클라이언트 생성자
 * 요청을 전달할 노드의 ip를 설정하고 자신의 소켓 초기화
 */

IlmsCli::IlmsCli(string ip)
{
	this->ip = ip;

	sock = socket(PF_INET, SOCK_DGRAM, 0);
	if(sock == -1)
		error_handling("UDP socket creation error");

	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(MYPORT);

	struct timeval tv;
	tv.tv_sec = TIME_LIMIT;
	tv.tv_usec = 0;

	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

	if(bind(sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
		error_handling("bind() error");

}

/*
 * ip설정. 다른노드로 이동할 때 쓰임
 */

void IlmsCli::set_ip(const string& ip)
{
	this->ip = ip;
}

bool IlmsCli::req_id_register(const string& id, const string& loc)
{
	char header[BUF_SIZE];
	int len=0;

	header[len++] = REQ_ID_REGISTER;
	for(size_t i=0; i < ID_SIZE; i++)
	{
		if(i < id.size())
			header[len++] = id[i];
		else
			header[len++] = 0;
	}

	header[len] = loc.length() + 1;
	strcpy(header+len+1, loc.c_str());

	len += header[len] + 1;

	this->send(header,len);
	this->recieve(header);
	return header[0] == REQ_SUCCESS;
}


/*
 * 데이터 추가 요청
 */

bool IlmsCli::req_loc_update(char mode, const string& id, const string& loc)
{
	char header[BUF_SIZE];
	int len=0;

	header[len++] = REQ_LOC_UPDATE;
	for(size_t i=0; i < ID_SIZE; i++)
	{
		if(i < id.size())
			header[len++] = id[i];
		else
			header[len++] = 0;
	}
	header[len++] = mode;
	header[len] = loc.length() + 1;
	strcpy(header+len+1, loc.c_str());

	len += header[len] + 1;

	this->send(header,len);
	this->recieve(header);
	return header[0] == REQ_SUCCESS;
}

/*
 * 데이터 검색 요청
 * 검색 결과를 받을때까지 기다림
 */

int IlmsCli::req_lookup(const string& id, string& buf)
{
	char header[BUF_SIZE];
	int len=0;

	header[len++] = REQ_LOOKUP;
	for(size_t i=0; i < ID_SIZE; i++)
	{
		if(i < id.size())
			header[len++] = id[i];
		else
			header[len++] = 0;
	}

	header[len++] = LOC_LOOKUP;
	header[len++] = 0;


	this->send(header,len);
	len = this->recieve(header);
	if(len < 0 || header[0] != REQ_SUCCESS)
		return -1;

	header[len] = 0;
	buf = header + ID_SIZE + 3;
	return buf.length();
}

/*
 * 데이터 삭제 요청
 */

bool IlmsCli::req_id_deregister(const string& id)
{
	char header[BUF_SIZE];
	int len=0;

	header[len++] = REQ_ID_DEREGISTER;
	for(size_t i=0; i < ID_SIZE; i++)
	{
		if(i < id.size())
			header[len++] = id[i];
		else
			header[len++] = 0;
	}

	this->send(header,len);
	return this->recieve(header) == len;
}

/*
 * 패킷 전송 함수
 */

void IlmsCli::send(const char *buf,int len)
{
	struct sockaddr_in clnt_adr;
	socklen_t clnt_adr_sz = sizeof(clnt_adr);

	memset(&clnt_adr,0,sizeof(clnt_adr));
	clnt_adr.sin_family = AF_INET;
	clnt_adr.sin_addr.s_addr = inet_addr(ip.c_str());
	clnt_adr.sin_port = htons(PORT);

	sendto(sock, buf, len, 0, (struct sockaddr *)&clnt_adr, clnt_adr_sz);
}

/*
 * 패킷을 받아오는 함수
 */

int IlmsCli::recieve(char *buf)
{
	struct sockaddr_in clnt_adr;
	socklen_t clnt_adr_sz = sizeof(clnt_adr);

	return recvfrom(sock, buf, BUF_SIZE, 0,(struct sockaddr*)&clnt_adr, &clnt_adr_sz);
}

/*
 * 에러 처리 함수
 */

void IlmsCli::error_handling(const char *message)
{
	fputs(message,stderr);
	fputc('\n',stderr);
	exit(1);
}
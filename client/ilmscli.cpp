#include <cstring>

#include "ilmscli.h"

#define REQ_ID_REGISTER					0x20
#define REQ_LOC_UPDATE						0x21
#define REQ_LOOKUP						0x22
#define REQ_ID_DEREGISTER						0x23


/*
 * Ilms 클라이언트 생성자
 * 요청을 전달할 노드의 ip를 설정하고 자신의 소켓 초기화
 */

IlmsCli::IlmsCli(std::string ip)
{
	this->ip = ip;

	sock = socket(PF_INET, SOCK_DGRAM, 0);
	if(sock == -1)
		error_handling("UDP socket creation error");

	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(MYPORT);

	if(bind(sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
		error_handling("bind() error");

}

/*
 * ip설정. 다른노드로 이동할 때 쓰임
 */

void IlmsCli::setIp(std::string ip)
{
	this->ip = ip;
}

void IlmsCli::req_id_register(char *data,std::string ip)
{
	char header[256];
	int len=0;

	header[len++] = REQ_ID_REGISTER;
	for(int i=0; i < 24; i++)
		header[len++] = data[i];
	
	header[len] = ip.length() + 1;
	strcpy(header+len+1,ip.c_str());

	len += header[len] + 1;

	this->send(header,len);
}


/*
 * 데이터 추가 요청
 */

void IlmsCli::req_loc_update(char mode, char *data,std::string ip)
{
	char header[256];
	int len=0;

	header[len++] = REQ_LOC_UPDATE;
	header[len++] = mode;
	for(int i=0; i < 24; i++)
		header[len++] = data[i];
	
	header[len] = ip.length() + 1;
	strcpy(header+len+1,ip.c_str());

	len += header[len] + 1;

	this->send(header,len);
}

/*
 * 데이터 검색 요청
 * 검색 결과를 받을때까지 기다림 
 */

int IlmsCli::req_lookup(char *data, char *buf)
{
	char header[BUF_SIZE];
	int len=0;
	
	header[len++] = REQ_LOOKUP;
	for(int i=0; i < 24; i++)
		header[len++] = data[i];
	

	this->send(header,len);
	int len = this->recieve(buf);
	memcpy(buf, buf + DATA_SIZE + 1, len - DATA_SIZE - 1);
	return len - DATA_SIZE - 1;
}

/*
 * 데이터 삭제 요청
 */

void IlmsCli::req_id_deregister(char *data)
{
	char header[BUF_SIZE];
	int len=0;

	header[len++] = REQ_ID_DEREGISTER;
	for(int i=0; i < 24; i++)
		header[len++] = data[i];


	this->send(header,len);
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

	std::cout << "Send OK!" << std::endl;
}

/*
 * 패킷을 받아오는 함수
 */

int IlmsCli::recieve(char *buf)
{
	struct sockaddr_in clnt_adr;
	socklen_t clnt_adr_sz = sizeof(clnt_adr);

	std::cout << "Recieve OK!" << std::endl;

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
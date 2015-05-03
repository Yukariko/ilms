#include <cstring>

#include "ilmscli.h"

#define REQ_DATA_ADD							0x20
#define REQ_DATA_SEARCH						0x21
#define REQ_DATA_DELETE						0x22


IlmsCli::IlmsCli(std::string ip)
{
	this->ip = ip;

	sock = socket(PF_INET, SOCK_DGRAM, 0);
	if(sock == -1)
		error_handling("UDP socket creation error");

	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(PORT);

	if(bind(sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
		error_handling("bind() error");

}

void IlmsCli::setIp(std::string ip)
{
	this->ip = ip;
}

void IlmsCli::req_data_add(long long data,std::string ip)
{
	char buf[256];
	int len=0;

	buf[len++] = REQ_DATA_ADD;
	*(long long *)buf[len] = data;
	len += 8;

	buf[len] = ip.length() + 1;
	strcpy(buf+len+1,ip.c_str());

	len += buf[len] + 1;

	this->send(buf,len);
}

int IlmsCli::req_data_search(long long data, char *buf)
{
	char header[BUF_SIZE];
	int len=0;
	
	header[len++] = REQ_DATA_SEARCH;
	
	*(long long *)header[len] = data;
	len += 8;

	this->send(header,len);
	return this->recieve(buf);
}

void IlmsCli::req_data_delete(long long data)
{
	char buf[BUF_SIZE];
	int len=0;

	buf[len++] = REQ_DATA_DELETE;

	*(long long *)buf[len] = data;
	len += 8;

	this->send(buf,len);
}

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

int IlmsCli::recieve(char *buf)
{
	struct sockaddr_in clnt_adr;
	socklen_t clnt_adr_sz = sizeof(clnt_adr);

	return recvfrom(sock, buf, BUF_SIZE, 0,(struct sockaddr*)&clnt_adr, &clnt_adr_sz);
}

void IlmsCli::error_handling(const char *message)
{
	fputs(message,stderr);
	fputc('\n',stderr);
	exit(1);
}
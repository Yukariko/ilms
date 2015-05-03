#include <cstring>

#include "ilmscli.h"

#define REQ_DATA_ADD							0x20
#define REQ_DATA_SEARCH						0x21
#define REQ_DATA_DELETE						0x22


IlmsCli::IlmsCli(std::string ip)
{
	this->ip = ip;
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

void IlmsCli::req_data_search(long long data)
{
	char buf[256];
	int len=0;
	
	buf[len++] = REQ_DATA_SEARCH;
	
	*(long long *)buf[len] = data;
	len += 8;

	this->send(buf,len);
}

void IlmsCli::req_data_delete(long long data)
{
	char buf[256];
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
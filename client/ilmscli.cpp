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
	serv_adr.sin_port = htons(MYPORT);

	if(bind(sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
		error_handling("bind() error");

}

void IlmsCli::setIp(std::string ip)
{
	this->ip = ip;
}

void IlmsCli::req_data_add(long long data,std::string ip)
{
	char header[256];
	int len=0;

	header[len++] = REQ_DATA_ADD;
	*(long long *)(header+len) = data;
	len += 8;

	header[len] = ip.length() + 1;
	strcpy(header+len+1,ip.c_str());

	len += header[len] + 1;

	this->send(header,len);
}

int IlmsCli::req_data_search(long long data, char *buf)
{
	char header[BUF_SIZE];
	int len=0;
	
	header[len++] = REQ_DATA_SEARCH;
	
	*(long long *)(header+len) = data;
	len += 8;

	this->send(header,len);
	return this->recieve(buf);
}

void IlmsCli::req_data_delete(long long data)
{
	char header[BUF_SIZE];
	int len=0;

	header[len++] = REQ_DATA_DELETE;

	*(long long *)(header+len) = data;
	len += 8;

	this->send(header,len);
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

	std::cout << "Send OK!" << std::endl;
}

int IlmsCli::recieve(char *buf)
{
	struct sockaddr_in clnt_adr;
	socklen_t clnt_adr_sz = sizeof(clnt_adr);

	std::cout << "Recieve OK!" << std::endl;

	return recvfrom(sock, buf, BUF_SIZE, 0,(struct sockaddr*)&clnt_adr, &clnt_adr_sz);
}

void IlmsCli::error_handling(const char *message)
{
	fputs(message,stderr);
	fputc('\n',stderr);
	exit(1);
}
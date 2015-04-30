#include <cstring>
#include <cstdio>
#include <cstdlib>
#include "ilms.h"

#define CMD_BF_ADD								0x00
#define CMD_DATA_ADD							0x10
#define CMD_DATA_SEARCH						0x11
#define CMD_DATA_SEARCH_FAIL			0x12
#define CMD_DATA_DELETE						0x13
#define CMD_DATA_REPLACE					0x14

/*
 * 테스트 해시 함수
 */

long long test(long long data){return data*997*1499;}
long long test2(long long data){return data*1009*1361;}
long long test3(long long data){return data*1013*1327;}
long long test4(long long data){return data*1013*1327;}
long long test5(long long data){return data*1013*1327;}
long long test6(long long data){return data*1013*1327;}
long long test7(long long data){return data*1013*1327;}
long long test8(long long data){return data*1013*1327;}
long long test9(long long data){return data*1013*1327;}
long long test10(long long data){return data*1013*1327;}
long long test11(long long data){return data*1013*1327;}


/*
 * Ilms 생성자
 * 설정들을 불러오고
 * 블룸필터를 초기화 해줌
 * 해시 함수 정의 필요
 */

const long long defaultSize = 8LL * 1024 * 1024;

Ilms::Ilms()
{
	// bloomfilter init
	long long (*hash[11])(long long) = {test,test2,test3,test4,test5,test6,test7,test8,test9,test10,test11};
	myFilter = new Bloomfilter(defaultSize, 11, hash);
	childFilter = new Bloomfilter(defaultSize, 11, hash);

	// socket init
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

/*
 * Ilms 소멸자
 * 할당 해제
 */

Ilms::~Ilms()
{
	delete myFilter;
	delete childFilter;

	close(sock);
}

void Ilms::start()
{
	char buf[BUF_SIZE];

	while(1)
	{
		clnt_adr_sz = sizeof(clnt_adr);
		int len = recvfrom(serv_sock, buf, BUF_SIZE, 0,
					(struct sockaddr*)&clnt_adr, &clnt_adr_sz);

		if(len > 0)
		{
			char cmd = buf[0];
			switch(cmd)
			{
			case CMD_BF_ADD: proc_bf_add(buf,len); break;
			case CMD_DATA_ADD: proc_data_add(buf,len); break;
			case CMD_DATA_SEARCH: proc_data_search(buf,len); break;

			}
		}
	}
}


void Ilms::error_handling(char *message)
{
	fputs(message,stderr);
	fputc('\n',stderr);
	exit(1);
}

void Ilms::send(const char *ip,char *buf,int len)
{
	struct sockaddr_in clnt_adr;
	socklen_t clnt_adr_sz = sizeof(clnt_adr);

	memset(&sockaddr_in,0,sizeof(clnt_adr));
	clnt_adr.sin_family = AF_INET;
	clnt_adr.sin_addr.s_addr = inet_addr(ip);
	clnt_adr.sin_port = htons(PORT);

	sendto(sock, buf, len, 0, (struct sockaddr *)&clnt_adr, &clnt_adr_sz);
}

void Ilms::proc_bf_add(char *buf, int len)
{
	long long data = *(long long *)(buf+1);
	childFilter.insert(data);
	this->send(tree->getParent().getIp(), buf, len);
}

void Ilms::proc_data_add(char *buf, int len)
{
	long long data = *(long long *)(buf+1);
	myFilter.insert(data);
	insert(data);

	buf[0] = CMD_BF_ADD;
	this->send(tree->getParent().getIp(), buf, len);
}

void Ilms::proc_data_search(char *buf, int len)
{
	long long data = *(long long *)(buf+1);
	char *ip = buf+9;
	if(myFilter.lookup(data))
	{
		int rlen = search(data);

		if(rlen > 0)
		{

			return;
		}
	}

	if(child.size() == 0)
	{
		buf[0] = CMD_DATA_SEARCH_FAIL;
		//~~

		
	}

	if(childFilter.lookup(data))
	{
		for(int i=0;i<child.size();i++)
		{
			if(strcmp(ip,child[i].getIp()))
				this->send(child[i].getIp(), buf, len);
		}
	}
	else
	{
		this->send(parent->getIp(), buf, len);
	}
}

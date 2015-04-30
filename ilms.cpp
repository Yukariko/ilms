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

#define MARK_SEARCH_FAIL					0x8000000000000000
#define MARK_UP										0x01
#define MARK_DOWN									0x00


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
 * 소켓의 초기화
 * UDP 통신이며 포트는 7979
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

/*
 * 서버 동작 시작
 * 입력을 받아와 명령에 따라 동작 수행
 */

void Ilms::start()
{
	char buf[BUF_SIZE];

	struct sockaddr_in clnt_adr; 
	socklen_t clnt_adr_sz;

	while(1)
	{
		clnt_adr_sz = sizeof(clnt_adr);
		int len = recvfrom(sock, buf, BUF_SIZE, 0,
					(struct sockaddr*)&clnt_adr, &clnt_adr_sz);

		if(len > 0)
		{
			sc = Scanner(buf, len);

			char cmd;
			if(!sc.next_value(cmd))
				continue;

			switch(cmd)
			{
			case CMD_BF_ADD: proc_bf_add(); break;
			case CMD_DATA_ADD: proc_data_add(); break;
			case CMD_DATA_SEARCH: proc_data_search(); break;
			case CMD_DATA_SEARCH_FAIL: proc_data_search_fail(); break;
			}
		}
	}
}

/*
 * 오류가 발생시 해당 오류를 출력하고 종료
 */

void Ilms::error_handling(const char *message)
{
	fputs(message,stderr);
	fputc('\n',stderr);
	exit(1);
}

/*
 * 버퍼 전송
 */

void Ilms::send(const char *ip,char *buf,int len)
{
	struct sockaddr_in clnt_adr;
	socklen_t clnt_adr_sz = sizeof(clnt_adr);

	memset(&clnt_adr,0,sizeof(clnt_adr));
	clnt_adr.sin_family = AF_INET;
	clnt_adr.sin_addr.s_addr = inet_addr(ip);
	clnt_adr.sin_port = htons(PORT);

	sendto(sock, buf, len, 0, (struct sockaddr *)&clnt_adr, clnt_adr_sz);
}

/*
 * 블룸필터 데이터 추가
 * 부모노드에도 추가해야 하므로 버퍼그대로 전송
 */

void Ilms::proc_bf_add()
{
	long long data;
	if(!sc.next_value(data))
		return;
	childFilter->insert(data);
	this->send(parent->getIp(), sc.buf, sc.len);
}

/*
 * 데이터 추가. 블룸필터에도 추가해야함
 * 처음 노드일 경우만 해당하며, 부모에는 블룸필터 추가로 전송
 */

void Ilms::proc_data_add()
{
	long long data;
	long long str;
	if(!sc.next_value(data))
		return;

	if(!sc.next_value(str))
		return;

	myFilter->insert(data);
	insert(data,str);

	sc.buf[0] = CMD_BF_ADD;
	this->send(parent->getIp(), sc.buf, sc.len);
}

/*
 * 데이터 검색. 다음의 순서로 동작
 * 1) 자신의 필터에 데이터가 있다면 자신의 DB에서 검색. 검색 결과가 있다면 처음 노드에 직접 전송
 * 2) 없다면 자식 필터를 확인하고, 있다면 자식들에 대해 검색 전송
 * 3) 자식필터에 데이터가 없다면 부모노드로 올라감. 부모노드에서 내려온 상태라면 부모에 검색 실패 전송 
 */

void Ilms::proc_data_search()
{
	long long data;
	char *ip;
	char *ip_org;

	if(!sc.next_value(data))
		return;
	if(!sc.next_value(ip))
		return;
	if(!sc.next_value(ip_org))
		return;
	
	char &up_down = *sc.get_cur();

	if(myFilter->lookup(data))
	{
		char res[BUF_SIZE];
		int rlen = search(data,res,BUF_SIZE);

		if(rlen > 0)
		{
			this->send(ip_org, res, rlen);
			return;
		}
	}

	if(childFilter->lookup(data))
	{
		insert(data | MARK_SEARCH_FAIL, child.size() - (up_down == MARK_UP));

		up_down = MARK_DOWN;

		for(int i=0;i<child.size();i++)
		{
			if(strcmp(ip,child[i].getIp()))
				this->send(child[i].getIp(), sc.buf, sc.len);
		}
	}
	else
	{
		if(up_down == MARK_DOWN)
		{
			sc.buf[0] = CMD_DATA_SEARCH_FAIL;
			sc.buf[sc.len++] = CMD_DATA_SEARCH_FAIL;
		}

		up_down = MARK_UP;
		this->send(parent->getIp(), sc.buf, sc.len);
	}
}

/*
 * 데이터 검색 실패. 노드에 false positive가 존재하는 상황
 * 다른 노드들에 대해
 */

void Ilms::proc_data_search_fail()
{
	long long data;
	if(!sc.next_value(data))
		return;

	long long fail_data = data | MARK_SEARCH_FAIL;

	char res[BUF_SIZE];

	int rlen = search(fail_data,res,BUF_SIZE);
	if(rlen == 0)
		return;

	Scanner rsc = Scanner(res,rlen);

	long long count;

	rsc.next_value(count);

	if(--count == 0)
	{
		remove(fail_data);

		sc.buf[0] = sc.buf[--sc.len];

		this->send(parent->getIp(), sc.buf, sc.len);
		return;
	}

	insert(fail_data, count);
}

/*
 * 데이터 삭제
 * 데이터 검색 매커니즘에서 검색후 알림 대신 삭제
 */

void Ilms::proc_data_delete()
{
	long long data;
	if(!sc.next_value(data))
		return;

	char &up_down = *sc.get_cur();

	if(myFilter->lookup(data))
	{
		if(remove(data))
			return;
	}

	if(childFilter->lookup(data))
	{
		insert(data | MARK_SEARCH_FAIL, child.size() - (up_down == MARK_UP));

		up_down = MARK_DOWN;

		for(int i=0;i<child.size();i++)
		{
			if(strcmp(ip,child[i].getIp()))
				this->send(child[i].getIp(), sc.buf, sc.len);
		}
	}
	else
	{
		if(up_down == MARK_DOWN)
		{
			sc.buf[0] = CMD_DATA_SEARCH_FAIL;
			sc.buf[sc.len++] = CMD_DATA_DELETE;
		}
		up_down = MARK_UP;
		this->send(parent->getIp(), sc.buf, sc.len);
	}
}


void Ilms::insert(long long key, long long value)
{

}
int search(long long key,char *buf, int len)
{
	return 1;
}
bool remove(long long key)
{
	return true;
}
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

	// db init
	options.create_if_missing = true;
	leveldb::Status status = leveldb::DB::Open(options, DB_PATH, &db);
	assert(status.ok());

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
 * 버퍼 전송
 */

void Ilms::send(const char *ip,const char *buf,int len)
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
	char *data;
	if(!sc.next_value(data,8))
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
	char *data;
	char *value;
	if(!sc.next_value(data,8))
		return;

	if(!sc.next_value(value))
		return;

	myFilter->insert(data);

	insert(data,8,value,*(unsigned char *)(value-1));

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
	char *data;
	char *ip;
	char *ip_org;

	if(!sc.next_value(data,8))
		return;

	unsigned char ip_len = *(unsigned char *)sc.get_cur();

	if(!sc.next_value(ip))
		return;
	if(!sc.next_value(ip_org))
		return;
	
	char &up_down = *sc.get_cur();

	if(myFilter->lookup(data))
	{
		std::string ret;
		if(search(data,8,ret))
		{
			this->send(ip_org, ret.c_str(), ret.length());
			return;
		}
	}

	strcpy(ip, me->getIp());

	if(childFilter->lookup(data))
	{
		unsigned char count = child.size() - (up_down == MARK_UP);
		insert(data,8+ip_len+1, (char *)&count, 1);

		up_down = MARK_DOWN;

		for(unsigned int i=0;i<child.size();i++)
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
	char *data;
	if(!sc.next_value(data,8))
		return;

	unsigned char ip_len = *(unsigned char *)sc.get_cur();

	std::string ret;

	if(!search(data,8+ip_len+1,ret))
		return;

	unsigned char count = *(unsigned char *)ret.c_str();

	if(--count == 0)
	{
		remove(data,8+ip_len+1);

		sc.buf[0] = sc.buf[--sc.len];

		this->send(parent->getIp(), sc.buf, sc.len);
		return;
	}

	insert(data,8+ip_len+1, (char *)&count, 1);
}

/*
 * 데이터 삭제
 * 데이터 검색 매커니즘에서 검색후 알림 대신 삭제
 */

void Ilms::proc_data_delete()
{
	char *data;
	char *ip;

	if(!sc.next_value(data,8))
		return;

	unsigned char ip_len = *(unsigned char *)sc.get_cur();

	if(!sc.next_value(ip))
		return;

	char &up_down = *sc.get_cur();

	if(myFilter->lookup(data))
	{
		if(remove(data,8))
			return;
	}

	strcpy(ip, me->getIp());

	if(childFilter->lookup(data))
	{
		unsigned char count = child.size() - (up_down == MARK_UP);
		insert(data,8+ip_len+1, (char *)&count, 1);

		up_down = MARK_DOWN;

		for(unsigned int i=0;i<child.size();i++)
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

/*
 * DB에 데이터 넣기
 * key, value 쌍으로 입력
 */

void Ilms::insert(char *key,int klen, char *val,int vlen)
{
	leveldb::Status s = db->Put(leveldb::WriteOptions(),leveldb::Slice(key,klen),leveldb::Slice(val,vlen));
	assert(s.ok());
}

/*
 * DB에 데이터 검색
 */

bool Ilms::search(char *key,int klen,std::string &val)
{
	leveldb::Status s = db->Get(leveldb::ReadOptions(),leveldb::Slice(key,klen),&val);
	return s.ok();
}

/*
 * DB에 데이터 삭제
 * 존재여부 확인을 위해 한번 검색함(효율 문제)
 */

bool Ilms::remove(char *key,int klen)
{
	std::string ret;
	if(!search(key,klen,ret))
		return false;

	leveldb::Status s = db->Delete(leveldb::WriteOptions(),leveldb::Slice(key,klen));
	assert(s.ok());
	return true;
}
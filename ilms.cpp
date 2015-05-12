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

#define REQ_DATA_ADD							0x20
#define REQ_DATA_SEARCH						0x21
#define REQ_DATA_DELETE						0x22

#define MARK_UP										0x01
#define MARK_DOWN									0x00


//#define MODE 1
#ifdef MODE
#define DEBUG(s) (std::cout << s << std::endl)
#endif

#ifndef MODE
#define DEBUG(s) 0
#endif

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

const long long defaultSize = 8LL * 2 * 1024 * 1024;

Ilms::Ilms()
{
	// bloomfilter init
	long long (*hash[11])(long long) = {test,test2,test3,test4,test5,test6,test7,test8,test9,test10,test11};
	
	myFilter = new Bloomfilter(defaultSize, 11, hash);

	if(child.size())
	{
		childFilter = new Bloomfilter*[child.size()];
		for(unsigned int i=0; i < child.size(); i++)
			childFilter[i] = new Bloomfilter(defaultSize, 11, hash);
	}
	
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
	delete[] childFilter;
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

		DEBUG("Recieve OK!");

		if(len > 0)
		{
			sc = Scanner(buf, len);

			char cmd;
			if(!sc.next_value(cmd))
				continue;

			DEBUG("Protocol OK!");

			int ip_num = inet_addr(inet_ntoa(clnt_adr.sin_addr));

			switch(cmd)
			{
			case CMD_BF_ADD: proc_bf_add(ip_num); break;
			case CMD_DATA_ADD: proc_data_add(); break;
			case CMD_DATA_SEARCH: proc_data_search(ip_num); break;
			case CMD_DATA_SEARCH_FAIL: proc_data_search_fail(); break;
			case CMD_DATA_DELETE: proc_data_delete(ip_num); break;

			case REQ_DATA_ADD: req_data_add(); break;
			case REQ_DATA_SEARCH: req_data_search(ip_num); break;
			case REQ_DATA_DELETE: req_data_delete(ip_num); break;
			}
		}
	}
}

/*
 * 버퍼 전송
 */

void Ilms::send(int ip_num,const char *buf,int len)
{
	if(ip_num==0)
		return;
	struct sockaddr_in clnt_adr;
	socklen_t clnt_adr_sz = sizeof(clnt_adr);

	memset(&clnt_adr,0,sizeof(clnt_adr));
	clnt_adr.sin_family = AF_INET;
	clnt_adr.sin_addr.s_addr = ip_num;
	clnt_adr.sin_port = htons(PORT);

	sendto(sock, buf, len, 0, (struct sockaddr *)&clnt_adr, clnt_adr_sz);

	DEBUG("Send OK!");
}

/*
 * 블룸필터 데이터 추가
 * 부모노드에도 추가해야 하므로 버퍼그대로 전송
 */

void Ilms::proc_bf_add(int ip_num)
{
	char *data;
	if(!sc.next_value(data,8))
		return;

	for(unsigned int i=0; i < child.size(); i++)
	{
		if(ip_num == child[i].get_ip_num())
		{
			childFilter[i]->insert(data);
			break;
		}
	}

	this->send(parent->get_ip_num(), sc.buf, sc.len);
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

	DEBUG("PROC_DATA_ADD OK!");

	myFilter->insert(data);

	DEBUG("BLOOMFILTER OK!");

	insert(data,8,value,*(unsigned char *)(value-1));

	DEBUG("DATA_INSERT OK!");

	sc.buf[0] = CMD_BF_ADD;
	this->send(parent->get_ip_num(), sc.buf, sc.len);
}

/*
 * 데이터 검색. 다음의 순서로 동작
 * 1) 자신의 필터에 데이터가 있다면 자신의 DB에서 검색. 검색 결과가 있다면 처음 노드에 직접 전송
 * 2) 없다면 자식 필터를 확인하고, 있다면 자식들에 대해 검색 전송
 * 3) 자식필터에 데이터가 없다면 부모노드로 올라감. 부모노드에서 내려온 상태라면 부모에 검색 실패 전송 
 */

void Ilms::proc_data_search(int ip_num)
{
	char *data;
	int ip_org_num;

	if(!sc.next_value(data,8))
		return;

	if(!sc.next_value(ip_org_num))
		return;

	char &up_down = *sc.get_cur();

	if(myFilter->lookup(data))
	{
		std::string ret;
		if(search(data,8,ret))
		{
			this->send(ip_org_num, ret.c_str(), ret.length());
			return;
		}
	}

	unsigned char count = 0;

	if(up_down == MARK_UP)
	{
		up_down = MARK_DOWN;
		for(unsigned int i=0;i<child.size();i++)
		{
			if(ip_num != child[i].get_ip_num() && childFilter[i]->lookup(data))
			{
				this->send(child[i].get_ip_num(), sc.buf, sc.len);
				count++;
			}
		}
		if(count)
		{
			insert(data,12, (char *)&count, 1);
		}
		else
		{
			up_down = MARK_UP;
			this->send(parent->get_ip_num(), sc.buf, sc.len);
		}
	}
	else
	{
		for(unsigned int i=0;i<child.size();i++)
		{
			if(childFilter[i]->lookup(data))
			{
				this->send(child[i].get_ip_num(), sc.buf, sc.len);
				count++;
			}
		}
		if(count)
		{
			insert(data,12, (char *)&count, 1);
		}
		else
		{
			sc.buf[0] = CMD_DATA_SEARCH_FAIL;
			sc.buf[sc.len++] = CMD_DATA_SEARCH_FAIL;
			up_down = MARK_UP;
			this->send(parent->get_ip_num(), sc.buf, sc.len);
		}
	}
}

/*
 * 데이터 검색 실패. 노드에 false positive가 존재하는 상황
 * 다른 노드들에 대해
 */

void Ilms::proc_data_search_fail()
{
	char *data = sc.get_cur();

	std::string ret;

	if(!search(data,12,ret))
		return;

	unsigned char count = *(unsigned char *)ret.c_str();

	if(--count == 0)
	{
		remove(data,12);

		sc.buf[0] = sc.buf[--sc.len];

		this->send(parent->get_ip_num(), sc.buf, sc.len);
		return;
	}

	insert(data,12, (char *)&count, 1);
}

/*
 * 데이터 삭제
 * 데이터 검색 매커니즘에서 검색후 알림 대신 삭제
 */

void Ilms::proc_data_delete(int ip_num)
{
	char *data;

	if(!sc.next_value(data,8))
		return;

	char &up_down = *sc.get_cur();

	if(myFilter->lookup(data))
	{
		if(remove(data,8))
			return;
	}

	unsigned char count = 0;

	if(up_down == MARK_UP)
	{
		up_down = MARK_DOWN;
		for(unsigned int i=0;i<child.size();i++)
		{
			if(ip_num != child[i].get_ip_num() && childFilter[i]->lookup(data))
			{
				this->send(child[i].get_ip_num(), sc.buf, sc.len);
				count++;
			}
		}
		if(count)
		{
			insert(data,12, (char *)&count, 1);
		}
		else
		{
			up_down = MARK_UP;
			this->send(parent->get_ip_num(), sc.buf, sc.len);
		}
	}
	else
	{
		for(unsigned int i=0;i<child.size();i++)
		{
			if(childFilter[i]->lookup(data))
			{
				this->send(child[i].get_ip_num(), sc.buf, sc.len);
				count++;
			}
		}
		if(count)
		{
			insert(data,12, (char *)&count, 1);
		}
		else
		{
			sc.buf[0] = CMD_DATA_SEARCH_FAIL;
			sc.buf[sc.len++] = CMD_DATA_SEARCH_FAIL;
			up_down = MARK_UP;
			this->send(parent->get_ip_num(), sc.buf, sc.len);
		}
	}
}

/*
 * 클라이언트로 부터의 데이터 추가 요청
 * 사실상 기존 프로토콜과 동일함
 */

void Ilms::req_data_add()
{
	sc.buf[0] = CMD_DATA_ADD;
	proc_data_add();
}

/*
 * 클라이언트로 부터의 데이터 검색 요청
 * 클라이언트는 자식이 아니므로 자식관련 처리과정이 생략됨
 */

void Ilms::req_data_search(int ip_num)
{
	sc.buf[0] = CMD_DATA_SEARCH;
	char *data;

	if(!sc.next_value(data,8))
		return;

	char *pos = sc.get_cur();

	*(int *)pos = ip_num;
	pos += 4;

	char &up_down = *pos;
	pos++;

	sc.len = pos - sc.buf;

	if(myFilter->lookup(data))
	{
		std::string ret;
		if(search(data,8,ret))
		{
			this->send(ip_num, ret.c_str(), ret.length());
			return;
		}
	}

	unsigned char count = 0;

	up_down = MARK_DOWN;

	for(unsigned int i=0;i<child.size();i++)
	{
		if(childFilter[i]->lookup(data))
		{
			this->send(child[i].get_ip_num(), sc.buf, sc.len);
			count++;
		}
	}
	if(count)
	{
		insert(data,12, (char *)&count, 1);
	}
	else
	{
		up_down = MARK_UP;
		this->send(parent->get_ip_num(), sc.buf, sc.len);
	}
}

/*
 * 클라이언트로 부터의 데이터 삭제 요청
 * 클라이언트는 자식이 아니므로 자식관련 처리과정이 생략됨
 */

void Ilms::req_data_delete(int ip_num)
{
	sc.buf[0] = CMD_DATA_DELETE;

	char *data;

	if(!sc.next_value(data,8))
		return;

	char *pos = sc.get_cur();

	*(int *)pos = ip_num;
	pos += 4;

	char &up_down = *pos;
	pos++;

	sc.len = pos - sc.buf;
  

	unsigned char count = 0;

	up_down = MARK_DOWN;

	for(unsigned int i=0;i<child.size();i++)
	{
		if(childFilter[i]->lookup(data))
		{
			this->send(child[i].get_ip_num(), sc.buf, sc.len);
			count++;
		}
	}
	if(count)
	{
		insert(data,12, (char *)&count, 1);
	}
	else
	{
		up_down = MARK_UP;
		this->send(parent->get_ip_num(), sc.buf, sc.len);
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
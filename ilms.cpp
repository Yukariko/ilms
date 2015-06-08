#include <cstring>
#include <cstdio>
#include <cstdlib>
#include "ilms.h"

#define CMD_BF_ADD								0x00
#define CMD_DATA_SEARCH						0x10
#define CMD_DATA_SEARCH_FAIL			0x11
#define CMD_DATA_SEARCH_DOWN			0x12
#define CMD_DATA_REPLACE					0x13

#define REQ_DATA_REGISTER					0x20
#define REQ_DATA_UPDATE						0x21
#define REQ_DATA_SEARCH						0x22
#define REQ_DATA_DELETE						0x23

#define PEER_BF_ADD								0x30
#define PEER_DATA_SEARCH					0x31
#define PEER_DATA_SEARCH_FAIL			0x32
#define PEER_DATA_SEARCH_DOWN			0x33

#define DATA_ADD									0x00
#define DATA_DELETE								0x01
#define DATA_REPLACE							0X02

#define MARK_UP										0x01
#define MARK_DOWN									0x00


#define MODE 1
#ifdef MODE
#define DEBUG(s) (std::cout << s << std::endl)
#endif

#ifndef MODE
#define DEBUG(s) 0
#endif

/*
 * 테스트 해시 함수
 */

const long long defaultSize = 8LL * 2 * 1024 * 1024;

long long test(char *data){return (*(unsigned short *)(data + 0) * 1009LL ) % (defaultSize / 10) + (defaultSize / 10) * 0;}
long long test2(char *data){return (*(unsigned short *)(data + 2) * 1009LL ) % (defaultSize / 10) + (defaultSize / 10) * 1;}
long long test3(char *data){return (*(unsigned short *)(data + 4) * 1009LL ) % (defaultSize / 10) + (defaultSize / 10) * 2;}
long long test4(char *data){return (*(unsigned short *)(data + 6) * 1009LL ) % (defaultSize / 10) + (defaultSize / 10) * 3;;}
long long test5(char *data){return (*(unsigned short *)(data + 8) * 1009LL ) % (defaultSize / 10) + (defaultSize / 10) * 4;;}
long long test6(char *data){return (*(unsigned short *)(data + 10) * 1009LL ) % (defaultSize / 10) + (defaultSize / 10) * 5;;}
long long test7(char *data){return (*(unsigned short *)(data + 12) * 1009LL ) % (defaultSize / 10) + (defaultSize / 10) * 6;;}
long long test8(char *data){return (*(unsigned short *)(data + 14) * 1009LL ) % (defaultSize / 10) + (defaultSize / 10) * 7;;}
long long test9(char *data){return (*(unsigned short *)(data + 16) * 1009LL ) % (defaultSize / 10) + (defaultSize / 10) * 8;;}
long long test10(char *data){return (*(unsigned short *)(data + 18) * 1009LL ) % (defaultSize / 10) + (defaultSize / 10) * 9;}
long long test11(char *data){return (*(unsigned int *)(data + 20) * 1009LL);}

std::vector<Node> Tree::child;
std::vector<Node> Tree::down_peer;
std::vector<Node> Tree::up_peer;

Bloomfilter* Ilms::my_filter;
Bloomfilter** Ilms::child_filter;
Bloomfilter** Ilms::peer_filter;
Scanner Ilms::sc;
std::atomic<int> Ilms::global_counter;
long long Ilms::bitArray[12];
int Ilms::sock;
struct sockaddr_in Ilms::serv_adr;



/*
 * Ilms 생성자
 * 설정들을 불러오고
 * 블룸필터를 초기화 해줌
 * 해시 함수 정의 필요
 * 소켓의 초기화
 * UDP 통신이며 포트는 7979
 */

Ilms::Ilms()
{
	// bloomfilter init
	long long (*hash[11])(char *) = {test,test2,test3,test4,test5,test6,test7,test8,test9,test10,test11};
	
	my_filter = new Bloomfilter(defaultSize, 11, hash);

	if(child.size())
	{
		child_filter = new Bloomfilter*[child.size()];
		for(unsigned int i=0; i < child.size(); i++)
			child_filter[i] = new Bloomfilter(defaultSize, 11, hash);
	}

	if(down_peer.size())
	{
		peer_filter = new Bloomfilter*[down_peer.size()];
		for(unsigned int i=0; i < down_peer.size(); i++)
			peer_filter[i] = new Bloomfilter(defaultSize, 11, hash);	
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

	// test
	test();
}

void Ilms::test()
{
	char data[BUF_SIZE];
	for(unsigned int i=0;i<child.size();i++)
	{
		if(child[i].get_ip_num() == 0)
		{
			for(int j=0;j<1000;j++)
			{
				for(int k=0;k<24;k+=2)
				{
					*(unsigned short *)(data+k) = (unsigned short)rand();
				}
				child_filter[i]->insert(data);
			}
		}
	}
}

/*
 * Ilms 소멸자
 * 할당 해제
 */

Ilms::~Ilms()
{
	delete my_filter;
	if(child.size())
		delete[] child_filter;
	if(down_peer.size())
		delete[] peer_filter;
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

			unsigned long ip_num = clnt_adr.sin_addr.s_addr;

			switch(cmd)
			{
			case CMD_BF_ADD: proc_bf_add(ip_num); break;
			case CMD_DATA_SEARCH: proc_data_search(ip_num); break;
			case CMD_DATA_SEARCH_FAIL: proc_data_search_fail(); break;
			case CMD_DATA_SEARCH_DOWN: proc_data_search_down(); break;

			case REQ_DATA_REGISTER: req_data_register(); break;
			case REQ_DATA_UPDATE: req_data_update(); break;
			case REQ_DATA_SEARCH: req_data_search(ip_num); break;
			case REQ_DATA_DELETE: req_data_delete(ip_num); break;

			case PEER_BF_ADD: peer_bf_add(ip_num); break;
			case PEER_DATA_SEARCH: peer_data_search(ip_num); break;
			case PEER_DATA_SEARCH_DOWN: peer_data_search_down(); break;
			}
		}
	}
}

/*
 * 버퍼 전송
 */

void Ilms::send(unsigned long ip_num,const char *buf,int len)
{
	if(ip_num==0)
		return;

	struct sockaddr_in clnt_adr;
	socklen_t clnt_adr_sz = sizeof(clnt_adr);

	memset(&clnt_adr,0,sizeof(clnt_adr));
	clnt_adr.sin_family = AF_INET;
	clnt_adr.sin_addr.s_addr = ip_num;
	clnt_adr.sin_port = htons(PORT);

	sendto(Ilms::sock, buf, len, 0, (struct sockaddr *)&clnt_adr, clnt_adr_sz);

	DEBUG("Send OK!");
}

void Ilms::child_run(unsigned int i)
{
	if(child_filter[i]->lookBitArray(bitArray))
	{
		Ilms::send(child[i].get_ip_num(), sc.buf, sc.len);
		Ilms::global_counter++;
	}
}

int Ilms::send_child(char *data)
{
	int ret = 0;
	for(unsigned int i=0; i < child.size();)
	{
		unsigned int range = std::min(NTHREAD, (unsigned int)child.size() - i);
		global_counter = 0;

		for(unsigned int j=0; j < range; j++, i++)
		{
			task[j] = std::thread(&Ilms::child_run,this,i);
		}

		for(unsigned int j=0; j < range; j++)
		{
			task[j].join();
		}
		ret += global_counter;
	}
	return ret;
}

int Ilms::send_child(unsigned long ip_num, char *data)
{
	int ret = 0;
	for(unsigned int i=0; i < child.size();)
	{
		unsigned int range = std::min(NTHREAD, (unsigned int)child.size() - i);
		global_counter = 0;

		for(unsigned int j=0; j < range; j++, i++)
		{
			if(ip_num == child[i].get_ip_num())
			{
				j--;
				range--;
				continue;
			}
			task[j] = std::thread(&Ilms::child_run,this,i);
		}

		for(unsigned int j=0; j < range; j++)
		{
			task[j].join();
		}
		ret += global_counter;
	}
	return ret;
}

void Ilms::peer_run(unsigned int i)
{
	if(peer_filter[i]->lookBitArray(bitArray))
	{
		Ilms::send(down_peer[i].get_ip_num(), sc.buf, sc.len);
		Ilms::global_counter++;
	}
}

int Ilms::send_peer(char *data)
{
	int ret = 0;
	for(unsigned int i=0; i < down_peer.size();)
	{
		unsigned int range = std::min(NTHREAD, (unsigned int)down_peer.size() - i);
		global_counter = 0;

		for(unsigned int j=0; j < range; j++, i++)
			task[j] = std::thread(&Ilms::peer_run,this,i);

		for(unsigned int j=0; j < range; j++)
		{
			task[j].join();
		}
		ret += global_counter;
	}
	return ret;
}

/*
 * 블룸필터 데이터 추가
 * 부모노드에도 추가해야 하므로 버퍼그대로 전송
 */

void Ilms::proc_bf_add(unsigned long ip_num)
{
	char *data;
	if(!sc.next_value(data,DATA_SIZE))
		return;

	bool find = false;
	for(unsigned int i=0; i < child.size(); i++)
	{
		if(ip_num == child[i].get_ip_num())
		{
			child_filter[i]->insert(data);
			find = true;
			break;
		}
	}
	if(find)
		this->send(parent->get_ip_num(), sc.buf, sc.len);
}


/*
 * 데이터 검색. 다음의 순서로 동작
 * 1) 자신의 필터에 데이터가 있다면 자신의 DB에서 검색. 검색 결과가 있다면 처음 노드에 직접 전송
 * 2) 없다면 자식 필터를 확인하고, 있다면 자식들에 대해 검색 전송
 * 3) 자식필터에 데이터가 없다면 부모노드로 올라감. 부모노드에서 내려온 상태라면 부모에 검색 실패 전송 
 */

void Ilms::proc_data_search(unsigned long ip_num)
{
	char *data;
	unsigned long ip_org_num;

	if(!sc.next_value(data,DATA_SIZE))
		return;

	if(!sc.next_value(ip_org_num))
		return;

	my_filter->getBitArray(bitArray,data);
	if(my_filter->lookBitArray(bitArray))
	{
		std::string ret;
		if(search(data,DATA_SIZE,ret))
		{
			this->send(ip_org_num, ret.c_str(), ret.length()+1);
			return;
		}
	}

	char *up_down;
	if(!sc.next_value(up_down,1))
		return;

	char *p_depth;
	if(!sc.next_value(p_depth,4))
		return;

	unsigned long depth = ntohl(*(unsigned long *)p_depth);

	int count = 0;

	*(unsigned long *)p_depth = htonl(depth+1);

	if(*up_down == MARK_UP)
	{
		*up_down = MARK_DOWN;
		count += send_child(ip_num,data);
	}
	else
	{
		count += send_child(data);
	}

	sc.buf[0] = PEER_DATA_SEARCH;
	count += send_peer(data);

	if(count)
	{
		insert(data,DATA_SIZE+4, (char *)&count, sizeof(count));
	}
	else
	{
		*(unsigned long *)p_depth = htonl(depth);

		*up_down = MARK_UP;

		if(depth > 0)
		{
			sc.buf[sc.len++] = CMD_DATA_SEARCH;
			sc.buf[0] = CMD_DATA_SEARCH_FAIL;
		}
		else
			sc.buf[0] = CMD_DATA_SEARCH;
		this->send(parent->get_ip_num(), sc.buf, sc.len);
	}
}


/*
 * 데이터 검색 실패. 노드에 false positive가 존재하는 상황
 * 다른 노드들에 대해
 */

void Ilms::proc_data_search_fail()
{
	char *data = sc.get_cur();
	char *p_depth = sc.get_cur() + DATA_SIZE + 4 + 1;
	unsigned long depth = ntohl(*(unsigned long *)p_depth);

	std::string ret;

	if(!search(data,DATA_SIZE+4,ret))
		return;

	int count = *(int *)ret.c_str();

	if(--count == 0)
	{
		remove(data,DATA_SIZE+4);
		
		*(unsigned long *)p_depth = htonl(depth-1);

		if(depth == 1)
			sc.buf[0] = sc.buf[--sc.len];
		this->send(parent->get_ip_num(), sc.buf, sc.len);
		return;
	}

	insert(data,DATA_SIZE+4, (char *)&count, sizeof(count));
}


void Ilms::req_data_register()
{
	char *data;
	char *value;

	if(!sc.next_value(data,DATA_SIZE))
		return;

	if(!sc.next_value(value))
		return;

	my_filter->insert(data);
	insert(data,DATA_SIZE,value,0);

	sc.buf[0] = CMD_BF_ADD;
	this->send(parent->get_ip_num(), sc.buf, sc.len);

	sc.buf[0] = PEER_BF_ADD;
	for(unsigned int i=0; i < up_peer.size(); i++)
		this->send(up_peer[i].get_ip_num(), sc.buf, sc.len);
}

/*
 * 클라이언트로 부터의 데이터 추가 요청
 * 사실상 기존 프로토콜과 동일함
 */

void Ilms::req_data_update()
{
	char mode;
	char *data;
	char *value;

	if(!sc.next_value(mode))
		return;

	if(!sc.next_value(data,DATA_SIZE))
		return;

	if(!sc.next_value(value))
		return;


	if(mode == DATA_ADD)
	{
		std::string ret;
		if(search(data,DATA_SIZE,ret))
		{
			ret += ":";
			ret += value;
			insert(data,DATA_SIZE,ret.c_str(),ret.size());
		}
	}
	else if(mode == DATA_DELETE)
	{
		std::string ret;
		if(search(data,DATA_SIZE,ret))
		{
			char res[BUF_SIZE];
			int len = 0;
			for(unsigned int i=0;i < ret.size(); i++)
			{
				if(ret[i] == ':')
				{
					if(strncmp(ret.c_str()+i+1,value,*(unsigned char *)(value-1)-1) == 0)
					{
						i += *(unsigned char *)(value-1)-1;
						continue;
					}
				}
				res[len++] = ret[i];
			}
			res[len] = 0;
			insert(data,DATA_SIZE,res,len);
		}
	}
	else if(mode == DATA_REPLACE)
	{
		insert(data,DATA_SIZE,value,*(unsigned char *)(value-1));
	}
}

/*
 * 클라이언트로 부터의 데이터 검색 요청
 * 클라이언트는 자식이 아니므로 자식관련 처리과정이 생략됨
 */

void Ilms::req_data_search(unsigned long ip_num)
{
	char *data;

	if(!sc.next_value(data,DATA_SIZE))
		return;

	char *pos = sc.get_cur();

	*(unsigned long *)pos = ip_num;
	pos += 4;

	char &up_down = *pos;
	pos++;

	char *p_depth = pos;
	pos += 4;

	*(unsigned long *)p_depth = 0;

	sc.len = pos - sc.buf;


	my_filter->getBitArray(bitArray,data);
	if(my_filter->lookBitArray(bitArray))
	{
		std::string ret;
		if(search(data,DATA_SIZE,ret))
		{
			this->send(ip_num, ret.c_str(), ret.length()+1);
			return;
		}
	}

	up_down = MARK_DOWN;
	*(unsigned long *)p_depth = htonl(1);

	int count = 0;

	sc.buf[0] = PEER_DATA_SEARCH;
	count += send_peer(data);

	sc.buf[0] = CMD_DATA_SEARCH;
	count += send_child(data);

	if(count)
	{
		insert(data,DATA_SIZE+4, (char *)&count, sizeof(count));
	}
	else
	{
		up_down = MARK_UP;
		*(unsigned long *)p_depth = 0;
		this->send(parent->get_ip_num(), sc.buf, sc.len);
	}
}

/*
 * 클라이언트로 부터의 데이터 삭제 요청
 * 클라이언트는 자식이 아니므로 자식관련 처리과정이 생략됨
 */

void Ilms::req_data_delete(unsigned long ip_num)
{
	char *data;

	if(!sc.next_value(data,DATA_SIZE))
		return;

	if(my_filter->lookup(data))
	{
		remove(data, DATA_SIZE);
	}
}

void Ilms::peer_bf_add(unsigned long ip_num)
{
	char *data;
	if(!sc.next_value(data,DATA_SIZE))
		return;

	for(unsigned int i=0; i < down_peer.size(); i++)
	{
		if(ip_num == down_peer[i].get_ip_num())
		{
			peer_filter[i]->insert(data);
			break;
		}
	}
}

void Ilms::peer_data_search(unsigned long ip_num)
{
	char *data;
	unsigned long ip_org_num;

	if(!sc.next_value(data,DATA_SIZE))
		return;

	if(!sc.next_value(ip_org_num))
		return;

	if(my_filter->lookup(data))
	{
		std::string ret;
		if(search(data,DATA_SIZE,ret))
		{
			this->send(ip_org_num, ret.c_str(), ret.length()+1);
			return;
		}
	}

	char *up_down;
	if(!sc.next_value(up_down,1))
		return;

	sc.buf[sc.len++] = CMD_DATA_SEARCH;
	sc.buf[0] = CMD_DATA_SEARCH_FAIL;
	*up_down = MARK_UP;
	this->send(ip_num, sc.buf, sc.len);
}

void Ilms::proc_data_search_down()
{
	char *data;
	unsigned long ip_org_num;

	if(!sc.next_value(data,DATA_SIZE))
		return;

	if(!sc.next_value(ip_org_num))
		return;

	my_filter->getBitArray(bitArray,data);
	if(my_filter->lookBitArray(bitArray))
	{
		std::string ret;
		if(search(data,DATA_SIZE,ret))
		{
			this->send(ip_org_num, ret.c_str(), ret.length()+1);
			return;
		}
	}
	send_child(data);
	sc.buf[0] = PEER_DATA_SEARCH_DOWN;
	send_peer(data);
}

void Ilms::peer_data_search_down()
{
	char *data;
	unsigned long ip_org_num;

	if(!sc.next_value(data,DATA_SIZE))
		return;

	if(!sc.next_value(ip_org_num))
		return;

	if(my_filter->lookup(data))
	{
		std::string ret;
		if(search(data,DATA_SIZE,ret))
		{
			this->send(ip_org_num, ret.c_str(), ret.length()+1);
			return;
		}
	}
}

/*
 * DB에 데이터 넣기
 * key, value 쌍으로 입력
 */

void Ilms::insert(char *key,int klen,const char *val,int vlen)
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
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include "ilms.h"

#define CMD_BF_UPDATE					0x00
#define CMD_LOOKUP						0x10
#define CMD_LOOKUP_NACK				0x11
#define CMD_LOOKUP_DOWN				0x12

#define REQ_ID_REGISTER				0x20
#define REQ_LOC_UPDATE				0x21
#define REQ_LOOKUP						0x22
#define REQ_ID_DEREGISTER			0x23

#define PEER_BF_UPDATE				0x30
#define PEER_LOOKUP						0x31
#define PEER_LOOKUP_DOWN			0x32

#define DATA_ADD									0x00
#define DATA_DELETE								0x01
#define DATA_REPLACE							0X02

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
std::vector<Node> Tree::peered;
std::vector<Node> Tree::peering;

Bloomfilter* Ilms::my_filter;
Bloomfilter** Ilms::child_filter;
Bloomfilter** Ilms::peer_filter;
Scanner Ilms::sc;
std::atomic<int> Ilms::global_counter;

std::atomic<int> Ilms::protocol[100];

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

	if(peered.size())
	{
		peer_filter = new Bloomfilter*[peered.size()];
		for(unsigned int i=0; i < peered.size(); i++)
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
	test_process();
}

void Ilms::test_process()
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
	if(peered.size())
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

	stat = std::thread(&Ilms::stat_run,this);

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

			if(cmd >= 0 && cmd < 100)
				protocol[cmd]++;

			unsigned long ip_num = clnt_adr.sin_addr.s_addr;

			switch(cmd)
			{
			case CMD_BF_UPDATE: proc_bf_update(ip_num); break;
			case CMD_LOOKUP: proc_lookup(ip_num); break;
			case CMD_LOOKUP_NACK: proc_lookup_nack(); break;
			case CMD_LOOKUP_DOWN: proc_lookup_down(); break;

			case REQ_ID_REGISTER: req_id_register(ip_num); break;
			case REQ_LOC_UPDATE: req_loc_update(ip_num); break;
			case REQ_LOOKUP: req_lookup(ip_num); break;
			case REQ_ID_DEREGISTER: req_id_deregister(ip_num); break;

			case PEER_BF_UPDATE: peer_bf_update(ip_num); break;
			case PEER_LOOKUP: peer_lookup(ip_num); break;
			case PEER_LOOKUP_DOWN: peer_lookup_down(); break;
			}
		}
	}
}

const int nProt[] = {
	CMD_BF_UPDATE, CMD_LOOKUP, CMD_LOOKUP_NACK, CMD_LOOKUP_DOWN, REQ_ID_REGISTER,
	REQ_LOC_UPDATE, REQ_LOOKUP, REQ_ID_DEREGISTER, PEER_BF_UPDATE, PEER_LOOKUP,
	PEER_LOOKUP_DOWN, -1
};

const char *sProt[] = {
	"CMD_BF_UPDATE", "CMD_LOOKUP", "CMD_LOOKUP_NACK", "CMD_LOOKUP_DOWN", "REQ_ID_REGISTER",
	"REQ_LOC_UPDATE", "REQ_LOOKUP", "REQ_ID_DEREGISTER", "PEER_BF_UPDATE", "PEER_LOOKUP",
	"PEER_LOOKUP_DOWN"
};

void Ilms::stat_run()
{
	while(1)
	{
		for(int i=0; nProt[i] != -1; i++)
			std::cout << sProt[i] << " : " << protocol[nProt[i]] << std::endl;
		sleep(60);
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
	for(unsigned int i=0; i < child.size(); i++)
	{
		if(child_filter[i]->lookBitArray(bitArray))
		{
			Ilms::send(child[i].get_ip_num(), sc.buf, sc.len);
			ret++;
		}
	}
	return ret;
}

int Ilms::send_child(unsigned long ip_num, char *data)
{
	int ret = 0;
	for(unsigned int i=0; i < child.size(); i++)
	{
		if(ip_num != child[i].get_ip_num() && child_filter[i]->lookBitArray(bitArray))
		{
			Ilms::send(child[i].get_ip_num(), sc.buf, sc.len);
			ret++;
		}
	}
	return ret;
}

void Ilms::peer_run(unsigned int i)
{
	if(peer_filter[i]->lookBitArray(bitArray))
	{
		Ilms::send(peered[i].get_ip_num(), sc.buf, sc.len);
		Ilms::global_counter++;
	}
}

int Ilms::send_peer(char *data)
{
	int ret = 0;
	for(unsigned int i=0; i < peered.size();)
	{
		if(peer_filter[i]->lookBitArray(bitArray))
		{
			Ilms::send(peered[i].get_ip_num(), sc.buf, sc.len);
			ret++;
		}
	}
	return ret;
}

void Ilms::send_id(unsigned long ip_num, char *id, const char *ret, int len)
{
	char buf[BUF_SIZE];
	int pos = 0;
	for(int i=0;i<DATA_SIZE;i++)
		buf[pos++] = id[i];
	int count = 0;
	for(int i=0; i < len; i++)
		if(ret[i] == ':')
			count++;
	buf[pos++] = count;
	strncpy(buf+pos,ret,len);
	pos += len;
	this->send(ip_num, buf, pos);
}

/*
 * 블룸필터 데이터 추가
 * 부모노드에도 추가해야 하므로 버퍼그대로 전송
 */

void Ilms::proc_bf_update(unsigned long ip_num)
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

void Ilms::proc_lookup(unsigned long ip_num)
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
			send_id(ip_org_num,data,ret.c_str(),ret.length());
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

	sc.buf[0] = PEER_LOOKUP;
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
			sc.buf[sc.len++] = CMD_LOOKUP;
			sc.buf[0] = CMD_LOOKUP_NACK;
		}
		else
			sc.buf[0] = CMD_LOOKUP;
		this->send(parent->get_ip_num(), sc.buf, sc.len);
	}
}


/*
 * 데이터 검색 실패. 노드에 false positive가 존재하는 상황
 * 다른 노드들에 대해
 */

void Ilms::proc_lookup_nack()
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


void Ilms::req_id_register(unsigned long ip_num)
{
	char *data;
	char *value;

	if(!sc.next_value(data,DATA_SIZE))
		return;

	if(!sc.next_value(value))
		return;

	my_filter->insert(data);
	insert(data,DATA_SIZE,value,0);

	sc.buf[0] = CMD_BF_UPDATE;
	this->send(parent->get_ip_num(), sc.buf, sc.len);

	sc.buf[0] = PEER_BF_UPDATE;
	for(unsigned int i=0; i < peering.size(); i++)
		this->send(peering[i].get_ip_num(), sc.buf, sc.len);
	this->send(ip_num, sc.buf, sc.len);
}

/*
 * 클라이언트로 부터의 데이터 추가 요청
 * 사실상 기존 프로토콜과 동일함
 */

void Ilms::req_loc_update(unsigned long ip_num)
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
	this->send(ip_num, sc.buf, sc.len);
}

/*
 * 클라이언트로 부터의 데이터 검색 요청
 * 클라이언트는 자식이 아니므로 자식관련 처리과정이 생략됨
 */

void Ilms::req_lookup(unsigned long ip_num)
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
			send_id(ip_num,data,ret.c_str(),ret.length());
			return;
		}
	}

	up_down = MARK_DOWN;
	*(unsigned long *)p_depth = htonl(1);

	int count = 0;

	sc.buf[0] = PEER_LOOKUP;
	count += send_peer(data);

	sc.buf[0] = CMD_LOOKUP;
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

void Ilms::req_id_deregister(unsigned long ip_num)
{
	char *data;

	if(!sc.next_value(data,DATA_SIZE))
		return;

	if(my_filter->lookup(data))
	{
		remove(data, DATA_SIZE);
	}
	this->send(ip_num, sc.buf, sc.len);
}

void Ilms::peer_bf_update(unsigned long ip_num)
{
	char *data;
	if(!sc.next_value(data,DATA_SIZE))
		return;

	for(unsigned int i=0; i < peered.size(); i++)
	{
		if(ip_num == peered[i].get_ip_num())
		{
			peer_filter[i]->insert(data);
			break;
		}
	}
}

void Ilms::peer_lookup(unsigned long ip_num)
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
			send_id(ip_org_num,data,ret.c_str(),ret.length());
			return;
		}
	}

	char *up_down;
	if(!sc.next_value(up_down,1))
		return;

	sc.buf[sc.len++] = CMD_LOOKUP;
	sc.buf[0] = CMD_LOOKUP_NACK;
	*up_down = MARK_UP;
	this->send(ip_num, sc.buf, sc.len);
}

void Ilms::proc_lookup_down()
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
			send_id(ip_org_num,data,ret.c_str(),ret.length());
			return;
		}
	}
	send_child(data);
	sc.buf[0] = PEER_LOOKUP_DOWN;
	send_peer(data);
}

void Ilms::peer_lookup_down()
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
			send_id(ip_org_num,data,ret.c_str(),ret.length());
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

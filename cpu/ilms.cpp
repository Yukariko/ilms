#include <cstring>
#include <cstdio>
#include <cstdlib>
#include "ilms.h"

#define CMD_BF_UPDATE					0x00
#define CMD_LOOKUP						0x10
#define CMD_LOOKUP_NACK				0x11
#define CMD_LOOKUP_DOWN				0x12

#define REQ_ID_REGISTER				0x20
#define REQ_LOOKUP						0x21
#define REQ_ID_DEREGISTER			0x22
#define REQ_SUCCESS						0x23
#define REQ_FAIL							0x24

#define PEER_BF_UPDATE				0x30
#define PEER_LOOKUP						0x31
#define PEER_LOOKUP_DOWN			0x32

#define LOC_LOOKUP						0x00
#define LOC_SET								0x01
#define LOC_SUB								0x02
#define LOC_REP								0X03

#define MARK_UP										0x01
#define MARK_DOWN									0x00

#define NOREFRESH							0x00
#define MYREFRESH							0x01
#define CHILDREFRESH					0x02

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

long long test(const char *data){return (*(unsigned short *)(data + 0) * 1009LL ) % (defaultSize / 10) + (defaultSize / 10) * 0;}
long long test2(const char *data){return (*(unsigned short *)(data + 2) * 1009LL ) % (defaultSize / 10) + (defaultSize / 10) * 1;}
long long test3(const char *data){return (*(unsigned short *)(data + 4) * 1009LL ) % (defaultSize / 10) + (defaultSize / 10) * 2;}
long long test4(const char *data){return (*(unsigned short *)(data + 6) * 1009LL ) % (defaultSize / 10) + (defaultSize / 10) * 3;;}
long long test5(const char *data){return (*(unsigned short *)(data + 8) * 1009LL ) % (defaultSize / 10) + (defaultSize / 10) * 4;;}
long long test6(const char *data){return (*(unsigned short *)(data + 10) * 1009LL ) % (defaultSize / 10) + (defaultSize / 10) * 5;;}
long long test7(const char *data){return (*(unsigned short *)(data + 12) * 1009LL ) % (defaultSize / 10) + (defaultSize / 10) * 6;;}
long long test8(const char *data){return (*(unsigned short *)(data + 14) * 1009LL ) % (defaultSize / 10) + (defaultSize / 10) * 7;;}
long long test9(const char *data){return (*(unsigned short *)(data + 16) * 1009LL ) % (defaultSize / 10) + (defaultSize / 10) * 8;;}
long long test10(const char *data){return (*(unsigned short *)(data + 18) * 1009LL ) % (defaultSize / 10) + (defaultSize / 10) * 9;}
long long test11(const char *data){return (*(unsigned int *)(data + 20) * 1009LL);}

leveldb::DB* Ilms::db;
leveldb::Options Ilms::options;

std::vector<Node> Tree::child;
std::vector<Node> Tree::peered;
std::vector<Node> Tree::peering;

Bloomfilter* Ilms::my_filter;
Bloomfilter** Ilms::child_filter;
Bloomfilter** Ilms::peer_filter;
Bloomfilter* Ilms::shadow_filter;

Scanner Ilms::sc;
std::atomic<int> Ilms::global_counter;
std::atomic<int> Ilms::global_switch;
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
	long long (*hash[11])(const char *) = {test,test2,test3,test4,test5,test6,test7,test8,test9,test10,test11};

	my_filter = new Bloomfilter(defaultSize, 11, hash);
	shadow_filter = new Bloomfilter(defaultSize, 11, hash);

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
			for(int j=0;j<VIRTUAL_FILTER_ID_NUM;j++)
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

void Ilms::test_filter()
{
	for(long long i=0; i < defaultSize / 8; i++)
		if(my_filter->filter[i] != shadow_filter->filter[i])
		{
			std::cout << "Refresh Error!" << std::endl;
			return;
		}
	std::cout << "Refresh OK!" << std::endl;
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
	refresh = std::thread(&Ilms::refresh_run, this);
	cmd = std::thread(&Ilms::cmd_run, this);

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
	REQ_LOOKUP, REQ_ID_DEREGISTER, PEER_BF_UPDATE, PEER_LOOKUP,
	PEER_LOOKUP_DOWN, -1
};

const char *sProt[] = {
	"CMD_BF_UPDATE", "CMD_LOOKUP", "CMD_LOOKUP_NACK", "CMD_LOOKUP_DOWN", "REQ_ID_REGISTER",
	"REQ_LOOKUP", "REQ_ID_DEREGISTER", "PEER_BF_UPDATE", "PEER_LOOKUP",
	"PEER_LOOKUP_DOWN"
};

void Ilms::stat_run()
{
	while(1)
	{
		for(int i=0; nProt[i] != -1; i++)
			std::cout << sProt[i] << " : " << protocol[nProt[i]] << std::endl;
		sleep(STAT_FREQUENCY);
	}
}

void Ilms::refresh_run()
{
	global_switch = NOREFRESH;
	if(child.size() > 0)
	{
		int sd = socket(PF_INET, SOCK_STREAM, 0);
		if(sd == -1)
		{
			perror("socket");
			exit(1);
		}

		int option;
		setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (void *)&option, 4);

		struct sockaddr_in ref_adr;
		memset(&ref_adr, 0, sizeof(ref_adr));
		ref_adr.sin_family = PF_INET;
		ref_adr.sin_addr.s_addr = htonl(INADDR_ANY);
		ref_adr.sin_port = htons(REFRESH_PORT);

		if(bind(sd, (struct sockaddr*)&ref_adr, sizeof(ref_adr)))
		{
			perror("bind");
			exit(1);
		}

		if(listen(sd, 0xFFF))
		{
			perror("listen");
			exit(1);
		}

		int ns;
		struct sockaddr_in cli;
		int clientlen;

		while((ns = accept(sd, (struct sockaddr *)&cli,  (socklen_t *)&clientlen)) != -1)
		{
			unsigned long ip_num = cli.sin_addr.s_addr;

			unsigned char *fpt = shadow_filter->filter;
			for(int len; (len = recv(ns, fpt, 4096, 0)) > 0; )
				fpt += len;

			close(ns);
			global_switch = CHILDREFRESH;
			for(unsigned int i=0; i < child.size(); i++)
			{
				if(child[i].get_ip_num() == ip_num)
				{
					child_filter[i]->setFilter(shadow_filter->filter);
					break;
				}
			}

			global_switch = NOREFRESH;
			shadow_filter->zeroFilter();

			global_switch = MYREFRESH;
			leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());
			for(it->SeekToFirst(); it->Valid(); it->Next())
			{
				std::string key = it->key().ToString();
				if(key.length() != DATA_SIZE)
					continue;
				shadow_filter->insert(key.c_str());
			}
			assert(it->status().ok());	// Check for any errors found during the scan
			delete it;
			global_switch = NOREFRESH;

			my_filter->setFilter(shadow_filter->filter);

			global_switch = CHILDREFRESH;
			for(unsigned int i=0; i < child.size(); i++)
				shadow_filter->mergeFilter(child_filter[i]->filter);

			global_switch = NOREFRESH;
			send_refresh(parent->get_ip_num(), shadow_filter->filter);
		}
	}
	else
	{
		while(1)
		{
			sleep(REFRESH_FREQUENCY);
			global_switch = MYREFRESH;
			shadow_filter->zeroFilter();

			leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());
			for(it->SeekToFirst(); it->Valid(); it->Next())
			{
				std::string key = it->key().ToString();
				if(key.length() != DATA_SIZE)
					continue;
				shadow_filter->insert(key.c_str());
			}
			assert(it->status().ok());	// Check for any errors found during the scan
			delete it;

			global_switch = NOREFRESH;
			//test_filter();

			my_filter->setFilter(shadow_filter->filter);
			send_refresh(parent->get_ip_num(), shadow_filter->filter);
		}
	}
}

void Ilms::cmd_run()
{
	std::string c;
	while(1)
	{
		std::cin >> c;
		if(c == "show")
		{
			std::cout << "--------------------" << std::endl;
			leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());
			for(it->SeekToFirst(); it->Valid(); it->Next())
			{
				std::string key = it->key().ToString();
				if(key.length() != DATA_SIZE)
					continue;
				std::cout << "[" << key.c_str() << "] " << it->value().ToString() << std::endl;
			}
			assert(it->status().ok());	// Check for any errors found during the scan
			delete it;
			std::cout << "--------------------" << std::endl;
		}
		else if(c == "crash")
		{
			leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());
			for(it->SeekToFirst(); it->Valid(); it->Next())
			{
				std::string key = it->key().ToString();
				if(key.length() != DATA_SIZE)
					continue;
				leveldb::Status s = db->Delete(leveldb::WriteOptions(),leveldb::Slice(key.c_str(),DATA_SIZE));
				assert(s.ok());
			}
			assert(it->status().ok());	// Check for any errors found during the scan
			delete it;

			std::cout << "delete complete" << std::endl;
		}
	}
}


/*
 * 버퍼 전송
 */

void Ilms::send_node(unsigned long ip_num,const char *buf,int len)
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

void Ilms::send_refresh(unsigned long ip_num, unsigned char *filter)
{
	if(ip_num == 0)
		return;

	int sd = socket(PF_INET, SOCK_STREAM, 0);
	if(sd == -1)
	{
		perror("sock open");
		exit(1);
	}

	struct sockaddr_in clnt_adr;

	memset(&clnt_adr,0,sizeof(clnt_adr));
	clnt_adr.sin_family = AF_INET;
	clnt_adr.sin_addr.s_addr = ip_num;
	clnt_adr.sin_port = htons(REFRESH_PORT);

	if(connect(sd, (struct sockaddr *)&clnt_adr, sizeof(clnt_adr)) == -1)
	{
		perror("connect");
		exit(1);
	}

	unsigned char *fpt = filter;
	unsigned char *endf = filter + defaultSize / 8 + 1;

	for(int len; endf - fpt > 0 && (len = send(sd, fpt, std::min(4096, (int)(endf - fpt)), 0)) > 0;)
		fpt += len;


	close(sd);

	DEBUG("Refresh Send OK!");

}

void Ilms::child_run(unsigned int i)
{
	if(child_filter[i]->lookBitArray(bitArray))
	{
		Ilms::send_node(child[i].get_ip_num(), sc.buf, sc.len);
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
			Ilms::send_node(child[i].get_ip_num(), sc.buf, sc.len);
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
			Ilms::send_node(child[i].get_ip_num(), sc.buf, sc.len);
			ret++;
		}
	}
	return ret;
}

void Ilms::peer_run(unsigned int i)
{
	if(peer_filter[i]->lookBitArray(bitArray))
	{
		Ilms::send_node(peered[i].get_ip_num(), sc.buf, sc.len);
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
			Ilms::send_node(peered[i].get_ip_num(), sc.buf, sc.len);
			ret++;
		}
	}
	return ret;
}

void Ilms::send_id(unsigned long ip_num, char *id, const char *ret, int len)
{
	char buf[BUF_SIZE];
	int pos = 0;
	buf[pos++] = REQ_SUCCESS;
	for(int i=0;i<DATA_SIZE;i++)
		buf[pos++] = id[i];

	buf[pos++] = LOC_LOOKUP;
	buf[pos++] = len;
	for(int i=0; i < len; i++)
		buf[pos++] = ret[i];
	this->send_node(ip_num, buf, pos);
}

void Ilms::loc_process(unsigned long ip_num, char *id, char mode, unsigned char vlen, char *value, std::string& ret)
{
	bool find = false;
	if(mode == LOC_LOOKUP)
	{
		send_id(ip_num,id,ret.c_str(),ret.length());
		return;
	}
	else if(mode == LOC_SET)
	{
		if(ret.back() != ':')
			ret += ":";
		ret += value;
		insert(id,DATA_SIZE,ret.c_str(),ret.size());
		find = true;
	}
	else if(mode == LOC_SUB)
	{
		int len = 0;
		std::string res;
		std::string loc;
		std::string val;
		for(unsigned char i=0; i < vlen-1; i++)
			val += value[i];

		for(unsigned int i=0;i < ret.size(); i++)
		{
			if(ret[i] == ':')
			{
				if(loc == val)
				{
					find = true;
					i += loc.size();
				}
				else if(loc.size())
				{
					res += ":";
					res += loc;
					loc = "";
				}
			}
			else
			{
				loc += ret[i];
			}
		}
		if(loc.size())
		{
			if(loc == val)
				find = true;
			else
			{
				res += ":";
				res += loc;
			}
		}
		insert(id,DATA_SIZE,res.c_str(),res.length());
	}
	else if(mode == LOC_REP)
	{
		insert(id,DATA_SIZE,value,vlen);
		find = true;
	}

	if(find == false)
		sc.buf[0] = REQ_FAIL;
	else
		sc.buf[0] = REQ_SUCCESS;
	sc.buf[DATA_SIZE+1] = mode;
	sc.buf[DATA_SIZE+2] = vlen;
	for(size_t i=0; i < vlen; i++)
		sc.buf[DATA_SIZE+3+i] = value[i];
	sc.len = DATA_SIZE + 4 + vlen;
	this->send_node(ip_num, sc.buf, sc.len);
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

			if(global_switch == CHILDREFRESH)
				shadow_filter->insert(data);
			break;
		}
	}
	if(find)
		this->send_node(parent->get_ip_num(), sc.buf, sc.len);
}


/*
 * 데이터 검색. 다음의 순서로 동작
 * 1) 자신의 필터에 데이터가 있다면 자신의 DB에서 검색. 검색 결과가 있다면 처음 노드에 직접 전송
 * 2) 없다면 자식 필터를 확인하고, 있다면 자식들에 대해 검색 전송
 * 3) 자식필터에 데이터가 없다면 부모노드로 올라감. 부모노드에서 내려온 상태라면 부모에 검색 실패 전송
 */

void Ilms::proc_lookup(unsigned long ip_num)
{
	char *id;
	if(!sc.next_value(id,DATA_SIZE))
		return;

	unsigned long ip_org_num;
	if(!sc.next_value(ip_org_num))
		return;

	char *up_down;
	if(!sc.next_value(up_down,1))
		return;

	char *p_depth;
	if(!sc.next_value(p_depth,4))
		return;

	char mode;
	if(!sc.next_value(mode))
		return;

	unsigned char vlen;
	if(!sc.next_value(vlen))
		return;

	char *value;
	if(!sc.next_value(value, vlen))
		return;


	my_filter->getBitArray(bitArray,id);
	if(my_filter->lookBitArray(bitArray))
	{
		std::string ret;
		if(search(id,DATA_SIZE,ret))
		{
			loc_process(ip_org_num, id, mode, vlen, value, ret);
			return;
		}
	}

	unsigned long depth = ntohl(*(unsigned long *)p_depth);

	int count = 0;

	*(unsigned long *)p_depth = htonl(depth+1);

	if(*up_down == MARK_UP)
	{
		*up_down = MARK_DOWN;
		count += send_child(ip_num,id);
	}
	else
	{
		count += send_child(id);
	}

	sc.buf[0] = PEER_LOOKUP;
	count += send_peer(id);

	if(count)
	{
		insert(id,DATA_SIZE+4, (char *)&count, sizeof(count));
	}
	else
	{
		*(unsigned long *)p_depth = htonl(depth);

		*up_down = MARK_UP;

		if(depth > 0)
			sc.buf[0] = CMD_LOOKUP_NACK;
		else
			sc.buf[0] = CMD_LOOKUP;
		this->send_node(parent->get_ip_num(), sc.buf, sc.len);
	}
}


/*
 * 데이터 검색 실패. 노드에 false positive가 존재하는 상황
 * 다른 노드들에 대해
 */

void Ilms::proc_lookup_nack()
{
	char *id = sc.get_cur();
	char *p_depth = sc.get_cur() + DATA_SIZE + 4 + 1;
	unsigned long depth = ntohl(*(unsigned long *)p_depth);

	std::string ret;

	if(!search(id,DATA_SIZE+4,ret))
		return;

	int count = *(int *)ret.c_str();

	if(--count == 0)
	{
		remove(id,DATA_SIZE+4);

		*(unsigned long *)p_depth = htonl(depth-1);

		if(depth == 1)
			sc.buf[0] = CMD_LOOKUP;
		this->send_node(parent->get_ip_num(), sc.buf, sc.len);
		return;
	}

	insert(id,DATA_SIZE+4, (char *)&count, sizeof(count));
}


void Ilms::req_id_register(unsigned long ip_num)
{
	char *id;
	char *value;

	if(!sc.next_value(id,DATA_SIZE))
		return;

	if(!sc.next_value(value))
		return;

	my_filter->getBitArray(bitArray,id);
	if(my_filter->lookBitArray(bitArray))
	{
		std::string ret;
		if(search(id,DATA_SIZE,ret))
		{
			sc.buf[0] = REQ_FAIL;
			this->send_node(ip_num, sc.buf, sc.len);
			return;
		}
	}

	my_filter->insert(id);

	std::string loc = ":";
	loc += value;

	insert(id,DATA_SIZE, loc.c_str(), loc.size());

	if(global_switch == MYREFRESH)
		shadow_filter->insert(id);

	sc.buf[0] = CMD_BF_UPDATE;
	this->send_node(parent->get_ip_num(), sc.buf, sc.len);

	sc.buf[0] = PEER_BF_UPDATE;
	for(unsigned int i=0; i < peering.size(); i++)
		this->send_node(peering[i].get_ip_num(), sc.buf, sc.len);

	sc.buf[0] = REQ_SUCCESS;
	this->send_node(ip_num, sc.buf, sc.len);
}

/*
 * 클라이언트로 부터의 데이터 추가 요청
 * 사실상 기존 프로토콜과 동일함
 */

/*
 * 클라이언트로 부터의 데이터 검색 요청
 * 클라이언트는 자식이 아니므로 자식관련 처리과정이 생략됨
 */

void Ilms::req_lookup(unsigned long ip_num)
{
	char *id;
	if(!sc.next_value(id,DATA_SIZE))
		return;

	char mode;
	if(!sc.next_value(mode))
		return;

	unsigned char vlen;
	if(!sc.next_value(vlen))
		return;

	char *value = sc.get_cur();
	char new_packet[BUF_SIZE];
	char *pos = new_packet;

	*pos = CMD_LOOKUP;
	pos++;

	for(int i=0; i < DATA_SIZE; i++)
	{
		*pos = id[i];
		pos++;
	}

	*(unsigned long *)pos = ip_num;
	pos += 4;

	char &up_down = *pos;
	pos++;

	char *p_depth = pos;
	*(unsigned long *)p_depth = 0;
	pos += 4;

	*pos = mode;
	pos++;

	*pos = vlen;
	pos++;

	for(unsigned char i=0; i < vlen; i++)
	{
		*pos = value[i];
		pos++;
	}

	int len = (int)(pos - new_packet);
	for(int i=0; i < len; i++)
		sc.buf[i] = new_packet[i];

	value = sc.buf + 1 + DATA_SIZE + 4 + 1 + 4 + 1 + 1;

	sc = Scanner(sc.buf, len);

	my_filter->getBitArray(bitArray,id);
	if(my_filter->lookBitArray(bitArray))
	{
		std::string ret;
		if(search(id,DATA_SIZE,ret))
		{
			loc_process(ip_num, id, mode, vlen, value, ret);
			return;
		}
	}

	up_down = MARK_DOWN;
	*(unsigned long *)p_depth = htonl(1);

	int count = 0;

	sc.buf[0] = PEER_LOOKUP;
	count += send_peer(id);

	sc.buf[0] = CMD_LOOKUP;
	count += send_child(id);

	if(count)
	{
		insert(id,DATA_SIZE+4, (char *)&count, sizeof(count));
	}
	else
	{
		up_down = MARK_UP;
		*(unsigned long *)p_depth = 0;
		this->send_node(parent->get_ip_num(), sc.buf, sc.len);
	}
}

/*
 * 클라이언트로 부터의 데이터 삭제 요청
 * 클라이언트는 자식이 아니므로 자식관련 처리과정이 생략됨
 */

void Ilms::req_id_deregister(unsigned long ip_num)
{
	char *id;

	if(!sc.next_value(id,DATA_SIZE))
		return;

	bool find = false;
	if(my_filter->lookup(id))
		if(remove(id, DATA_SIZE))
			find = true;

	if(find)
		sc.buf[0] = REQ_SUCCESS;
	else
		sc.buf[0] = REQ_FAIL;
	this->send_node(ip_num, sc.buf, sc.len);
}

void Ilms::peer_bf_update(unsigned long ip_num)
{
	char *id;
	if(!sc.next_value(id,DATA_SIZE))
		return;

	for(unsigned int i=0; i < peered.size(); i++)
	{
		if(ip_num == peered[i].get_ip_num())
		{
			peer_filter[i]->insert(id);
			break;
		}
	}
}

void Ilms::peer_lookup(unsigned long ip_num)
{
	char *id;
	if(!sc.next_value(id,DATA_SIZE))
		return;

	unsigned long ip_org_num;
	if(!sc.next_value(ip_org_num))
		return;

	char *up_down;
	if(!sc.next_value(up_down,1))
		return;

	char *p_depth;
	if(!sc.next_value(p_depth,4))
		return;

	char mode;
	if(!sc.next_value(mode))
		return;

	unsigned char vlen;
	if(!sc.next_value(vlen))
		return;

	char *value;
	if(!sc.next_value(value,vlen))
		return;

	my_filter->getBitArray(bitArray,id);
	if(my_filter->lookBitArray(bitArray))
	{
		std::string ret;
		if(search(id,DATA_SIZE,ret))
		{
			loc_process(ip_org_num, id, mode, vlen, value, ret);
			return;
		}
	}

	sc.buf[0] = CMD_LOOKUP_NACK;
	*up_down = MARK_UP;
	this->send_node(ip_num, sc.buf, sc.len);
}

void Ilms::proc_lookup_down()
{
	char *id;
	if(!sc.next_value(id,DATA_SIZE))
		return;

	unsigned long ip_org_num;
	if(!sc.next_value(ip_org_num))
		return;

	char mode;
	if(!sc.next_value(mode))
		return;

	unsigned char vlen;
	if(!sc.next_value(vlen))
		return;

	char *value;
	if(!sc.next_value(value,vlen))
		return;

	my_filter->getBitArray(bitArray,id);
	if(my_filter->lookBitArray(bitArray))
	{
		std::string ret;
		if(search(id,DATA_SIZE,ret))
		{
			loc_process(ip_org_num, id, mode, vlen, value, ret);
			return;
		}
	}

	send_child(id);
	sc.buf[0] = PEER_LOOKUP_DOWN;
	send_peer(id);
}

void Ilms::peer_lookup_down()
{
	char *id;
	if(!sc.next_value(id,DATA_SIZE))
		return;

	unsigned long ip_org_num;
	if(!sc.next_value(ip_org_num))
		return;

	char mode;
	if(!sc.next_value(mode))
		return;

	unsigned char vlen;
	if(!sc.next_value(vlen))
		return;

	char *value;
	if(!sc.next_value(value,vlen))
		return;

	my_filter->getBitArray(bitArray,id);
	if(my_filter->lookBitArray(bitArray))
	{
		std::string ret;
		if(search(id,DATA_SIZE,ret))
		{
			loc_process(ip_org_num, id, mode, vlen, value, ret);
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

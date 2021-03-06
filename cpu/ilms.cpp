#include <cstring>
#include <cstdio>
#include <cstdlib>
#include "ilms.h"

// 프로토콜 식별 번호

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

// mode 식별 번호

#define LOC_LOOKUP						0x00
#define LOC_SET								0x01
#define LOC_SUB								0x02
#define LOC_REP								0X03

#define MARK_UP										0x01
#define MARK_DOWN									0x00

// refresh 상태

#define NOREFRESH							0x00
#define MYREFRESH							0x01
#define CHILDREFRESH					0x02

// 디버깅 관련

//#define MODE 1
#ifdef MODE
#define DEBUG(s) (std::cout << s << std::endl)
#endif

#ifndef MODE
#define DEBUG(s) 0
#endif


// 블룸필터 기본 크기(2MB)
const long long defaultSize = 8LL * 2 * 1024 * 1024;

// 테스트 해시 함수
long long test(const char *id){return (*(unsigned short *)(id + 0) * 1009LL ) % (defaultSize / 10) + (defaultSize / 10) * 0;}
long long test2(const char *id){return (*(unsigned short *)(id + 2) * 1009LL ) % (defaultSize / 10) + (defaultSize / 10) * 1;}
long long test3(const char *id){return (*(unsigned short *)(id + 4) * 1009LL ) % (defaultSize / 10) + (defaultSize / 10) * 2;}
long long test4(const char *id){return (*(unsigned short *)(id + 6) * 1009LL ) % (defaultSize / 10) + (defaultSize / 10) * 3;;}
long long test5(const char *id){return (*(unsigned short *)(id + 8) * 1009LL ) % (defaultSize / 10) + (defaultSize / 10) * 4;;}
long long test6(const char *id){return (*(unsigned short *)(id + 10) * 1009LL ) % (defaultSize / 10) + (defaultSize / 10) * 5;;}
long long test7(const char *id){return (*(unsigned short *)(id + 12) * 1009LL ) % (defaultSize / 10) + (defaultSize / 10) * 6;;}
long long test8(const char *id){return (*(unsigned short *)(id + 14) * 1009LL ) % (defaultSize / 10) + (defaultSize / 10) * 7;;}
long long test9(const char *id){return (*(unsigned short *)(id + 16) * 1009LL ) % (defaultSize / 10) + (defaultSize / 10) * 8;;}
long long test10(const char *id){return (*(unsigned short *)(id + 18) * 1009LL ) % (defaultSize / 10) + (defaultSize / 10) * 9;}
long long test11(const char *id){return (*(unsigned int *)(id + 20) * 1009LL);}

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

IDPAddress Ilms::eid;



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
	// ip가 0인 노드는 가상필터로 간주하여, VIRTUAL_FILTER_ID_NUM개의 가상 ID데이터를 생성하여 등록
	char id[BUF_SIZE];
	for(unsigned int i=0;i<child.size();i++)
	{
		if(child[i].get_ip_num() == 0)
		{
			for(int j=0;j<VIRTUAL_FILTER_ID_NUM;j++)
			{
				for(int k=0;k<24;k+=2)
				{
					*(unsigned short *)(id+k) = (unsigned short)rand();
				}
				child_filter[i]->insert(id);
			}
		}
	}
}

void Ilms::test_filter()
{
	// shadow필터와 자신의 필터가 다른점이 있다면 Error
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


		if(len > 0)
		{
			sc = Scanner(buf, len);

			// 프로토콜 판별
			char cmd;
			if(!sc.next_value(cmd))
				continue;

			DEBUG("Protocol OK!");

			// 통계 갱신
			if(cmd >= 0 && cmd < 100)
				protocol[(int)cmd]++;

			unsigned int ip_num = clnt_adr.sin_addr.s_addr;

			//프로토콜 별 처리
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

const char *modes[] {
	"GET", "SET", "SUB", "REP"
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

void Ilms::reset(Bloomfilter *filter)
{
	leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());
	for(it->SeekToFirst(); it->Valid(); it->Next())
	{
		std::string key = it->key().ToString();
		if(key.length() != ID_SIZE)
			continue;
		filter->insert(key.c_str());
	}
	assert(it->status().ok());	// Check for any errors found during the scan
	delete it;
}

void Ilms::refresh_run()
{
	global_switch = NOREFRESH;

	reset(my_filter);

	// 자신이 leaf 노드라면 주기마다 refresh를 부모에 전송
	// leaf 노드가 아니라면 refresh 요청을 기다림
	// refresh 도중 update의 영향으로 문제가 발생하지 않도록 global_switch로 체크

	if(child.size() > 0)
	{
		// socket init
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

		// refresh 요청을 기다림
		// 요청온 child노드의 블룸필터를 갱신하고 각 필터를 merge시켜 parent노드로 전송
		while((ns = accept(sd, (struct sockaddr *)&cli,  (socklen_t *)&clientlen)) != -1)
		{
			unsigned int ip_num = cli.sin_addr.s_addr;

			unsigned char *fpt = shadow_filter->filter;
			for(int len; (len = recv(ns, fpt, 4096, 0)) > 0; )
				fpt += len;

			close(ns);

			// 요청온 노드를 찾고 필터를 갱신

			global_switch = CHILDREFRESH;
			for(unsigned int i=0; i < child.size(); i++)
			{
				if(child[i].get_ip_num() == ip_num)
				{
					child_filter[i]->setFilter(shadow_filter->filter);
					break;
				}
			}

			// 자신의 필터를 재구성

			global_switch = NOREFRESH;
			shadow_filter->zeroFilter();

			global_switch = MYREFRESH;

			reset(shadow_filter);

			global_switch = NOREFRESH;

			my_filter->setFilter(shadow_filter->filter);

			// 각 필터를 merge하여 parent 노드에 전송

			global_switch = CHILDREFRESH;
			for(unsigned int i=0; i < child.size(); i++)
				shadow_filter->mergeFilter(child_filter[i]->filter);

			global_switch = NOREFRESH;
			send_refresh(parent->get_ip_num(), shadow_filter->filter);
		}
	}
	else
	{
		// leaf 노드의 경우 주기마다 자신의 필터를 재구성하고 parent노드에게 전송
		while(1)
		{
			global_switch = MYREFRESH;
			shadow_filter->zeroFilter();

			reset(shadow_filter);

			global_switch = NOREFRESH;
			//test_filter();

			my_filter->setFilter(shadow_filter->filter);
			send_refresh(parent->get_ip_num(), shadow_filter->filter);
			sleep(REFRESH_FREQUENCY);			
		}
	}
}

void Ilms::cmd_run()
{
	std::string c;

	IDPAddress eid_parser;

	while(std::cin >> c)
	{
		// mdb의 모든 id정보 출력
		if(c == "show")
		{
			std::cout << "--------------------" << std::endl;
			leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());
			for(it->SeekToFirst(); it->Valid(); it->Next())
			{
				std::string key = it->key().ToString();
				if(key.length() != ID_SIZE)
					continue;
				eid_parser.setBinary(key.c_str());
				std::cout << "[" << eid_parser.toString() << "] " << it->value().ToString() << std::endl;
			}
			assert(it->status().ok());	// Check for any errors found during the scan
			delete it;
			std::cout << "--------------------" << std::endl;
		}

		// mdb의 모든 id정보 제거
		else if(c == "crash")
		{
			leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());
			for(it->SeekToFirst(); it->Valid(); it->Next())
			{
				std::string key = it->key().ToString();
				if(key.length() != ID_SIZE)
					continue;
				leveldb::Status s = db->Delete(leveldb::WriteOptions(),leveldb::Slice(key.c_str(),ID_SIZE));
				assert(s.ok());
			}
			assert(it->status().ok());	// Check for any errors found during the scan
			delete it;

			std::cout << "delete complete" << std::endl;
		}

		// 특정 id를 찾아 출력
		else if(c == "pick")
		{
			std::string id;
			std::cin >> id;

			eid_parser = IDPAddress(QString(id.c_str()));

			char buf[BUF_SIZE];
			eid_parser.toBinary(buf);

			std::string ret;
			if(search(buf,ID_SIZE,ret))
				std::cout << "[" << id.c_str() << "] " << ret << std::endl;
			else
				std::cout << "[" << id.c_str() << "] " << "Not Found" << std::endl;
		}
	}
}

void Ilms::print_log(const char *id, const char *mode, const char *state, unsigned char vlen, const char *value)
{
	std::cout << "ID : " << eid.toString() << ", ";
	if(vlen && *value)
		std::cout << "LOC : " << value << ", ";
	std::cout << mode << " " << state;
	std::cout << std::endl;
}

void Ilms::send_node(unsigned int ip_num,const char *buf,int len)
{
	// ip가 0인 경우(가상필터)는 버퍼를 전송하지 않음
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

void Ilms::send_refresh(unsigned int ip_num, unsigned char *filter)
{
	if(ip_num == 0)
		return;

	int sd = socket(PF_INET, SOCK_STREAM, 0);
	if(sd == -1)
	{
		perror("sock open");
		return;
	}

	struct sockaddr_in clnt_adr;

	memset(&clnt_adr,0,sizeof(clnt_adr));
	clnt_adr.sin_family = AF_INET;
	clnt_adr.sin_addr.s_addr = ip_num;
	clnt_adr.sin_port = htons(REFRESH_PORT);

	if(connect(sd, (struct sockaddr *)&clnt_adr, sizeof(clnt_adr)) == -1)
	{
		perror("connect");
		return;
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

int Ilms::send_child(char *id)
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

int Ilms::send_child(unsigned int ip_num, char *id)
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

int Ilms::send_peer(char *id)
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

void Ilms::send_id(unsigned int ip_num, char *id, const char *ret, int len)
{
	char buf[BUF_SIZE];
	int pos = 0;
	buf[pos++] = REQ_SUCCESS;
	for(int i=0;i<ID_SIZE;i++)
		buf[pos++] = id[i];

	buf[pos++] = LOC_LOOKUP;
	buf[pos++] = len;
	for(int i=0; i < len; i++)
		buf[pos++] = ret[i];
	this->send_node(ip_num, buf, pos);
}

void Ilms::loc_process(unsigned int ip_num, char *id, char mode, unsigned char vlen, char *value, std::string& ret)
{
	bool find = false;
	if(mode == LOC_LOOKUP)
	{
		send_id(ip_num,id,ret.c_str(),ret.length());
		print_log(id, modes[(int)mode], "Success", vlen, value);
		print_log(id, modes[(int)mode], "Response", vlen, value);
		return;
	}
	else if(mode == LOC_SET)
	{
		if(ret.back() == '\0')
			ret.back() = ':';
		else if(ret.back() != ':')
			ret += ":";
		ret += value;
		insert(id,ID_SIZE,ret.c_str(),ret.size());
		find = true;
	}
	else if(mode == LOC_SUB)
	{
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
					loc = "";
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
		insert(id,ID_SIZE,res.c_str(),res.length());
	}
	else if(mode == LOC_REP)
	{
		insert(id,ID_SIZE,value,vlen);
		find = true;
	}

	if(find == false)
	{
		sc.buf[0] = REQ_FAIL;
		print_log(id, modes[(int)mode], "Fail", vlen, value);
	}
	else
	{
		sc.buf[0] = REQ_SUCCESS;
		print_log(id, modes[(int)mode], "Success", vlen, value);
	}
	print_log(id, modes[(int)mode], "Response", vlen, value);
	sc.buf[ID_SIZE+1] = mode;
	sc.buf[ID_SIZE+2] = vlen;
	for(size_t i=0; i < vlen; i++)
		sc.buf[ID_SIZE+3+i] = value[i];
	sc.len = ID_SIZE + 4 + vlen;
	this->send_node(ip_num, sc.buf, sc.len);
}

/*
 * 블룸필터 데이터 갱신
 * 부모노드에도 추가해야 하므로 버퍼그대로 전송
 */

void Ilms::proc_bf_update(unsigned int ip_num)
{
	char *id;
	if(!sc.next_value(id,ID_SIZE))
		return;

	bool find = false;
	for(unsigned int i=0; i < child.size(); i++)
	{
		if(ip_num == child[i].get_ip_num())
		{
			child_filter[i]->insert(id);
			find = true;

			if(global_switch == CHILDREFRESH)
				shadow_filter->insert(id);
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

void Ilms::proc_lookup(unsigned int ip_num)
{
	char *id;
	if(!sc.next_value(id,ID_SIZE))
		return;

	eid.setBinary(id);

	unsigned int ip_org_num;
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

	print_log(id, modes[(int)mode], "Request", vlen, value);

	my_filter->getBitArray(bitArray,id);
	if(my_filter->lookBitArray(bitArray))
	{
		std::string ret;
		if(search(id,ID_SIZE,ret))
		{
			loc_process(ip_org_num, id, mode, vlen, value, ret);
			return;
		}
	}
	unsigned int depth = ntohl(*(unsigned int *)p_depth);

	int count = 0;

	*(unsigned int *)p_depth = htonl(depth+1);

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
		print_log(id, modes[(int)mode], "Forward", vlen, value);
		insert(id,ID_SIZE+4, (char *)&count, sizeof(count));
	}
	else
	{
		*(unsigned int *)p_depth = htonl(depth);

		*up_down = MARK_UP;

		if(parent->get_ip_num())
		{
			if(depth > 0)
			{
				print_log(id, modes[(int)mode], "Send Nack", vlen, value);
				sc.buf[0] = CMD_LOOKUP_NACK;
			}
			else
			{
				print_log(id, modes[(int)mode], "Forward", vlen, value);
				sc.buf[0] = CMD_LOOKUP;
			}
			
			this->send_node(parent->get_ip_num(), sc.buf, sc.len);
		}
		else
		{
			print_log(id, modes[(int)mode], "Fail", vlen, value);
			print_log(id, modes[(int)mode], "Response", vlen, value);
			sc.buf[0] = REQ_FAIL;
			for(int i=1+ID_SIZE+4+1+4; i < sc.len; i++)
				sc.buf[i-9] = sc.buf[i];
			sc.len -= 9;
			this->send_node(ip_org_num, sc.buf, sc.len);
			return;
		}
	}
}


/*
 * 데이터 검색 실패. 노드에 false positive가 존재하는 상황
 * 검색 단계에서 갱신한 count값을 가지고 모든 노드에서 검색 실패했는지 여부를 판단함
 * 모든 노드에서 실패했다면 parent노드로 그 사실을 알림
 * 하나라도 성공했다면 데이터를 찾은것이므로 검색 종료
 */

void Ilms::proc_lookup_nack()
{
	char *id = sc.get_cur();
	char *p_depth = sc.get_cur() + ID_SIZE + 4 + 1;
	unsigned int depth = ntohl(*(unsigned int *)p_depth);

	eid.setBinary(id);

	char *mode = sc.get_cur() + ID_SIZE + 4 + 1 + 4;
	print_log(id, modes[(int)*mode], "Receive Nack");

	std::string ret;

	if(!search(id,ID_SIZE+4,ret))
		return;

	int count = *(int *)ret.c_str();

	if(--count == 0)
	{
		remove(id,ID_SIZE+4);

		*(unsigned int *)p_depth = htonl(depth-1);

		if(parent->get_ip_num())
		{
			if(depth == 1)
			{
				print_log(id, modes[(int)*mode], "Forward");
				sc.buf[0] = CMD_LOOKUP;
			}
			else
				print_log(id, modes[(int)*mode], "Send Nack");
			this->send_node(parent->get_ip_num(), sc.buf, sc.len);
		}
		else
		{
			print_log(id, modes[(int)*mode], "Fail");
			print_log(id, modes[(int)*mode], "Response");
			sc.buf[0] = REQ_FAIL;
			unsigned int ip_org_num = *(unsigned int *)&sc.buf[1+ID_SIZE];
			for(int i=1+ID_SIZE+4+1+4; i < sc.len; i++)
				sc.buf[i-9] = sc.buf[i];
			sc.len -= 9;
			this->send_node(ip_org_num, sc.buf, sc.len);
		}
		return;
	}

	insert(id,ID_SIZE+4, (char *)&count, sizeof(count));
}


void Ilms::req_id_register(unsigned int ip_num)
{
	char *id;
	char *value;

	if(!sc.next_value(id,ID_SIZE))
		return;

	if(!sc.next_value(value))
		return;

	eid.setBinary(id);

	print_log(id, "REG", "Request", *(unsigned char*)(value-1), value);

	my_filter->getBitArray(bitArray,id);
	if(my_filter->lookBitArray(bitArray))
	{
		std::string ret;
		if(search(id,ID_SIZE,ret))
		{
			print_log(id, "REG", "Fail", *(unsigned char*)(value-1), value);
			print_log(id, "REG", "Response", *(unsigned char*)(value-1), value);
			sc.buf[0] = REQ_FAIL;
			sc.buf[1+ID_SIZE] = 0;
			sc.len = 1 + ID_SIZE + 1;
			this->send_node(ip_num, sc.buf, sc.len);
			return;
		}
	}
	my_filter->insert(id);

	std::string loc = ":";
	loc += value;

	insert(id,ID_SIZE, loc.c_str(), loc.size());

	if(global_switch == MYREFRESH)
		shadow_filter->insert(id);

	sc.buf[0] = CMD_BF_UPDATE;
	this->send_node(parent->get_ip_num(), sc.buf, sc.len);

	sc.buf[0] = PEER_BF_UPDATE;
	for(unsigned int i=0; i < peering.size(); i++)
		this->send_node(peering[i].get_ip_num(), sc.buf, sc.len);

	print_log(id, "REG", "Success", *(unsigned char*)(value-1), value);
	print_log(id, "REG", "Response", *(unsigned char*)(value-1), value);

	sc.buf[0] = REQ_SUCCESS;
	sc.buf[1+ID_SIZE] = 0;
	sc.len = 1 + ID_SIZE + 1;
	this->send_node(ip_num, sc.buf, sc.len);
}

/*
 * 클라이언트로 부터의 데이터 검색 요청
 * 클라이언트는 자식이 아니므로 자식관련 처리과정이 생략됨
 */

void Ilms::req_lookup(unsigned int ip_num)
{
	char *id;
	if(!sc.next_value(id,ID_SIZE))
		return;

	eid.setBinary(id);

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

	for(int i=0; i < ID_SIZE; i++)
	{
		*pos = id[i];
		pos++;
	}

	*(unsigned int *)pos = ip_num;
	pos += 4;

	char *up_down = pos;
	pos++;

	char *p_depth = pos;
	*(unsigned int *)p_depth = 0;
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

	value = sc.buf + 1 + ID_SIZE + 4 + 1 + 4 + 1 + 1;

	up_down = sc.buf + 1 + ID_SIZE + 4;
	p_depth = sc.buf + 1 + ID_SIZE + 4 + 1;

	sc = Scanner(sc.buf, len);

	print_log(id, modes[(int)mode], "Request", vlen, value);

	my_filter->getBitArray(bitArray,id);
	if(my_filter->lookBitArray(bitArray))
	{
		std::string ret;
		if(search(id,ID_SIZE,ret))
		{
			loc_process(ip_num, id, mode, vlen, value, ret);
			return;
		}
	}

	*up_down = MARK_DOWN;
	*(unsigned int *)p_depth = htonl(1);

	int count = 0;

	sc.buf[0] = PEER_LOOKUP;
	count += send_peer(id);

	sc.buf[0] = CMD_LOOKUP;
	count += send_child(id);

	if(count)
	{
		print_log(id, modes[(int)mode], "Forward", vlen, value);
		insert(id,ID_SIZE+4, (char *)&count, sizeof(count));
	}
	else
	{
		*up_down = MARK_UP;
		*(unsigned int *)p_depth = 0;
		if(parent->get_ip_num())
		{
			print_log(id, modes[(int)mode], "Forward", vlen, value);
			this->send_node(parent->get_ip_num(), sc.buf, sc.len);
		}
		else
		{
			print_log(id, modes[(int)mode], "Fail");
			print_log(id, modes[(int)mode], "Response");
			sc.buf[0] = REQ_FAIL;
			for(int i=1+ID_SIZE+4+1+4; i < sc.len; i++)
				sc.buf[i-9] = sc.buf[i];
			sc.len -= 9;
			this->send_node(ip_num, sc.buf, sc.len);
		}
	}
}

/*
 * 클라이언트로 부터의 데이터 삭제 요청
 * 클라이언트는 자식이 아니므로 자식관련 처리과정이 생략됨
 */

void Ilms::req_id_deregister(unsigned int ip_num)
{
	char *id;

	if(!sc.next_value(id,ID_SIZE))
		return;

	eid.setBinary(id);

	print_log(id, "DEL", "Request");
	bool find = false;
	if(my_filter->lookup(id))
		if(remove(id, ID_SIZE))
			find = true;

	if(find)
	{
		sc.buf[0] = REQ_SUCCESS;
		print_log(id, "DEL", "Success");
	}
	else
	{
		sc.buf[0] = REQ_FAIL;
		print_log(id, "DEL", "Fail");
	}
	sc.buf[sc.len++] = 1;
	this->send_node(ip_num, sc.buf, sc.len);
	print_log(id, "DEL", "Response");
}

void Ilms::peer_bf_update(unsigned int ip_num)
{
	char *id;
	if(!sc.next_value(id,ID_SIZE))
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

void Ilms::peer_lookup(unsigned int ip_num)
{
	char *id;
	if(!sc.next_value(id,ID_SIZE))
		return;

	eid.setBinary(id);

	unsigned int ip_org_num;
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
		if(search(id,ID_SIZE,ret))
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
	if(!sc.next_value(id,ID_SIZE))
		return;

	eid.setBinary(id);

	unsigned int ip_org_num;
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
		if(search(id,ID_SIZE,ret))
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
	if(!sc.next_value(id,ID_SIZE))
		return;

	eid.setBinary(id);

	unsigned int ip_org_num;
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
		if(search(id,ID_SIZE,ret))
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

void Ilms::insert(const char *key,int klen,const char *val,int vlen)
{
	leveldb::Status s = db->Put(leveldb::WriteOptions(),leveldb::Slice(key,klen),leveldb::Slice(val,vlen));
	assert(s.ok());
}

/*
 * DB에 데이터 검색
 */

bool Ilms::search(const char *key,int klen,std::string &val)
{
	leveldb::Status s = db->Get(leveldb::ReadOptions(),leveldb::Slice(key,klen),&val);
	return s.ok();
}

/*
 * DB에 데이터 삭제
 * 존재여부 확인을 위해 한번 검색함(효율 문제)
 */

bool Ilms::remove(const char *key,int klen)
{
	std::string ret;
	if(!search(key,klen,ret))
		return false;

	leveldb::Status s = db->Delete(leveldb::WriteOptions(),leveldb::Slice(key,klen));
	assert(s.ok());
	return true;
}

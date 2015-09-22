#include <cstring>

#include "ilmscli.h"

#define REQ_ID_REGISTER				0x20
#define REQ_LOOKUP						0x21
#define REQ_ID_DEREGISTER			0x22
#define REQ_SUCCESS						0x23
#define REQ_FAIL							0x24

/*
 * Ilms 클라이언트 생성자
 * 요청을 전달할 노드의 ip를 설정하고 자신의 소켓 초기화
 */
const char* modes[] = {
	"GET", "SET", "SUB", "REP"
};

IlmsCli::IlmsCli(string ip)
{
	this->ip = ip;

	sock = socket(PF_INET, SOCK_DGRAM, 0);
	if(sock == -1)
		error_handling("UDP socket creation error");

	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(MYPORT);

	struct timeval tv;
	tv.tv_sec = TIME_LIMIT;
	tv.tv_usec = 0;

	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

	if(bind(sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
		error_handling("bind() error");

}

/*
 * ip설정. 다른노드로 이동할 때 쓰임
 */

void IlmsCli::set_ip(const string& ip)
{
	this->ip = ip;
}

int IlmsCli::print_response(char *buf, int len)
{
	if(len == 1 + ID_SIZE + 1)
	{
		std::cout << "ID : " << buf+1 << ", ";
		std::cout << (buf[1+ID_SIZE] == 0? "REG " : "DEL ");
		std::cout << (buf[0] == REQ_SUCCESS? "Success" : "Fail") << std::endl;
		return buf[1+ID_SIZE] == 0? REQ_ID_REGISTER: REQ_ID_DEREGISTER;
	}
	else
	{
		std::cout << "ID : " << buf+1 << ", ";
		int mode = buf[1+ID_SIZE];
		if(mode)
		{
			if(buf[1+ID_SIZE+1])
				std::cout << "LOC : " << buf+1+ID_SIZE+1+1 << ", ";
			std::cout << modes[mode] << " " << (buf[0] == REQ_SUCCESS? "Success" : "Fail") << std::endl;
		}
		else
		{
			if(buf[0] == REQ_FAIL)
				std::cout << modes[0] << " No ID" << std::endl;
			else
				std::cout << "LOC : " << (buf[1+ID_SIZE+1] < 2? "No LOC" : buf+1+ID_SIZE+1+1) << std::endl;
		}
		return mode;
	}
}

bool IlmsCli::req_id_register(const string& id, const string& loc)
{
	char header[BUF_SIZE];
	char response[BUF_SIZE];
	int len=0;

	header[len++] = REQ_ID_REGISTER;
	for(size_t i=0; i < ID_SIZE; i++)
	{
		if(i < id.size())
			header[len++] = id[i];
		else
			header[len++] = 0;
	}

	header[len] = loc.length() + 1;
	strcpy(header+len+1, loc.c_str());

	len += header[len] + 1;

	this->send(header,len);
	for(int i=0; i < RETRANSMISSION_FREQUENCY; i++)
	{
		int rlen = this->recieve(response);
		if(rlen > 0)
		{
			int prot = print_response(response, rlen);
			if(prot == REQ_ID_REGISTER)
			{
				bool find = true;
				for(size_t i=0; i < ID_SIZE; i++)
				{
					if(i < id.size() && id[i] != response[1+i])
					{
						find = false;
						break;
					}
					else if(i >= id.size() && response[1+i])
					{
						find = false;
						break;
					}
				}
				if(find)
					return 1;
				i--;
			}
		}
		else if(i != RETRANSMISSION_FREQUENCY - 1)
		{
			std::cout << "ID : " << id << ", REG Retransmission..." << std::endl;
			this->send(header,len);
		}
	}
	std::cout << "ID : " << id << ", REG Fail" << std::endl;
	return false;
}


/*
 * 데이터 추가 요청
 */

int IlmsCli::req_loc_update(char mode, const string& id, const string& loc)
{
	char header[BUF_SIZE];
	char response[BUF_SIZE];
	int len=0;

	header[len++] = REQ_LOOKUP;
	for(size_t i=0; i < ID_SIZE; i++)
	{
		if(i < id.size())
			header[len++] = id[i];
		else
			header[len++] = 0;
	}
	header[len++] = mode;
	header[len] = loc.length() + 1;
	strcpy(header+len+1, loc.c_str());

	len += header[len] + 1;

	this->send(header,len);
	for(int i=0; i < RETRANSMISSION_FREQUENCY; i++)
	{
		int rlen = this->recieve(response);
		if(rlen > 0)
		{
			int prot = print_response(response, rlen);
			if(mode == prot)
			{
				bool find = true;
				for(size_t i=0; i < ID_SIZE; i++)
				{
					if(i < id.size() && id[i] != response[1+i])
					{
						find = false;
						break;
					}
					else if(i >= id.size() && response[1+i])
					{
						find = false;
						break;
					}
				}

				for(size_t i=0; i < loc.size(); i++)
				{
					if(response[1+ID_SIZE+2 + i] != loc[i])
					{
						find = false;
						break;
					}
				}
				if(find)
					return 1;
				i--;
			}
		}
		else if(i != RETRANSMISSION_FREQUENCY - 1)
		{
			std::cout << "ID : " << id << ", LOC : " << loc << ", " << modes[(int)mode] << " Retransmission..." << std::endl;
			this->send(header,len);
		}
	}
	std::cout << "ID : " << id << ", LOC : " << loc << ", " << modes[(int)mode] << " Fail" << std::endl;
	return false;
}

/*
 * 데이터 검색 요청
 * 검색 결과를 받을때까지 기다림
 */

int IlmsCli::req_lookup(const string& id)
{
	char header[BUF_SIZE];
	char response[BUF_SIZE];
	int len=0;

	header[len++] = REQ_LOOKUP;
	for(size_t i=0; i < ID_SIZE; i++)
	{
		if(i < id.size())
			header[len++] = id[i];
		else
			header[len++] = 0;
	}

	header[len++] = LOC_LOOKUP;
	header[len++] = 0;


	this->send(header,len);
	for(int i=0; i < RETRANSMISSION_FREQUENCY; i++)
	{
		int rlen = this->recieve(response);
		if(rlen > 0)
		{
			int prot = print_response(response, rlen);
			if(prot == LOC_LOOKUP)
			{
				bool find = true;
				for(size_t i=0; i < ID_SIZE; i++)
				{
					if(i < id.size() && id[i] != response[1+i])
					{
						find = false;
						break;
					}
					else if(i >= id.size() && response[1+i])
					{
						find = false;
						break;
					}
				}

				if(find)
					return 1;
				i--;
			}
		}
		else if(i != RETRANSMISSION_FREQUENCY - 1)
		{
			std::cout << "ID : " << id << ", " << modes[0] << " Retransmission..." << std::endl;
			this->send(header,len);
		}
	}
	std::cout << "ID : " << id << ", " << modes[0] << " Fail" << std::endl;
	return len;
}

/*
 * 데이터 삭제 요청
 */

bool IlmsCli::req_id_deregister(const string& id)
{
	char header[BUF_SIZE];
	char response[BUF_SIZE];
	int len=0;

	header[len++] = REQ_ID_DEREGISTER;
	for(size_t i=0; i < ID_SIZE; i++)
	{
		if(i < id.size())
			header[len++] = id[i];
		else
			header[len++] = 0;
	}

	this->send(header,len);
	for(int i=0; i < RETRANSMISSION_FREQUENCY; i++)
	{
		int rlen = this->recieve(response);
		if(rlen > 0)
		{
			int prot = print_response(response, rlen);
			if(prot == REQ_ID_DEREGISTER)
			{
				bool find = true;
				for(size_t i=0; i < ID_SIZE; i++)
				{
					if(i < id.size() && id[i] != response[1+i])
					{
						find = false;
						break;
					}
					else if(i >= id.size() && response[1+i])
					{
						find = false;
						break;
					}
				}
				if(find)
					return 1;
				i--;
			}
		}
		else if(i != RETRANSMISSION_FREQUENCY - 1)
		{
			std::cout << "ID : " << id << ", DEL Retransmission..." << std::endl;
			this->send(header,len);
		}
	}
	std::cout << "ID : " << id << ", DEL Fail" << std::endl;
	return false;
}

/*
 * 패킷 전송 함수
 */

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

/*
 * 패킷을 받아오는 함수
 */

int IlmsCli::recieve(char *buf)
{
	struct sockaddr_in clnt_adr;
	socklen_t clnt_adr_sz = sizeof(clnt_adr);

	return recvfrom(sock, buf, BUF_SIZE, 0,(struct sockaddr*)&clnt_adr, &clnt_adr_sz);
}

/*
 * 에러 처리 함수
 */

void IlmsCli::error_handling(const char *message)
{
	fputs(message,stderr);
	fputc('\n',stderr);
	exit(1);
}
#include <cstdio>
#include <cstdlib>
#include <arpa/inet.h>

#include "topology.h"


/* 
 * Node 클래스 맴버함수
 */

/*
 * 노드 생성자
 */

Node::Node(std::string ip)
{
	this->ip = ip;
	ip_num = inet_addr(ip.c_str());
}

/*
 * ip를 반환하는 함수
 * 데이터 복사 오버헤드를 줄이기 위해 내부 배열을 포인터로 전달
 */

const char *Node::get_ip()
{
	return ip.c_str();
}

/*
 * 길이를 반환하는 함수
 */

int Node::length()
{
	return ip.length();
}

/*
 * ip를 int형으로 바꾼 결과를 리턴하는 함수
 * 비교를 더 빠르게 하기 위함
 */

unsigned long Node::get_ip_num()
{
	return ip_num;
}

/*
 * Tree 클래스 맴버함수
 */

/*
 * 트리 생성자
 * 파일로부터 트리 정보를 받아와야 함
 * try catch 구현 필요
 */

Tree::Tree()
{
	FILE *fp = fopen(TREE_PATH,"r");

	if(!fp)
	{
		error_handling("fopen failed");
	}

	char buf[256];
	int num;
	
	// 탑 노드
	if(fscanf(fp,"%d",&num) != 1)
	{
		error_handling("fgets failed");
	}

	for(int i=0;i<num;i++)
	{
		if(fscanf(fp,"%s",buf) != 1)
		{
			error_handling("fgets failed");
		}
		top.push_back(Node(buf));
	}

	

	// 자식 노드
	if(fscanf(fp,"%d ",&num) != 1)
	{
		error_handling("fscanf failed");
	}

	for(int i=0;i<num;i++)
	{
		if(fscanf(fp,"%s",buf) != 1)
		{
			error_handling("fgets failed");
		}
		child.push_back(Node(buf));
	}

	// down peer 노드
	if(fscanf(fp,"%d ",&num) != 1)
	{
		error_handling("fscanf failed");
	}

	for(int i=0;i<num;i++)
	{
		if(fscanf(fp,"%s",buf) != 1)
		{
			error_handling("fgets failed");
		}
		down_peer.push_back(Node(buf));
	}


	// up peer 노드
	if(fscanf(fp,"%d ",&num) != 1)
	{
		error_handling("fscanf failed");
	}

	for(int i=0;i<num;i++)
	{
		if(fscanf(fp,"%s",buf) != 1)
		{
			error_handling("fgets failed");
		}
		up_peer.push_back(Node(buf));
	}

	fclose(fp);
}

/*
 * 트리 소멸자
 * 할당 해제
 */

Tree::~Tree()
{
	delete parent;
}


/*
 * 부모 노드를 반환하는 함수
 * 부모 노드는 하나라고 가정하기 때문에 Node 클래스 리턴
 */

Node Tree::get_parent()
{
	return *parent;
}

/*
 * 자식 노드를 반환하는 함수
 * 자식은 여러개 있을 수 있기 때문에 벡터로 리턴
 */

std::vector<Node> Tree::get_child()
{
	return child;
}

/*
 * 피어노드를 반환하는 함수
 * 피어노드는 여러개 있을 수 있어 벡터로 리턴
 */

std::vector<Node> Tree::get_peer()
{
	return down_peer;
}


/*
 * 오류가 발생시 해당 오류를 출력하고 종료
 */

void Tree::error_handling(const char *message)
{
	fputs(message,stderr);
	fputc('\n',stderr);
	exit(1);
}
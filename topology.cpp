#include <cstdio>
#include <cstdlib>
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
}

/*
 * ip를 반환하는 함수
 */

const char *Node::getIp()
{
	return ip.c_str();
}

int Node::length()
{
	return ip.length();
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
	
	if(!fgets(buf,sizeof(buf),fp))
	{
		error_handling("fgets failed");
	}

	parent = new Node(buf);

	int num;
	if(fscanf(fp,"%d ",&num) != 1)
	{
		error_handling("fscanf failed");
	}

	for(int i=0;i<num;i++)
	{
		if(!fgets(buf,sizeof(buf),fp))
		{
			error_handling("fgets failed");
		}
		child.push_back(Node(buf));
	}

	if(fscanf(fp,"%d ",&num) != 1)
	{
		error_handling("fscanf failed");
	}

	for(int i=0;i<num;i++)
	{
		if(!fgets(buf,sizeof(buf),fp))
		{
			error_handling("fgets failed");
		}
		peer.push_back(Node(buf));
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

Node Tree::getParent()
{
	return *parent;
}

/*
 * 자식 노드를 반환하는 함수
 * 자식은 여러개 있을 수 있기 때문에 벡터로 리턴
 */

std::vector<Node> Tree::getChild()
{
	return child;
}

/*
 * 피어노드를 반환하는 함수
 * 피어노드는 여러개 있을 수 있어 벡터로 리턴
 */

std::vector<Node> Tree::getPeer()
{
	return peer;
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
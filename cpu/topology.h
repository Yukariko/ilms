#ifndef TOPOLOGY_H
#define TOPOLOGY_H

#include <iostream>
#include <vector>

#define TREE_PATH "./tree.conf"

/*
 * 노드 클래스
 */

class Node
{
public:
	Node(std::string ip);

	// ip를 string 형태로 반환
	const char *get_ip();

	// string 형태의 길이 반환
	int length();

	// 4바이트 형태의 ip 반환
	unsigned int get_ip_num();

private:
	// string 형태의 ip
	std::string ip;
	// 4바이트 형태의 ip
	unsigned int ip_num;
};

/*
 * 트리 클래스
 * vector를 통해 parent, child, peer노드를 가지고 있음
 */

class Tree
{
public:
	Tree();
	~Tree();

protected:
	void error_handling(const char *message);

	Node *parent;
	static std::vector<Node> child;
	static std::vector<Node> peered;
	static std::vector<Node> peering;
};

#endif
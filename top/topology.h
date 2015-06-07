#ifndef TOPOLOGY_H
#define TOPOLOGY_H

#include <iostream>
#include <vector>

#define TREE_PATH "./tree.conf"

class Node
{
public:
	Node(std::string ip);
	const char *get_ip();
	int length();
	unsigned long get_ip_num();

private:
	std::string ip;
	unsigned long ip_num;
};

class Tree
{
public:
	Tree();
	~Tree();

protected:
	void error_handling(const char *message);

	static std::vector<Node> top;
	static std::vector<Node> child;
	static std::vector<Node> down_peer;
	static std::vector<Node> up_peer;
};

#endif
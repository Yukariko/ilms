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
	Node get_parent();
	std::vector<Node> get_child();
	std::vector<Node> get_peer();

protected:
	void error_handling(const char *message);

	std::vector<Node> top;
	std::vector<Node> child;
	std::vector<Node> down_peer;
	std::vector<Node> up_peer;
};

#endif
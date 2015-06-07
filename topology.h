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

	Node *parent;
	static std::vector<Node> child;
	static std::vector<Node> down_peer;
	static std::vector<Node> up_peer;
};

std::vector<Node> Tree::child;
std::vector<Node> Tree::down_peer;
std::vector<Node> Tree::up_peer;

#endif
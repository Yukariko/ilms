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
	int get_ip_num();

private:
	std::string ip;
	int ip_num;
};

class Tree
{
public:
	Tree();
	~Tree();
	Node getParent();
	std::vector<Node> getChild();
	std::vector<Node> getPeer();

protected:
	void error_handling(const char *message);

	// is parent Node always one?
	Node *parent;
	std::vector<Node> child;
	std::vector<Node> peer;

};

#endif
#ifndef TOPOLOGY_H
#define TOPOLOGY_H

#include <iostream>
#include <vector>

#define TREE_PATH "./tree.conf"

class Node
{
public:
	Node(std::string ip);
	const char *getIp();
	int length();


private:
	std::string ip;
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
	Node *me;
	Node *parent;
	std::vector<Node> child;
	std::vector<Node> peer;

};

#endif
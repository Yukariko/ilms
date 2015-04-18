#ifndef TOPOLOGY_H
#define TOPOLOGY_H

#include <iostream>
#include <vector>

#define TREE_PATH "./tree.conf"

class Node
{
public:
	Node(){}
	Node(std::string ip) : this->ip(ip) {}
	std::string getIp();

private:
	std::string ip;
};

class Tree
{
public:
	Tree();
	Node getParent();
	std::vector<Node> getChild();
	std::vector<Node> getPeer();

private:
	// is parent Node always one?
	Node parent;
	std::vector<Node> child;
	std::vector<Node> peer;

};

#endif
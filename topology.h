#ifndef TOPOLOGY_H
#define TOPOLOGY_H

#include <iostream>
#include <vector>

class Node
{
public:
	Node(std::string ip);
	std::string getIp();

private:
	std::string ip;
};

class Tree
{
public:
	Tree();
	Node getParent();
	std::vector<Node> child;

private:
	// is parent Node always one?
	Node parent;
	std::vector<Node> child;
	std::vector<Node> peer;

};

#endif
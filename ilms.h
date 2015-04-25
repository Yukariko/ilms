#ifndef ILMS_H
#define ILMS_H

#include "bloomfilter.h"
#include "topology.h"

class Ilms
{
public:
	Ilms();
	~Ilms();
	int start();
	
private:
	Tree tree;
	Bloomfilter *myFilter;
	Bloomfilter *childFilter;
};

#endif
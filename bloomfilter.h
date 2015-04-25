#ifndef BLOOMFILTER_H
#define BLOOMFILTER_H

class Bloomfilter
{
public:
	Bloomfilter(long long size, int numHash,long long (**hash)(long long));
	~Bloomfilter();
	void insert(long long data);
	bool lookup(long long data);

private:
	unsigned char *field;
	long long size;
	int numHash;
	long long (**hash)(long long);
};

#endif
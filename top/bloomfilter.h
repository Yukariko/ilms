#ifndef BLOOMFILTER_H
#define BLOOMFILTER_H

class Bloomfilter
{
public:
	Bloomfilter(long long size, int numHash,long long (**hash)(long long));
	~Bloomfilter();
	void insert(char *data);
	bool lookup(char *data);

private:
	unsigned char *field;
	long long size;
	int numHash;
	long long (**hash)(long long);
};

#endif
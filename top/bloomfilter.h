#ifndef BLOOMFILTER_H
#define BLOOMFILTER_H

class Bloomfilter
{
public:
	Bloomfilter(long long size, int numHash,long long (**hash)(char *));
	~Bloomfilter();
	void insert(char *data);
	bool lookup(char *data);
	bool lookBitArray(long long *bitArray);
	void getBitArray(long long *bitArray, char *data);

private:
	unsigned char *field;
	long long size;
	int numHash;
	long long (**hash)(char *);
};

#endif
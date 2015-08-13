#ifndef BLOOMFILTER_H
#define BLOOMFILTER_H

class Bloomfilter
{
public:
	Bloomfilter(long long size, int numHash,long long (**hash)(const char *));
	~Bloomfilter();
	void insert(const char *data);
	bool lookup(const char *data);

	void setFilter(unsigned char *hostFilter);
	void mergeFilter(unsigned char *hostFilter);
	void zeroFilter();

	bool lookBitArray(long long *bitArray);
	void getBitArray(long long *bitArray, const char *data);

private:
	unsigned char *filter;
	long long size;
	int numHash;
	long long (**hash)(const char *);
};

#endif
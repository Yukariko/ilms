#ifndef SCANNER_H
#define SCANNER_H

class Scanner
{
public:
	Scanner();
	Scanner(char *buf, int len);

	bool next_value(char &val);
	bool next_value(unsigned char &val);
	bool next_value(int &val);
	bool next_value(long long &val);
	bool next_value(char *&val);

	char *get_cur();

	bool isEnd(int pos = 0);

	char *buf;
	int len;

private:
	char *cur;
	char *end;
};

#endif
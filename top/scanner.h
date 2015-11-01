#ifndef SCANNER_H
#define SCANNER_H

/*
 * 스캐너 클래스
 * binary stream에서 값을 쉽게 읽어오기 위해 사용
 */

class Scanner
{
public:
	Scanner();
	Scanner(char *buf, int len);
	Scanner& operator=(const Scanner& there);

	// 버퍼에서 타입에 맞게 저장
	bool next_value(char &val);
	bool next_value(unsigned char &val);
	bool next_value(int &val);
	bool next_value(unsigned int &val);
	bool next_value(long long &val);
	bool next_value(char *&val);
	bool next_value(char *&val, int len);

	// 버퍼의 현재 위치 반환
	char *get_cur();

	// 현재 버퍼에서 pos뒤가 끝인지를 반환
	bool isEnd(int pos = 0);

	// 버퍼
	char *buf;
	// 버퍼 길이
	int len;

private:
	// 현재 버퍼 위치
	char *cur;
	// 버퍼 끝 위치
	char *end;
};

#endif
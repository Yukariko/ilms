#include <iostream>
#include <cstdio>
#include <cstring>

#include "ilmscli.h"

using namespace std;

const char *oper[] = {"IP","REG","SET","GET","REP","SUB","DELETE","EXIT",NULL};
enum {IP,REG,SET,GET,REP,SUB,DELETE,EXIT,ERROR};


int main()
{
	string cmd;
	IlmsCli ilms("0");
	while(cin >> cmd)
	{
		string value;
		string data;

		int op = 0;
		while(oper[op] && cmd != oper[op])
			op++;

		bool ret = false;
		int len = 0;

		switch(op)
		{
		case IP:
			if(cin >> value)
				ilms.set_ip(value);
			break;
		case REG:
			if(cin >> data)
			{
				getline(cin, value);
				if(value.size())
					ret = ilms.req_id_register(data, value.substr(1));
				else
					ret = ilms.req_id_register(data, value);
			}
			break;
		case SET:
			if(cin >> data >> value)
				ret = ilms.req_loc_update(0, data, value);
			break;
		case GET:
			if(cin >> data)
			{
				len = ilms.req_lookup(data, value);
				if(len == 1)
					cout << "No LOC" << endl;
				else if(len == -1)
					cout << "No ID" << endl;
				else
					cout << value << endl;
			}
			break;
		case REP:
			if(cin >> data >> value)
				ret = ilms.req_loc_update(2, data, value);
			break;
		case SUB:
			if(cin >> data >> value)
				ret = ilms.req_loc_update(1, data, value);
			break;
		case DELETE:
			if(cin >> data)
				ret = ilms.req_id_deregister(data);
			break;
		case EXIT:
			return 0;
		default:
			cout << "ERROR" << endl;
		}

		if(op == REG || op == SET || op == REP || op == SUB || op == DELETE)
			cout << oper[op] << " " << (ret? "SUCCESS": "FAIL") << endl;
	}
	return 0;
}
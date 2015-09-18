#include <iostream>
#include <cstdio>
#include <cstring>

#include "ilmscli.h"

using namespace std;

const char *oper[] = {"IP","REG","SET","GET","REP","SUB","DEL","EXIT",NULL};
enum {IP,REG,SET,GET,REP,SUB,DEL,EXIT,ERROR};

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

		int ret = false;
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
				ret = ilms.req_loc_update(LOC_SET, data, value);
			break;
		case GET:
			if(cin >> data)
				len = ilms.req_lookup(data);
			break;
		case REP:
			if(cin >> data >> value)
				ret = ilms.req_loc_update(LOC_REP, data, value);
			break;
		case SUB:
			if(cin >> data >> value)
				ret = ilms.req_loc_update(LOC_SUB, data, value);
			break;
		case DEL:
			if(cin >> data)
				ret = ilms.req_id_deregister(data);
			break;
		case EXIT:
			return 0;
		default:
			cout << "ERROR" << endl;
		}

		//if(op == REG || op == SET || op == REP || op == SUB || op == DELETE)
		//	cout << oper[op] << " " << (ret == 1? "SUCCESS": ret == 0? "FAIL" : "No ID") << endl;
	}
	return 0;
}
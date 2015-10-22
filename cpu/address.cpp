//======================================
// Name         : addresses.cpp
// Author       : Woojik Chun
// Version      : 0.1
// Description  : address related function definitions
//=======================================

#include "address.h"
#include <netinet/in.h>
#include <iostream>
using namespace std;

const char *IDPAddress::toString() {
	return _data_str.constData();
}

QByteArray IDPAddress::toByteArray()
{
	QByteArray writeData;

	QDataStream stream(&writeData, QIODevice::WriteOnly);
	stream.setVersion(QDataStream::Qt_4_8);

	unsigned char temp;
	memcpy(&temp, &_data, 1);

	stream << temp;
	stream << _data.rsv;
	stream << _data.scope;

	for(int i = 0 ; i < 5 ; i++)
		stream << _data.hash[i];

	return writeData;
}

void IDPAddress::setData(const QByteArray &data)
{
	QByteArray readData = data;

	QDataStream stream(&readData, QIODevice::ReadWrite);
	stream.setVersion(QDataStream::Qt_4_8);

	unsigned char temp;
	stream >> temp;
	memcpy(&_data, &temp, 1);
	stream >> _data.rsv;
	stream >> _data.scope;

	for(int i = 0 ; i < 5 ; i++)
		stream >> _data.hash[i];
	setString();
}

IDPAddress::IDPAddress()
{
	_data.ver = 1 & 0xF;
	_data.mode = 0;
	_data.rsv = 0;
	_data.scope = 0;
	for(int i = 0; i < 5; i++)
		_data.hash[i] = 0;
	setString();
}

IDPAddress::IDPAddress(const QString &str) {
	unsigned int val[4];
	val[0] = 1; val[1] = 0; val[2] = 0; val[3] = 0;
	bool ok;

	idpid_t *id = (reinterpret_cast<idpid_t *>(&_data));

	QStringList slist = str.split(":", QString::KeepEmptyParts);
	QStringList hlist = slist[0].split(".", QString::SkipEmptyParts);

	for(int i = 0; i < hlist.size(); i++) {
		val[i] = hlist[i].toInt(&ok, 16);
	}

	id->ver = val[0] & 0xF;
	id->mode = val[1] & 0xF;
	id->rsv = val[2] & 0xFF;
	id->scope = val[3] & 0xFFFF;

	int index = 5;

	while (slist.size() > 7) slist.removeLast();


	for(int i = slist.size() - 1; i > 1; i--)
	{
		id->hash[--index] = slist[i].toInt(&ok, 16);
	}

	while(index > 0)
	{
		id->hash[--index] = 0;
	}
	setString();
}

bool IDPAddress::operator ==(const IDPAddress &id)
{
	return ( _data.ver == id._data.ver
			 && _data.mode == id._data.mode
			 && _data.rsv == id._data.rsv
			 && _data.scope == id._data.scope
			 && _data.hash[0] == id._data.hash[0]
			 && _data.hash[1] == id._data.hash[1]
			 && _data.hash[2] == id._data.hash[2]
			 && _data.hash[3] == id._data.hash[3]
			 && _data.hash[4] == id._data.hash[4] );
}

void IDPAddress::toBinary(char *buf)
{
	memcpy(buf, &_data, sizeof(_data));
}
void IDPAddress::setBinary(char *buf)
{
	memcpy(&_data, buf, sizeof(_data));
	setString();
}

void IDPAddress::setString()
{
	QStringList buf;
	buf << QString("%1.%2.%3.%4")
		   .arg(_data.ver, 0, 16).arg(_data.mode, 0, 16)
		   .arg(_data.rsv, 2, 16, QChar('0')).arg(_data.scope, 4, 16, QChar('0')).toUpper();

	if(_data.hash[0] == 0)
		buf << ":";
	else buf << QString(":%1").arg(_data.hash[0], 8, 16, QChar('0')).toUpper();
	for(int i = 1; i < 4; i++) {
		if(_data.hash[i])
			buf << QString(":%1").arg(_data.hash[i], 8, 16, QChar('0')).toUpper();
	}
	buf << QString(":%1").arg(_data.hash[4], 8, 16, QChar('0')).toUpper();
	QString res = buf.join("");
	_data_str = res.toUtf8();
}

int IDPAddress::getSize()
{
	return _data_str.size();
}
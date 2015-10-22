#ifndef ADDRESSES_H
#define ADDRESSES_H
#include <QString>
#include <QStringList>

#pragma pack(1)
typedef struct IDP_id {
	unsigned mode:4;
	unsigned ver:4;
	unsigned char rsv;
	unsigned short scope;
	unsigned int hash[5];
} idpid_t;

class IDPAddress
{
public:
	IDPAddress();
	IDPAddress(const QString &str);
	void setString();
	const char * toString();
	int getSize();
	QByteArray toByteArray();
	void toBinary(char *buf);
	void setBinary(const char *buf);
	void setData(const QByteArray &data);
	bool operator ==(const IDPAddress &id);


private:
	idpid_t _data;
	QByteArray _data_str;
};

typedef IDPAddress idpaddress_t;
#endif // ADDRESSES_H



#include <iostream>
#include <algorithm>

#include <leveldb/db.h>
#include <leveldb/write_batch.h>

#define DB_PATH "./db"
#define DATA_SIZE 24

using namespace std;

int main()
{
	leveldb::DB* db;
	leveldb::Options options;
	
	options.create_if_missing = true;
	leveldb::Status status = leveldb::DB::Open(options, DB_PATH, &db);
	assert(status.ok());

	leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());
	for(it->SeekToFirst(); it->Valid(); it->Next())
	{
		std::string key = it->key().ToString();
		if(key.length() != DATA_SIZE)
			continue;
		cout << it->key().ToString() << ": " << it->value().ToString() << endl;
	}
	assert(it->status().ok());	// Check for any errors found during the scan
	delete it;
	return 0;
}
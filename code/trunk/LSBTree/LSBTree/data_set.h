#pragma once


#include <map>
#include <cstdio>

class Data_Set {

private:
	int _n;
	int _d;
	int *_data;
	std::map<const int, int*> _id_to_index_map;

public:
	Data_Set(char* path);
	~Data_Set();

public:
	int* at(const int id);
	int get_d() const;
};



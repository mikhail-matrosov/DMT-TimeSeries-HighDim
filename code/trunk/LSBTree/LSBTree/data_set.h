#pragma once


#include <map>
#include <cstdio>


/**
 *	Description:
 *		Class stores spatial data(points) and allows access by id.
*/
class Data_Set {

protected:
	int _n;		// cardinality
	int _d;		// dimensionality
	int _t;		// maximal absolute value
	int *_data; // data array in memory

	std::map<const int, int*> _id_to_index_map;
	std::map<const int, int> _number_to_id_map;

public:
	Data_Set(char* path);
	virtual	~Data_Set();

public:
	int get_id(const int number);
	int* at(const int id);
	int* operator [](int number) const;

	int len() const;
	int get_d() const;
	virtual int cost() const;
};


/**
 *	Description:
 *		Class stores spatial data(points), allows access by id
 *		and simulates BTree that indexes data by its id.
 *		Gives rough IO cost estimation.
*/
class Indexed_Data_Set: public Data_Set {

protected:
	int _B;		// memory block size
	int _cost;	// access cost

public:
	Indexed_Data_Set(char* path, int B);
	~Indexed_Data_Set();

public:
	int cost() const;
};
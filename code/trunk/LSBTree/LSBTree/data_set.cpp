#include "data_set.h"


Data_Set::Data_Set(char* path){
	FILE* ifile = fopen(path, "r");

	if (NULL == ifile){
		throw -1;
	}

	fscanf(ifile, "%d %d\n", &_n, &_d);
	_data = new int[_n * _d];
	
	// i ~ point number
	for(int i = 0; i < _n; i++){

		int id;
		fscanf(ifile, "%d", &id);

		// set map's entry
		_id_to_index_map[id] = (_data + i * _d);

		// reads coordiantes of current point
		for (int coor = 0; coor < _d; coor++){
			fscanf(ifile, " %d", (_data + i * _d + coor));
		}

		fscanf(ifile, "\n");
	}

	fclose(ifile);
}


Data_Set::~Data_Set(){
	free(_data);
}


int* Data_Set::at(const int id){
	return _id_to_index_map[id];
}

int Data_Set::get_d() const {
	return _d;
}
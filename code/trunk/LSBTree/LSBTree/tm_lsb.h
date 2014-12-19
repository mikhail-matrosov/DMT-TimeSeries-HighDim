#pragma once


#include <queue>


#include "../../lsb/lsb.h"
#include "data_set.h"


# define ERROR_EXCEPTION -1;


class TM_LSB: public LSB{

	/* variables for tm_knn */
private:
	int	_numTrees;

	LSBbiPtr* lptrs;
	LSBbiPtr* rptrs;
	LSB_Hentry *_rslt;

	BinHeap* hp;	
	int	** qz;  
	float * qg; 
	
	LSBentry* e;
	LSBentry* qe;
	LSB_Hentry* he;

	int block;
	B_Node* nd;
	B_Node* oldnd;

	int ret;
	bool lescape;
	int	follow;

	BinHeapEntry* bhe;
	bool again;
	bool E1;

	float knnDist;
	float old_knnDist;

	int	limit;
	int	readCnt;    
	int	rsltCnt;

	int	thisLevel;
	int	thisTreeId;
	int	pos;

	float tm_knnDist;
	float tm_old_knnDist;
	LSB_Hentry *tm_rslt;
	LSB_Hentry tm_he;

private:
	Data_Set* _data_set;
	
	int _k;
	int *_q;

	int _tm_d;
	int* _tm_q;

protected:
	void init(Data_Set* data_set, int* tm_q, int* q, int numTrees, int k);
	void first_stage();
	void second_stage();
	void tm_second_stage();
	void tm_updateknn(LSB_Hentry * _rslt, LSB_Hentry *_he, int _k);
	void tm_recycle();

public:
	TM_LSB(): hp(NULL), qg(NULL), qz(NULL), e(NULL), qe(NULL), he(NULL), block(-1),
			  nd(NULL), oldnd(NULL), ret(0), lescape(false), follow(-1), bhe(NULL),
			  again(false), E1(true), knnDist(-1), old_knnDist(-1), limit(-1),
			  readCnt(0), rsltCnt(0), thisLevel(-1), thisTreeId(-1), pos(-1){};

public:
	int	tm_knn(Data_Set* data_set, int* tm_q, int *q, int L, int k, LSB_Hentry* rslt, LSB_Hentry* transformed_rslt);
};
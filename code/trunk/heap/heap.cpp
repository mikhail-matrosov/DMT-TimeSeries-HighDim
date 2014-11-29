/* this file implements the class Heap */

#include "heap.h"
//#include "../func/gendef.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

//------------------------------------------
HeapEntry::HeapEntry()
{
}
//------------------------------------------
HeapEntry::~HeapEntry()
{
}
//------------------------------------------
void HeapEntry::init_HeapEntry(int _dim)
{
	dim = _dim;
}
//------------------------------------------
void HeapEntry::copy(HeapEntry *_he)
{
	key = _he -> key;
	level = _he -> level;
	son = _he -> son;
}
//------------------------------------------
//------------------------------------------
//------------------------------------------
Heap::Heap()
{
	cont = NULL;
}
//------------------------------------------
Heap::~Heap()
{
	printf("maximum heap size in execution = %d\n", maxused);
	
	if (cont)
	{
		for (int i = 0; i < hsize + 1; i ++)
			delete cont[i];
		delete [] cont;
	}

	cont = NULL;
}
//------------------------------------------
void Heap::enter(HeapEntry *_he, int _pos)
//this function enters a new entry into the heap at position _pos
{

	for (int i = used - 1; i >= _pos; i --)
	{
		cont[i + 1]->copy(cont[i]);
	}
	cont[_pos]->copy(_he);
	used ++;

	if (maxused<used)
		maxused=used;
}
/*****************************************************************
This function enheaps an entry. It does NOT destroy the input entry.

Parameters:
- _he: the heap entry being en-heaped

Output: 
- None

Coded by yufei tao
*****************************************************************/

void Heap::insert(HeapEntry *_he)
{
	int pos = used;  //pos is the position _he will be inserted

	enter(_he, pos);
	// now perform swapping
	pos++;
	int parent = pos;
	while (parent != 1)
	{
		int child = parent;
		parent /= 2;
		if (cont[parent - 1]->key > cont[child - 1]->key)
		{
			HeapEntry *tmp_he = new_one_HeapEntry();
			tmp_he -> init_HeapEntry(cont[parent - 1]->dim);
			tmp_he -> copy(cont[parent - 1]);
			cont[parent - 1]->copy(cont[child - 1]);
			cont[child - 1]->copy(tmp_he);
			delete tmp_he;
		}
		else 
			parent = 1;
	}

	if (used > hsize)  
		//this is why the heap size is initiated with one more place than hsize (to facilitate
		//coding)
	{
		printf("heap exceeded...\n", true);
		exit(1);
	}
}
//------------------------------------------
bool Heap::remove(HeapEntry *_he)
//this function deheaps an entry. the return value indicates whether successful: false
//means heap is already empty
{
	if (used==0) 
		return false;
	_he -> copy(cont[0]);
	used--;
	cont[0]->copy(cont[used]);
	int parent = 1;
	while (2 * parent <= used)
	{
		int child = 2 * parent;
		if (2 * parent + 1 > used)
			child = 2 * parent;
		else
			if (cont[2 * parent - 1]->key < cont[2 * parent]->key)
				child = 2 * parent;
			else 
				child = 2 * parent + 1;

		if (cont[parent - 1]->key > cont[child - 1]->key)
		{
			HeapEntry *the = new_one_HeapEntry();
			the -> init_HeapEntry(cont[parent - 1]->dim);
			the -> copy(cont[parent - 1]);
			cont[parent - 1]->copy(cont[child - 1]);
			cont[child - 1]->copy(the);
			delete the;
			parent = child; 
		}
		else
			parent = used;
	}

	//if (!check())
	//	printf("testing...\n");
	return true;
};
//------------------------------------------
void Heap::init(int _dim, int _hsize)
{
	if (cont)
	{
		for (int i = 0; i < hsize + 1; i ++)
			delete cont[i];
		delete [] cont;
	}
	hsize = _hsize;
	cont = new HeapEntryptr [hsize + 1];
	for (int i = 0; i < hsize + 1; i ++)
	{
		cont[i] = new_one_HeapEntry();
		cont[i]->init_HeapEntry(_dim);
	}
	used = 0;
	maxused=0;
}

/*****************************************************************
this function checks the integrity of the heap. it is an auxiliary
function for debugging

Coded by Yufei Tao 09/01/02
*****************************************************************/

bool Heap::check()
{
	for (int i = 0; i < used; i ++)
	{
		if (cont[i]->son < 0)
			return false;
	}
	return true;
}

/*****************************************************************
this function initiates a new heap entry;

Parameters:
none

Return:
The heap entry

Coded by Yufei Tao 09/01/02
*****************************************************************/

HeapEntry * Heap::new_one_HeapEntry()
{
	HeapEntry *he = new HeapEntry();
	return he;
}

	
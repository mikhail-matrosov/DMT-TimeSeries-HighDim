/* this file contains implementation of the class Heap */
#ifndef HEAP_H
#define HEAP_H

#define MAX_HEAP_SIZE 100000

class HeapEntry;
class Heap;

class HeapEntry
{
public:
	int dim;
	int level;
	int son;
	float key;

	//-----functions-----
	HeapEntry();
	virtual ~HeapEntry();
	virtual void init_HeapEntry(int _dim);
	virtual void copy(HeapEntry *_he);
};

typedef HeapEntry * HeapEntryptr;

class Heap
{
public:
	int b;            // needed by HB for access condition
	int hsize;        // the heap size
	int used;         // number of used places
	int maxused;
	HeapEntryptr *cont;  // content of the heap

	//-----functions-----
	Heap();
	virtual ~Heap();
	virtual bool check();
	virtual void enter(HeapEntry *_he, int _pos);
	virtual void init(int _dim, int _hsize=MAX_HEAP_SIZE);
	virtual void insert(HeapEntry *_he);
	virtual HeapEntry * new_one_HeapEntry();
	virtual bool remove(HeapEntry *_he);
};

#endif
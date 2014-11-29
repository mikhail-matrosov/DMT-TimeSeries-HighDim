#ifndef __GADGET_H
#define __GADGET_H

#include <stdio.h>
#include "../heap/binheap.h"

//----- typedefs -----
	
typedef FILE * FILEptr;
typedef void * voidptr;

//----- macros -----

#define min(a, b)	(((a) < (b)) ? (a) : (b))
#define max(a, b)	(((a) > (b)) ? (a) : (b))
#define FLOATZERO	1e-6
#define MAXREAL		1e20

//----- functions -----

void	blank_print			(int _n);	
int		compfloats			(float _v1, float _v2);
void	error				(char *_msg, bool _exit);
void	get_leading_folder	(char *_path, char *_folder);				
void	getFNameFromPath	(char *_path, char *_fname);
char	getnextChar			(char **_s);
void	getnextWord			(char **_s, char *_w);
bool	is_pow_of_2			(int _v);
float	l2_dist_int			(int *_p1, int *_p2, int _dim);		
void	printINT_in_BIN		(int _n, int _m);							

//----- classes -----

struct ExtSort_heap_entry_data
{
	int		file_id;
	voidptr * elemadd;
};

class ExtSort
{
public:
	int n;														//mem size in the # of elements
	char *src_fname;											//source file name
	char *tar_fname;											//target file name
	char *working_folder;
	int (*compare_func)(const void *_elemadd1, const void *_elemadd2);

	ExtSort();
	virtual ~ExtSort();

	//----- functions to overload -----
	virtual void destroy_elem(void **_elem) = 0;
	virtual void * new_elem() = 0;
	virtual bool read_elem(FILE *_fp, void * _elem) = 0;
	virtual void write_elem(FILE *_fp, void * _elem) = 0;
	//---------------------------------

	virtual void esort();
	virtual int get_initial_runs();
	virtual void get_run_fname(int _step, int _run, char *_fname);
	virtual void init(int _n, char *_src_fname, char *_tar_fname);
	virtual int merge_runs(int _pass, int _run_num);
};

class ExtSortBinHeap :public BinHeap
{
public:
	int (* ext_sort_comp_func)(const void *_e1, const void *_e2);

	virtual int compare(const void *_e1, const void *_e2);
};

class exExtSort :public ExtSort
{
public:
	int keysize;

	virtual void destroy_elem(void **_elem);
	virtual void * new_elem();
	virtual bool read_elem(FILE *_fp, void * _elem);
	virtual void write_elem(FILE *_fp, void * _elem);
};

#endif
/* this file implements class B_Tree */

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <fstream>
#include "b-tree.h"
#include "b-node.h"
#include "b-entry.h"
#include "b-def.h"
#include "../heap/heap.h"
#include "../blockfile/blk_file.h"
#include "../gadget/gadget.h"

/*****************************************************************
Use this constructor to create an empty b-tree 
para:


Coded by Yufei Tao 30/12/01
*****************************************************************/

B_Tree::B_Tree()
{
	quiet = 8;

	root_ptr = NULL;

	for (int i = 0; i < 100; i ++)
		debug_info[i] = -1;

	emergency = false;
}

/*****************************************************************
loads the tree from a tree file

para:
- fname: 
- c:

Coded by Yufei Tao 31 July 08
*****************************************************************/

void B_Tree::init_restore(char *_fname, Cache *_c)
{
	char *fname = new char[strlen(_fname) + 10]; //allow 9 characters for extension name
	strcpy(fname, _fname);
	add_fname_ext(fname);

	//--=== testing if the file exists ===--
	FILE *fp = fopen(fname, "r");
	if (!fp)
	{
		printf("tree file %s does not exist\n", fname);
		delete []fname;
		error("", true);
	}

	fclose(fp);
	//--=== done testing the file existence ===--

    file = new BlockFile(fname, 0);
    cache = _c;

	root_ptr = NULL;
	char *header = new char [file -> get_blocklength()];
    file->read_header(header);
    read_header(header);
	delete [] header;
	delete [] fname;
}

/*****************************************************************
Coded by Yufei Tao, 3 aug 08
*****************************************************************/

void B_Tree::close()
{
	if (root_ptr)
	{
		delete root_ptr;
		root_ptr = NULL;
	}

	char *header = new char[file -> get_blocklength()];
    write_header(header);
    file -> set_header(header);
    delete [] header;

	if (cache)
      cache -> flush();

    if (file)
	{
		delete file;
		file = NULL;
	}
}

//-----------------------------------------------

B_Tree::~B_Tree()
{
	if (root_ptr) 
	{
		error("b_tree::~b_tree - root_ptr not null. you forgot to call close().\n", true);
	}

	if (file)
	{
		error("b_tree::~b_tree - file not null. you forgot to call close().\n", true);
	}
}

/*****************************************************************
This function deletes the root

Coded by Yufei Tao 29/12/01
*****************************************************************/

void B_Tree::delroot()
{
	delete root_ptr; root_ptr=NULL;
}

//-----------------------------------------------
/*
bool B_Tree::find_key(float _k)
{
	load_root();
	bool ret = root_ptr -> find_key(_k);
	delete root_ptr; root_ptr = NULL;
	return ret;
}
*/
//-----------------------------------------------

void B_Tree::insert(B_Entry *_new_e)
{
	if (_new_e->son != _new_e->leafson)
		error("B_Tree::insert - son and leafson unequal\n", true);

	load_root(); 

//--=== debugging ===--
/*
if (debug_info[0] == 0)
{
	delete root_ptr->entries[0];
	delete root_ptr->entries[1];
}
*/
//--=================--

	B_Node *new_nd = NULL;
	BINSRT c_ret = root_ptr -> insert(_new_e, &new_nd);
	if (c_ret == B_OVRFLW)
	{
		B_Node *new_root = new_one_node();
		new_root->init(root_ptr->level+1, this); 
			//the first paramemter just needs to be any value larger than 0
		new_root -> level = root_ptr -> level + 1;
		new_root -> add_new_child(root_ptr);
		new_root -> add_new_child(new_nd);
		root = new_root -> block;
		root_ptr = new_root;
	}

//--=== debugging ===--
/*
if (debug_info[0] == 0)
{
	delete root_ptr->entries[0];
	delete root_ptr->entries[1];
}
*/
//--=================--

	delroot();
}

//-----------------------------------------------

void B_Tree::load_root()
{
	if (root_ptr == NULL) 
	{
		root_ptr = new_one_node();
		root_ptr->init(this, root);
	}
}

//-----------------------------------------------
/*
void B_Tree::qry_agg(int _k1, int _k2)
{
	load_root();
	root_ptr -> qry_agg(_k1, _k2, B_MAXINT);
	delete root_ptr;
	root_ptr = NULL;
}
*/
//-----------------------------------------------

int B_Tree::read_header(char *_buf)
{
	int i = 0;
	memcpy(&root, _buf, sizeof(root));
	i += sizeof(root);

	memcpy(&keysize, &(_buf[i]), sizeof(keysize));
	i += sizeof(keysize);

	return i;
}

//-----------------------------------------------

int B_Tree::write_header(char *_buf)
{
	int i = 0;
	memcpy(_buf, &root, sizeof(root));
	i += sizeof(root);

	memcpy(&(_buf[i]), &keysize, sizeof(keysize));
	i += sizeof(keysize);

	return i;
}

/*****************************************************************
define the rank of an record o as the sum of the weights of all the records with keys smaller than o.key
this function returns the record whose rank is "just larger" than given a rank value,
para:
rank: 
coded by yufei tao june 2003
*****************************************************************/
/*
B_Entry *B_Tree::rank_find(float _rank)
{
	load_root();
	B_Entry *cret=root_ptr->rank_find(_rank);
	delroot();
	return cret;
}
*/

/*****************************************************************
this function deletes an entry from the b-tree

para:
- del_e: the entry to be deleted

the return value is false if the record is not found
coded by yufei tao june 2003
*****************************************************************/

bool B_Tree::delete_entry(B_Entry * _del_e)
{
	bool ret = true; 
	load_root();
	BDEL cret = root_ptr->delete_entry(_del_e);
	if (cret == B_UNDRFLW)
	{
		root=root_ptr->entries[0]->son;
	}
	delroot();
	if (cret == B_NOTFOUND)
		ret = false;
	return ret;
}

/*****************************************************************
bulkloads a tree. the records of the dataset must have been sorted
in ascending order of their keys

para:
- fname: dataset 

Coded by Yufei Tao, 4 aug 08
*****************************************************************/

void B_Tree::bulkload(char *_fname)
{

	FILE *fp = fopen(_fname, "r");
	if (!fp)
		error("could not open the data source file\n", true);
	
	if (quiet <= 5) 
		printf("building the leaf level...\n");

	int cnt = 0;													//cardinality

	int start_block = 0, end_block = 0;
	int prev_block = -1;
	bool first_node = true; 

	B_Node *act_nd = NULL;
	B_Node *prev_nd = NULL;

//--- + ---
B_Entry *prev_e = NULL;
//---------

	while (!feof(fp))
	{
		B_Entry *e = new_one_entry();
		e->init(this, 0);

		fread_next_entry(fp, e);

		if (!act_nd)
		{
			act_nd = new_one_node();
			act_nd->init(0, this);
//--- + ---
/*
if (act_nd->block == 11)
printf("testing...\n");
*/
//---------
			if (first_node)
			{
				first_node = false;
				start_block = act_nd->block;
			}
			else
			{
				act_nd->left_sibling = prev_nd->block;
				prev_nd->right_sibling = act_nd->block;
				delete prev_nd;
				prev_nd = NULL;
			}
			end_block = act_nd->block;
		}

//--- + ---
if (!prev_e)
{
	prev_e = new_one_entry();
	prev_e->init(this, 0);
}
else
{
	if (prev_e->compare(e) == 1 && prev_e->leafson < e->leafson)
	{
		error("Error in B_Tree::bulkload -- ordering error. Perhaps something wrong in sorting.\n", true);
	}
}
prev_e->set_from(e);
//---------


		act_nd->enter(e);
		e->close();
		delete e;

		if (act_nd->num_entries == act_nd->capacity)
		{
			prev_nd = act_nd;
			act_nd = NULL;
		}

		cnt ++;
	}

	if (prev_nd)
	{
		delete prev_nd;
		prev_nd = NULL;
	}

	if (act_nd)
	{
		delete act_nd;
		act_nd = NULL;
	}

//--- + ---
prev_e->close();
delete prev_e;
prev_e = NULL;
//---------

	fclose(fp);
	
	int current_level = 1; 
	int last_start_block = start_block, last_end_block = end_block;
	first_node = true;
	
	while (last_end_block > last_start_block)
	{
		if (quiet <= 5)
		{
			blank_print(5);
			printf("building nodes at level %d...\n", current_level);
		}

		for (int i = last_start_block; i <= last_end_block; i ++)
		{
			B_Node *child = new_one_node();
			child->init(this, i);

//--- + ---
/*
if (current_level == 1 && child->block == 1670)
printf("testing...\n");
*/
//---------

			B_Entry *e = new_one_entry();
			e->init(this, current_level);
			e->set_from_child(child);

//--- + ---
if (!prev_e)
{
	prev_e = new_one_entry();
	prev_e->init(this, current_level);
}
else
{
	if (prev_e->compare(e) == 1 && prev_e->leafson < e->leafson)
	{
		printf("Building level %d.\n", current_level);
		error("Error in B_Tree::bulkload -- ordering error in building intermediate levels.\n", true);
	}
}
prev_e->set_from(e);
//---------

			if (!act_nd)
			{
				act_nd = new_one_node();
				act_nd->init(current_level, this);
				if (first_node)
				{
					first_node = false;
					start_block = act_nd->block;
				}
				else
				{
					act_nd->left_sibling = prev_nd->block;
					prev_nd->right_sibling = act_nd->block;
					delete prev_nd;
					prev_nd = NULL;
				}
				end_block = act_nd->block;
			}

			act_nd->enter(e);
			e->close();
			delete e;

			if (act_nd->num_entries == act_nd->capacity)
			{
				prev_nd = act_nd;
				act_nd = NULL;
			} 

			delete child;
		}

		if (prev_nd)
		{
			delete prev_nd;
			prev_nd = NULL;
		}

		if (act_nd)
		{
			delete act_nd;
			act_nd = NULL;
		}

		last_start_block = start_block; last_end_block = end_block;
		current_level++;
		first_node = true;
//--- + ---
prev_e->close();
delete prev_e;
prev_e = NULL;
//---------
	}

	root = last_start_block; 
}

/*****************************************************************
bulkload a tree. difference from bulkload() is that here the 
sorted dataset is in memory.

-para-
ds:		dataset 
n:		cardinality

-return-
0		success
1		failure

-by-
yf tao

-last touch-
19 aug 09
*****************************************************************/

int B_Tree::bulkload2(void *_ds, int _n)
{
	int			ret				= 0;

	B_Entry		* e				= NULL;
	B_Node		* actNd			= NULL;
	B_Node		* child			= NULL;
	B_Node		* prevNd		= NULL;
	bool		firstNode		= false;
	int			cnt				= -1;
	int			currentLevel	= -1; 
	int			endBlock		= -1;
	int			i				= -1;
	int			lastStartBlock	= -1;
	int			lastEndBlock	= -1;
	int			prevBlock		= -1;
	int			startBlock		= -1;
	void		* ptr			= NULL;

	cnt = 0;													
	startBlock = 0;
	endBlock = 0;

	prevBlock = -1;
	firstNode = true; 

	ptr = _ds;

	for (i = 0; i < _n; i ++)
	{
		e = new_one_entry();
		e->init(this, 0);

		if (read_next_entry(&ptr, e))
		{
			ret = 1;
			goto recycle;
		}

		if (!actNd)
		{
			actNd = new_one_node();
			actNd->init(0, this);

			if (firstNode)
			{
				firstNode = false;
				startBlock = actNd->block;
			}
			else
			{
				actNd->left_sibling = prevNd->block;
				prevNd->right_sibling = actNd->block;
				delete prevNd;
				prevNd = NULL;
			}

			endBlock = actNd->block;
		}

		actNd->enter(e);

		e->close();
		delete e;

		if (actNd->num_entries == actNd->capacity)
		{
			prevNd = actNd;
			actNd = NULL;
		}
	}

	if (prevNd)
	{
		delete prevNd;
		prevNd = NULL;
	}

	if (actNd)
	{
		delete actNd;
		actNd = NULL;
	}

	currentLevel	= 1; 
	lastStartBlock	= startBlock; 
	lastEndBlock	= endBlock;

	while (lastEndBlock > lastStartBlock)
	{
		firstNode = true;

		for (i = lastStartBlock; i <= lastEndBlock; i ++)
		{
			child = new_one_node();
			child->init(this, i);

			e = new_one_entry();
			e->init(this, currentLevel);

			e->set_from_child(child);

			if (!actNd)
			{
				actNd = new_one_node();
				actNd->init(currentLevel, this);

				if (firstNode)
				{
					firstNode = false;
					startBlock = actNd->block;
				}
				else
				{
					actNd->left_sibling = prevNd->block;
					prevNd->right_sibling = actNd->block;

					delete prevNd;
					prevNd = NULL;
				}

				endBlock = actNd->block;
			}

			actNd->enter(e);

			e->close();
			delete e;
			e = NULL;

			if (actNd->num_entries == actNd->capacity)
			{
				prevNd = actNd;
				actNd = NULL;
			} 

			delete child;
			child = NULL;
		}

		if (prevNd)
		{
			delete prevNd;
			prevNd = NULL;
		}

		if (actNd)
		{
			delete actNd;
			actNd = NULL;
		}

		lastStartBlock = startBlock; 
		lastEndBlock = endBlock;

		currentLevel++;
	}

	root = lastStartBlock; 

recycle:

	if (e)
	{
		e->close();
		delete e;
	}

	if (prevNd)
	{
		delete prevNd;
		prevNd = NULL;
	}

	if (actNd)
	{
		delete actNd;
		actNd = NULL;
	}
	if (child)
	{
		delete child;
		child = NULL;
	}

	return ret;
}

/*****************************************************************
builds a tree from a data file.
record format: 
			id key_1 key_2 ... key_keysize

para:
- dsfname: the dataset file name

Coded by Yufei Tao 31 July 08
*****************************************************************/

void B_Tree::build_from_file(char *_dsname)
{
    B_Entry *d;
    FILE *fp;
    
	int record_count = 0;

    if((fp = fopen(_dsname,"r")) == NULL)
    {
		delete this;
		error("Could not open the data file", TRUE);
    }
    else
    {
		int id = 0;
		float info[10]; 

		while (!feof(fp))
		{
			record_count ++;

		if (record_count % 100 == 0)
		{
			printf("inserting object %d\n", record_count);
		}

//===== debugging =====
/*
if (record_count == 15)
{
printf("testing...\n");
debug_info[0] = 0;
}
*/
//=====================

		d = new_one_entry();
		d->init(this, 0);
				
		fread_next_entry(fp, d);

//d->key = 1000;

    	insert(d);
///*
		if (record_count % 100 == 0)
		{
			info[0] = 0;
			traverse(info);
			if (compfloats(info[0], (float) record_count) != 0)
				error("wrong number of objects in the tree\n", true);
		}
//*/
      }
    }

	fclose(fp); 
	printf("\n");
	
	delroot();
}

/*****************************************************************
retrieves the next entry from a data file

Coded by Yufei Tao, 31 July 08
*****************************************************************/

void B_Tree::fread_next_entry(FILE *_fp, B_Entry *_d)
{
	fscanf(_fp, "%d ", &(_d->son));
	for (int i = 0; i < keysize; i ++)
		fscanf(_fp, "%d", &(_d->key[i]));
	
	fscanf(_fp, "\n");

	_d->leafson = _d->son;
}

/*****************************************************************
This function adds an extension ".b" to the file name 

Parameters:
- _fname: the file name before adding the extension

Coded by Yufei Tao, 31 July 08
*****************************************************************/

void B_Tree::add_fname_ext(char * _fname)
{
	strcat(_fname, ".b");
}

/*****************************************************************
creates a new tree

Parameters:
- fname: the file name before adding the extension
- blength: page size
- c: 
- keysize:

Coded by Yufei Tao, 31 July 08
*****************************************************************/

void B_Tree::init(char *_fname, int _b_length, Cache *_c, int _keysize)
{
	char *fname = new char[strlen(_fname) + 10]; //allow 9 characters for extension name
	strcpy(fname, _fname);
	add_fname_ext(fname);

	FILE *fp = fopen(fname, "r");
	if (fp)
	{
		fclose(fp);
		printf("The file \"%s\" exists. Replace? (y/n)", fname);
		char c = getchar(); getchar();
		if (c != 'y' && c != 'Y') 
			error("", true);
		remove(fname);
	}

    file = new BlockFile(fname, _b_length);
    cache = _c;
	keysize = _keysize;

	//init the first node-----------------------------------------
	//we will deliberately waste a node so that all nodes' addresses are positive
    root_ptr = new_one_node();
	root_ptr->init(0, this);
    root = root_ptr->block;
	delroot();

	root_ptr = new_one_node();
	root_ptr->init(0, this);
	root = root_ptr->block;
	delroot();
	//------------------------------------------------------------

	delete []fname;
}

/*****************************************************************
DESCRIPTION
Traverse the tree and collect information.

PARAMETERS:
- _info (out): carries the collected info outside the function.
               Currently, _info[0] is the number of objects in the tree, _info[1] the number of nodes, and
			   _info[2] the node capacity. 

RETURN
1 -- Everything all right.
0 -- Error found.

KNOWN ISSUES:
None.

PROGRAMMER
Yufei Tao

DATE OF LAST UPDATE
18 Nov. 08
*****************************************************************/

int B_Tree::traverse(float *_info)
{
	last_leaf = -1;
	last_right_sibling = -1;

	load_root();
	_info[2] = (float) root_ptr->capacity;
	int ret = root_ptr->traverse(_info);
	delroot();

	return ret;
}

/*****************************************************************
by yufei tao, 31 july 08
*****************************************************************/

B_Entry *B_Tree::new_one_entry()
{
	B_Entry *e = new B_Entry();

	return e;
}

/*****************************************************************
by yufei tao, 31 july 08
*****************************************************************/

B_Node *B_Tree::new_one_node()
{
	B_Node *nd = new B_Node();

	return nd;
}
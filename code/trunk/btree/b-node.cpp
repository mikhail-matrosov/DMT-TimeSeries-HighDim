/* this file implements class B_Node */

#include <math.h>
#include <memory.h>
#include "b-node.h"
#include "b-entry.h"
#include "b-tree.h"
#include "b-def.h"
#include "../blockfile/blk_file.h"
#include "../blockfile/cache.h"
#include "../gadget/gadget.h"

//-----------------------------------------------

B_Node::B_Node()
{
	block = -1;
	capacity = -1;
	dirty = false;
	entries = NULL;
	left_sibling = right_sibling = -1;
	level = -1;
	my_tree = NULL;
	num_entries = -1;
}

/*****************************************************************
use this function to init a new node on disk

para:
- level
- B_Tree: the instance of the B-tree that this node belongs to

by yufei tao, 1 aug 08
*****************************************************************/

void B_Node::init(int _level, B_Tree *_B_Tree)
{
	my_tree = _B_Tree;
	int b_length = my_tree -> file -> get_blocklength();
	
	left_sibling = -1;
	right_sibling = -1;
	num_entries = 0;
	level = _level;

	B_Entry *e = new_one_entry();
	e->init(_B_Tree, _level);
	int entry_size = e->get_size(level);  
	e->close();
	delete e;

	capacity = (b_length - get_header_size()) / entry_size;
	if (capacity < 3)
	{
		printf("capacity = %d\n", capacity);
		error("b_node::init - capacity too small.\n", true);
	}
	entries = get_entries(capacity);

	char *blk = new char[b_length];
    block = my_tree -> file -> append_block(blk);
    delete [] blk;

	dirty = true;
}

/*****************************************************************
use this function to load a node from the disk disk

para:
- B_Tree: the instance of the B-tree that this node belongs to
- block: the address to load from

by yufei tao, 1 aug 08
*****************************************************************/

void B_Node::init(B_Tree *_B_Tree, int _block)
{
    my_tree = _B_Tree;
    num_entries = 0;
	dirty = false;

    int header_size = get_header_size();
	int b_len = my_tree -> file -> get_blocklength();
    
    block = _block;
    char *blk = new char[b_len];
    if (my_tree -> cache == NULL) // no cache
        my_tree -> file -> read_block(blk, block);
    else
        my_tree -> cache -> read_block(blk, block, my_tree);

	memcpy(&level, blk, sizeof(level));
	B_Entry *e = new_one_entry();
	e->init(_B_Tree, level);
	int entry_size = e->get_size(level);  
	e->close();
	delete e;

	capacity = (b_len - header_size) / entry_size;
	if (capacity < 3)
	{
		printf("capacity = %d\n", capacity);
		error("b_node::init - capcity too small.\n", true);
	}
	entries = get_entries(capacity);

	read_from_buffer(blk);
    delete [] blk;
}

//-----------------------------------------------

B_Node::~B_Node()
{
	char *buf;
    if (dirty)
    {
        buf = new char[my_tree -> file -> get_blocklength()];
        write_to_buffer(buf);

        if (my_tree -> cache == NULL) // no cache
            my_tree -> file-> write_block(buf, block);
        else
            my_tree -> cache -> write_block(buf, block, my_tree);

        delete [] buf;
    }

	if (entries)
	{
		for (int i = 0; i < capacity; i ++)
		{
			entries[i]->close();
			delete entries[i];
			entries[i] = NULL;
		}
	}

    delete [] entries;
	entries = NULL;
}

//-----------------------------------------------

void B_Node::add_new_child(B_Node *_cnd)
{
	B_Entry *e = new_one_entry();
	e->init(my_tree, level); 

	e->set_from_child(_cnd);

	enter(e);
	e->close();
	delete e;
	delete _cnd;
}

//-----------------------------------------------

bool B_Node::chk_ovrflw()
{
	if (num_entries == capacity - 1) return true;
	else return false;
}

//-----------------------------------------------

bool B_Node::chk_undrflw()
{
	if (my_tree->root_ptr->level==level)
		if (num_entries==1 && level>0)
			return true;
		else return false;
	if (num_entries <= (capacity-2)/2) return true;
	else return false;
}

//-----------------------------------------------

int B_Node::choose_subtree(B_Entry *_new_e)
{
	int follow = max_lesseq_key_pos(_new_e); 
	
	return follow;
}

//-----------------------------------------------

void B_Node::enter(B_Entry *_new_e)
{
	int pos = -1;
	
	pos = max_lesseq_key_pos(_new_e);
	pos ++;														//this is the position of the new key

	for (int i = num_entries; i > pos; i --)
	{
		entries[i]->set_from(entries[i - 1]);
	}

	entries[pos]->set_from(_new_e);
	num_entries ++;
	dirty = true;
}

//-----------------------------------------------
/*
bool B_Node::find_key(float _k)
{
	bool ret = false;

	if (level == 0)
	{
		for (int i = 0; i < num_entries; i ++)
			if (fabs(entries[i]->key - _k) < FLOATZERO)
			{ 
				ret = true; i = num_entries; 
			}
		return ret;
	}

	int follow = max_lesseq_key_pos(_k);
	if (follow != -1)
	{
		B_Node *succ = entries[follow]->get_son();
		ret = succ -> find_key(_k);
		entries[follow]->del_son();
	}

	return ret;
}
*/
//-----------------------------------------------

B_Entry ** B_Node::get_entries(int _cap)
{
	B_Entry **en = new B_Entryptr [_cap];
	for (int i = 0; i < _cap; i ++)
	{
		en[i] = new_one_entry();
		en[i]->init(my_tree, level);
	}

	return en;
}

//-----------------------------------------------

int B_Node::get_header_size()
{ 
	return sizeof(level) + sizeof(block) + sizeof(left_sibling) + sizeof(right_sibling);
}

//-----------------------------------------------

BINSRT B_Node::insert(B_Entry *_new_e, B_Node **_new_nd)
{
	BINSRT ret = B_NRML;
	if (level == 0)
	{
		enter(_new_e);  
		_new_e->close();
		delete _new_e;
		if (chk_ovrflw())
		{
			trt_ovrflw(_new_nd);
			ret = B_OVRFLW;
		}
		return ret;
	}

	int follow = choose_subtree(_new_e); 

	bool need_update = false;

	if (follow == -1)
	{
		need_update = true;
		follow = 0;
	}

	B_Node *succ = entries[follow]->get_son();  
	B_Node *new_nd = NULL;
	BINSRT c_ret = succ->insert(_new_e, &new_nd);
	
	if (need_update)
	{
		entries[follow]->set_from_child(succ);
		dirty = true;
	}

	//entries[follow].agg=succ->sumagg();
	//dirty=true;

	entries[follow]->del_son();  
	
	if (c_ret == B_OVRFLW)
		add_new_child(new_nd); 
	if (chk_ovrflw())
	{
		trt_ovrflw(_new_nd);
		ret = B_OVRFLW;
	}
	return ret;
}

//-----------------------------------------------
/*
int B_Node::max_lesseq_key_pos(float _key)
{
	int pos = -1;
	for (int i = num_entries - 1; i >= 0; i --)
		if (entries[i]->key <= _key)
		{ pos = i; i = -1;}
	return pos;
}
*/

/*****************************************************************
finds the entry that is just before the given (key, leafson) pair

para:
- key: 
- leafson:

Coded by Yufei Tao, 31 july 08
*****************************************************************/

int B_Node::max_lesseq_key_pos(B_Entry *_e)
{
	int pos = -1;
	for (int i = num_entries - 1; i >= 0; i --)
	{
		int rslt = entries[i]->compare(_e);
		if (rslt == -1 || rslt == 0)
		{
			pos = i;
			break;
		}
	}

	return pos;
}

/*****************************************************************
return:
- return the left sibling node if it exists. otherwise, null.

Coded by Yufei Tao, 31 july 08
*****************************************************************/

B_Node* B_Node::get_left_sibling()
{
	B_Node *ret = NULL;

	if (left_sibling != -1)
	{
		ret = new_one_node();
		ret->init(my_tree, left_sibling);
	}

	return ret;
}

/*****************************************************************
return:
- return the right sibling node if it exists. otherwise, null.

Coded by Yufei Tao, 31 july 08
*****************************************************************/

B_Node* B_Node::get_right_sibling()
{
	B_Node *ret = NULL;

	if (right_sibling != -1)
	{
		ret = new_one_node();
		ret->init(my_tree, right_sibling);
	}

	return ret;
}

/*****************************************************************
finds the entry to be followed in finding a key

para:
- _e: the entry containing the key

return:
- the entry id

Coded by Yufei Tao, 31 july 08
*****************************************************************/

int B_Node::ptqry_next_follow(B_Entry *_e)
{
	int pos = num_entries - 1;
	for (int i = 0; i < num_entries; i ++)
	{
		int rslt = entries[i]->compare_key(_e);
		if (rslt == 0)
		{
			pos = i;
			break;
		}
		else if (rslt == 1)
		{
			pos = i - 1;
			break;
		}
	}

	return pos;
}

//-----------------------------------------------

void B_Node::read_from_buffer(char *_buf)
{
	int i;
	memcpy(&level, _buf, sizeof(level));
	i = sizeof(level);
	memcpy(&num_entries, &_buf[i], sizeof(num_entries));
	i += sizeof(num_entries);
	memcpy(&left_sibling, &_buf[i], sizeof(left_sibling));
	i += sizeof(left_sibling);
	memcpy(&right_sibling, &_buf[i], sizeof(right_sibling));
	i += sizeof(right_sibling);

	for (int j = 0; j < num_entries; j ++)
	{
		entries[j]->read_from_buffer(&_buf[i]); 
		i += entries[j]->get_size(entries[j]->level);
	}
}

//-----------------------------------------------
/*
void B_Node::qry_agg(int _k1, int _k2, int _r_bnd)
{
	if (level == 0)
		return;
	for (int i = 0; i < num_entries; i ++)
	{
		int st = entries[i].key, ed;
		if (i < num_entries - 1) ed = entries[i + 1].key;
							else ed = _r_bnd;
		if (!(_k1 <= st && ed <= _k2) &&
			(max(_k1, st) <= min(_k2, ed)))
		{
			B_Node *succ = entries[i].get_son();
			succ -> qry_agg(_k1, _k2, ed);
			entries[i]. del_son();
		}
	}
}
*/
//-----------------------------------------------

void B_Node::rmv_entry(int _pos)
{
	for (int i = _pos; i < num_entries - 1; i ++)
	{
		entries[i]->set_from(entries[i + 1]);
	}
	num_entries--;
	dirty = true;
}

//-----------------------------------------------

void B_Node::trt_ovrflw(B_Node **_new_nd)
{
	*_new_nd = new_one_node();
	(*_new_nd)->init(level, my_tree);  
	(*_new_nd)->level = level;

	int i = (capacity - 1) / 2 ;
	while (i < num_entries)
	{
		(*_new_nd)->enter(entries[i]);
		rmv_entry(i);
	}

	(*_new_nd)->right_sibling = right_sibling;
	(*_new_nd)->left_sibling = block;
	right_sibling = (*_new_nd) -> block;

	if ((*_new_nd)->right_sibling != -1)
	{
		B_Node *nd = new_one_node();
		nd->init(my_tree, (*_new_nd)->right_sibling);
		nd->left_sibling = (*_new_nd)->block;
		nd->dirty = true;
		delete nd; 
	}
}

//-----------------------------------------------

void B_Node::write_to_buffer(char *_buf)
{
	int i;
	memcpy(_buf, &level, sizeof(level));
	i = sizeof(level);
	memcpy(&_buf[i], &num_entries, sizeof(num_entries));
	i += sizeof(num_entries);
	memcpy(&_buf[i], &left_sibling, sizeof(left_sibling));
	i += sizeof(left_sibling);
	memcpy(&_buf[i], &right_sibling, sizeof(right_sibling));
	i += sizeof(right_sibling);

	for (int j = 0; j < num_entries; j ++)
	{
		entries[j]->write_to_buffer(&_buf[i]); 
		i += entries[j]->get_size(entries[j]->level);
	}
}

//-----------------------------------------------

/*
float B_Node::sumagg()
{
	float ret=0;
	for (int i=0; i<num_entries; i++)
		ret += entries[i].agg;
	return ret;
}
*/

/*****************************************************************
define the rank of an record o as the sum of the weights of all the records with keys smaller than o.key
this function returns the record whose rank is "just larger" than given a rank value,
para:
rank: 
coded by yufei tao june 2003
*****************************************************************/
/*
B_Entry *B_Node::rank_find(float _rank)
{
	float acc_rank=0;
	if (level==0) 
	{
		for (int i=0; i<num_entries; i++)
		{
			acc_rank += entries[i].agg;
			if (acc_rank>=_rank)
			{
				B_Entry *e=new_one_entry();
				e->set_from(entries[i]);
				e->agg=acc_rank;
				return e;
			}
		}
		error("the entry not found\n", true);
		return NULL;
	}	
	for (int i=0; i<num_entries; i++)
	{
		acc_rank += entries[i].agg;
		if (acc_rank>=_rank)
		{
			acc_rank -= entries[i].agg;
			B_Node *succ = entries[i].get_son();
			B_Entry *cret = succ->rank_find(_rank-acc_rank);
			entries[i].del_son();
			if (cret)
			{
				cret->agg+=acc_rank;
				return cret;
			}
		}
	}
	return NULL;
}
*/

/*****************************************************************
this function deletes an entry from the b-tree

para:
- del_e: the entry to be deleted

coded by yufei tao june 2003
*****************************************************************/

BDEL B_Node::delete_entry(B_Entry *_del_e)
{
	BDEL ret = B_NOTFOUND;
	if (level == 0)
	{
		for (int i = 0; i < num_entries; i++)
		{
			if (entries[i]->equal_to(_del_e))
			{
				_del_e->close();
				delete _del_e;

				rmv_entry(i);
				if (chk_undrflw())
					ret = B_UNDRFLW;
				else
					ret = B_NONE;  
				break; 
			}
		}
		return ret;
	}
	
	int follow = choose_subtree(_del_e);

	if (follow == -1)
		error("follow = -1 in B_Node::delete_entry, meaning entry not found.\n", true);

	B_Node *succ = entries[follow]->get_son();
	BDEL cret = succ->delete_entry(_del_e);

	entries[follow]->set_from_child(succ);
	dirty = true;

	entries[follow]->del_son();
	
	if (cret == B_NONE)
		ret = B_NONE;
	if (cret == B_UNDRFLW)
	{
		trt_undrflw(follow);
		if (chk_undrflw())
			ret = B_UNDRFLW;
		else
			ret = B_NONE;
	}

	return ret;
}

/*****************************************************************
this function fixes an underflow by merging

para:
follow: the subscript of the non-leaf entry whose child node incurs an underflow
coded by yufei tao june 2003
*****************************************************************/

void B_Node::trt_undrflw(int _follow)
{
	int mergesub = _follow + 1;									//the subscript of the non-leaf entry to merge with
	if (_follow == num_entries - 1)
		mergesub = _follow - 1;
	
	B_Node *succ1 = entries[mergesub]->get_son();
	B_Node *succ2 = entries[_follow]->get_son();

	int totalnum = succ1->num_entries + succ2->num_entries;
	if (totalnum >= capacity-1)
	{
		//the merged node does not fit in one page.
		//remember succ1 points to mergesub and succ2 points to follow

		int n = succ1->num_entries; 
		if (mergesub > _follow)
		{
			for (int i = 0; i < totalnum / 2 - succ2->num_entries; i++)
			{
				succ2->enter(succ1->entries[0]);
				succ1->rmv_entry(0);
			}
		}
		else
		{
			for (int i = totalnum / 2; i < n; i++)
			{
				succ2->enter(succ1->entries[totalnum / 2]);
				succ1->rmv_entry(totalnum / 2);
			}
		}
		entries[mergesub]->set_from_child(succ1); 
		entries[_follow]->set_from_child(succ2);
		
		entries[mergesub]->del_son();
		entries[_follow]->del_son();
	}
	else
	{
		//copy all the entries from succ2 to succ1
		//remember succ1 points to mergesub and succ2 points to follow

		for (int i = 0; i < succ2->num_entries; i++)
		{
			succ1->enter(succ2->entries[i]);
		}

		//next, fix the sibling pointers

		if (_follow < mergesub)
		{
			succ1->left_sibling = succ2->left_sibling;

			if (succ1->left_sibling != -1)
			{
				B_Node *nd = new_one_node();
				nd->init(my_tree, succ1->left_sibling);
				nd->right_sibling = succ1->block;
				nd->dirty = true;
				delete nd;
			}
		}
		else
		{
			succ1->right_sibling = succ2->right_sibling;

			if (succ1->right_sibling != -1)
			{
				B_Node *nd = new_one_node();
				nd->init(my_tree, succ1->right_sibling);
				nd->left_sibling = succ1->block;
				nd->dirty = true;
				delete nd;
			} 
		}

		succ1->dirty = true;

		entries[mergesub]->set_from_child(succ1);

		entries[mergesub]->del_son();
		entries[_follow]->del_son();

		rmv_entry(_follow);
	}

	dirty = true;
}

/*****************************************************************
traverse the suB_Tree 

para:
- info: an array for taking info out of the function

return value: return 1 if everything is all right. otherwise 0.

by yufei tao, 31 july 08
*****************************************************************/

int B_Node::traverse(float *_info)
{
	int ret = 1;
	_info[1] ++;     //Counting the number of nodes.

	//first check if the ordering of the entries is correct

	for (int i = 0; i < num_entries - 1; i ++)
	{
		if (entries[i]->compare(entries[i + 1]) != -1)
		{
			printf("Entry ordering error\n");
			printf("Block %d, level %d, entry %d\n", block, level, i);
			error("I am aborting...\n", true);
		}
	}

	if (level > 0)
	{
		for (int i = 0; i < num_entries; i ++)
		{
			B_Node *nd = entries[i]->get_son();

			bool equal = true; 

			for (int j = 0; j < my_tree->keysize; j ++)
				if (compfloats((float) entries[i]->key[j], (float) nd->entries[0]->key[j]) != 0)
				{
					equal = false; break;
				}

			if (!equal ||
				entries[i]->leafson != nd->entries[0]->leafson)
			{
				printf("Entry content error\n");
				printf("Block %d, level %d, entry %d\n", block, level, i);
				error("I am aborting...\n", true);
			}
			int cret = nd->traverse(_info);

			if (cret == 0)
				ret = 0;
			entries[i]->del_son();
		}
	}
	else
	{
		_info[0] += num_entries;     //Counting the number of objects.

		//Now check the sibling pointers.

		if (left_sibling != my_tree->last_leaf)
		{
			printf("Left sibling ptr error\n");
			//printf("Block %d, level %d, entry %d\n", block, level, i);
			printf("Block %d, level %d, entry %d\n", block, level, 777);
			printf("Capacity = %d\n", capacity);
			printf("Seen %d entries so far.\n", (int) _info[0]);
			error("I am aborting...\n", true);
		}

		if (my_tree->last_right_sibling != -1 && my_tree->last_right_sibling != block)
		{
			printf("Right sibling ptr error\n");
			//printf("Block %d, level %d, entry %d\n", block, level, i);
			printf("Block %d, level %d, entry %d\n", block, level, 777);
			error("I am aborting...\n", true);
		}
		
		my_tree->last_leaf = block;
		my_tree->last_right_sibling = right_sibling;
	}

	return ret;
}

/*****************************************************************
by yufei tao, 31 july 08
*****************************************************************/

B_Entry *B_Node::new_one_entry()
{
	B_Entry *e = new B_Entry();

	return e;
}

/*****************************************************************
by yufei tao, 31 july 08
*****************************************************************/

B_Node *B_Node::new_one_node()
{
	B_Node *nd = new B_Node();

	return nd;
}
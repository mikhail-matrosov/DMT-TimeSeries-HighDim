/* this file defines class B_Entry */

#include <stdio.h>
#include <memory.h>
#include "b-node.h"
#include "b-entry.h"
#include "b-tree.h"
#include "b-def.h"
#include "../gadget/gadget.h"

//-----------------------------------------------

B_Entry::B_Entry() 
		          
{
	son = -1;
	key = NULL;
	son_ptr = NULL;
	level = -1;
	my_tree = NULL;
}

/****************************************************************
frees up the memory

coded by yufei tao, 4 aug 08
*****************************************************************/

void B_Entry::close()
{
	if (son_ptr)
	{
		delete son_ptr;
		son_ptr = NULL;
	}

	if (key)
	{
		delete [] key;
		key = NULL;
	}
}

//-----------------------------------------------

B_Entry::~B_Entry()
{
	if (son_ptr)
	{
		error("B_Entry desctrutor -- son_ptr not NULL. maybe you forgot to call close\n", true);
	}

	if (key)
	{
		error("B_Entry desctrutor -- key not NULL. maybe you forgot to call close\n", true);
	}
}

//-----------------------------------------------

int B_Entry::get_size(int _level)
{ 
	int size;
	
	if (_level == 0) 
		size = sizeof(key) + sizeof(son) + sizeof(float) * my_tree->keysize;
	else
		size = sizeof(key) + sizeof(son) + sizeof(leafson) + sizeof(float) * my_tree->keysize;

	return size;
}

//-----------------------------------------------

B_Node *B_Entry::get_son()
{
	if (son_ptr == NULL)
	{
		son_ptr = new_one_node();
		son_ptr->init(my_tree, son); 
	}
	return son_ptr;
}

//-----------------------------------------------

void B_Entry::del_son()
{
	if (son_ptr) 
	{ 
		delete son_ptr; 
		son_ptr = NULL;
	}  
}

//-----------------------------------------------

int B_Entry::read_from_buffer(char *_buf)
{
	int i;
	memcpy(key, _buf, my_tree->keysize * sizeof(float));
	i = my_tree->keysize * sizeof(float);

	memcpy(&son, &_buf[i], sizeof(son));
	i += sizeof(son);

	if (level > 0)
	{
		memcpy(&leafson, &_buf[i], sizeof(leafson));
		i += sizeof(leafson);
	}
	else
		leafson = son;

	return i;
}

//-----------------------------------------------

void B_Entry::init(B_Tree *_B_Tree, int _level)
{ 
	my_tree = _B_Tree; 
	level = _level;
	key = new int[my_tree->keysize];
}

//-----------------------------------------------

int B_Entry::write_to_buffer(char *_buf)
{
	int i;
	memcpy(_buf, key, my_tree->keysize * sizeof(float));
	i = my_tree->keysize * sizeof(float);

	memcpy(&_buf[i], &son, sizeof(son));
	i += sizeof(son);

	if (level > 0)
	{
		memcpy(&_buf[i], &leafson, sizeof(leafson));
		i += sizeof(leafson);
	}

	return i;
}

//-----------------------------------------------

void B_Entry::set_from(B_Entry *_e)
{
	if (level != _e->level)
		error("Error in B_Entry::set_from -- different levels",	true);

	memcpy(key, _e->key, _e->my_tree->keysize * sizeof(float));
	my_tree = _e->my_tree;
	son = _e->son;
	leafson = _e->leafson;
	son_ptr = _e->son_ptr;
}

/*****************************************************************
check if two entries are identical

para:
- e

return value: true or false

coded by yufei tao june 2003
*****************************************************************/

bool B_Entry::equal_to(B_Entry *_e)
{
	bool ret = true; 

	if (level != _e->level || son != _e->son || leafson != _e->leafson)
		ret = false;

	if (ret)
	{
		for (int i = 0; i < my_tree->keysize; i ++)
		{
			if (key[i] != _e->key[i])
			{
				ret = false; break;
			}
		}
	}

	return ret;
}

/*****************************************************************
sets the info of this entry from the child node

para:
- nd: the child node

coded by yufei tao june 2003
*****************************************************************/

void B_Entry::set_from_child(B_Node *_nd)
{
	son = _nd->block;
	memcpy(key, _nd->entries[0]->key, my_tree->keysize * sizeof(int));
	leafson = _nd->entries[0]->leafson;
}

/*****************************************************************
by yufei tao, 31 july 08
*****************************************************************/

B_Node *B_Entry::new_one_node()
{
	B_Node *nd = new B_Node();

	return nd;
}

/*****************************************************************
compares which of two entries comes first by their keys only

para:
- e: the entry to compare with

return
- -1: the host entry comes first
  0: tie
  1: the host entry comes later
  
by yufei tao, 31 july 08
*****************************************************************/

int B_Entry::compare_key(B_Entry *_e)
{
	int ret = 0; 

	for (int i = 0; i < my_tree->keysize; i ++)
	{
		if (key[i] < _e->key[i])
		{
			ret = -1;
			break;
		}
		else if (key[i] > _e->key[i])
		{
			ret = 1;
			break;
		}
	}

	return ret;
}

/*****************************************************************
compares which of two entries comes first.

para:
- e: the entry to compare with

return
- -1: the host entry comes first
  0: tie
  1: the host entry comes later
  
by yufei tao, 31 july 08
*****************************************************************/

int B_Entry::compare(B_Entry *_e)
{
	int ret = compare_key(_e);

	if (ret == 0)
	{
		if (leafson < _e->leafson)
			ret = -1;
		else if (leafson > _e->leafson)
			ret = 1; 
	}

	return ret;
}
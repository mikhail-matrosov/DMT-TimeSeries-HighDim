#include <stdio.h>
#include <memory.h>
#include "../btree/b-entry.h"
#include "lsbentry.h"
#include "lsbnode.h"
#include "lsbtree.h"

/*****************************************************************
by yf tao, 31 july 08
*****************************************************************/

LSBentry::LSBentry()           
{
	pt = NULL;
}

/*****************************************************************
by yf tao, 31 july 08
*****************************************************************/

LSBentry::~LSBentry()
{
	if (pt)
	{
		delete [] pt;
		pt = NULL;
	}
}

/*****************************************************************
by yf tao, 31 july 08
*****************************************************************/

int LSBentry::get_size(int _level)
{ 
	int ret = 0;
	
	ret = B_Entry::get_size(_level);
	
	if (_level == 0) 
		ret += sizeof(int) * ((LSBtree *) my_tree)->dim;

	return ret;
}

/*****************************************************************
by yf tao, 31 july 08
*****************************************************************/

int LSBentry::read_from_buffer(char *_buf)
{
	int i = B_Entry::read_from_buffer(_buf);

	int dim = ((LSBtree *)my_tree)->dim;

	if (level == 0)
	{
		for (int j = 0; j < dim; j ++)
		{
			memcpy(&pt[j], &_buf[i], sizeof(int));
			i += sizeof(int);
		}
	}

	return i;
}

/*****************************************************************
by yf tao, 31 july 08
*****************************************************************/

void LSBentry::init(B_Tree *_btree, int _level)
{ 
	int		i;

	B_Entry::init(_btree, _level);

	pt = new int[((LSBtree *) _btree)->dim];

	for (i = 0; i < ((LSBtree *) _btree)->dim; i ++)
		pt[i] = -1;
}

/*****************************************************************
by yf tao, 31 july 08
*****************************************************************/

int LSBentry::write_to_buffer(char *_buf)
{
	int i = B_Entry::write_to_buffer(_buf);

	int dim = ((LSBtree *)my_tree)->dim;

	if (level == 0)
	{
		for (int j = 0; j < dim; j ++)
		{
			memcpy(&_buf[i], &pt[j], sizeof(int));
			i += sizeof(int);
		}
	}

	return i;
}

/*****************************************************************
by yf tao, 31 july 08
*****************************************************************/

void LSBentry::set_from(B_Entry *_e)
{
	B_Entry::set_from(_e);

	int dim = ((LSBtree *) my_tree)->dim;
	memcpy(pt, ((LSBentry *) _e)->pt, sizeof(int) * dim);
}

/*****************************************************************
check if two entries are identical

para:
- e

return value: true or false

coded by yufei tao june 2003
*****************************************************************/

bool LSBentry::equal_to(B_Entry *_e)
{
	bool ret = B_Entry::equal_to(_e); 

	if (ret)
	{
		int dim = ((LSBtree *) my_tree)->dim;

		for (int i = 0; i < dim; i ++)
		{
			if (pt[i] != ((LSBentry *) _e)->pt[i])
			{
				ret = false;
				break;
			}
		}
	}

	return ret;
}

/*****************************************************************
by yf tao, 31 july 08
*****************************************************************/

B_Node *LSBentry::new_one_node()
{
	LSBnode *nd = new LSBnode();
	return nd;
}

/*****************************************************************
by yf tao, 31 july 08
*****************************************************************/

void LSBentry::close()
{
	if (pt)
	{
		delete [] pt;
		pt = NULL;
	}
	B_Entry::close();
}
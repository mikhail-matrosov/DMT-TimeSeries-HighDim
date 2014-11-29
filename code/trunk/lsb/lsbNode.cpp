#include <math.h>
#include <memory.h>
#include "lsbnode.h"
#include "lsbentry.h"
#include "lsbtree.h"
#include "../blockfile/blk_file.h"
#include "../blockfile/cache.h"

/*****************************************************************
by yf tao, 2 aug 08
*****************************************************************/

LSBnode::LSBnode()
{
	
}

/*****************************************************************
by yf tao, 2 aug 08
*****************************************************************/

LSBnode::~LSBnode()
{
	
}

/*****************************************************************
by yf tao, 2 aug 08
*****************************************************************/

B_Entry *LSBnode::new_one_entry()
{
	LSBentry *e = new LSBentry();

	return e;
}

/*****************************************************************
by yf tao, 2 aug 08
*****************************************************************/

B_Node *LSBnode::new_one_node()
{
	LSBnode *nd = new LSBnode();

	return nd;
}
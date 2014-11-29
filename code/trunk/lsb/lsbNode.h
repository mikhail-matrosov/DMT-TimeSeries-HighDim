#ifndef _LSBNode
#define _LSBNode

//-----------------------------------------------

#include "../btree/b-node.h"

class LSBnode : public B_Node
{
public:

	//--==functions==--
	LSBnode();
	virtual ~LSBnode();
	
	virtual B_Entry *new_one_entry();
	virtual B_Node *new_one_node();
};

//-----------------------------------------------

#endif
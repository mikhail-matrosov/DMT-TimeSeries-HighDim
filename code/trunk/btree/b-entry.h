/* this file defines class B_Entry 
   coded by TAO Yufei */

#ifndef __B_Entry
#define __B_Entry

//-----------------------------------------------

class B_Tree;
class B_Node;

class B_Entry
{
public:
	//--==write to disk==--
	//float agg; // agg was in the original B-tree code

	int *key;

	int leafson;										//this field is needed only at nonleaf levels
														//(key, leafson) forms the search key. 
														//this is to ensure fast deletions when many leaf entries have
														//the same key.
	int son;
	int level; 

	//--==others==--
	B_Tree *my_tree;
	B_Node *son_ptr;	

	//--==functions==--
	B_Entry();
	virtual ~B_Entry();
	virtual void close();
	virtual int compare_key(B_Entry *_e);
	virtual int compare(B_Entry *_e);
	virtual void del_son();
	virtual bool equal_to(B_Entry *_e);
	virtual int get_size(int _level);
	virtual B_Node *get_son();
	virtual void init(B_Tree *_B_Tree, int _level);
	virtual B_Node *new_one_node();
	virtual int read_from_buffer(char *_buf);
	virtual void set_from(B_Entry *_e);
	virtual void set_from_child(B_Node *_nd);
	virtual int write_to_buffer(char *_buf);
};

typedef B_Entry * B_Entryptr;

//-----------------------------------------------

#endif
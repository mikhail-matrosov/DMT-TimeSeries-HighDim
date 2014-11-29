/* this class defines class B_Node 
   coded by TAO Yufei */

#ifndef _B_Node
#define _B_Node

#include "b-def.h"
//-----------------------------------------------

class B_Tree;
class B_Entry;

class B_Node
{
public:
	//--==disk residential variables==--
	char level;
	int num_entries;
	int left_sibling;
	int right_sibling;
	B_Entry **entries;

	//--==others==--
	bool dirty;
	int block;
	int capacity;
	B_Tree *my_tree;

	//--==functions==--
	B_Node();
	virtual ~B_Node();

	virtual void add_new_child(B_Node *_cnd);
	virtual bool chk_undrflw();
	virtual bool chk_ovrflw();
	virtual int choose_subtree(B_Entry *_new_e);
	virtual BDEL delete_entry(B_Entry * _del_e);
	virtual void enter(B_Entry *_new_e);
	//virtual bool find_key(float _k);
	virtual int get_header_size();
	virtual B_Entry ** get_entries(int _cap);
	virtual B_Node* get_left_sibling();
	virtual B_Node* get_right_sibling();
	virtual void init(int _level, B_Tree *_B_Tree);
	virtual void init(B_Tree *_B_Tree, int _block);
	virtual BINSRT insert(B_Entry *_new_e, B_Node **_new_nd);
	//virtual int max_lesseq_key_pos(float _key);
	virtual int max_lesseq_key_pos(B_Entry *_e);
	virtual B_Entry *new_one_entry();
	virtual B_Node *new_one_node();
	int ptqry_next_follow(B_Entry *_e);
	//void qry_agg(int _k1, int _k2, int _r_bnd);
	virtual void read_from_buffer(char *_buf);
	virtual void rmv_entry(int _pos);
	virtual int traverse(float *_info);
	virtual void trt_ovrflw(B_Node **_new_nd);
	virtual void trt_undrflw(int _follow);
	virtual void write_to_buffer(char *_buf);

	//B_Entry *rank_find(float _rank);
	//float sumagg();
	
};

//-----------------------------------------------

#endif
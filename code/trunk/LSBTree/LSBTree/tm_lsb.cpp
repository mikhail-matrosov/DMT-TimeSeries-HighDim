#include "tm_lsb.h"


/**
 *	Description:
 *		Finds k NN time series for a given one.
 *	Arguments:
 *		Data_Set& _data_set - data set of non-transformed time series
 *		int* tm_q			- non-transformed query point
 *		int* q				- transformed query point
 *		int k				- number of searched NN
 *		int L				- number of used trees
 *		LSB_Hentry* rslt	- structure for storing results
 *	Returns:
 *		number of IO accesses done during execution
*/
int	TM_LSB::tm_knn(Data_Set* data_set, int* tm_q, int *q, int L, int k, LSB_Hentry* rslt, LSB_Hentry* transformed_rslt){

	init(data_set, tm_q, q, L, k);
	first_stage();
	tm_second_stage();

	// stores results in a given structure
	for(int i = 0; i < _k; i++){
		rslt[i].id = tm_rslt[i].id;
		rslt[i].dist = tm_rslt[i].dist;
		memcpy(rslt[i].pt, tm_rslt[i].pt, sizeof(int) * rslt[i].d);

		transformed_rslt[i].id = _rslt[i].id;
		transformed_rslt[i].dist = _rslt[i].dist;
		memcpy(transformed_rslt[i].pt, _rslt[i].pt, sizeof(int) * transformed_rslt[i].d);
	}

	tm_recycle();
	return ret;
}

void TM_LSB::init(Data_Set* data_set, int* tm_q, int *q, int numTrees, int k){
	_data_set = data_set;
	_tm_q = tm_q;
	_tm_d = data_set->get_d();
	_q = q;
	_k = k;
	ret = 0;
	
	if (numTrees < 0)
	{
		printf("-l must be followed by a nonnegative integer.\n", _numTrees);
		throw ERROR_EXCEPTION;
	}

	if (numTrees == 0)
		_numTrees = L;
	else
		_numTrees = min(L, numTrees);

	// tm
	_rslt = new LSB_Hentry[_k];
	tm_rslt = new LSB_Hentry[_k];
	for (int i = 0; i < _k; i ++)
	{
		// in image space
		_rslt[i].d		= d;
		_rslt[i].dist	= (float) MAXREAL;
		_rslt[i].pt		= new int[d];
		_rslt[i].id		= -1;
		_rslt[i].dist	= (float) MAXREAL;

		// in base space
		tm_rslt[i].d		= _tm_d;
		tm_rslt[i].treeId	= 1;
		tm_rslt[i].pt		= new int[_tm_d];
		tm_rslt[i].id		= -1;
		tm_rslt[i].dist		= (float) MAXREAL;
	}

	// pointers to BTree nodes for each Tree
	lptrs = new LSBbiPtr[numTrees];
	rptrs = new LSBbiPtr[numTrees];
	for (int i = 0; i < numTrees; i ++)
	{
		lptrs[i].nd = rptrs[i].nd = NULL;
		lptrs[i].entryId = rptrs[i].entryId = -1;
	}

	// Binary Heap
	hp = new BinHeap();
	hp->compare_func = &LSB_hcomp;
	hp->destroy_data = &LSB_hdestroy;

	// m - dimensioanly of image space
	qg = new float[m];
	qz = new intPtr[numTrees];  	 
	for (int i = 0; i < numTrees; i ++){
		qz[i] = new int[pz];
	}

	// pointer to LSB Entry 
	qe = new LSBentry();

	// sets its level to zero to mark it as a leaf node
	qe->init(trees[0], 0);
	memcpy(qe->pt, _q, sizeof(int) * d);
}

void TM_LSB::first_stage(){

	// works with each entry
	for (int i = 0; i < _numTrees; i ++)
	{
		// finds point qg which is image of point _q
		getHashVector(i, _q, qg);

		// finds z-value for point in image space
		if (getZ(qg, qz[i]))
		{
			throw ERROR_EXCEPTION;
		}

		// sets key of LSB entry equal to z-value
		memcpy(qe->key, qz[i], sizeof(int)* pz);
		
		// addreess of a root block in current Tree
		block = trees[i]->root;

		// initializes root block node
		nd = new LSBnode();
		nd->init(trees[i], block);

		// takes into acconunt one block IO access
		ret++;

		lescape = false;

		// finds appropriate leaf node
		while (nd->level > 0)
		{
			// finds first node which key is >= qe key
			follow = nd->ptqry_next_follow(qe);

			// checks if current LSB is empty
			if (follow == -1)
			{
				if (lescape)
					follow = 0; 
				else
				{
					if (block != trees[i]->root)
					{
						throw ERROR_EXCEPTION;
					}
					else
					{
						follow = 0;
						lescape = true;
					}
				}
			}

			// gets an appropriate child
			block = nd->entries[follow]->son;
			delete nd;

			// initializes next block node
			nd = new LSBnode();
			nd->init(trees[i], block);

			// takes into acconunt one block IO access
			ret++;
		}

		// sets rptrs and lptrs
		if (lescape)
		{
			rptrs[i].nd	= nd;
			rptrs[i].entryId = 0;
		}
		else
		{
			follow = nd->ptqry_next_follow(qe);
			lptrs[i].nd = nd;
			lptrs[i].entryId = follow;

			if (follow < nd->num_entries - 1)
			{
				rptrs[i].nd = nd;
				rptrs[i].entryId = follow + 1;
			}
			else
			{
				rptrs[i].nd = nd->get_right_sibling(); 
				if (rptrs[i].nd)
				{
					rptrs[i].entryId = 0;

					// takes into acconunt one block IO access
					ret++;
				}
			}
		}

		if (lptrs[i].nd)
		{
			// gets appropriate node entry
			e			= (LSBentry *) lptrs[i].nd->entries[lptrs[i].entryId];						
			he			= new LSB_Hentry;
			he->d		= d;
			he->pt		= new int[d];
			he->treeId	= i;
			he->lr		= 0;
			
			// gets lcl = u - floor(llcp / m)
			setHe(he, e, qz[i]);

			he->dist	= l2_dist_int(he->pt, _q, d);
			bhe			= new BinHeapEntry();
			bhe->data	= he;

			// puts he entry for current tree in common heap for all trees
			hp->insert(bhe);
		}

		if (rptrs[i].nd)
		{
			// gets appropriate node entry
			e			= (LSBentry *) rptrs[i].nd->entries[rptrs[i].entryId];
			he  		= new LSB_Hentry;
			he->d		= d;
			he->pt		= new int[d];
			he->treeId	= i;
			he->lr		= 1;

			// gets lcl = u - floor(llcp / m)
			setHe(he, e, qz[i]);

			he->dist	= l2_dist_int(he->pt, _q, d);
			bhe			= new BinHeapEntry();
			bhe->data	= he;

			// puts he entry for current tree in common heap for all trees
			hp->insert(bhe);
		}
	}

	qe->close();
	delete qe;
	qe = NULL;
}

void TM_LSB::tm_second_stage(){

	// initalizes necessary values
	knnDist = (float) MAXREAL;
	tm_knnDist = (float) MAXREAL;
	tm_he.pt = new int[_tm_d];
	tm_he.d = _tm_d;

	readCnt = 0;     
	rsltCnt = 0;	
	
	// sets limit for termination condition
	E1 = false;
	if (!E1)
		limit = n * L;
	else
		limit = (int) ceil ((float) 2 * B * L / d); 

	// sets flag as a loop condition
	again = true;
	while (again)
	{
		// gets the best candidate from a binary heap
		bhe = hp->remove();

		if (!bhe)
			again = false;								
		else
		{
			he = (LSB_Hentry *) bhe->data;
			thisLevel = he->lcl;
			thisTreeId = he->treeId;
			readCnt ++;

			// tm
			tm_he.id = he->id;
			tm_he.dist = l2_dist_int(_data_set->at(tm_he.id), _tm_q, _tm_d);
			tm_updateknn(_rslt, he, _k);

			// takes into acconunt IO access to base point value
			ret += _data_set->cost();

			// sets flag for Termination Condition E2
			// original: if (knnDist <= pow((float)ratio, thisLevel + 1))     
			//std::cout << std::endl << "tm_knnDist: " << tm_knnDist << "   "
			//		  << "pow((float)ratio, thisLevel + 1): " << pow((float)ratio, thisLevel + 1);
			//if (sqrt(tm_knnDist) <= pow((float)ratio, thisLevel + 1))
			//std::cout << std::endl << knnDist;
			if (sqrt(tm_knnDist) <= pow((float)ratio, thisLevel + 1))
			{
				again = false;
			}

			// checks Termination Condions E1 and E2
			if (again && readCnt < limit)
			{
				// check if moving must be to left
				if (he->lr == 0)
				{												
					nd = lptrs[thisTreeId].nd;
					pos = lptrs[thisTreeId].entryId;

					// checks if we are at the border
					if (pos > 0)
					{
						lptrs[thisTreeId].entryId--;
						setHe(he, nd->entries[pos - 1], qz[thisTreeId]);
						he->dist = l2_dist_int(he->pt, _q, d);
						hp->insert(bhe);
					}
					// reads left sibling node
					else
					{
						oldnd = nd;
						nd = nd->get_left_sibling();

						// takes into acconunt one block IO access
						ret++;

						// checks whether all left nodes were checked
						if (nd)
						{
							lptrs[thisTreeId].nd = nd;
							lptrs[thisTreeId].entryId = nd->num_entries - 1;

							setHe(he, nd->entries[nd->num_entries - 1], qz[thisTreeId]);
							he->dist = l2_dist_int(he->pt, _q, d);

							hp->insert(bhe);
						}
						// all left nodes were checked
						else
						{
							// frees memory for left part of current tree set of structures

							lptrs[thisTreeId].nd = NULL;
							lptrs[thisTreeId].entryId = -1;

							delete [] he->pt;
							he->pt = NULL;
							delete he;

							bhe->data = NULL;
							delete bhe;
						}

						// frees useless memory if node was changed
						if (rptrs[thisTreeId].nd != (LSBnode *) oldnd)
							delete oldnd;
					}
				}
				// moving must be to right
				else
				{														
					nd = rptrs[thisTreeId].nd;
					pos = rptrs[thisTreeId].entryId;

					// checks if we are at the border
					if (pos < nd->num_entries - 1)
					{
						rptrs[thisTreeId].entryId++;

						// gets lcl = u - floor(llcp / m)
						setHe(he, nd->entries[pos + 1], qz[thisTreeId]);

						he->dist = l2_dist_int(he->pt, _q, d);
						hp->insert(bhe);
					}
					// reads right sibling node
					else
					{
						oldnd = nd;
						nd = nd->get_right_sibling();

						// takes into acconunt one block IO access
						ret++;
						
						// checks whether all left nodes were checked
						if (nd)
						{
							rptrs[thisTreeId].nd = nd;
							rptrs[thisTreeId].entryId = 0;

							// gets lcl = u - floor(llcp / m)
							setHe(he, nd->entries[0], qz[thisTreeId]);

							he->dist = l2_dist_int(he->pt, _q, d);
							hp->insert(bhe);
						}
						// all left nodes were checked
						else
						{
							// frees memory for right part of current tree traversing set of structures

							rptrs[thisTreeId].nd = NULL;
							rptrs[thisTreeId].entryId = -1;

							delete [] he->pt;
							he->pt = NULL;
							delete he;

							bhe->data = NULL;
							delete bhe;
						}
						
						// frees useless memory if node was changed
						if (lptrs[thisTreeId].nd != (LSBnode *) oldnd)
							delete oldnd;
					}
				}
			}
			else
			{
				// fires termination flag
				again = false;			

				delete [] he->pt;
				he->pt = NULL;
				delete he;

				bhe->data = NULL;
				delete bhe;
			}
		}
	}
}

void TM_LSB::tm_updateknn(LSB_Hentry* rslt, LSB_Hentry* he, int k){
	old_knnDist = knnDist;
	knnDist = (float)(updateknn(rslt, he, k));
				
	tm_old_knnDist = tm_knnDist;
	tm_knnDist = (float)(updateknn(tm_rslt, &tm_he, k));
}

void TM_LSB::tm_recycle(){

	if (qz)
	{
		for (int i = 0; i < _numTrees; i ++){
			delete [] qz[i];
		}
		delete [] qz;
	}

	if (qe)
	{
		qe->close();
		delete qe;
	}

	if (qg)
		delete [] qg;

	if (hp->root)
		hp->root->recursive_data_wipeout(hp->destroy_data);
	delete hp;

	for (int i = 0; i < _numTrees; i ++)
	{
		if (lptrs[i].nd)
		{
			if (rptrs[i].nd == lptrs[i].nd)
				rptrs[i].nd = NULL;

			delete lptrs[i].nd;
			lptrs[i].nd = NULL;
		}

		if (rptrs[i].nd)
		{
			delete rptrs[i].nd;
			rptrs[i].nd = NULL;
		}
	}

	if (lptrs)
		delete [] lptrs;

	if (rptrs)
		delete [] rptrs;

	if (_rslt){
		for(int i = 0; i < _k; i++){
			delete[] _rslt[i].pt;
		}
		delete[] _rslt;
	}

	if (tm_rslt){
		for(int i = 0; i < _k; i++){
			delete[] tm_rslt[i].pt;
		}
		delete[] tm_rslt;
	}
}
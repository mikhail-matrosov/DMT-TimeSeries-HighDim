#include "tm_lsb.h"


/**
 *	Description:
 *		k NN time series for a given one
 *	Arguments:
 *		Data_Set& _data_set - data set of non-transformed time series
 *		int* _tm_q			- non-transformed query point
 *		int *_q				- transformed query point
 *		int _k				- number of needed NN
 *		int _numTrees		- number of used trees
 *	Returns:
 *		number of IO accesses done during execution
*/
int	TM_LSB::tm_knn(Data_Set* data_set, int* tm_q, LSB_Hentry * rslt, int *q, int k, int numTrees){
	_rslt = rslt;
	init(data_set, tm_q, numTrees, k, q);
	first_stage();
	tm_second_stage();
	recycle();

	return ret;
}


void TM_LSB::init(Data_Set* data_set, int* tm_q, int numTrees, int k, int *q){
	_data_set = data_set;
	_tm_q = tm_q;
	_tm_d = data_set->get_d();
	
	if (numTrees < 0)
	{
		printf("-l must be followed by a nonnegative integer.\n", _numTrees);
		throw ERROR_EXCEPTION;
	}

	if (numTrees == 0)
		_numTrees = L;
	else
		_numTrees = min(L, numTrees);

	_k = k;
	_q = q;

	// tm
	tm_rslt = new LSB_Hentry[_k];
	for (int i = 0; i < _k; i ++)
	{
		_rslt[i].id		= -1;
		_rslt[i].dist	= (float) MAXREAL;

		tm_rslt[i].d  = _tm_d;
		tm_rslt[i].treeId  = 1;
		tm_rslt[i].pt = new int[_tm_d];
		tm_rslt[i].id		= -1;
		tm_rslt[i].dist	= (float) MAXREAL;
	}

	lptrs = new LSBbiPtr[numTrees];
	rptrs = new LSBbiPtr[numTrees];

	for (int i = 0; i < numTrees; i ++)
	{
		lptrs[i].nd = rptrs[i].nd = NULL;
		lptrs[i].entryId = rptrs[i].entryId = -1;
	}

	hp = new BinHeap();
	hp->compare_func = &LSB_hcomp;
	hp->destroy_data = &LSB_hdestroy;

	qg = new float[m];     

	qz = new intPtr[numTrees];  	 
	for (int i = 0; i < numTrees; i ++){
		qz[i] = new int[pz];
	}

	qe = new LSBentry();
	qe->init(trees[0], 0);
	memcpy(qe->pt, _q, sizeof(int) * d);
}


void TM_LSB::first_stage(){
	for (int i = 0; i < _numTrees; i ++)
	{
		getHashVector(i, _q, qg);
		if (getZ(qg, qz[i]))
		{
			throw ERROR_EXCEPTION;
		}

		memcpy(qe->key, qz[i], sizeof(int) * pz);

		block = trees[i]->root;

		nd = new LSBnode();
		nd->init(trees[i], block);

		ret++;

		lescape = false;

		while (nd->level > 0)
		{
			follow = nd->ptqry_next_follow(qe);

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

			block = nd->entries[follow]->son;

			delete nd;

			nd = new LSBnode();
			nd->init(trees[i], block);

			ret++;
		}

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
					ret++;
				}

			}
		}

		if (lptrs[i].nd)
		{
			e			= (LSBentry *) lptrs[i].nd->entries[lptrs[i].entryId];			
			
			he			= new LSB_Hentry;
			he->d		= d;
			he->pt		= new int[d];
			he->treeId	= i;
			he->lr		= 0;
			
			setHe(he, e, qz[i]);
			he->dist	= l2_dist_int(he->pt, _q, d);

			bhe			= new BinHeapEntry();
			bhe->data	= he;

			hp->insert(bhe);
		}

		if (rptrs[i].nd)
		{
			e			= (LSBentry *) rptrs[i].nd->entries[rptrs[i].entryId];

			he  		= new LSB_Hentry;
			he->d		= d;
			he->pt		= new int[d];
			he->treeId	= i;
			he->lr		= 1;
			setHe(he, e, qz[i]);
			he->dist	= l2_dist_int(he->pt, _q, d);

			bhe			= new BinHeapEntry();
			bhe->data	= he;

			hp->insert(bhe);
		}
	}

	qe->close();
	delete qe;
	qe = NULL;
}


/*
void TM_LSB::second_stage(){
	knnDist = (float) MAXREAL;

	readCnt = 0;     
	rsltCnt = 0;	
	
	if (!E1)
		limit = n * L;
	else
		limit = (int) ceil ((float) 2 * B * L / d); 

	again = true;
	while (again)
	{
		bhe = hp->remove();
		
		if (!bhe)
			again = false;								
		else
		{
			he = (LSB_Hentry *) bhe->data;

			thisLevel = he->lcl;
			thisTreeId = he->treeId;

			readCnt ++;

			
			knnDist = (float) sqrt(updateknn(_rslt, he, _k));

			if (knnDist <= pow((float)ratio, thisLevel + 1))     
			{
				again = false;
			}

			if (again && readCnt < limit)
			{
				if (he->lr == 0)
				{												
					nd = lptrs[thisTreeId].nd;

					pos = lptrs[thisTreeId].entryId;

					if (pos > 0)
					{
						lptrs[thisTreeId].entryId--;

						setHe(he, nd->entries[pos - 1], qz[thisTreeId]);
						he->dist = l2_dist_int(he->pt, _q, d);

						hp->insert(bhe);
					}
					else
					{
						oldnd = nd;

						nd = nd->get_left_sibling(); 
						
						ret++;

						if (nd)
						{
							lptrs[thisTreeId].nd = nd;
							lptrs[thisTreeId].entryId = nd->num_entries - 1;

							setHe(he, nd->entries[nd->num_entries - 1], qz[thisTreeId]);
							he->dist = l2_dist_int(he->pt, _q, d);

							hp->insert(bhe);
						}
						else
						{
							lptrs[thisTreeId].nd = NULL;
							lptrs[thisTreeId].entryId = -1;

							delete [] he->pt;
							he->pt = NULL;
							delete he;

							bhe->data = NULL;
							delete bhe;
						}

						if (rptrs[thisTreeId].nd != (LSBnode *) oldnd)
							delete oldnd;
					}
				}
				else
				{														
					nd = rptrs[thisTreeId].nd;

					pos = rptrs[thisTreeId].entryId;

					if (pos < nd->num_entries - 1)
					{
						rptrs[thisTreeId].entryId++;

						setHe(he, nd->entries[pos + 1], qz[thisTreeId]);
						he->dist = l2_dist_int(he->pt, _q, d);

						hp->insert(bhe);
					}
					else
					{
						oldnd = nd;

						nd = nd->get_right_sibling(); 
						
						ret++;

						if (nd)
						{
							rptrs[thisTreeId].nd = nd;
							rptrs[thisTreeId].entryId = 0;

							setHe(he, nd->entries[0], qz[thisTreeId]);
							he->dist = l2_dist_int(he->pt, _q, d);

							hp->insert(bhe);
						}
						else
						{
							rptrs[thisTreeId].nd = NULL;
							rptrs[thisTreeId].entryId = -1;

							delete [] he->pt;
							he->pt = NULL;
							delete he;

							bhe->data = NULL;
							delete bhe;
						}
						
						if (lptrs[thisTreeId].nd != (LSBnode *) oldnd)
							delete oldnd;
					}
				}
			}
			else
			{
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
*/


void TM_LSB::tm_second_stage(){
	knnDist = (float) MAXREAL;
	tm_knnDist = (float) MAXREAL;
	tm_he.pt = new int[_tm_d];
	tm_he.d = _tm_d;

	readCnt = 0;     
	rsltCnt = 0;	
	
	if (!E1)
		limit = n * L;
	else
		limit = (int) ceil ((float) 2 * B * L / d); 

	again = true;
	while (again)
	{
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

			// updating cost after loading tm_he
			ret += _data_set->cost();

			// TODO: think better about pruning condition and test on different parameters
			//if (knnDist <= pow((float)ratio, thisLevel + 1))     
			if (tm_knnDist <= pow((float)ratio, thisLevel + 1))     
			{
				again = false;
			}

			if (again && readCnt < limit)
			{
				if (he->lr == 0)
				{												
					nd = lptrs[thisTreeId].nd;

					pos = lptrs[thisTreeId].entryId;

					if (pos > 0)
					{
						lptrs[thisTreeId].entryId--;

						setHe(he, nd->entries[pos - 1], qz[thisTreeId]);
						he->dist = l2_dist_int(he->pt, _q, d);

						hp->insert(bhe);
					}
					else
					{
						oldnd = nd;

						nd = nd->get_left_sibling(); 
						
						ret++;

						if (nd)
						{
							lptrs[thisTreeId].nd = nd;
							lptrs[thisTreeId].entryId = nd->num_entries - 1;

							setHe(he, nd->entries[nd->num_entries - 1], qz[thisTreeId]);
							he->dist = l2_dist_int(he->pt, _q, d);

							hp->insert(bhe);
						}
						else
						{
							lptrs[thisTreeId].nd = NULL;
							lptrs[thisTreeId].entryId = -1;

							delete [] he->pt;
							he->pt = NULL;
							delete he;

							bhe->data = NULL;
							delete bhe;
						}

						if (rptrs[thisTreeId].nd != (LSBnode *) oldnd)
							delete oldnd;
					}
				}
				else
				{														
					nd = rptrs[thisTreeId].nd;

					pos = rptrs[thisTreeId].entryId;

					if (pos < nd->num_entries - 1)
					{
						rptrs[thisTreeId].entryId++;

						setHe(he, nd->entries[pos + 1], qz[thisTreeId]);
						he->dist = l2_dist_int(he->pt, _q, d);

						hp->insert(bhe);
					}
					else
					{
						oldnd = nd;

						nd = nd->get_right_sibling(); 
						
						ret++;

						if (nd)
						{
							rptrs[thisTreeId].nd = nd;
							rptrs[thisTreeId].entryId = 0;

							setHe(he, nd->entries[0], qz[thisTreeId]);
							he->dist = l2_dist_int(he->pt, _q, d);

							hp->insert(bhe);
						}
						else
						{
							rptrs[thisTreeId].nd = NULL;
							rptrs[thisTreeId].entryId = -1;

							delete [] he->pt;
							he->pt = NULL;
							delete he;

							bhe->data = NULL;
							delete bhe;
						}
						
						if (lptrs[thisTreeId].nd != (LSBnode *) oldnd)
							delete oldnd;
					}
				}
			}
			else
			{
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


void TM_LSB::recycle(){

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

}
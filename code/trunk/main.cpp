#include <iostream>
#include <fstream>
#include <math.h>
#include <time.h>
#include <queue>
#include <vector>

#include "./blockfile/blk_file.h"
#include "./blockfile/cache.h"
#include "./gadget/gadget.h"
#include "./heap/binheap.h"
#include "./heap/heap.h"
#include "./lsb/lsb.h"
#include "./lsb/lsbentry.h"
#include "./lsb/lsbnode.h"
#include "./lsb/lsbtree.h"
#include "./rand/rand.h"

#include "./LSBTree/LSBTree/data_set.h"
#include "./LSBTree/LSBTree/tm_lsb.h"

/*****************************************************************
this function finds the k closest pairs using the brute-force 
O(n^2) algorithm. 

-para-
dfname			dataset file		
n				cardinality
d				dimensionality
k				number of pairs
ofname			output file

-return-
0		success
1		failure

-by- 
yf tao

-last touch-
21 aug 09
*****************************************************************/

int bruteforce_kcp(char *_dfname, int _n, int _d, int _k, char *_ofname)
{
	int			ret			= 0;

	FILE		* dfp		= NULL;
	FILE		* ofp		= NULL;
	float		dist		= -1;
	float		* kcpdist	= NULL;
	int			c			= -1;
	int			* ds		= NULL;
	int			i			= -1;
	int			ii			= -1;
	int			j			= -1;
	int			jj			= -1;
	int			* pairid	= NULL;

	dfp = fopen(_dfname, "r");
	if (!dfp)
	{
		printf("Could not open the data file.\n");

		ret = 1;
		goto recycle; 
	}

	ofp = fopen(_ofname, "w");
	if (!ofp)
	{
		printf("Could not create the output file.\n");

		ret = 1;
		goto recycle;
	}

	printf("Finding the accurate results (closest pair)\n");

	ds = new int[_n * (_d + 1)];
	
	c = 0;

	while (!feof(dfp) && c < _n)
	{
		fscanf(dfp, "%d", &ds[c * (_d + 1)]);

		for (i = 0; i < _d; i ++)
			fscanf(dfp, " %d", &ds[c * (_d + 1) + i + 1]);

		fscanf(dfp, "\n");

		c ++;
	}
	
	if (!feof(dfp) && c == _n)
	{
		printf("Dataset larger than you said.\n");

		ret = 1;
		goto recycle;
	}
	else if (feof(dfp) && c < _n)
	{
		printf("Set the dataset size to %d and try again.\n", c);
		
		ret = 1;
		goto recycle;
	}

	kcpdist = new float[_k];

	pairid = new int[2 * _k];

	for (i = 0; i < _k; i ++)
	{
		kcpdist[i] = (float) MAXREAL;

		pairid[2 * i] = pairid[2 * i + 1] = -1;
	}

	for (i = 0; i < _n; i ++)
	{
		for (j = i + 1; j < _n; j ++)
		{
			dist = l2_dist_int(&(ds[i * (_d + 1) + 1]), &(ds[j * (_d + 1) + 1]), _d);

			for (ii = 0; ii < _k; ii++)
			{
				if (compfloats(dist, kcpdist[ii]) == -1)
					break;
			}

			if (ii < _k)
			{
				for (jj = _k - 1; jj >= ii + 1; jj --)
				{
					kcpdist[jj] = kcpdist[jj - 1];
					
					pairid[2 * jj] = pairid[2 * (jj - 1)];
					pairid[2 * jj + 1] = pairid[2 * (jj - 1) + 1];
				}

				kcpdist[ii] = dist;

				pairid[2 * ii] = ds[i * (_d + 1)];
				pairid[2 * ii + 1] = ds[j * (_d + 1)];
			}
		}

		if ((i + 1) % 100 == 0)
			printf("\t%.2f%% done.\n", (float) i * 100 / _n);
	}

	fprintf(ofp, "%d\n", _k);

	fprintf(ofp, "id1 id2 dist\n");
	fprintf(ofp, "------------\n");

	for (i = 0; i < _k; i ++)
		fprintf(ofp, " %d %d %f\n", pairid[2 * i], pairid[2 * i + 1], sqrt(kcpdist[i]));

	fprintf(ofp, "\n");

recycle:
	if (dfp)
		fclose(dfp);

	if (ofp)
		fclose(ofp);

	if (ds)
		delete [] ds;

	if (kcpdist)
		delete [] kcpdist;

	if (pairid)
		delete [] pairid;

	return ret;
}

/*****************************************************************
builds a lsb forest by incremental insertions

-para-
dfname		dataset file
ffolder		forest folder
n			cardinality
d			dimensionality
B			page size
t			max coordinate of each dimension
L			number of lsb-trees. set it to 0 to build a complete
			forest

-return-
0			success
1			failure

-by- 
yf tao

-last touch-
19 aug 09
*****************************************************************/

int build(char *_dfname, char * _ffolder, int _n, int _d, int _B, int _t, int _L)
{
	int		ret				= 0;
	LSB		* lsb			= NULL;

	printf("Build LSB-forest.\n\n");

	lsb	= new LSB();

	lsb->quiet = 5;
	lsb->init(_t, _d, _n, _B, _L, 2);
		
	if (lsb->buildFromFile(_dfname, _ffolder))
	{
		ret = 1; 
		goto recycle;
	}

recycle:
	if (lsb)
	{
		delete lsb; 
		lsb = NULL;
	}

	return ret;
}

/*****************************************************************
builds a lsb forest by bulkloading

-para-
dfname		dataset file
ffolder		forest folder
n			cardinality
d			dimensionality
B			page size
t			max coordinate of each dimension
L			number of lsb-trees. set it to 0 to build a complete
			forest

-return-
0			success
1			failure

-by- 
yf tao

-last touch-
19 aug 09
*****************************************************************/

int bulkload(char *_dfname, char * _ffolder, int _n, int _d, int _B, int _t, int _L)
{
	int		ret				= 0;

	LSB		* lsb			= NULL;

	printf("Bulkload LSB-forest.\n\n");

	lsb = new LSB();

	lsb->quiet = 5;
	lsb->init(_t, _d, _n, _B, _L, 2);						

	if (lsb->bulkload(_dfname, _ffolder))
	{
		ret = 1;
		goto recycle;
	}
	
recycle:
	if (lsb)
		delete lsb;

	return ret;
}

/*****************************************************************
displays the basic info of a lsb-forest

-para-
ffolder			folder of the lsb-forest
numTrees		number of lsb-trees used for querying
rfname			file of accurate results
qfname			query file 
k				number of neighbors 
ofname			output file

-return-
0		success
1		failure

-by- 
yf tao

-last touch-
20 aug 09
*****************************************************************/

int info(char *_ffolder)
{
	int			ret						= 0;

	float		info[10];
	LSB			* lsb					= NULL;

	lsb = new LSB();

	if (lsb->restore(_ffolder))
	{
		ret = 1;
		goto recycle;
	}

	printf("Parameters:\n");
	printf("\tm = %d\n", lsb->m);
	printf("\tl = %d\n", lsb->L);
	printf("\tu = %d\n", lsb->u);
	printf("\tU = %.1f\n\n", lsb->U);

	if (!lsb->trees[0]->traverse(info))
	{
		ret = 1;
		goto recycle;
	}

	printf("Number of objects: %d\n", (int) info[0]);
	printf("Number of pages per tree: %d\n", (int) info[1]);

recycle:
	if (lsb)
		delete lsb;

	return ret;
}

/*****************************************************************
answer a workload of knn queries

-para-
ffolder			folder of the lsb-forest
numTrees		number of lsb-trees used for querying
rfname			file of accurate results
qfname			query file 
k				number of neighbors 
ofname			output file

-return-
0		success
1		failure

-by- 
yf tao

-last touch-
20 aug 09
*****************************************************************/

int lsbknn(char *_ffolder, int _numTrees, char *_rfname, char *_qfname, int _k, char *_ofname)
{
	int			ret						= 0;

	FILE		* rfp					= NULL;
	FILE		* ofp					= NULL;
	FILE		* qfp					= NULL;
	float		dist					= -1;
	float		* knndist				= NULL;
	float		overallRatioSum			= -1;
	float		thisOverallRatio		= -1;
	float		* R						= NULL;
	float		* rankOverallRatioSum	= NULL;
	float		p						= -1;
	int			c						= -1;
	int			costSum					= -1;
	int			* ds					= NULL;
	int			i						= -1;
	int			j						= -1;
	int			maxk					= -1;
	int			* q						= NULL;
	int			qn						= -1;
	int			rsltCnt					= -1;
	int			thiscost				= -1;
	LSB			* lsb					= NULL;
	LSB_Hentry	* rslt					= NULL;

	/* creating LSB */
	lsb = new LSB();

	/* reading LSB Tree */
	if (lsb->restore(_ffolder))
	{
		ret = 1;
		goto recycle;
	}
	_numTrees = (_numTrees == 0) ? lsb->L : _numTrees;

	rfp = fopen(_rfname, "r");
	if (!rfp)
	{
		printf("Could not open the file of accurate results.\n");

		ret = 1;
		goto recycle;
	}

	qfp = fopen(_qfname, "r");
	if (!qfp)
	{
		printf("Could not open the query file.\n");

		ret = 1;
		goto recycle;
	}

	ofp = fopen(_ofname, "w");
	if (!ofp)
	{
		printf("Could not create the output file.\n");

		ret = 1;
		goto recycle;
	}

	fscanf(rfp, "%d %d\n", &qn, &maxk);

	if (_k > maxk)
	{
		printf("Result file good for k <= %d only.\n", maxk);

		ret = 1;
		goto recycle;
	}

	printf("Approximate nearest neighbor \n");

	/* reading exact solutions */
	R = new float[qn * maxk];
	for (i = 0; i < qn; i ++)
	{
		fscanf(rfp, "%d", &j);

		for (j = 0; j < maxk; j ++)
			fscanf(rfp, "%f", &(R[i * maxk + j]));
	}

	/* initialising structure to store results */
	rslt = new LSB_Hentry[_k];
	for (i = 0; i < _k; i ++)
	{
		rslt[i].d  = lsb->d;
		rslt[i].pt = new int[lsb->d];
	}

	overallRatioSum = 0;

	rankOverallRatioSum = new float[_k];
	for (i = 0; i < _k; i ++)
		rankOverallRatioSum[i] = 0;

	c = 0;

	costSum = 0;

	q = new int[lsb->d];

	fprintf(ofp, "cost\toverall Ratio\n");
	
	while(!feof(qfp))
	{
		if (c == qn)
		{
			printf("Query file longer than expected. I thought there were %d queries only.\n", qn);

			ret = 1;
			goto recycle;
		}

		fscanf(qfp, "%d", &i);

		for (i = 0; i < lsb->d; i ++)
			fscanf(qfp, "%d", &(q[i]));

		fscanf(qfp, "\n");

		thiscost = lsb->knn(q, _k, rslt, _numTrees);

		thisOverallRatio = 0;

		for (i = 0; i < _k; i ++)
		{
			p = sqrt(rslt[i].dist) / R[c * maxk + i];

			thisOverallRatio += p;

			rankOverallRatioSum[i] += p;
		}
		thisOverallRatio /= _k;

		fprintf(ofp, "%d\t%f\n", thiscost, thisOverallRatio);

		overallRatioSum += thisOverallRatio;
		costSum += thiscost;

		c ++;
	}

	fprintf(ofp, "------------------------------------------\n");
	fprintf(ofp, "%d\t%f\n", costSum / c, overallRatioSum / c);

	fprintf(ofp, "\n\n");
	fprintf(ofp, "k\taverage rank ratios\n");

	for (i = 0; i < _k; i ++)
		fprintf(ofp, "%d\t%f\n", i + 1, rankOverallRatioSum[i] / c);

recycle:
	if (rfp)
		fclose(rfp);

	if (qfp)
		fclose(qfp);

	if (ofp)
		fclose(ofp);

	if (lsb)
		delete lsb;
	
	if (R)
		delete [] R;

	if (q)
		delete [] q;

	if (rslt)
	{
		for (i = 0; i < _k; i ++)
			delete [] rslt[i].pt;

		delete [] rslt;
	}

	if (rankOverallRatioSum)
		delete [] rankOverallRatioSum;

	return ret;
}

/*****************************************************************
answer a workload of knn queries

-para-
ffolder			folder of the lsb-forest
numTrees		number of lsb-trees used for querying
rfname			file of accurate results
k				number of neighbors 
ofname			output file

-return-
0		success
1		failure

-by- 
yf tao

-last touch-
20 aug 09
*****************************************************************/

int lsbkcp(char *_ffolder, char *_rfname, int _k, char *_ofname)
{
	int			ret						= 0;

	FILE		* rfp					= NULL;
	FILE		* ofp					= NULL;
	float		* dist					= NULL;
	float		overallRatioSum			= -1;
	float		thisRatio				= -1;
	float		* R						= NULL;
	int			cost					= -1;
	int			i						= -1;
	int			id1						= -1;
	int			id2						= -1;
	int			j						= -1;
	int			maxk					= -1;
	int			* pairid				= NULL;
	int			r						= -1;
	LSB			* lsb					= NULL;

	lsb = new LSB();

	if (lsb->restore(_ffolder))
	{
		ret = 1;
		goto recycle;
	}

	rfp = fopen(_rfname, "r");
	if (!rfp)
	{
		printf("Could not open the file of accurate results.\n");

		ret = 1;
		goto recycle;
	}

	ofp = fopen(_ofname, "w");
	if (!ofp)
	{
		printf("Could not create the output file.\n");

		ret = 1;
		goto recycle;
	}

	fscanf(rfp, "%d\n", &maxk);

	if (_k > maxk)
	{
		printf("Result file good for k <= %d only.\n", maxk);

		ret = 1;
		goto recycle;
	}

	printf("Approximate closest pair\n");

	R = new float[maxk];

	fscanf(rfp, "id1 id2 dist\n");
	fscanf(rfp, "------------\n");

	for (i = 0; i < _k; i ++)
	{
		fscanf(rfp, "%d %d %f\n", &id1, &id2, &R[i]);
	}

	dist = new float[_k];

	pairid = new int[2 * _k];

	overallRatioSum = 0;

	printf("Finding r...\n");
	cost = lsb->cpFind_r(&r);

	printf("Close pair with r = %d\n", r >> 3);
	cost += lsb->closepair(r >> 3, _k, dist, pairid);
	
	fprintf(ofp, "k\tratio\n");

	for (i = 0; i < _k; i ++)
	{
		if (R[i] == 0)
			thisRatio = 1; 
		else
			thisRatio = sqrt(dist[i]) / R[i];

		fprintf(ofp, "%d\t%.3f\n", i + 1, thisRatio);

		overallRatioSum += thisRatio; 
	}

	fprintf(ofp, "\n");
	fprintf(ofp, "cost = %d\n", cost);
	fprintf(ofp, "average ratio = %.3f\n", overallRatioSum / _k);

recycle:
	if (rfp)
		fclose(rfp);

	if (ofp)
		fclose(ofp);

	if (lsb)
		delete lsb;

	if (R)
		delete [] R;

	if (dist)
		delete [] dist;

	if (pairid)
		delete [] pairid;

	return ret;
}

/*****************************************************************
answer a workload of knn queries using as many lsb-trees as 
specified by the user.

-para-
ffolder			folder of the lsb-forest
numTrees		number of lsb-trees used for querying
rfname			file of accurate results
k				number of neighbors 
ofname			output file
numTrees		how many trees you would like to use

-return-
0		success
1		failure

-by- 
yf tao

-last touch-
20 aug 09
*****************************************************************/

int lsbkcp2(char *_ffolder, char *_rfname, int _k, char *_ofname, int _numTrees)
{
	int			ret						= 0;

	FILE		* rfp					= NULL;
	FILE		* ofp					= NULL;
	float		* dist					= NULL;
	float		overallRatioSum			= -1;
	float		thisRatio				= -1;
	float		* R						= NULL;
	int			cost					= -1;
	int			i						= -1;
	int			id1						= -1;
	int			id2						= -1;
	int			j						= -1;
	int			maxk					= -1;
	int			* pairid				= NULL;
	int			r						= -1;
	LSB			* lsb					= NULL;


	// reading forest from file
	lsb = new LSB();
	if (lsb->restore(_ffolder))
	{
		ret = 1;
		goto recycle;
	}

	// exact CP results
	rfp = fopen(_rfname, "r");
	if (!rfp)
	{
		printf("Could not open the file of accurate results.\n");

		ret = 1;
		goto recycle;
	}
	fscanf(rfp, "%d\n", &maxk);
	if (_k > maxk)
	{
		printf("Result file good for k <= %d only.\n", maxk);

		ret = 1;
		goto recycle;
	}

	R = new float[maxk];
	fscanf(rfp, "id1 id2 dist\n");
	fscanf(rfp, "------------\n");
	for (i = 0; i < _k; i ++)
	{
		fscanf(rfp, "%d %d %f\n", &id1, &id2, &R[i]);
	}

	printf("Approximate closest pair (fast)\n");
	dist = new float[_k];
	pairid = new int[2 * _k];
	overallRatioSum = 0;

	// search call to lsb forest
	cost = lsb->cpFast(_k, _numTrees, dist, pairid);
	
	// output
	ofp = fopen(_ofname, "w");
	if (!ofp)
	{
		printf("Could not create the output file.\n");

		ret = 1;
		goto recycle;
	}	
	fprintf(ofp, "k\tratio\n");
	for (i = 0; i < _k; i ++)
	{
		if (R[i] == 0)
			thisRatio = 1; 
		else
			thisRatio = sqrt(dist[i]) / R[i];

		//printf("%.0f\t%.0f\t%.3f\n", sqrt(dist[i]), R[i], thisRatio);
		fprintf(ofp, "%d\t%.3f\n", i + 1, thisRatio);

		overallRatioSum += thisRatio; 
	}

	fprintf(ofp, "\n");
	fprintf(ofp, "cost = %d\n", cost);
	fprintf(ofp, "average ratio = %.3f\n", overallRatioSum / _k);

recycle:
	if (rfp)
		fclose(rfp);

	if (ofp)
		fclose(ofp);

	if (lsb)
		delete lsb;

	if (R)
		delete [] R;

	if (dist)
		delete [] dist;

	if (pairid)
		delete [] pairid;

	return ret;
}

/*****************************************************************
this function finds the exact k NN distances of a workload of
queries. these results are written to a file for computing the
approximation ratios of approximate methods.

-para-
dfname			dataset file		
n				cardinality
d				dimensionality
qfname			query file 
k				number of neighbors 
ofname			output file

-return-
0		success
1		failure

-by- 
yf tao

-last touch-
20 aug 09
*****************************************************************/

int seqscan(char *_dfname, int _n, int _d, char *_qfname, int _k, char *_ofname)
{
	int			ret			= 0;

	FILE		* dfp		= NULL;
	FILE		* qfp		= NULL;
	FILE		* ofp		= NULL;
	float		dist		= -1;
	float		* knndist	= NULL;
	int			c			= -1;
	int			* ds		= NULL;
	int			i			= -1;
	int			ii			= -1;
	int			j			= -1;
	int			rsltCnt		= -1;
	int			* q			= NULL;
	int			qn			= -1;

	dfp = fopen(_dfname, "r");
	if (!dfp)
	{
		printf("Could not open the data file.\n");

		ret = 1;
		goto recycle; 
	}

	qfp = fopen(_qfname, "r");
	if (!qfp)
	{
		printf("Could not open the query file.\n");

		ret = 1;
		goto recycle;
	}

	ofp = fopen(_ofname, "w");
	if (!ofp)
	{
		printf("Could not create the output file.\n");

		ret = 1;
		goto recycle;
	}

	printf("Finding the accurate results (nearest neighbor)\n");

	ds = new int[_n * _d];
	
	c = 0;

	while (!feof(dfp) && c < _n)
	{
		fscanf(dfp, "%d", &j);

		for (i = 0; i < _d; i ++)
			fscanf(dfp, " %d", &(ds[c * _d + i]));

		fscanf(dfp, "\n");

		c ++;
	}
	
	if (!feof(dfp) && c == _n)
	{
		printf("Dataset larger than you said.\n");

		ret = 1;
		goto recycle;
	}
	else if (feof(dfp) && c < _n)
	{
		printf("Set the dataset size to %d and try again.\n", c);
		
		ret = 1;
		goto recycle;
	}

	q = new int[_d];

	knndist = new float[_k];

	qn = 0;

	while (!feof(qfp))
	{
		fscanf(qfp, "%d", &c);

		for (i = 0; i < _d; i ++)
			fscanf(qfp, " %d", &(q[i]));

		fscanf(qfp, "\n");

		qn ++;
	}

	fprintf(ofp, "%d %d\n", qn, _k);

	if (fseek(qfp, 0, SEEK_SET))
	{
		printf("Could not rewind to the beginning of the query file.\n");

		ret = 1;
		goto recycle;
	}

	while (!feof(qfp))
	{
		fscanf(qfp, "%d", &c);

		for (i = 0; i < _d; i ++)
			fscanf(qfp, " %d", &(q[i]));

		fscanf(qfp, "\n");

		for (i = 0; i < _k; i ++)
			knndist[i] = (float) MAXREAL;

		for (i = 0; i < _n; i ++)
		{
			dist = l2_dist_int(&(ds[i * _d]), q, _d);

			for (j = 0; j < _k; j++)
			{
				if (compfloats(dist, knndist[j]) == -1)
					break;
			}

			if (j < _k)
			{
				for (ii = _k - 1; ii >= j + 1; ii --)
					knndist[ii] = knndist[ii - 1];

				knndist[j] = dist;
			}
		}

		fprintf(ofp, "%d", c);

		for (i = 0; i < _k; i ++)
		{
			fprintf(ofp, " %f", sqrt(knndist[i]));
		}

		fprintf(ofp, "\n");
	}		

recycle:
	if (dfp)
		fclose(dfp);

	if (qfp)
		fclose(qfp);

	if (ofp)
		fclose(ofp);

	if (ds)
		delete [] ds;

	if (q)
		delete [] q;

	if (knndist)
		delete [] knndist;

	return ret;
}


/*****************************************************************
answer a workload of knn queries

-para-
ffolder			folder of the lsb-forest
numTrees		number of lsb-trees used for querying
rfname			file of accurate results
qfname			query file 
k				number of neighbors 
ofname			output file

-return-
0		success
1		failure

-by- 
yf tao

-last touch-
20 aug 09
*****************************************************************/

int tm_lsbknn(char *_ffolder, char* tm_fdata, int _numTrees, int _B, char *_rfname, char *_qfname, int _k, char *_ofname)
{
	std::cout << "tm_lsbknn()\n";

	int			ret						= 0;

	FILE		* rfp					= NULL;
	FILE		* ofp					= NULL;
	FILE		* qfp					= NULL;
	float		dist					= -1;
	float		* knndist				= NULL;
	float		overallRatioSum			= -1;
	float		thisOverallRatio		= -1;
	float		* R						= NULL;
	float		* rankOverallRatioSum	= NULL;
	float		p						= -1;
	int			c						= -1;
	int			costSum					= -1;
	int			* ds					= NULL;
	int			i						= -1;
	int			j						= -1;
	int			maxk					= -1;
	int			* q						= NULL;
	int			qn						= -1;
	int			rsltCnt					= -1;
	int			thiscost				= -1;
	TM_LSB		* lsb					= NULL;
	LSB_Hentry	* rslt					= NULL;

	/* reading data set*/
	Data_Set& tm_data_set = Indexed_Data_Set(tm_fdata, _B);

	/* creating LSB */
	lsb = new TM_LSB();

	/* reading LSB Tree */
	if (lsb->restore(_ffolder))
	{
		ret = 1;
		goto recycle;
	}
	_numTrees = (_numTrees == 0) ? lsb->L : _numTrees;

	rfp = fopen(_rfname, "r");
	if (!rfp)
	{
		printf("Could not open the file of accurate results.\n");

		ret = 1;
		goto recycle;
	}

	qfp = fopen(_qfname, "r");
	if (!qfp)
	{
		printf("Could not open the query file.\n");

		ret = 1;
		goto recycle;
	}

	ofp = fopen(_ofname, "w");
	if (!ofp)
	{
		printf("Could not create the output file.\n");

		ret = 1;
		goto recycle;
	}

	fscanf(rfp, "%d %d\n", &qn, &maxk);

	if (_k > maxk)
	{
		printf("Result file good for k <= %d only.\n", maxk);

		ret = 1;
		goto recycle;
	}

	printf("Approximate nearest neighbor \n");

	/* reading exact solutions */
	R = new float[qn * maxk];
	for (i = 0; i < qn; i ++)
	{
		fscanf(rfp, "%d", &j);

		for (j = 0; j < maxk; j ++)
			fscanf(rfp, "%f", &(R[i * maxk + j]));
	}

	/* initialising structure to store results */
	rslt = new LSB_Hentry[_k];
	for (i = 0; i < _k; i ++)
	{
		rslt[i].d  = lsb->d;
		rslt[i].pt = new int[lsb->d];
	}

	overallRatioSum = 0;

	rankOverallRatioSum = new float[_k];
	for (i = 0; i < _k; i ++)
		rankOverallRatioSum[i] = 0;

	c = 0;
	costSum = 0;

	q = new int[lsb->d];

	fprintf(ofp, "cost\toverall Ratio\n");

	while(!feof(qfp))
	{
		if (c == qn)
		{
			printf("Query file longer than expected. I thought there were %d queries only.\n", qn);

			ret = 1;
			goto recycle;
		}

		fscanf(qfp, "%d", &i);
		int* tm_q = tm_data_set.at(i);

		for (i = 0; i < lsb->d; i ++)
			fscanf(qfp, "%d", &(q[i]));


		fscanf(qfp, "\n");

		// CALL TO LSB
		thiscost = lsb->tm_knn(&tm_data_set, tm_q, rslt, q, _k, _numTrees);

		thisOverallRatio = 0;

		for (i = 0; i < _k; i ++)
		{
			p = sqrt(rslt[i].dist) / R[c * maxk + i];

			thisOverallRatio += p;

			rankOverallRatioSum[i] += p;
		}
		thisOverallRatio /= _k;

		fprintf(ofp, "%d\t%f\n", thiscost, thisOverallRatio);

		overallRatioSum += thisOverallRatio;
		costSum += thiscost;

		c ++;
	}

	fprintf(ofp, "------------------------------------------\n");
	fprintf(ofp, "%d\t%f\n", costSum / c, overallRatioSum / c);

	fprintf(ofp, "\n\n");
	fprintf(ofp, "k\taverage rank ratios\n");

	for (i = 0; i < _k; i ++)
		fprintf(ofp, "%d\t%f\n", i + 1, rankOverallRatioSum[i] / c);

recycle:
	if (rfp)
		fclose(rfp);

	if (qfp)
		fclose(qfp);

	if (ofp)
		fclose(ofp);

	if (lsb)
		delete lsb;
	
	if (R)
		delete [] R;

	if (q)
		delete [] q;

	if (rslt)
	{
		for (i = 0; i < _k; i ++)
			delete [] rslt[i].pt;

		delete [] rslt;
	}

	if (rankOverallRatioSum)
		delete [] rankOverallRatioSum;

	return ret;
}


void usage()
{
	printf("LSB (v2.0) by Yufei Tao, Chinese Univ of Hong Kong \n");	
	printf("Options\n");
	printf("-b {value}\tpage size in bytes (need to be a multiple of 4)\n");
	printf("-d {value}\tdimensionality\n");
	printf("-ds {string}\tdataset\n");
	printf("-f {string}\tfolder for lsb-forest\n");
	printf("-fb {string}\tfolder for lsb-forest (bulkloading)\n");
	printf("-k {value}\tnumber of neighbors wanted\n");
	printf("-l {value}\tnumber of lsb-trees to build or used to answer queries\n");
	printf("-n {value}\tcardinality\n");
	printf("-o {string}\toutput file\n");
	printf("-r {string}\tfile of exact results\n");
	printf("-t {value}\tmaximum coordinate of each axis\n");
	printf("-q {string}\tquery set\n");
	printf("\n");
	printf("Build a forest\n");
	printf("-b -d -ds -f [-l] -n -t\n");
	printf("Bulkload a forest\n");
	printf("-b -d -ds -fb [-l] -n -t\n");
	printf("Exact knn results for a query workload\n");
	printf("-d -ds -k -n -r -q\n");
	printf("Use LSB to answer a knn workload\n");
	printf("-f -k [-l] -o -r -q\n");
	printf("Exact kcp results\n");
	printf("-d -ds -k -n -r\n");
	printf("Use LSB for kcp\n");
	printf("-f -k [-l] -o -r\n");
	printf("Display the information of a forest\n");
	printf("-f\n");
}

void base_main(int argc, char* argv[])
{
//	test();
//	return;

//	srand((unsigned) time(NULL));

	bool		blkld			= false;
	bool		failed			= false;
	char		c				= -1;
	char		* cp			= NULL;
	char		dfname	[100]	= "";
	char		ffolder	[100]	= "";
	char		ofname	[100]	= "";
	char		qfname	[100]	= "";
	char		rfname	[100]	= "";
	char		para	[100]	= "";
	clock_t		stTime			= -1;
	clock_t		edTime			= -1;
	int			cnt				= -1;
	int			Bb				= -1;
	int			d				= -1;
	int			k				= -1;
	int			L				= 0;
	int			n				= -1;
	int			t				= -1;

	cnt = 1;							

	while (cnt < argc && !failed)
	{
		cp = argv[cnt];

		c = getnextChar(&cp);
		if (c != '-')
		{
			failed = true;
			break;
		}

		getnextWord(&cp, para);

		cnt ++;
		if (cnt == argc)
		{
			failed = true;
			break;
		}
		
		cp = argv[cnt];

		if (strcmp(para, "ds") == 0)
		{
			getnextWord(&cp, dfname);
		}
		else if (strcmp(para, "f") == 0)
		{
			getnextWord(&cp, ffolder);
		}
		else if (strcmp(para, "fb") == 0)
		{
			getnextWord(&cp, ffolder);
			blkld = true;
		}
		else if (strcmp(para, "q") == 0)
		{
			getnextWord(&cp, qfname);
		}
		else if (strcmp(para, "o") == 0)
		{
			getnextWord(&cp, ofname);
		}
		else if (strcmp(para, "r") == 0)
		{
			getnextWord(&cp, rfname);
		}
		else if (strcmp(para, "n") == 0)
		{
			n = atoi(cp);
			if (n <= 0)
			{
				failed = true;
				break;
			}
		}
		else if (strcmp(para, "d") == 0)
		{
			d = atoi(cp);
			if (d <= 0)
			{
				failed = true;
				break;
			}
		}
		else if (strcmp(para, "b") == 0)
		{
			Bb = atoi(cp);
			if (Bb <= 0 || (Bb / 4 * 4 != Bb))
			{
				printf("Illegal page size.\n\n");

				failed = true;
				break;
			}
		}
		else if (strcmp(para, "t") == 0)
		{
			t = atoi(cp);
			if (t <= 0)
			{
				failed = true;
				break;
			}
		}
		else if (strcmp(para, "k") == 0)
		{
			k = atoi(cp);
			if (k <= 0)
			{
				failed = true;
				break;
			}
		}
		else if (strcmp(para, "l") == 0)
		{
			L = atoi(cp);
			if (L <= 0)
			{
				failed = true;
				break;
			}
		}
		else
		{
			printf("Unknown option '-%s'.\n\n", para);
			failed = true;
			break;
		}

		cnt ++;
	}

	stTime = clock();

	if (!failed)
	{
		if (dfname[0] != '\0' && ffolder[0] != '\0' && n != -1 && d != -1 && Bb != -1 && t != -1 && !blkld)
		{
			build(dfname, ffolder, n, d, Bb/4, t, L);
			goto end;
		}
		else if (dfname[0] != '\0' && ffolder[0] != '\0' && n != -1 && d != -1 && Bb != -1 && t != -1 && blkld)
		{
			bulkload(dfname, ffolder, n, d, Bb/4, t, L);
			goto end;
		} 
		else if (dfname[0] != '\0' && qfname[0] != '\0' && rfname[0] != '\0' && n != -1 && d != -1 && k != -1)
		{
			seqscan(dfname, n, d, qfname, k, rfname);
			goto end;
		}
		else if (ffolder[0] != '\0' && rfname[0] != '\0' && qfname[0] != '\0' && ofname[0] != '\0' && k != -1)
		{
			lsbknn(ffolder, L, rfname, qfname, k, ofname);
			goto end;
		}
		else if (dfname[0] != '\0' && ffolder[0] != '\0' && rfname[0] != '\0' && qfname[0] != '\0'
				&& ofname[0] != '\0' && k != -1 && Bb != -1)
		{
			tm_lsbknn(ffolder, dfname, L, Bb, rfname, qfname, k, ofname);
			goto end;
		}
		else if (dfname[0] != '\0' && rfname[0] != '\0' && n != -1 && d != -1 && k != -1)
		{
			bruteforce_kcp(dfname, n, d, k, rfname);
			goto end;
		}
		else if (ffolder[0] != '\0' && rfname[0] != '\0' && ofname[0] != '\0' && k != -1 && L == 0)
		{
			lsbkcp(ffolder, rfname, k, ofname);
			goto end;
		}
		else if (ffolder[0] != '\0' && rfname[0] != '\0' && ofname[0] != '\0' && k != -1 && L != 0)
		{
			lsbkcp2(ffolder, rfname, k, ofname, L);
			goto end;
		}
		else if (ffolder[0] != '\0')
		{
			info(ffolder);
		}
		else
		{
			failed = true;
		}
	}

end:
	if (failed)
	{
		usage();
	}
	else
	{
		edTime = clock();

		printf("Running time %.1f seconds\n", ((float) edTime - stTime) / CLOCKS_PER_SEC);
	}

	return;
}


int main(int argc, char* argv[]){

	//base_main(argc, argv);
	//return 0;


	// paths
	char path_to_forest[] =			"forest//";
	char path_to_reference_knn[] =	"answer.txt";
	char path_to_query[] =			"query.txt";
	char path_to_output[] =			"knn.txt";
	char path_to_tm[] =				"time_series.txt";
	char path_to_transformed_tm[] = "fft.txt";
	

	// parameters
	int n = 1000;
	int d = 34;
	int B = 1024;
	int t = 3300;
	int L = 0;
	int k = 3;

	
	//build(path_to_transformed_tm, path_to_forest, n, d, B, t, L);
	tm_lsbknn(path_to_forest, path_to_tm, L, B, path_to_reference_knn, path_to_query, k, path_to_output);


	return 0;
}
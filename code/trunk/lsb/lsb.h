#ifndef __LSB
#define __LSB

#include <memory.h>
#include "lsbentry.h"
#include "lsbnode.h"
#include "lsbtree.h"
#include "../blockfile/cache.h"
#include "../gadget/gadget.h"

typedef char * charPtr;
typedef int * intPtr;
typedef LSBtree * LSBtreePtr;

struct LSB_Hentry
{
	int		d;
	int		id;								/* id of the point */
	int		lcl;							/* lowest common level with the query lcl = u - floor(llcp / m) */
	int		lr;								/* moving left or right */
	int		* pt;							/* coordinates */
	int		treeId;
	float	dist;							/* distance to query */

	void setto(LSB_Hentry * _e)
	{
		d = _e->d;
		id = _e->id;		
		lr = _e->lr;
		lcl = _e->lcl;
		memcpy(pt, _e->pt, sizeof(int) * d);
		treeId = _e->treeId;
		dist = _e->dist;
	}
};

class LSB
{
public:
	//--=== on disk ===--
	int			t;								/* largest coordinate of a dimension */
	int			d;								/* dimensionality */
	int			n;								/* cardinality */
	int			B;								/* page size in words */
	int			ratio;							/* approximation ratio */

	//--=== debug ==--
	int			quiet;
	bool		emergency;

	//--=== others ===--
	
	char		dsPath[100];					/* folder containing the dataset file */
	char		dsName[100];					/* dataset file name */
	char		forestPath[100];				/* folder containing the forest */

	int			f;
	int			pz;								/* number of pages to store the Z-value of a point */
	int			L;								/* number of lsb-trees */
	int			m;								/* dimensionality of the hash space */
	int			qNumTrees;						/* number of lsb-trees used to answer a query */
	int			u;								/* log_2 (U/w) */

	float		* a_array;												
	float		* b_array;						/* each lsb-tree requires m hash functions, and each has function
												   requires a d-dimensional vector a and a 1d value b. so a_array contains 
												   l * m * d values totally and b_array contains l *m values */

	float		U;								/* each dimension of the hash space has domain [-U/2, U/2] */
	float		w;
		
	LSBtreePtr *trees;							/* lsbtrees */

	//--=== Functions ===--
	LSB();
	~LSB();

	//--=== internal ===--
	virtual	int		cpFind_r			(int * _r);
	virtual void	freadNextEntry		(FILE *_fp, int * _son, int * _key);		
	virtual void	gen_vectors			();									
	virtual void	getTreeFname		(int _i, char *_fname);			
	virtual void	getHashVector		(int _tableID, int *_key, float *_g);
	virtual void	getHashPara			(int _u, int _v, float **_a_vector, float *_b);
	virtual float	get1HashV			(int _u, int _v, int *_key);		
	virtual	int		getLowestCommonLevel(int *_z1, int *_z2);
	virtual int		get_m				(int _r, float _w, int _n, int _B, int _d);
	virtual double	get_rho				(int _r, float _w);					
	virtual int		get_obj_size		(int _dim);							
	virtual int		get_u				();										
	virtual int		getZ				(float *_g, int *_z);				
	virtual int		insert				(int _treeID, int _son, int * _key);	
	virtual int		readParaFile		(char *_fname);
	virtual	void	setHe				(LSB_Hentry *_he, B_Entry *_e, int *_qz);
	virtual	float	updateknn			(LSB_Hentry * _rslt, LSB_Hentry *_he, int _k);
	virtual int		writeParaFile		(char *_fname);

	//--=== external ===--
	virtual int		buildFromFile		(char *_dsPath, char *_forestPath);
	virtual int		bulkload			(char *_dspath, char *_target_folder);	
	virtual	int		closepair			(int _r, int _k, float *_dist, int *_pairid);
	virtual int     cpFast				(int _k, int _numTrees, float *_dist, int *_pairid);
	virtual void	init				(int _t, int _d, int _n, int _B, int _L, int _ratio);	
	virtual int		knn					(int *_q, int _k, LSB_Hentry *_rslt, int _numTrees);
	virtual int		restore				(char *_paraPath);
};

struct LSBqsortElem
{
	int	* ds;
	int	pos;		/* position of the object in array ds */
	int	pz;			/* size of the z array below */
	int * z;
};

struct LSBbiPtr
{
	B_Node	* nd;
	int		entryId;
};

int		LSB_hcomp		(const void *_e1, const void *_e2);
int		LSBqsortComp	(const void *_e1, const void *_e2);
void	LSB_hdestroy	(const void *_e);

#endif
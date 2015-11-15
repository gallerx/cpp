//cPH.h Header file


#ifndef AFX_cPH_H_INCLUDED
#define AFX_cPH_H_INCLUDED

typedef struct tagELEMENT     // Matrix Element
   {
   long LinkFKID;             // identifies the LINK_PKID associated with element
   long Value;              // identifies the value associated with the element
							// generally either -1, meaning existence, or
  							// a non zero long corresponding to the PKID of the cyclic recursion 
							//point
   }
   ELEMENT, FAR *LPELEMENT;


typedef struct tagPERMUTATION     // Matrix Element
   {
   long  EventFKID;             // identifies the LINK_PKID associated with element
   long  LinkFKID;              // identifies the value associated with the element
   ULONG *plCycles;							// generally either -1, meaning existence, or
   long  lCycleCount;
   //new code
   ULONG *plPriorCycles;	
   long  lPriorCycleCount;
   //end new code
	}
   PERMUTATION, FAR *LPPERMUTATION;


/*************************************************************************************
* CALLING CYCLE FOR PERMUTATION HANDLING
*   Assume k vertexs in DAG
*   Assume j cycles  in DCG
*	Call Constructor              x 1 (one)
*	PushVertex		              x k
*   Push Cycle                    x j
*   InitializeMatrix              x 1 (one)
*     -initializes the (k,k) predecessor matrix and the (k,j) vertex-cycle membership matrix
*	SetVertexHigh	              as needed
*   SetVertexCycleHigh            as needed
*   GeneratePermutations               x 1 (one)
*   Call Destructor               x 1 (one)
*
**************************************************************************************/

class cPH
{
public:
			cPH					();
	virtual ~cPH					();
	BOOL	PushVertex				(long lPKID);
	BOOL 	PushCycle				(long lPKID);
	BOOL	InitializePredMatrix	(void);
	BOOL	InitializeVertexCycleMatrix	(void);
	BOOL	SetVertexHigh			(long lRowPKID,long lColPKID,long lValue,long lLinkPKID);
	BOOL	SetVertexCycleHigh		(long lVertexPKID, long lCyclePKID);
	void	GetPermutations			(long& lNumPerms,LPPERMUTATION &pPERM);
	
private:
	void    GeneratePermutations	(void);
	int	    HasPredecessor			(long lRowIndex);
	void    DepthWiseWalk			(long lColIndex, ULONG * plCycleStack,long lNUM_CYCLES);
	int	    CheckVertexCycleMatrix  (long lRowIndex, long lCyclePKID);
	BOOL    VertexBoundsCheck		(long lRowIndex);
	BOOL    CycleBoundsCheck		(long lColIndex);
	long    GetVertexIndexByPKID	(long lPKID);
	long    GetCycleIndexByPKID		(long lPKID); 
	//new code
	BOOL	SortAndRemoveDuplicatePermutations ();
	void	PermSortValue			(LPPERMUTATION lpPERM,char * szBuffer);
	void    AddPermutation          (long lEventFKID, long lLinkFKID,long lCycleCount, ULONG * plCycles,long lPriorCycleCount, ULONG * plPriorCycles);
	//end new code

	LPELEMENT		m_pELPredMatrix;
	BYTE			*m_pbVertexCycleMatrix;
	LPPERMUTATION	m_pPMPermutations;
	long			*m_plVertexVector;
	long			*m_plCycleVector;
	long			m_lVertexCount;
	long			m_lCycleCount;
	long			m_lPermutationCount;
};


#endif //AFX_cPH_H_INCLUDED
	
/*************************************************************************************
*LPELEMENT g_pELPredMatrix
*   square predecessor matrix of size g_lVertexCount x g_lVertex Count
**************************************************************************************/
/*************************************************************************************
* BYTE *g_pbCycleVertexMatrix
*   Rectangular  matrix of size g_lVertexCount x g_lCycleCount
*	rows = vertexs, columns = cycles
*   shows membership of vertex (row) within Cycle (column)
**************************************************************************************/
/*************************************************************************************
*  LPPERMUTATION g_pPMPermutations
*   global array of Permutation structures
**************************************************************************************/
/*************************************************************************************
* long *g_plVertexVector
*   single dimension of array all vertexs in DAG, listed by vertex PKID
**************************************************************************************/
/*************************************************************************************
* long *g_plCycleVector
*   single dimension array of all cycles in DCG, listed by cycle PKID
**************************************************************************************/
/*************************************************************************************
* long g_lVertexCount 
*   number of vertexs in DAG
**************************************************************************************/
/*************************************************************************************
* long g_lCycleCount 
*   number of cycles in DCG
**************************************************************************************/
/*************************************************************************************
* long g_lPermutationCount
*   number of permutations in the g_pPMPermutations Vector
**************************************************************************************/


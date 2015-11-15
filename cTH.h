//cTH.h  Header File

#if !defined(AFX_TH_H_INCLUDED)
#define      AFX_TH_H_INCLUDED
/********************************************************************************************/
#include <vector>
#include <map>
#include <algorithm>
#include <cctype>
#include <utility>
#include "StdAfx.h"
#include "cNME.h"
/********************************************************************************************/
using namespace std;
/********************************************************************************************/
enum {ME_MODE = 1,NME_MODE = 2};
/********************************************************************************************/
class cTH
{
public:
		     cTH					(short int iMode);
	virtual ~cTH					();
	BOOL	 PushVertex				(ULONG	 lExclusionSpace,  const LPULONG plPriorPerm,
								     ULONG	 lNumPriorPerm,    LPULONG plPerm,  
									 ULONG   lNumPerm,		   ULONG   lActualEdge,
									 ULONG   lDuration,		   ULONG   lModeID);
	BOOL	 PushVertex				(const   LPULONG plDJS,	   ULONG   lNumDJS,    
									 ULONG   lExclusionSpace,  const LPULONG plPriorPerm, 
									 ULONG   lNumPriorPerm,    const LPULONG plPerm,
									 ULONG   lNumPerm,         ULONG   lActualEdge,
									 ULONG   lDuration,		   ULONG   lModeID);
	BOOL     PushDeadCycle			(ULONG   lCyclePKID);
	BOOL     PushDeadEdge           (ULONG   lDeadPKID,		   ULONG lAssignedTo);
	BOOL	 GetIterator			(vector<cNME>::iterator& itrNME, ULONG& lNumMembers);
	short	 GetMode				(void);
	BOOL	 RingCycle				(void);
	
	//public members
	vector<cNME> 					m_vNME;
	friend bool operator<(const pair<ULONG const,ULONG>& x, const ULONG& lValue);
	friend bool operator<(const ULONG& lValue, const pair<ULONG const,ULONG>& x);

private:
	
	BOOL	 InspectPermutation	   (void);
	BOOL	 ModifyActualEdge	   (void);
	BOOL     ModifyActualEdgeAndDJS(void);
	BOOL	 Pass1				   (void);
	BOOL	 Pass2				   (void);
	typedef void (cTH::* UAction)  (vector<cNME>::iterator& a);						
	typedef void (cTH::* BAction)  (vector<cNME>::iterator& a, vector<cNME>::iterator& b);
	void	      CALL_MACRO	   (UAction uAct,BAction bAct,
		                            vector<cNME>::iterator& p, vector<cNME>::iterator  q);
										//q passed by value
	

	//CRITICAL MAKE SURE THAT a IS RETURNED/INCREMENTED PROPERLY, 
	//i.e. SHOULD BE ON NEXT RECORD TO READ AFTER OPERATION IS COMPLETE!!!!
	void   DoNothing			   (vector<cNME>::iterator& a);
	void   DoNothing			   (vector<cNME>::iterator& a,  vector<cNME>::iterator& b);
	void   Erase				   (vector<cNME>::iterator& a);
	void   Erase				   (vector<cNME>::iterator& a,  vector<cNME>::iterator& b);
	void   ReplaceActualEdge	   (vector<cNME>::iterator& a,  vector<cNME>::iterator& b);
	void   ReplaceDJS			   (vector<cNME>::iterator& a,  vector<cNME>::iterator& b);		 
	void   ReplaceActualEdgeAndDJS (vector<cNME>::iterator& a,  vector<cNME>::iterator& b);
	void   CommonReplacementAlgorithm(vector<cNME>::iterator& iterRef,
									   vector<cNME>::iterator& b, BYTE  bytSwitch);
	
	typedef bool(cTH::* BoolComp)  (cNME& x, ULONG lData, vector<ULONG>* pvData);

	void   FindMaxExtent(vector<cNME>::iterator& p, vector<cNME>::iterator& q,
						BoolComp pfComp, ULONG lData,  vector<ULONG>* pvData);
	
	bool	SamePerm		       (cNME& x, ULONG lData, vector<ULONG>* pvData);
	bool	SameActualEdge         (cNME& x, ULONG lData, vector<ULONG>* pvData);										
	bool	SameDJS		           (cNME& x, ULONG lData, vector<ULONG>* pvData);
	bool	SameActualEdgeAndDJS   (cNME& x, ULONG lData, vector<ULONG>* pvData);

	//private members
	short							   m_iMode;
	vector<ULONG>					   m_vDeadCycle;
	map<ULONG,ULONG>			       m_mDeadEdge;
};
/********************************************************************************************/
#endif //AFX_TH_H_INCLUDED
/*
handling of Permutation field
********************************************************************************************
  Name: InspectPermutation
  Applied To: NME_MODE, ME_MODE
  Functionality:
	Check if any dead cycle is in the permutation binary
	If it is, remove the row from potential result set
  Sort assumptions: Members should be sorted by Permutation	

Handling of Actual Edge
********************************************************************************************
  Name: ModifyActualEdge
  Applied To: ME_MODE,NME_MODE
  Functionality:
  Sort Assumptions: Members should be sorted byActual Edge	
	 Dead|Assigned 

  If Dead and no assigned, delete member
  If Dead and assigned, change value of Actual_Edge to assigned
	
Handling of Disjoint Subset
********************************************************************************************
	Name: ModifyDisjointSubset
	Appled To: NME_MODE
	Sort Assumptions: members should be sorted by disjoint subset ascending

	  //Intersect Dead and assigned with Disjoint subset:
	// if both dead and assigned are present, remove the dead, leave assigned
	// if just dead is present, but not assigned, insert assigned in correct

										DEAD
				  P												A
     ASSIGNED P   remove dead, leave assigned			      do nothing
	          A   transform dead with assigned,set dirty      do nothing   

  3.15.04
BASED ON ANALYSIS TONIGHT, I THINK YOU HAVE TO CONSIDER WHAT IS STATE OF ACTUAL EDGE
IF ACTUAL EDGE IS THE DEAD, AND BOTH (DEAD AND ASSIGNED) IS PRESENT, THEN MUST REMOVE ROW, OTHERWISE
WOULD CREATE A DUPLICATE

So seems that in the dead and assigned case you must evaluate if assigned is present:
if assigned is present, then redundant


  (remember to sort any dirty disjoint subsets)

where p = present and A = absent	
********************************************************************************************
*/












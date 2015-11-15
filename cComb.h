//cCombH.h Header file

#ifndef AFX_cCombH_H_INCLUDED
#define AFX_cCombH_H_INCLUDED

typedef ULONG FAR *         LPULONG;

typedef struct tagCOMBINATION     // Combination entry
   {
   ULONG  NME_SPACE;             // identifies the outcome-type edge associated with 
								// the Non-MutuallyExclusive Space
   ULONG   LinkFKID;              
   LPULONG plDisjointRegions;		// 
   long    lRegionCount;
   }
   COMBINATION, FAR *LPCOMBINATION;


/*************************************************************************************
* CALLING CYCLE FOR COMBINATION HANDLING
*	Call Constructor				  x 1 (one)
*   BEGIN
*	 BEGIN 
		PushCombinationToBuffer       x 1 (one)
*		Push DisjointRegionToBuffer   x 1 (one)
*    LOOP (one loop per link)
*    FlushBuffer					  x1   
*   LOOP (one loop per unique event)
*   GetCombinations			          x 1 (one)
*   Call Destructor				      x 1 (one)
*
**************************************************************************************/

class cCombH
{
public:
					cCombH						();
	virtual		   ~cCombH						();
	BOOL			PushCombinationToBuffer		(ULONG lNmeSpace, ULONG lLinkFkid);
	BOOL 			PushDisjointRegionToBuffer	(ULONG lRegionID);
	BOOL			FlushBuffer					(void);
	void			GetCombinations				(long& lNumCombs,LPCOMBINATION &pCOMB);
private:
	BOOL			CheckRegionsAreSequentialAndUnique();
	BOOL			IsValidULONG				(long lData);
	
	LPCOMBINATION	m_pCMCombinations;
	long			m_lCombinationCount;

	LPCOMBINATION	m_pCMComboBuffer;
	long			m_lComboBufferCount;

	LPULONG		    m_plRegionBuffer;
	ULONG		    m_lRegionBufferCount;

};

#endif //AFX_cCombH_H_INCLUDED
	
//documentation
/*************************************************************************************/
/*  MEMBER VARIABLES
/*************************************************************************************/
/*************************************************************************************
*   LPCOMBINATION m_pCMCombinations
*   member array of Combination structures
**************************************************************************************/
/*************************************************************************************
*   long m_lCombinationCount
*   number of combinations in the m_pCMCombinations Vector
**************************************************************************************/
/*************************************************************************************
*   LPCOMBINATION m_pCMComboBuffer
*   member array of Combination structures
*     --these combinations will possess incomplete info (lack a disjoint region entry)
*       which will be made available before posting them to the "main" collection
*	    m_pCMCombinations by a call to ::FlushBuffer
**************************************************************************************/
/*************************************************************************************
*   long m_lComboBufferCount
*   number of combinations in the buffer
*     --these combinations will possess incomplete info (lack a disjoint region entry)
*       which will be made available before posting them to the "main" collection
*	    m_pCMCombinations by a call to ::FlushBuffer
**************************************************************************************/
/*************************************************************************************
*   ULONG		   *m_plRegionBuffer
*   member array of regions
*   maintained as a buffer
*   upon flushing, make a copy of each region and attach to each Combo sitting in the 
*   buffer, then copy the buffer into the main combo array m_pCMCombinations
**************************************************************************************/
/*************************************************************************************
*   ULONG m_lRegionBufferCount
*   number of disjoint regions in the region buffer
**************************************************************************************/

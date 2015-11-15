//cCombH.cpp Implementation File for Permutation Handler


#include "StdAfx.h"
#include "cCombH.h"
#include <windows.h>

/*************************************************************************************
* Constructor: zero out member variables
**************************************************************************************/
cCombH::cCombH()
{
	m_pCMCombinations       = NULL;
	m_lCombinationCount 	= 0;
	
	m_pCMComboBuffer		= NULL;
	m_lComboBufferCount		= 0;
	
	m_plRegionBuffer		= NULL;
	m_lRegionBufferCount	= 0;
}
/*************************************************************************************
* Destructor: deallocates memory
**************************************************************************************/
cCombH::~cCombH()
{
	/*Clean up the m_pCMCombination vector and all plDisjoint regions*/
	if (NULL != m_pCMCombinations) 
	{	
		for(LPCOMBINATION pCOMB  = m_pCMCombinations; 
			pCOMB<m_pCMCombinations+m_lCombinationCount;
			pCOMB++)
		{	

			if (0 < pCOMB->lRegionCount)
			{
				if (NULL != pCOMB->plDisjointRegions)
				{
					delete [] pCOMB->plDisjointRegions;
				}
			}
		}
		delete []  m_pCMCombinations;
		m_pCMCombinations		= NULL;
		m_lCombinationCount		= 0;
	}

	/*Clean up the m_pCMComboBuffer vector and all plDisjoint regions*/
	if (NULL != m_pCMComboBuffer) 
	{	
		for(LPCOMBINATION pCOMB =  m_pCMComboBuffer;
			pCOMB<m_pCMComboBuffer+m_lComboBufferCount;
			pCOMB++)
		{	

			if (0 < pCOMB->lRegionCount)
			{
				if (NULL != pCOMB->plDisjointRegions)
				{
					delete [] pCOMB->plDisjointRegions;

				}
			}
		}
		delete []  m_pCMComboBuffer;
		m_pCMComboBuffer		= NULL;
		m_lComboBufferCount		= 0;
	}

	/*Clean up the region Buffer*/
	if (NULL != m_plRegionBuffer)
	{
		delete [] m_plRegionBuffer;
		m_plRegionBuffer		= NULL;
		m_lRegionBufferCount	= 0;
	}
}
/*************************************************************************************
* PushCombinationToBuffer: pushes a combination onto the buffer
*   leaves the region count and disjoint regions empty
*
**************************************************************************************/
BOOL cCombH::PushCombinationToBuffer(ULONG lNmeSpace, ULONG lLinkFKID)
{
	if (!IsValidULONG(lNmeSpace)) {return false;}
	if (!IsValidULONG(lLinkFKID)) {return false;}

	//initialize a  new buffer with an additional combo member
	LPCOMBINATION pCMNewBuffer = new COMBINATION[m_lComboBufferCount + 1];

	//copy the existing buffer into a new array
	if (0 < m_lComboBufferCount)
	{ 
		CopyMemory(pCMNewBuffer,m_pCMComboBuffer,m_lComboBufferCount*sizeof(COMBINATION));
		delete [] m_pCMComboBuffer;
	}
	m_pCMComboBuffer = pCMNewBuffer;						

	//get a pointer to the last element of the buffer array
	LPCOMBINATION p = m_pCMComboBuffer+m_lComboBufferCount;
	//increment total number of combinations in buffer
	m_lComboBufferCount++;
	
	p->NME_SPACE			= lNmeSpace;
	p->LinkFKID				= lLinkFKID;
	p->lRegionCount			= 0;        //initialize to zero: will populate on ::FlushBuffer()
	p->plDisjointRegions	= NULL;     //initialize to NULL: will populate on ::FlushBuffer()

	return true;
}

/*************************************************************************************
* PushDisjointRegionToBuffer: pushes a combination onto the buffer
*   leaves the region count and disjoint regions empty
*
**************************************************************************************/
BOOL cCombH::PushDisjointRegionToBuffer(ULONG lRegionID)
{
	if (!IsValidULONG(lRegionID)) {return false;}
	
	//initialize a  new regoion buffer with an additional ULONG member
	LPULONG plNewBuffer = new ULONG[m_lRegionBufferCount + 1];

	//copy the existing buffer into a new array
	if (0 < m_lRegionBufferCount)
	{ 
		CopyMemory(plNewBuffer,m_plRegionBuffer,m_lRegionBufferCount*sizeof(ULONG));
		delete [] m_plRegionBuffer;
	}
	m_plRegionBuffer = plNewBuffer;						

	//get a pointer to the last element of the buffer array
	LPULONG p = m_plRegionBuffer+m_lRegionBufferCount;
	//increment total number of regions in buffer
	m_lRegionBufferCount++;
	
	*p = lRegionID;

	return true;
}

/*************************************************************************************
* FlushBuffer: pushes the buffer onto main array
*
**************************************************************************************/
BOOL cCombH::FlushBuffer()
{
	//4  steps:
	//0. check that there is data to flush.
	//a. if socheck that regions are sequential and unique
	//b. wham the regionBuffer onto each of the combos in the buffer
	//c. copy the buffer to the main buffer
	//d. clean out the buffer memory
	BOOL		  bRetCode	= false;
	LPCOMBINATION pCMNew	= NULL;
	LPCOMBINATION p			= NULL;
	LPCOMBINATION q			= NULL;

	//0. check that there is data to flush.
	if (!(0 < m_lComboBufferCount))				{goto SAFE_EXIT;}
	if (!(0 < m_lRegionBufferCount))			{goto SAFE_EXIT;}
	
	//a. if socheck that regions are sequential and unique
	if (!CheckRegionsAreSequentialAndUnique())	{goto SAFE_EXIT;}

	//b. wham the regionBuffer onto each of the combos in the buffer
	for(q = m_pCMComboBuffer; q<m_pCMComboBuffer+m_lComboBufferCount;q++)
	{	
		q->lRegionCount = m_lRegionBufferCount;
		LPULONG pl = new ULONG[m_lRegionBufferCount];
		CopyMemory(pl,m_plRegionBuffer,m_lRegionBufferCount*sizeof(ULONG));
		q->plDisjointRegions = pl;
		pl = NULL;
	}

	//c. copy the buffer to the main buffer
	//initialize a  new buffer with additional combo members
	pCMNew = new COMBINATION[m_lCombinationCount + m_lComboBufferCount];

	//copy the existing array into a new array
	if (0 < m_lCombinationCount)
	{ 
		CopyMemory(pCMNew,m_pCMCombinations,m_lCombinationCount*sizeof(COMBINATION));
		delete [] m_pCMCombinations;
	}
	m_pCMCombinations = pCMNew;						

	//get a pointer to the last elements of the combination array
	p = m_pCMCombinations+m_lCombinationCount;

	//copy the combo buffer over to the main array
	CopyMemory(p,m_pCMComboBuffer,m_lComboBufferCount*sizeof(COMBINATION));

	//increment total number of combinations 
	m_lCombinationCount += m_lComboBufferCount;
	
	bRetCode = true;
SAFE_EXIT:
	//d. clean out the buffer memory
	delete[] m_pCMComboBuffer;
	m_pCMComboBuffer	 = NULL;
	m_lComboBufferCount  = 0;
	
	delete [] m_plRegionBuffer;
	m_plRegionBuffer	 = NULL;
	m_lRegionBufferCount = 0;

	return bRetCode;
}
/*************************************************************************************
* IsValidULONG: tests that lData > 0
*
**************************************************************************************/

BOOL cCombH::IsValidULONG(long lData)
{
	return (0 < lData);	
}

/*************************************************************************************
* GetCombination: gives the member variables to the calling function
*
**************************************************************************************/
void  cCombH::GetCombinations(long &lNumMembers,LPCOMBINATION& pCOMB)
{
	if (0 < m_lCombinationCount)
	{
		lNumMembers = m_lCombinationCount;
		pCOMB		= m_pCMCombinations;
	}
	else
	{
		lNumMembers = 0;
		pCOMB		= NULL;
	}
}

/*************************************************************************************
* Checks regions in region buffer are sorted sequentialy (ascending)  and unique
**************************************************************************************/
BOOL cCombH::CheckRegionsAreSequentialAndUnique(void)
{
	//trivially sorted list
	if (1 >= m_lRegionBufferCount) {return true;}
	
	//sort ascending
	for(LPULONG p = m_plRegionBuffer;p<m_plRegionBuffer+m_lRegionBufferCount-1;p++)
	{
		if (!(*(p+1) > *p)) {return false;}
	}
	
	return true;
}

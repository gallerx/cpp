//cNME.cpp Implementation File for cNME class
#include "stdafx.h"
#include "cNME.h"
/********************************************************************************************/
//destructor
cNME::~cNME(){}
/********************************************************************************************/
//9 arg constructor
cNME::cNME(
		const LPULONG plDJS,
			  ULONG   lNumDJS,
			  ULONG   lExclusionSpace,
		const LPULONG plPriorPerm,
			  ULONG   lNumPriorPerm,
		const LPULONG plPerm,
			  ULONG   lNumPerm,
			  ULONG   lActualEdge,
			  ULONG   lDuration,
			  ULONG   lModeID): cME(lExclusionSpace,
									  plPriorPerm,
									  lNumPriorPerm,
									  plPerm,
									  lNumPerm,
									  lActualEdge,
									  lDuration,
									  lModeID)
{
	BufferToVector(plDJS,&m_vDJS,lNumDJS);
}
/********************************************************************************************/
bool  SmallerActualEdgeAndDJS(const cNME &x, const cNME &y)
{
	if (x.m_lActualEdge < y.m_lActualEdge)
	{return true;}
	else
	{	
		if (x.m_lActualEdge == y.m_lActualEdge)
		{
			return (x.m_vDJS < y.m_vDJS);
		}
	}
	return false;
}
/********************************************************************************************/
bool  SmallerDJS(const cNME &x, const cNME &y)
{	return (x.m_vDJS < y.m_vDJS);}


/********************************************************************************************/
BOOL cNME::GetDJS(LPULONG plBuf,ULONG maxbytes)
{
	vector<ULONG>::iterator p = m_vDJS.begin();
	VectorToBuffer(plBuf,p,m_vDJS.size(),maxbytes);
	return true;
}

/********************************************************************************************/
ULONG cNME::GetNumDJS()
{return m_vDJS.size();}

/********************************************************************************************/

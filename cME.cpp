//cME.cpp Implementation File for cME class
#include "stdafx.h"
#include "cME.h"

/********************************************************************************************/
BOOL cME::GetPerm(LPULONG plBuf,ULONG maxbytes)
{
	vector<ULONG>::iterator p = m_vPerm.begin();
	VectorToBuffer(plBuf,p,m_vPerm.size(),maxbytes);	
	return true;
}
/********************************************************************************************/
BOOL  cME::GetPriorPerm(LPULONG plBuf,ULONG maxbytes)
{
	vector<ULONG>::iterator p = m_vPriorPerm.begin();
	VectorToBuffer(plBuf,p,m_vPriorPerm.size(),maxbytes);
	return true;
}
/********************************************************************************************/
BOOL  cME::VectorToBuffer(LPULONG plBuf,vector<ULONG>::iterator& p, ULONG lLimit,ULONG maxbytes)
{
	for(ULONG i = 0; i< lLimit;i++)
	{
		if(maxbytes > i*sizeof(ULONG))
		{CopyMemory(plBuf+i,(p+i),sizeof(ULONG));}
		else
		{break;}
	}
	
	return true;
}

/********************************************************************************************/
BOOL  cME::BufferToVector(LPULONG plBuf,vector<ULONG> *pV,ULONG lLimit)
{
	
	if (0 < lLimit)
	{
		for(ULONG i = 0; i <lLimit;i++)
		{pV->push_back(*(plBuf+i));}
	}

	return true;
}


/********************************************************************************************/
//destructor
cME::~cME(){}
/********************************************************************************************/
//7 - arg constructor constructor
cME::cME(ULONG   lExclusionSpace,
		 const   LPULONG plPriorPerm,
				 ULONG   lNumPriorPerm,
		 const	 LPULONG plPerm,
				 ULONG   lNumPerm,
				 ULONG   lActualEdge,
				 ULONG   lDuration,
				 ULONG   lMode)

{	
	m_lExclusionSpace = lExclusionSpace;
	m_lActualEdge	  = lActualEdge;
	m_lDuration		  = lDuration;
	m_lModeID		  = lMode;

	BufferToVector(plPriorPerm,&m_vPriorPerm,lNumPriorPerm);
	BufferToVector(plPerm,     &m_vPerm,     lNumPerm);

}
/********************************************************************************************/
bool operator<(const cME &x, const cME &y)
{	return (x.m_vPerm < y.m_vPerm);}

/********************************************************************************************/
bool SmallerActualEdge(const cME &x, const cME &y)
{	return (x.m_lActualEdge < y.m_lActualEdge);}

/********************************************************************************************/
bool SameActualEdge(const cME &x, const cME &y)
{	return (x.m_lActualEdge == y.m_lActualEdge);}

/********************************************************************************************/
void cME::SetActualEdge(ULONG lData)
{	m_lActualEdge = lData;}

/********************************************************************************************/
ULONG cME::GetExclusionSpace()
{	return m_lExclusionSpace;}

/********************************************************************************************/
ULONG cME::GetActualEdge()
{	return m_lActualEdge;}

/********************************************************************************************/
ULONG cME::GetDuration()
{	return m_lDuration;}

/********************************************************************************************/
ULONG cME::GetMode()
{	return m_lModeID;}

/********************************************************************************************/
ULONG cME::GetNumPerm()
{	return m_vPerm.size();}

/********************************************************************************************/
ULONG cME::GetNumPriorPerm()
{	return m_vPriorPerm.size();}

/********************************************************************************************/


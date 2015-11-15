z//cCH.cpp. Implementation file for CycleHandler algorithm

#include "StdAfx.h"
#include "cCH.h"

BOOL cCH::PushVertex(long lPKID)
{
	long lVertexCountSoFar = m_lVertexCount;
	long *plNewVector = 0;

	if (0 == lPKID) {return false;}
	
	lVertexCountSoFar++;
	plNewVector = new long [lVertexCountSoFar];
	if (0 == plNewVector) {return false;}

	if (0 != m_lVertexCount) 
	{
		CopyMemory(plNewVector,m_plVertexVector,(lVertexCountSoFar-1)*sizeof(long));
    	delete [] m_plVertexVector;
	}
	plNewVector[lVertexCountSoFar-1] = lPKID;
	m_plVertexVector = plNewVector;
	m_lVertexCount = lVertexCountSoFar;
	return true;
}



BOOL cCH::InitializeMatrix()
{
	
	if (0>= m_lVertexCount) {return false;}


	m_pbColumnBus = new BYTE[m_lVertexCount];
	ZeroMemory(m_pbColumnBus,m_lVertexCount*sizeof(BYTE));

	
	m_pbColumnSwitchVector = new BYTE[m_lVertexCount];
	FillMemory(m_pbColumnSwitchVector,m_lVertexCount*sizeof(BYTE),1);


	m_plScheduleVector = new long[m_lVertexCount];
	ZeroMemory(m_plScheduleVector,m_lVertexCount*sizeof(long));

	m_pbPredMatrix = new BYTE [m_lVertexCount*m_lVertexCount];	//allocate  square matrix
	ZeroMemory(m_pbPredMatrix,(m_lVertexCount)*(m_lVertexCount)*sizeof(BYTE));
	
	if(
		(0 == m_pbColumnBus)		  || 
		(0 == m_pbColumnSwitchVector) || 
		(0 ==m_plScheduleVector)      || 
		(0 == m_pbPredMatrix)
	  )
	{return false;}

	return true;

}

cCH::cCH()
{
	m_pbColumnBus			=	0;
	m_pbColumnSwitchVector	=	0;
	m_pbPredMatrix			=	0;
	m_plVertexVector		=	0;
	m_plScheduleVector		=	0;
	m_lVertexCount			=	0;
	m_lLayerCount			=	0;
}

cCH::~cCH()
{
	
	if (0 != m_pbColumnBus) 			{delete [] m_pbColumnBus;			m_pbColumnBus = 0; }
	if (0 != m_pbColumnSwitchVector)	{delete [] m_pbColumnSwitchVector;  m_pbColumnSwitchVector =0;}
	if (0 != m_plScheduleVector)		{delete [] m_plScheduleVector;		m_plScheduleVector =0;}
	if (0 != m_pbPredMatrix)			{delete [] m_pbPredMatrix;			m_pbPredMatrix =0;}
	if (0 != m_plVertexVector)			{delete [] m_plVertexVector;		m_plVertexVector = 0;}
	m_lVertexCount		= 0;
	m_lLayerCount		= 0;


}

long  cCH::GetVertexIndexByPKID(long lPKID)
{
	for(long i = 0 ; i <m_lVertexCount ;i++)
	{
		if (m_plVertexVector[i] == lPKID)
			{return i;}
	}

	return -1;
}

BOOL cCH::VertexBoundsCheck(long lRowIndex)
{
	if (0 <= lRowIndex  && m_lVertexCount >= lRowIndex)
		{return true;}
	else
		{return false;}
}
//NEW CODE
void cCH::BitShiftRight(void)
{
	//bitwise shift each element by 1 bit; moves the connectivity bit into the 
	//least significant bit position
	for(BYTE *p = m_pbPredMatrix ; p < (m_pbPredMatrix +  m_lVertexCount*m_lVertexCount);p++)
	{*p = (*p) >> 1;} 
	
}
//END NEW CODE

BOOL cCH::SetHigh(long lRowPKID, long lColPKID, long lDead)

{
	long lRowIndex = GetVertexIndexByPKID(lRowPKID);
	long lColIndex = GetVertexIndexByPKID(lColPKID);
	BYTE bytValue;

	if(!VertexBoundsCheck(lRowIndex) || !VertexBoundsCheck(lColIndex))
	{return false;}

	if (!(0 == lDead || 1 == lDead))
	{return false;}

	switch(lDead)
	{
	case 0:
		bytValue = LIVE_EDGE;
		break;
	case 1:
		bytValue = DEAD_EDGE;
		break;
	default:
		return false;

	}
			
	*(m_pbPredMatrix+((m_lVertexCount*lRowIndex)+lColIndex)) = bytValue;
	
	return true;

}
/*
BOOL cCH::SetHigh(long lRowPKID, long lColPKID)

{


	long lRowIndex = GetVertexIndexByPKID(lRowPKID);
	long lColIndex = GetVertexIndexByPKID(lColPKID);

	if(!VertexBoundsCheck(lRowIndex) || !VertexBoundsCheck(lColIndex))
	{return false;}

	*(m_pbPredMatrix+((m_lVertexCount*lRowIndex)+lColIndex)) = 1;
	
	return true;

}
*/
void cCH::QuickSchedule()
{
   ScheduleLayer(1);
}

void cCH::ScheduleLayer(long lCurrentLayer)
{
	bool bIterate  = false;
	long *plAffectedColumns;
	
	
	plAffectedColumns = new long [m_lVertexCount];
	ZeroMemory(plAffectedColumns,m_lVertexCount*sizeof(long),);
	
	
	
	long j = 0;
	for(long i = 0; i<m_lVertexCount;i++)
	{
		if (0 == m_plScheduleVector[i] && 0 == RowTotal(i) )
		{
		       m_plScheduleVector[i] = lCurrentLayer;
			   //slap an additional 1 on plAffected columns to avoid a possible zero value
			   //subtract the one when we read in for the call to ZeroColumn four lines below
			   plAffectedColumns[j] = i+1; j++; 
			   bIterate = true;
		}
	}

	
	
	//slap an additional 1 on plAffected columns to avoid a possible zero value
	//subtract the one when we read in for the call to ZeroColumn four lines below
	for(long *p = plAffectedColumns;(p<plAffectedColumns+m_lVertexCount)&&*p; p++) 
	{ ZeroColumn(*(p) -1);}
	
	delete [] plAffectedColumns;

	if (bIterate)
		{lCurrentLayer++;
		ScheduleLayer(lCurrentLayer);}
	else
		{m_lLayerCount = lCurrentLayer - 1;}
	
}

long cCH::RowTotal(long lRowIndex)
{

	long lSum = 0;
	BYTE* pbStart =  m_pbPredMatrix + lRowIndex*m_lVertexCount;
	for(BYTE *p = pbStart;p < m_pbPredMatrix + (lRowIndex+1)*m_lVertexCount;p++)
	
	//NEW CODE	
	//{lSum += *p &  *(m_pbColumnSwitchVector +(p - pbStart))   ;}
	{lSum += LSB_MASK(*p) &  *(m_pbColumnSwitchVector +(p - pbStart))   ;}
	//END NEW CODE
	return lSum;


}

void cCH::ZeroColumn(long lColumnIndex)
{
	*(m_pbColumnSwitchVector+lColumnIndex) =0;
}




BOOL cCH::IsPredecessor(long lPredPKID, long lSucPKID)
{

	ZeroMemory(m_pbColumnBus,m_lVertexCount*sizeof(BYTE));
	FillMemory(m_pbColumnSwitchVector,m_lVertexCount*sizeof(BYTE),1);


	//find Row and Column by index value given PKID

	long lPredIndex = GetVertexIndexByPKID(lPredPKID);
	long lSucIndex = GetVertexIndexByPKID(lSucPKID);

	if (!VertexBoundsCheck(lPredIndex) || !VertexBoundsCheck(lSucIndex))
	{return false;}

	//turn off columns which are at same layer or earlier as predIndex
	//turn off columns which are at same layer or later than sucIndex
	
	long lPredLayer = *(m_plScheduleVector+lPredIndex);
	long lSucLayer = *(m_plScheduleVector+lSucIndex);
	for(long i = 0;i<m_lVertexCount;i++)
	{
		
		if ((*(m_plScheduleVector+i) <= lPredLayer) && (i != lPredIndex))
			{ *(m_pbColumnSwitchVector+i) = 0;}

		if ((*(m_plScheduleVector+i) >= lSucLayer) && (i != lSucIndex))
			{ *(m_pbColumnSwitchVector+i) = 0;}

	}


	return Electrify(lPredIndex,lSucIndex); 
	

}

//slap the row identified by lSucIndex onto the ColumnBus with a bitwise OR
//exclusive or the new COlumnBus with the old columnbus, and recursively call Electrify
//on changed columns. Stop when the Predecessor we're looking for has been charged(value = 1)

BOOL cCH::Electrify(long lPredIndex, long lSucIndex)
{
	
	BYTE  *pbOldColumnBus = new BYTE[m_lVertexCount];
	CopyMemory(pbOldColumnBus,m_pbColumnBus,m_lVertexCount*sizeof(BYTE));

	for(BYTE *p =m_pbColumnBus; p<m_pbColumnBus + m_lVertexCount; p++)
	{
	
		*p = *p |  *(m_pbColumnSwitchVector+(p-m_pbColumnBus)) &  LSB_MASK(*(m_pbPredMatrix + ((lSucIndex*m_lVertexCount)+(p-m_pbColumnBus))) );
		//*p = *p |  *(m_pbColumnSwitchVector+(p-m_pbColumnBus)) &  *(m_pbPredMatrix + ((lSucIndex*m_lVertexCount)+(p-m_pbColumnBus)) );

	}

	//check if we've electrified the terminal point
	if( 1 == *(m_pbColumnBus+lPredIndex))
		{return TRUE;}
	
	for(p = m_pbColumnBus; p<m_pbColumnBus + m_lVertexCount; p++)
	{
		if(1 == (*p ^ *(pbOldColumnBus+(p-m_pbColumnBus)) )  )
		{
		 if (Electrify(lPredIndex,p-m_pbColumnBus))
		{	 delete []  pbOldColumnBus;
			 return true;} //if false, keep going through iteration until we fall out.
		
		
		}

	}
	
	delete []  pbOldColumnBus;
	return false;

	
}

//Returns all vertexs between the PredPKID and SUCPKID in a Win32 SAFEARRAY

int cCH::CycleMembers(long lPredPKID, long lSucPKID,long& lNumMembers,long* &plMembers)
{

	
	lNumMembers = 0;
	plMembers   = 0;//new long[lNumMembers];
			
	if (IsPredecessor(lPredPKID,lSucPKID)) 
	{	
		//add the endpoints
		lNumMembers +=2;
		plMembers = new long[lNumMembers];
		plMembers[lNumMembers-2] = lPredPKID;
		plMembers[lNumMembers-1] = lSucPKID;
		
		for (long *p = m_plVertexVector;p<m_plVertexVector + m_lVertexCount;p++)
		{
		
			if( IsPredecessor(lPredPKID,*p) && IsPredecessor(*p,lSucPKID) )
			{
				lNumMembers++;
				long *plNewMembers = new long[lNumMembers];
				plNewMembers[lNumMembers-1] = *p;
				CopyMemory(plNewMembers,plMembers,(lNumMembers-1)*sizeof(long));
				delete [] plMembers;
				plMembers = plNewMembers;
			}

		}
	}

	return 0;
}



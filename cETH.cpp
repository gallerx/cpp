//cETH.cpp  Implementation file for EventTimeHandler algorithm
//broadly, schedules event times and determines whether conflicts exists

#include "StdAfx.h"
#include "cETH.h"


BOOL cETH::PushVertex(ULONG lPKID, TIMESTAMP_STRUCT tsStart, TIMESTAMP_STRUCT tsFinish,
					  long lDuration, long ciConstraint,long bConflict,ULONG* plBrokenRules,
					  ULONG cbBrokenRules)
{

	if (0 >= lPKID)		  {return false;}
	if (0 > lDuration)	  {return false;}
	if (0 > cbBrokenRules) {return false;}
	if (!( CI_ASAP == ciConstraint ||
		   CI_SDK  == ciConstraint ||
		   CI_CE   == ciConstraint)) 
	{return false;}

	ULARGE_INTEGER ulTempStart  = TimestampToULargeInteger(tsStart);
	ULARGE_INTEGER ulTempFinish = TimestampToULargeInteger(tsFinish); 

	if (ulTempStart.QuadPart > ulTempFinish.QuadPart)
	{return false;}
	
	long lVertexCountSoFar = m_lVertexCount;
	LPEVENTTIME pETNewVector = 0;

	lVertexCountSoFar++;
	pETNewVector = new EVENTTIME [lVertexCountSoFar];
	if (0 == pETNewVector) {return false;}

	if (0 != m_lVertexCount) 
	{
		CopyMemory(pETNewVector,m_pETVector,(lVertexCountSoFar-1)*sizeof(EVENTTIME));
    	delete [] m_pETVector;
	}
	
	EVENTTIME *p	= (pETNewVector+lVertexCountSoFar-1);
	p->lPKID		= lPKID;
	p->ulStart		= ulTempStart;
	p->ulFinish     = ulTempFinish; 
	p->lDuration	= lDuration;
	p->ciConstraint = ciConstraint;
	p->bDirty		= false;
	p->bConflict    = bConflict;
	p->cbBrokenRules = cbBrokenRules;
	if (cbBrokenRules)
	{
		p->plBrokenRules = new ULONG[cbBrokenRules/sizeof(long)];
		CopyMemory(p->plBrokenRules,plBrokenRules,cbBrokenRules);
	}
	else
	{
		p->plBrokenRules = 0;
	
	}
	m_pETVector = pETNewVector;
	m_lVertexCount = lVertexCountSoFar;
	return true;
}

BOOL cETH::InitializeMatrix()
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

cETH::cETH()
{
	m_pbColumnBus			=	0;
	m_pbColumnSwitchVector	=	0;
	m_pbPredMatrix			=	0;
	m_pETVector				=	0;
	m_plScheduleVector		=	0;
	m_lVertexCount			=	0;
	m_lLayerCount           =   0;
}

cETH::~cETH()
{
	
	if (0 != m_pbColumnBus) 
	{
		delete [] m_pbColumnBus;
		 m_pbColumnBus = 0; 
	}
	if (0 != m_pbColumnSwitchVector)
	{
		delete [] m_pbColumnSwitchVector;
		m_pbColumnSwitchVector =0;
	}
	if (0 != m_plScheduleVector)
	{
		delete [] m_plScheduleVector;
		m_plScheduleVector =0;
	}
	if (0 != m_pbPredMatrix)
	{
		delete [] m_pbPredMatrix;
		m_pbPredMatrix =0;
	}
	if (0 != m_pETVector)
	{
		for (long i = 0; i<m_lVertexCount;i++)
		{
			LPEVENTTIME p = (m_pETVector+i);
		    
			if (p->plBrokenRules) 
			{ delete[] p->plBrokenRules;}
	
		}		
		delete [] m_pETVector;
		m_pETVector = 0;
	}
	
	m_lVertexCount		= 0;

}

long  cETH::GetVertexIndexByPKID(ULONG lPKID)
{
	for(long i = 0 ; i <m_lVertexCount ;i++)
	{
		if ((m_pETVector+i)->lPKID  == lPKID)
			{return i;}
	}

	return -1;
}

BOOL cETH::VertexBoundsCheck(long lRowIndex)
{
	if (0 <= lRowIndex  && m_lVertexCount >= lRowIndex)
		{return true;}
	else
		{return false;}
}

BOOL cETH::SetHigh(ULONG lRowPKID, ULONG lColPKID)

{

	long lRowIndex = GetVertexIndexByPKID(lRowPKID);
	long lColIndex = GetVertexIndexByPKID(lColPKID);

	if(!VertexBoundsCheck(lRowIndex) || !VertexBoundsCheck(lColIndex))
	{return false;}

	*(m_pbPredMatrix+((m_lVertexCount*lRowIndex)+lColIndex)) = 1;
	
	return true;
}


void cETH::ScheduleLayer(long lCurrentLayer)
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
long cETH::RowTotal(long lRowIndex)
{

	long lSum = 0;
	BYTE* pbStart =  m_pbPredMatrix + lRowIndex*m_lVertexCount;
	for(BYTE *p = pbStart;p < m_pbPredMatrix + (lRowIndex+1)*m_lVertexCount;p++)
	{lSum += *p &  *(m_pbColumnSwitchVector +(p - pbStart))   ;}
	return lSum;


}
void cETH::ZeroColumn(long lColumnIndex)
{
	*(m_pbColumnSwitchVector+lColumnIndex) =0;
}


void cETH::QuickSchedule()
{
   ScheduleLayer(1);
}

void cETH::TimeHandler(TIMESTAMP_STRUCT tsLimitDate, long& lNumDirtyET, LPEVENTTIME &pETDirty)
{


	QuickSchedule();

//Presumption: all members of vertex have been scheduled
//Start with layer 1 elements. Since they have no predecessor,
//their earliest date will be dLimit Date
//Note that layers are numbered from 1 upward

  ULARGE_INTEGER ulLimitDate = TimestampToULargeInteger(tsLimitDate);
	
	for (long lLayer = FIRST_LAYER; lLayer <= m_lLayerCount;lLayer++) //iterate through the layers
	{
		for (long i = 0;i < m_lVertexCount; i ++)//iterate through the vertices
		{	
			
			LPEVENTTIME p = (m_pETVector + i);
			
			if (m_plScheduleVector[i] == lLayer)
			{
			
				ULONG *plBrokenRules = 0;
				ULONG lBrokenRuleCount = 0;
			
				switch(p->ciConstraint)
				{
					case CI_ASAP:
						if( FIRST_LAYER == lLayer)
						{
							if (ulLimitDate.QuadPart != p->ulStart.QuadPart)
							{
								p->ulStart = ulLimitDate;
								p->ulFinish = AddDays(p->ulStart, p->lDuration);
								p->bDirty = true;
							}
						}
						else
						{
						  ULARGE_INTEGER ulTemp = LatestPredecessorFinish(i);	
					      if (ulTemp.QuadPart != p->ulStart.QuadPart)
						  {
							  p->ulStart = ulTemp; 
						      p->ulFinish = AddDays(p->ulStart, p->lDuration);
						      p->bDirty = true;
						  }
						}
						break;
					default:
						//evaluate scheduling Conflict Rules against predecessors
						if (FIRST_LAYER ==lLayer)
						{
						    //Rule 2: actual start earlier than deal announce date
							if (p->ulStart.QuadPart < ulLimitDate.QuadPart) 
							{ AddBrokenRule(plBrokenRules,BROKEN_RULE_TWO,lBrokenRuleCount);}

						}
						else
						{
						  //check each predecessor of vertex i
						  for (long j =0; j < m_lVertexCount; j++)
						  {
							if (1 == *(m_pbPredMatrix+i*m_lVertexCount+j)) //j is a predecessor of i
							{
								LPEVENTTIME e = (m_pETVector + j);
								switch(e->ciConstraint)
								{
									case CI_ASAP:
										if(p->ulStart.QuadPart < e->ulStart.QuadPart)
										{AddBrokenRule(plBrokenRules,BROKEN_RULE_ONE,lBrokenRuleCount);}
										break;
									case CI_SDK:
										 if(p->ulStart.QuadPart < e->ulStart.QuadPart)
										{AddBrokenRule(plBrokenRules,BROKEN_RULE_TWO,lBrokenRuleCount);}
										 break;
									case CI_CE:
										if(p->ulStart.QuadPart < e->ulFinish.QuadPart)
										{AddBrokenRule(plBrokenRules,BROKEN_RULE_THREE,lBrokenRuleCount);}
										 break;
								}//switch(e->ciConstraint)
								
							}//if(j is a predecessor of i)
						  }//forfor (long j =0; j < m_lVertexCount; j++)
							
						}//else
						break;
				}//switch(p->ciConstraint)

			//now check plBrokenRules against p->plBrokenRules and decide whether to set dirty or not
			if (lBrokenRuleCount)//any broken rules?
			{
				p->bConflict = true; //set conflict = true
				if (lBrokenRuleCount != p->cbBrokenRules/sizeof(long))//number of broken rules doens't match
				{	//set dirty, and replace whatever existing
					p->bDirty = true;
					p->cbBrokenRules = lBrokenRuleCount*sizeof(long);
					if(0 != p->plBrokenRules) {delete [] p->plBrokenRules;}
   					p->plBrokenRules = plBrokenRules;
				}
				else//number of broken rules matches, check if they are same
				{
					if (0 != memcmp(p->plBrokenRules,plBrokenRules,p->cbBrokenRules))
					{	//set dirty, and replace whatever existing
						p->bDirty = true;
						p->cbBrokenRules = lBrokenRuleCount*sizeof(long);
						if(0 != p->plBrokenRules) {delete [] p->plBrokenRules;}
   						p->plBrokenRules = plBrokenRules;
					}
				}
			}
			else
			{
				if (p->bConflict) 
				{p->bConflict = false;
				 p->cbBrokenRules = 0;
				 p->bDirty = true;
				}
				
			}
			
			
			
			
			
			
			}//if (m_plScheduleVector[i] == lLayer)

			
				
		}
	}

	
	//clean out the non-dirty members of the EventTime Vector
	for (long i = 0; i < m_lVertexCount; i++)
	{
		LPEVENTTIME p = (m_pETVector+i);
		LPEVENTTIME pETNewVector = 0;
		if (!(p->bDirty))
		{
			//get rid of memory associated with p->szBrokenRules
			if (p->plBrokenRules) 
			{ delete[] p->plBrokenRules;}
			
			m_lVertexCount--;

			if (0 < m_lVertexCount)
			{
				
				pETNewVector = new EVENTTIME[m_lVertexCount];
				CopyMemory(pETNewVector,m_pETVector,(i)*sizeof(EVENTTIME));
				CopyMemory(pETNewVector+i,m_pETVector+(i+1),(m_lVertexCount - i)*sizeof(EVENTTIME));
				i--; //step back 
			}
			delete [] m_pETVector;
			m_pETVector = pETNewVector;
		}

	}
	

	lNumDirtyET = m_lVertexCount;
	pETDirty    = m_pETVector;	

}

ULARGE_INTEGER cETH::LatestPredecessorFinish(long lRowIndex)
{
	ULARGE_INTEGER ulTemp;
	ulTemp.QuadPart =0;
	for (long j =0; j < m_lVertexCount; j++)
	{
		if (1 == *(m_pbPredMatrix+lRowIndex*m_lVertexCount+j)) //j is a predecessor of lRowIndex
		{
			LPEVENTTIME p = (m_pETVector + j);
			if (ulTemp.QuadPart  < p->ulFinish.QuadPart)
			{ulTemp = p->ulFinish;} 
		}
	}
	return ulTemp;
}



ULARGE_INTEGER cETH::TimestampToULargeInteger(TIMESTAMP_STRUCT tsDate)
{
	SYSTEMTIME stDate;
	FILETIME   ftDate;
	ULARGE_INTEGER ulDate;	

	stDate.wYear		 = tsDate.year;
	stDate.wMonth		 = tsDate.month;
	stDate.wDay			 = tsDate.day;
	stDate.wDayOfWeek	 = 0;
	stDate.wHour		 = 0;
	stDate.wMinute		 = 0;
	stDate.wSecond		 = 0;
	stDate.wMilliseconds = 0;

	SystemTimeToFileTime(&stDate,&ftDate);
	CopyMemory(&ulDate,&ftDate,sizeof(ulDate));
	
	return ulDate;

}

TIMESTAMP_STRUCT cETH::ULargeIntegerToTimestamp(ULARGE_INTEGER ulDate)
{
	SYSTEMTIME stDate;
	FILETIME   ftDate;
	TIMESTAMP_STRUCT tsDate;
	
	CopyMemory(&ftDate,&ulDate,sizeof(ulDate));
	FileTimeToSystemTime(&ftDate,&stDate);
	tsDate.year		= stDate.wYear; 
	tsDate.month	= stDate.wMonth;
	tsDate.day		= stDate.wDay;
	tsDate.fraction = 0;
	tsDate.hour		= 0;
	tsDate.minute	= 0;
	tsDate.second	= 0;
	
	return tsDate;

}

ULARGE_INTEGER cETH::AddDays(ULARGE_INTEGER ulDate,long nDays)
{
		#define NEG7_SEC_PER_DAY 864000000000
		ulDate.QuadPart = ulDate.QuadPart + nDays * NEG7_SEC_PER_DAY;
		return ulDate;
}


BOOL cETH::AddBrokenRule(ULONG *&plBrokenRules,ULONG lBrokenRule,ULONG &lBrokenRuleCount)
{
	long lVertexCountSoFar = lBrokenRuleCount;
	ULONG *plNewVector = 0;

	if (!(BROKEN_RULE_ONE	== lBrokenRule||
		  BROKEN_RULE_TWO   == lBrokenRule||
		  BROKEN_RULE_THREE == lBrokenRule
		))
		{return false;}
	
	lVertexCountSoFar++;
	plNewVector = new ULONG [lVertexCountSoFar];
	if (0 == plNewVector) {return false;}

	if (0 != lBrokenRuleCount) 
	{
		CopyMemory(plNewVector,plBrokenRules,(lVertexCountSoFar-1)*sizeof(long));
    	delete [] plBrokenRules;
	}
	plNewVector[lVertexCountSoFar-1] = lBrokenRule;
	plBrokenRules = plNewVector;
	lBrokenRuleCount = lVertexCountSoFar;
	return true;

}

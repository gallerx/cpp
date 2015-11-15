//cTH.cpp implementation file for TransformHandler
#include "stdafx.h"
#include "cTH.h"

/********************************************************************************************/
//destructor
cTH::~cTH() {}

/********************************************************************************************/
//one argument constructor
cTH::cTH(short int iMode)
{	 m_iMode = iMode;}

/********************************************************************************************/
//PushVertex for evaluating ME
BOOL cTH::PushVertex(ULONG   lExclusionSpace, const LPULONG plPriorPerm,  ULONG   lNumPriorPerm,
											  const LPULONG plPerm,       ULONG   lNumPerm, 
													  ULONG lActualEdge,  ULONG	lDuration,
													  ULONG lMode)
{
		m_vNME.push_back(cNME(NULL,0,lExclusionSpace,plPriorPerm,lNumPriorPerm,plPerm,lNumPerm,
			                lActualEdge,lDuration,lMode));
		return true;
}
/********************************************************************************************/
//Overloaded PushVertex for when evaluating NME
BOOL cTH::PushVertex(const LPULONG plDJS,     	 ULONG   lNumDJS,		 ULONG   lExclusionSpace,
					 const LPULONG plPriorPerm,	 ULONG   lNumPriorPerm,	
					 const LPULONG plPerm,       ULONG   lNumPerm,		 ULONG   lActualEdge,
												 ULONG   lDuration,		 ULONG	 lMode)
{
		m_vNME.push_back(cNME(plDJS,lNumDJS,lExclusionSpace,plPriorPerm,lNumPriorPerm,plPerm,
							 lNumPerm,lActualEdge,lDuration,lMode));
		return true;
}
/********************************************************************************************/
BOOL cTH::PushDeadCycle(ULONG lCyclePKID)
{
	m_vDeadCycle.push_back(lCyclePKID);
	return true;
}
/********************************************************************************************/
BOOL cTH::PushDeadEdge(ULONG lDeadEdgePKID, ULONG lAssignedToPKID)
{
	m_mDeadEdge.insert(pair<ULONG,ULONG>(lDeadEdgePKID,lAssignedToPKID));
	return true;
}

/********************************************************************************************/
BOOL  cTH::RingCycle()
{
	if (!InspectPermutation())		   {return false;}

	switch (this->GetMode())
	{
	case ME_MODE:
		if (!ModifyActualEdge())	   {return false;} 
		break;
	case NME_MODE:
		if (!ModifyActualEdgeAndDJS()) {return false;}
		break;
	}
	return true;
}
/********************************************************************************************/
BOOL cTH::GetIterator(vector<cNME>::iterator& itrNME, ULONG& lNumMembers)
{
	itrNME		= m_vNME.begin();
	lNumMembers = m_vNME.size();
	return true;
}

/********************************************************************************************/
void cTH::CALL_MACRO(UAction UCall, BAction BCall,
					 vector<cNME>::iterator& p,vector<cNME>::iterator q)
{
	if (q != p )
		{(this->*BCall)(p,q);} 
	else
		{(this->*UCall)(p);}   
}

/********************************************************************************************/
void cTH::FindMaxExtent(vector<cNME>::iterator& p, vector<cNME>::iterator& q,
					    BoolComp pfComp, ULONG lData, vector<ULONG>* pvData)
{
	q = p;
	{	
		if ((p!=m_vNME.end()) && ((p+1) != m_vNME.end()))  //check if we're already at the last entry. 
		{
			//if not, run q out to one past the last point where the requested comparison 
			//holds true

			for(q = p+1;((q != m_vNME.end())&&((this->*pfComp)(*q,lData,pvData)));q++){}
		}
	}
	
}
/********************************************************************************************/
void cTH::DoNothing(vector<cNME>::iterator& a)
{	a++;} //increment the a iterator [keep the same type of syntax as STL .erase()]

/********************************************************************************************/
void cTH::DoNothing(vector<cNME>::iterator& a,vector<cNME>::iterator& b)
{	a = b;} //move the a iterator to the end, keeps same syntax as erase()

/********************************************************************************************/
void cTH::Erase(vector<cNME>::iterator& a)
{	m_vNME.erase(a);}

/********************************************************************************************/
void  cTH::Erase(vector<cNME>::iterator& a, vector<cNME>::iterator& b)
{	m_vNME.erase(a,b);}

/********************************************************************************************/
void  cTH::ReplaceActualEdge(vector<cNME>::iterator& a,vector<cNME>::iterator& b)
{	CommonReplacementAlgorithm(a,b,0x01);}

/********************************************************************************************/
void cTH::ReplaceActualEdgeAndDJS(vector<cNME>::iterator& a, vector<cNME>::iterator& b)
{	CommonReplacementAlgorithm(a,b,0x11);}
	
/********************************************************************************************/
void cTH::ReplaceDJS(vector<cNME>::iterator& a, vector<cNME>::iterator& b)
{	CommonReplacementAlgorithm(a,b,0x10);}

/********************************************************************************************/
void cTH::CommonReplacementAlgorithm(vector<cNME>::iterator& itRef,
									 vector<cNME>::iterator& b, BYTE  bytSwitch)
{
	//assumptions: replace all  DJSs and/or Actual edges from one past itRef up to but not including b
	//with the values contained in *pRef
	for(vector<cNME>::iterator x = itRef+1; x < b;x++)
	{
		if (bytSwitch & 0x01)
		{	x->SetActualEdge(itRef->GetActualEdge());}
	
		if (bytSwitch & 0x10)
		{	x->m_vDJS = itRef->m_vDJS;}
	}
	itRef = b;
}
/********************************************************************************************/
short	cTH::GetMode(void)
{	return m_iMode;}			

/********************************************************************************************/
bool	cTH::SamePerm		        (cNME& x, ULONG lData, vector<ULONG>* pvData)
{	return (x.m_vPerm == *pvData);}

/********************************************************************************************/
bool	cTH::SameActualEdge         (cNME& x, ULONG lData, vector<ULONG>*  pvData)
{	return (x.m_lActualEdge == lData);}

/********************************************************************************************/
bool	cTH::SameDJS		        (cNME& x, ULONG lData, vector<ULONG>*  pvData)
{	return (x.m_vDJS == *pvData);}

/********************************************************************************************/
bool	cTH::SameActualEdgeAndDJS   (cNME& x, ULONG lData, vector<ULONG>*  pvData)
{	return (cTH::SameActualEdge(x,lData,pvData) && cTH::SameDJS(x,lData,pvData));}


/********************************************************************************************/
BOOL cTH::InspectPermutation()
{
	//need to be sorted by permutation, ascending
	sort(m_vNME.begin(),m_vNME.end());  //default sort is by permutation
	vector<cNME>::iterator p = m_vNME.begin();
	while(p < m_vNME.end())
	{
		//check if any edges in the permutation cycle are dead
		vector<ULONG>::iterator z = find_first_of(m_vDeadCycle.begin(),m_vDeadCycle.end(),
												  p->m_vPerm.begin(),p->m_vPerm.end());
		

		if(z != m_vDeadCycle.end()) //has found a match
		{
		
			vector<ULONG> vOriginalPerm = p->m_vPerm;

			BAction  BCall = &cTH::Erase;
			UAction  UCall = &cTH::Erase; 
			
			vector<cNME>::iterator q;
			FindMaxExtent(p,q,&cTH::SamePerm,0,&vOriginalPerm);
			CALL_MACRO(UCall,BCall,p,q);
		}
		else
		{p++;}  //only increment p if we didn't erase. if we did erase, p is automatically
				//pointing to the next element to handle
	}
	return true;
}

/********************************************************************************************/
BOOL	cTH::ModifyActualEdge(void)
{
	switch(m_iMode)
	{
	case ME_MODE:
		sort(m_vNME.begin(),m_vNME.end(),SmallerActualEdge); //sort by Actual Edge, ascending
		break;
	case NME_MODE:
		return false;
		break;
	}
	
	vector<cNME>::iterator p = m_vNME.begin();	
	while (p<m_vNME.end())
	{
		ULONG lOriginalEdge = p->GetActualEdge();

		map<ULONG,ULONG>::iterator z = m_mDeadEdge.find(lOriginalEdge);
		if (z!= m_mDeadEdge.end())//has found a match
		{
			BAction  BCall;
			UAction  UCall;
			
			ULONG lAssignedToPKID = z->second;
			
			if (0== lAssignedToPKID)
			{
				BCall  = &cTH::Erase;
				UCall  = &cTH::Erase;
			}
			else
			{
				p->SetActualEdge(lAssignedToPKID);
				BCall  = &cTH::ReplaceActualEdge;
				UCall  = &cTH::DoNothing; //we've already taken the action
			}	

			vector<cNME>::iterator q;
			FindMaxExtent(p,q,&cTH::SameActualEdge,lOriginalEdge,0);
			CALL_MACRO(UCall,BCall,p,q);
		}
		else
		{p++;}
	}
	return true;
};
	
/********************************************************************************************/
BOOL	cTH::ModifyActualEdgeAndDJS(void)
{
	if(!Pass1()) {return false;}
	if(!Pass2()) {return false;}
	return true;
}

/********************************************************************************************/
BOOL  cTH::Pass1(void)
{
	switch(m_iMode)
	{
	case ME_MODE:
		return false;
		break;
	case NME_MODE:
		sort(m_vNME.begin(),m_vNME.end(),SmallerActualEdgeAndDJS); //sort by Actual Edge and DJS, ascending
		break;
	}

	vector<cNME>::iterator p = m_vNME.begin();
	while (p<m_vNME.end())
	{
		map<ULONG,ULONG>::iterator z = m_mDeadEdge.find(p->GetActualEdge());
		if (z!= m_mDeadEdge.end())//has found matches
		{
			BAction BCall;
			UAction  UCall;

			ULONG lDeadPKID	      = z->first;
			ULONG lAssignedToPKID = z->second;

			vector<ULONG> vOriginalDJS	= p->m_vDJS;
			ULONG		  lOriginalEdge = p->GetActualEdge();

			if (0== lAssignedToPKID) //no Assignment
			{
				BCall = &cTH::Erase;
				UCall = &cTH::Erase;
			}
			else
			{
				vector<ULONG>::iterator D = find(p->m_vDJS.begin(),p->m_vDJS.end(),lDeadPKID);
				vector<ULONG>::iterator A = find(p->m_vDJS.begin(),p->m_vDJS.end(),lAssignedToPKID);

				if(D == p->m_vDJS.end()) {return false;} //somethings wrong, we should have found D

				if(A!=p->m_vDJS.end()) //Dead and Assigned Present
				{	//get rid of record to avoid double counting
					BCall		        = &cTH::Erase;
					UCall				= &cTH::Erase;
				}
				else //Dead present, Assigned missing
				{
					//change D1A0 -> D0A1
					{*D = lAssignedToPKID; 
					p->m_lActualEdge = lAssignedToPKID;
					sort(p->m_vDJS.begin(),p->m_vDJS.end());}

					BCall		      = &cTH::ReplaceActualEdgeAndDJS;
					UCall			  = &cTH::DoNothing; //as we've already done the work above
				}
			}	

			vector<cNME>::iterator q;
			FindMaxExtent(p,q,&cTH::SameActualEdgeAndDJS,lOriginalEdge,&vOriginalDJS);
			CALL_MACRO(UCall,BCall,p,q);
		}
		else
		{p++;}
	}
	return true;
}
/********************************************************************************************/
BOOL cTH::Pass2(void)
{
	switch(m_iMode)
	{
	case ME_MODE:
		return false;
		break;
	case NME_MODE:
		sort(m_vNME.begin(),m_vNME.end(),SmallerDJS); //sort by DJS, ascending
		break;
	}

	vector<cNME>::iterator p = m_vNME.begin();

	vector<ULONG>    vOriginalDJS;
	map<ULONG,ULONG> mDest;
	

	bool bDirty = false;

	while (p<m_vNME.end())
	{
		vOriginalDJS = p->m_vDJS;
		mDest.clear();
		
		set_intersection(m_mDeadEdge.begin(),m_mDeadEdge.end(),p->m_vDJS.begin(),p->m_vDJS.end(),inserter(mDest,mDest.begin()));//mDest.end());//,CompareMapWithULONG);
		
		ULONG BillTheCat = mDest.size();

		for(map<ULONG,ULONG>::iterator z = mDest.begin();z != mDest.end();z++)
		{
				
				ULONG lDeadPKID	      = z->first;
				ULONG lAssignedToPKID = z->second;
				
				vector<ULONG>::iterator D = find(p->m_vDJS.begin(),p->m_vDJS.end(),lDeadPKID);
				if (D == p->m_vDJS.end()) {return false;} //something's gone wrong if couldn't find D

				if (0== lAssignedToPKID) //no Assignment D1 -> D0
				{
					p->m_vDJS.erase(D); //no need to sort, we knew dead was present because of the set intersection
					bDirty = true;
				}
				else
				{
					vector<ULONG>::iterator A = find(p->m_vDJS.begin(),p->m_vDJS.end(),lAssignedToPKID);

					if(A!=p->m_vDJS.end()) //Dead and Assigned Present, D1A1->D0A1
					{
						p->m_vDJS.erase(D); //no need to sort, we knew dead was present because of the set intersection
						bDirty = true;
					}
					else  //Dead Present, assigned missing, D1A0->D0A1
					{
						*D = lAssignedToPKID; 
						sort(p->m_vDJS.begin(),p->m_vDJS.end()); //sort the DJS, as the assigned will be out of order
						bDirty = true;						
					}
				}
		}	
		
		if (bDirty)
			{
				BAction BCall		      = &cTH::ReplaceDJS;
				UAction UCall			  = &cTH::DoNothing; //as we've already done the work above

				vector<cNME>::iterator q;
				FindMaxExtent(p,q,&cTH::SameDJS,0,&vOriginalDJS);
				CALL_MACRO(UCall,BCall,p,q);

				bDirty = false;
			}
		else
			{p++;}
	}
	return true;
}
/********************************************************************************************/
bool operator<(const pair<ULONG const,ULONG>& x, const ULONG& lValue)
{
	return (x.first < lValue);

}

/********************************************************************************************/
bool operator<(const ULONG& lValue, const pair<ULONG const,ULONG>& x)
{
	return (lValue < x.first) ;
}

/********************************************************************************************/

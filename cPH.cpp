//cPH.cpp Implementation File for Permutation Handler


#include "StdAfx.h"
#include "cPH.h"
#include <windows.h>

#define LINKED_BUT_NOT_RECURSION_POINT -1
cPH::cPH()
{
	m_pELPredMatrix			= 0;
	m_pbVertexCycleMatrix	= 0;
	m_pPMPermutations		= 0;
	m_plVertexVector		= 0;
	m_plCycleVector			= 0;
	m_lVertexCount			= 0;
	m_lCycleCount			= 0;
	m_lPermutationCount		= 0;

}

/*************************************************************************************
* PushVertex: pushes a vertex onto the global Vertex Vector
*
*
**************************************************************************************/
BOOL cPH::PushVertex(long lPKID)
{
	long lVertexCountSoFar = m_lVertexCount;
	long *plNewVector = 0;

	if ( 0>= lPKID)	{return false;}

	lVertexCountSoFar++;
	plNewVector = new long [lVertexCountSoFar];
	if (0 == plNewVector) {return false;}

	if (0 != m_lVertexCount) 
	{
		CopyMemory(plNewVector,m_plVertexVector,(lVertexCountSoFar-1)*sizeof(long));
    	delete [] m_plVertexVector;
	}
	*(plNewVector+lVertexCountSoFar-1) = lPKID;
	m_plVertexVector = plNewVector;
	m_lVertexCount = lVertexCountSoFar;
	return true;
}

/*************************************************************************************
* PushCycle: pushes a cycle onto the global Cycle Vector
*
*
**************************************************************************************/
BOOL cPH::PushCycle(long lPKID)
{
	long lCycleCountSoFar = m_lCycleCount;
	long *plNewVector = 0;

	if (0 >= lPKID)	{return false;}


	lCycleCountSoFar++;
	plNewVector = new long [lCycleCountSoFar];
	if (0 == plNewVector) {return false;}	

	if (0 != m_lCycleCount) 
	{
		CopyMemory(plNewVector,m_plCycleVector,(lCycleCountSoFar-1)*sizeof(long));
    	delete [] m_plCycleVector;
	}
	*(plNewVector+lCycleCountSoFar-1) = lPKID;
	m_plCycleVector = plNewVector;
	m_lCycleCount = lCycleCountSoFar;

	return true;
}

long cPH::GetVertexIndexByPKID(long lPKID)
{
	for(long i = 0 ; i <m_lVertexCount ;i++)
	{
		if (m_plVertexVector[i] == lPKID)
			{return i;}
	}

	return -1;
}

long cPH::GetCycleIndexByPKID(long lPKID) 
{
	for(long i = 0 ; i <m_lCycleCount ;i++)
	{

		if (m_plCycleVector[i] == lPKID)
			{return i;}
		
	}
	
	return -1;
}


BOOL cPH::VertexBoundsCheck(long lRowIndex)
{
	if (0 <= lRowIndex  && m_lVertexCount >= lRowIndex)
		{return true;}
	else
		{return false;}
}

BOOL cPH::CycleBoundsCheck(long lColIndex)
	{
	if (0 <= lColIndex  && m_lCycleCount >= lColIndex)
		{return true;}
	else
		{return false;}
}



/*************************************************************************************
* InitializeMatrix: Initializes all the globals
*
*
**************************************************************************************/

BOOL  cPH::InitializePredMatrix()
{

	if (0 >= m_lVertexCount) 
	{return false;}

	m_pELPredMatrix = new ELEMENT [m_lVertexCount*m_lVertexCount];	//allocate  square matrix
	ZeroMemory(m_pELPredMatrix,(m_lVertexCount)*(m_lVertexCount)*sizeof(ELEMENT));

	if (0 == m_pELPredMatrix)
	{return false;}

	return true;
}


BOOL  cPH::InitializeVertexCycleMatrix()
{

	if (0 >= m_lVertexCount || 0 >= m_lCycleCount) 
	{return false;}

	m_pbVertexCycleMatrix = new BYTE[m_lVertexCount*m_lCycleCount];
	ZeroMemory(m_pbVertexCycleMatrix,(m_lVertexCount)*(m_lCycleCount)*sizeof(BYTE));

	if ( 0 == m_pbVertexCycleMatrix)
	{return false;}

	return true;
}



/*************************************************************************************
* void APIENTRY TerminateMatrix()
*   destroys allocated memory for globals
*
**************************************************************************************/


cPH::~cPH()
{
	if (0 != m_pPMPermutations) 
	{	
		for(long i = 0; i<m_lPermutationCount;i++)
		{	if (0 < (m_pPMPermutations+i)->lCycleCount)
			{
				if (0 !=(m_pPMPermutations+i)->plCycles)
				{
					delete [] (m_pPMPermutations+i)->plCycles;

				}
			}
			if (0 < (m_pPMPermutations+i)->lPriorCycleCount)
			{
				if (0 !=(m_pPMPermutations+i)->plPriorCycles)
				{
					delete [] (m_pPMPermutations+i)->plPriorCycles;

				}
			}

		}
		
		delete []  m_pPMPermutations;
	}
	if (0 != m_plVertexVector)
	{
		 delete [] m_plVertexVector;
		 m_plVertexVector =0;
	}
	if (0 != m_plCycleVector)
	{
		 delete [] m_plCycleVector;
		 m_plCycleVector =0;
	}
	if (0 != m_pELPredMatrix)
	{
		delete [] m_pELPredMatrix;
		m_pELPredMatrix = 0;
	}
	if (0 != m_pbVertexCycleMatrix)
	{
		delete [] m_pbVertexCycleMatrix;
		m_pbVertexCycleMatrix = 0;
	}
	m_lVertexCount		= 0;
	m_lCycleCount		= 0;
	m_lPermutationCount	= 0;
}





//usage, Row = Event, Col = Predecessor, Value = CyclePKID or -1,
//cyclePKID if a recursion point, -1 if not
// LinkPKID = LinkPKID
//between predecessor and event
BOOL cPH::SetVertexHigh(long lRowPKID,long lColPKID,long lValue,long lLinkPKID)

{
	long lRowIndex = GetVertexIndexByPKID(lRowPKID);
	long lColIndex = GetVertexIndexByPKID(lColPKID);

	if (!VertexBoundsCheck(lRowIndex) || !VertexBoundsCheck(lColIndex))
		{return false;}
	
	if (!(LINKED_BUT_NOT_RECURSION_POINT == lValue || 0 < lValue))
		{return false;}

	LPELEMENT p =  m_pELPredMatrix+(m_lVertexCount*lRowIndex)+lColIndex;

	p->Value = lValue;
	p->LinkFKID = lLinkPKID;

	return true;

}


BOOL cPH::SetVertexCycleHigh(long lVertexPKID, long lCyclePKID)

{
	long lRowIndex = GetVertexIndexByPKID(lVertexPKID);
	long lColIndex = GetCycleIndexByPKID(lCyclePKID);
	
	if (!VertexBoundsCheck(lRowIndex) || !CycleBoundsCheck(lColIndex))
		{return false;}

	*(m_pbVertexCycleMatrix+((m_lCycleCount*lRowIndex)+lColIndex)) = 1;

	return true;

}


/*************************************************************************************
* long HasPredecessor(long lRowIndex)
*   row-wise addition of Predecessor Matrix
*   A zero return value indicates no predecessors exist for the referenced (input) vertex
**************************************************************************************/

BOOL cPH::HasPredecessor(long lRowIndex)
{
	
	if (!VertexBoundsCheck(lRowIndex)) {return false;}


	//check for existence of any nonzero values
	ELEMENT* pELStart =  m_pELPredMatrix + lRowIndex*m_lVertexCount;

	for(ELEMENT *p = pELStart;p < pELStart + m_lVertexCount;p++)
	{ if (0 != p->Value ) {return true;} }
	
	return false;

}



void  cPH::GetPermutations(long &lNumMembers,LPPERMUTATION& pPERM)
{
	GeneratePermutations();
	
	if (0 < m_lPermutationCount)
	{
		
		//NewCode
		cPH::SortAndRemoveDuplicatePermutations();
		//End NewCode
		lNumMembers = m_lPermutationCount;
		pPERM = m_pPMPermutations;
	}
	else
	{
		lNumMembers = 0;
		pPERM = 0;
	}
	

}

//New CODE
BOOL cPH::SortAndRemoveDuplicatePermutations()
{
	
	if (1 >= m_lPermutationCount) //trivially sorted list
		return true;
	
	//sort ascending
	for(long i = 0;i<m_lPermutationCount-1;i++)
	{
		for(long j = i+1; j>0;j--)
		{
			char szComp1[4096];
			char szComp2[4096];

			cPH::PermSortValue(m_pPMPermutations+j,szComp1);
			cPH::PermSortValue(m_pPMPermutations+j-1,szComp2);

			

			if 	(0 > strcmpi(szComp1,szComp2))
			{//swap
				PERMUTATION permTEMP     =  *(m_pPMPermutations+j);
				*(m_pPMPermutations+j)   =  *(m_pPMPermutations+j-1); 
				*(m_pPMPermutations+j-1) =  permTEMP;
			}

		}
	}
	//delete

	for (i = 0; i < m_lPermutationCount-1; i++)
	{
		
		LPPERMUTATION pPMNew = 0;
		//check if duplicate
		char szComp1[4096];
		char szComp2[4096];

		cPH::PermSortValue(m_pPMPermutations+i,szComp1);
		cPH::PermSortValue(m_pPMPermutations+i+1,szComp2);

		if (0 == strcmpi(szComp1,szComp2))
		{
			m_lPermutationCount--;

			if (0 < m_lPermutationCount)
			{
				
				pPMNew = new PERMUTATION[m_lPermutationCount];
				CopyMemory(pPMNew,m_pPMPermutations,(i)*sizeof(PERMUTATION));
				CopyMemory(pPMNew+i,m_pPMPermutations+(i+1),(m_lPermutationCount - i)*sizeof(PERMUTATION));
				i--; //step back 
			}
			delete [] m_pPMPermutations;
			m_pPMPermutations = pPMNew;

		}

	}
	return true;
}

void cPH::PermSortValue(LPPERMUTATION lpPERM, char * szBuf)
{
	const CHAR_PER_LONG = 10; //takes 10 alphanumeric characters to capture highest value of long
	const RADIX			= 10; //base 10 for humans

	char cEventBUF[CHAR_PER_LONG];
	char cLinkBUF [CHAR_PER_LONG];
	char cpermBUF [CHAR_PER_LONG];

	ltoa(lpPERM->EventFKID,cEventBUF,RADIX);
	ltoa(lpPERM->LinkFKID,cLinkBUF,RADIX);
	
	strcpy(szBuf,cEventBUF);
	strcat(szBuf,",");//terminating comma
	strcat(szBuf,cLinkBUF);//terminating comma
	strcat(szBuf,",");//terminating comma


	for(long i = 0; i< lpPERM->lCycleCount;i++)
	{
		ltoa(*(lpPERM->plCycles+i),cpermBUF,RADIX);
		strcat(szBuf,cpermBUF);
		strcat(szBuf,",");//terminating comma
		ZeroMemory(cpermBUF,CHAR_PER_LONG);
	}
		
	return;

}


/*************************************************************************************
* void APIENTRY TerminateMatrix()
*   destroys allocated memory for globals
*
**************************************************************************************/
void  cPH::GeneratePermutations()
{
	
	for(int i = 0;i < m_lVertexCount; i++)
	{ 
		if (0 == HasPredecessor(i))
		{
			ULONG *plCycleStack = 0;
			DepthWiseWalk(i,plCycleStack,0);
			if (0 != plCycleStack)
			{ delete [] plCycleStack;}
		}

	}
}

//cycle stack is an array of all the cycles that we have traversed to get
//to this point
//lNUM_CYCLES holds the number of cycles in the stack

void cPH::DepthWiseWalk(long lColIndex, ULONG * plCycleStack,long lNUM_CYCLES)
{

	if (!VertexBoundsCheck(lColIndex)) {return ;}
	if (!(0<=lNUM_CYCLES))             {return ;}
	
	//walk down columns
	for (int i = 0; i < m_lVertexCount; i++)
	{
		LPELEMENT q = m_pELPredMatrix + lColIndex + i*m_lVertexCount;
		long lValue =  q->Value;
		if (0 < lValue)
		{		
		
					//element is a successor and a recursion point, so add the 
					//name of the cycle to the cycle stack and keep going

				 //create a new cycle stack  and copy the old data into it
				  //don't delete the old cycle stack, as it will get used 
				  //on next iteration of i.
				  //old cycle stack only gets deleted after we fall through the 
				 //for loop, above.
				  //also, don't increment lNUM_CYCLES, because 
				 //it will get used on next iteration of i.
				   ULONG * plNewCycleStack	= new ULONG[lNUM_CYCLES+1];
				   if (0 < lNUM_CYCLES )
				   {CopyMemory(plNewCycleStack,plCycleStack,lNUM_CYCLES*sizeof(ULONG));}

				   //write the PKID of the cycle recursion point (q->Value) at the end
				   //of the new cycle stack.
				   //increment the number of cycles
				   

					*(plNewCycleStack+lNUM_CYCLES) = q->Value;


				   //associate this vertex with a new permutation
				   //create a new member of the global Permutation Collection
				  if (0<lNUM_CYCLES)
				  {AddPermutation(*(m_plVertexVector+i),q->LinkFKID,lNUM_CYCLES+1,plNewCycleStack,lNUM_CYCLES,plCycleStack);}
				  else
				  {AddPermutation(*(m_plVertexVector+i),q->LinkFKID,lNUM_CYCLES+1,plNewCycleStack,1,NULL);}
				   
				   //call depth wise walk with the new values
				  DepthWiseWalk(i,plNewCycleStack,lNUM_CYCLES+1);
				  delete [] plNewCycleStack;
				  plNewCycleStack = 0;
             
		}
		else if(LINKED_BUT_NOT_RECURSION_POINT == lValue)
		{
				  //element is a succesor, but not a recursion point,
				  //so check if we're a member of a cycle that shows up 
				  //on the cycle stack
				  if(0 == lNUM_CYCLES)
				  {

					  AddPermutation (*(m_plVertexVector+i), q->LinkFKID,1, NULL,1,NULL);
  					  //WHAM THIS LINK-EVENT ONTO THE PERMUTATION STACK WITH A BINARY OF 0x0000

					  //we're off any cycle
						//create a uninitialized new CycleStack and call DepthWiseWalk again
					   ULONG * plNewCycleStack2 = NULL;
					   DepthWiseWalk(i,plNewCycleStack2,0);
						//new code
					   if (0 != plNewCycleStack2)
					   {delete [] plNewCycleStack2;
					   plNewCycleStack2 = 0;}

				  }
				  else
				  {

					  for (long j = lNUM_CYCLES-1 ; j > -1;j--)
					  {
						if (CheckVertexCycleMatrix(i,*(plCycleStack+j)))
						{break;}
					  }

					  if (-1 == j) //we're not on a permuatation anymore
					  {   //but we were on a permutation. Set prior_permutation appropriately
						  AddPermutation(*(m_plVertexVector+i),q->LinkFKID,1,NULL,lNUM_CYCLES,plCycleStack);
						//we're off any cycle
						//create a uninitialized new CycleStack and call DepthWiseWalk again
						   ULONG * plNewCycleStack2 = NULL;
						   DepthWiseWalk(i,plNewCycleStack2,0);
							//new code
						   if (0 != plNewCycleStack2)
						   {delete [] plNewCycleStack2;
						   plNewCycleStack2 = 0;}
						
						  

					  }
					  else if ((-1 < j) && (lNUM_CYCLES -1 > j)) //we're still on a permutation, but an inner one
					  {		//strip the appropriate
					  
						    ULONG * plStrippedCycleStack = new ULONG[j+1];
							CopyMemory(plStrippedCycleStack,plCycleStack,sizeof(ULONG)*(j+1));
							AddPermutation(*(m_plVertexVector+i),q->LinkFKID,(j+1),plStrippedCycleStack,lNUM_CYCLES,plCycleStack);
							DepthWiseWalk(i,plStrippedCycleStack,j+1);
							//new code
						   if (0 != plStrippedCycleStack)
						   {delete [] plStrippedCycleStack;
						   plStrippedCycleStack = NULL;}
							
								
					  
					  }
					  else if ((lNUM_CYCLES - 1) == j) //we're on the same permutation
					  {
					  
					  	//we're still on the cycle, so associate this vertex with 
						//a new permutation. Continue to use the original plCycleStack
						AddPermutation(*(m_plVertexVector+i),q->LinkFKID,lNUM_CYCLES,plCycleStack,lNUM_CYCLES,plCycleStack);
					    DepthWiseWalk(i,plCycleStack,lNUM_CYCLES);
					  
					  }
				  }//else
					  
			}//else if(LINKED_BUT_NOT_RECURSION_POINT == lValue)
		
	}//for (int i = 0; i < m_lVertexCount; i++)
	
}//depthwise walk

//NEW CODE 0.2.26.04
void cPH::AddPermutation(long lEventFKID, long lLinkFKID,long lCycleCount, ULONG * plCycles,long lPriorCycleCount, ULONG * plPriorCycles)
{
	//Initialize a new permutation array with an additional member
	LPPERMUTATION pPMNewPermutations = new PERMUTATION[m_lPermutationCount+1];
	//copy the existing permutation array into new array
	if (0 < m_lPermutationCount)
	{ 
		CopyMemory(pPMNewPermutations,m_pPMPermutations,m_lPermutationCount*sizeof(PERMUTATION));
		delete [] m_pPMPermutations;
	}
	m_pPMPermutations = pPMNewPermutations;						
	
	//get a pointer to the last element of the permutation array
	LPPERMUTATION p =  pPMNewPermutations+m_lPermutationCount;
	//increment total number of permutations
	m_lPermutationCount++;
										  
	p->EventFKID = lEventFKID;
	p->LinkFKID = lLinkFKID;
	p->lCycleCount = lCycleCount;
	p->plCycles = new ULONG[lCycleCount];
	if (NULL == plCycles)
	{			  
		ZeroMemory (p->plCycles,(lCycleCount)*sizeof(ULONG));
	}
	else
	{
		CopyMemory (p->plCycles,plCycles,(lCycleCount)*sizeof(ULONG));
	}
	p->lPriorCycleCount = lPriorCycleCount;
	p->plPriorCycles = new ULONG[lPriorCycleCount];
	if (NULL == plPriorCycles)
	{			  
		ZeroMemory (p->plPriorCycles,(lPriorCycleCount)*sizeof(ULONG));
	}
	else
	{
		CopyMemory (p->plPriorCycles,plPriorCycles,(lPriorCycleCount)*sizeof(ULONG));
	}


}
//End New code

int cPH::CheckVertexCycleMatrix(long lRowIndex, long lCyclePKID)
{
	long lColIndex = GetCycleIndexByPKID(lCyclePKID);
	
	if (!VertexBoundsCheck(lRowIndex) || !CycleBoundsCheck(lColIndex))
		{return 0;}

	return *(m_pbVertexCycleMatrix+((m_lCycleCount*lRowIndex)+lColIndex));
}


//medium old 3.23.04 15:35 EST
/*
void cPH::MEDIUM OLDDepthWiseWalk(long lColIndex, ULONG * plCycleStack,long lNUM_CYCLES)
{

	if (!VertexBoundsCheck(lColIndex)) {return ;}
	if (!(0<=lNUM_CYCLES))             {return ;}
	
	//walk down columns
	for (int i = 0; i < m_lVertexCount; i++)
	{
		LPELEMENT q = m_pELPredMatrix + lColIndex + i*m_lVertexCount;
		long lValue =  q->Value;
			if (lValue > 0)
			{		
		
					//element is a successor and a recursion point, so add the 
					//name of the cycle to the cycle stack and keep going

				 //create a new cycle stack  and copy the old data into it
				  //don't delete the old cycle stack, as it will get used 
				  //on next iteration of i.
				  //old cycle stack only gets deleted after we fall through the 
				 //for loop, above.
				  //also, don't increment lNUM_CYCLES, because 
				 //it will get used on next iteration of i.
				   ULONG * plNewCycleStack	= new ULONG[lNUM_CYCLES+1];
				   if (0 < lNUM_CYCLES )
				   {CopyMemory(plNewCycleStack,plCycleStack,lNUM_CYCLES*sizeof(ULONG));}

				   //write the PKID of the cycle recursion point (q->Value) at the end
				   //of the new cycle stack.
				   //increment the number of cycles
				   

					*(plNewCycleStack+lNUM_CYCLES) = q->Value;


				   //associate this vertex with a new permutation
				   //create a new member of the global Permutation Collection
				  if (0<lNUM_CYCLES)
				  {AddPermutation(*(m_plVertexVector+i),q->LinkFKID,lNUM_CYCLES+1,plNewCycleStack,lNUM_CYCLES,plCycleStack);}
				  else
				  {AddPermutation(*(m_plVertexVector+i),q->LinkFKID,lNUM_CYCLES+1,plNewCycleStack,1,NULL);}
				   
				   //call depth wise walk with the new values
				  DepthWiseWalk(i,plNewCycleStack,lNUM_CYCLES+1);
				  delete [] plNewCycleStack;
				  plNewCycleStack = 0;
             
			}
			else if(LINKED_BUT_NOT_RECURSION_POINT == lValue)
			{
				  //element is a succesor, but not a recursion point,
				  //so check if we're a member of a cycle that shows up 
				  //on the cycle stack
				  BOOL bAction = false;
				  for (long j = 0; j<lNUM_CYCLES;j++)
				  {		
					 
					  //modify me: this has to return how many permutations we've fallen off of
					  if (CheckVertexCycleMatrix(i,*(plCycleStack+j))) 		
					 {
						//we're still on the cycle, so associate this vertex with 
						//a new permutation
						AddPermutation(*(m_plVertexVector+i),q->LinkFKID,lNUM_CYCLES,plCycleStack,lNUM_CYCLES,plCycleStack);
					    DepthWiseWalk(i,plCycleStack,lNUM_CYCLES);
					    bAction = true;
					    break;
					 }
					 
				  }
				  if (!bAction) //we're not on any permutation now.....
				  { 

					 //NEW CODE
						if (0 < lNUM_CYCLES) //but we were on a permutation. Set prior_permutation appropriately
						{  AddPermutation(*(m_plVertexVector+i),q->LinkFKID,1,NULL,lNUM_CYCLES,plCycleStack);}
						//END NEW CODE
						else  //we weren't on a permutation before
						{AddPermutation (*(m_plVertexVector+i), q->LinkFKID,1, NULL,1,NULL);}
  					  //WHAM THIS LINK-EVENT ONTO THE PERMUTATION STACK WITH A BINARY OF 0x0000

					  //we're off any cycle
						//create a uninitialized new CycleStack and call DepthWiseWalk again
					   ULONG * plNewCycleStack2 =0;
					   DepthWiseWalk(i,plNewCycleStack2,0);
						//new code
					   if (0 != plNewCycleStack2)
					   {delete [] plNewCycleStack2;
						plNewCycleStack2 = 0;
					   }

				  }
			}
			

		}
	
}

*/

/*
03.23.04 15:00EST
void cPH::VERYOLDDepthWiseWalk(long lColIndex, ULONG * plCycleStack,long lNUM_CYCLES)
{

	if (!VertexBoundsCheck(lColIndex)) {return ;}
	if (!(0<=lNUM_CYCLES))             {return ;}
	
	//walk down columns
	for (int i = 0; i < m_lVertexCount; i++)
	{
		LPELEMENT q = m_pELPredMatrix + lColIndex + i*m_lVertexCount;
		long lValue =  q->Value;
			if (lValue > 0)
			{		
		
					//element is a successor and a recursion point, so add the 
					//name of the cycle to the cycle stack and keep going

				 //create a new cycle stack  and copy the old data into it
				  //don't delete the old cycle stack, as it will get used 
				  //on next iteration of i.
				  //old cycle stack only gets deleted after we fall through the 
				 //for loop, above.
				  //also, don't increment lNUM_CYCLES, because 
				 //it will get used on next iteration of i.
				   ULONG * plNewCycleStack	= new ULONG[lNUM_CYCLES+1];
				   if (0 < lNUM_CYCLES )
				   {CopyMemory(plNewCycleStack,plCycleStack,lNUM_CYCLES*sizeof(ULONG));}

				   //write the PKID of the cycle recursion point (q->Value) at the end
				   //of the new cycle stack.
				   //increment the number of cycles
				   

					*(plNewCycleStack+lNUM_CYCLES) = q->Value;


				   //associate this vertex with a new permutation
				   //create a new member of the global Permutation Collection
				  if (0<lNUM_CYCLES)
				  {AddPermutation(*(m_plVertexVector+i),q->LinkFKID,lNUM_CYCLES+1,plNewCycleStack,lNUM_CYCLES,plCycleStack);}
				  else
				  {AddPermutation(*(m_plVertexVector+i),q->LinkFKID,lNUM_CYCLES+1,plNewCycleStack,1,NULL);}
				   
				   //call depth wise walk with the new values
				  DepthWiseWalk(i,plNewCycleStack,lNUM_CYCLES+1);
				  delete [] plNewCycleStack;
				  plNewCycleStack = 0;
             
			}
			else if(LINKED_BUT_NOT_RECURSION_POINT == lValue)
			{
				  //element is a succesor, but not a recursion point,
				  //so check if we're a member of a cycle that shows up 
				  //on the cycle stack
				  BOOL bAction = false;
				  for (long j = 0; j<lNUM_CYCLES;j++)
				  {		
					 if (CheckVertexCycleMatrix(i,*(plCycleStack+j))) 		
					 {
						//we're still on the cycle, so associate this vertex with 
						//a new permutation
						AddPermutation(*(m_plVertexVector+i),q->LinkFKID,lNUM_CYCLES,plCycleStack,lNUM_CYCLES,plCycleStack);
						 
					    //NEW CODE
					    DepthWiseWalk(i,plCycleStack,lNUM_CYCLES);
					    //END NEW CODE
					    bAction = true;
					    break;
					 }
				  }
				  if (!bAction)  
				  { 
					  
					  //NEW CODE FOR ZERO PERMUTATIONS
					  //WHAM THIS LINK-EVENT ONTO THE PERMUTATION STACK WITH A BINARY OF 0x0000
					  //MEANING ITS NOT IN THE CONTEXT OF ANY PERMUTATION
					  
					  AddPermutation (*(m_plVertexVector+i), q->LinkFKID,1, NULL,1,NULL);

					  //we're off any cycle
						//create a uninitialized new CycleStack and call DepthWiseWalk again
					   ULONG * plNewCycleStack2 =0;
					   DepthWiseWalk(i,plNewCycleStack2,0);
						//new code
					   if (0 != plNewCycleStack2)
					   {delete [] plNewCycleStack2;
						plNewCycleStack2 = 0;
					   }

				  }
			}
			

		}
	
}



*/

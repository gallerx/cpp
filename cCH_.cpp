#ifndef AFX_cCH_H_INCLUDED
#define AFX_cCH_H_INCLUDED


//forward declarations
/*************************************************************************************
* CALLING CYCLE FOR CYCLE HANDLING
*   Assume n vertexs in DAG
*	PushVertex		              x n
*   InitializeMatrix              x 1
*	SetHigh		                  x n
*   QuickSchedule                 x 1
*   CycleMembers                  as needed
*   TerminateMatrix               x 1
*
**************************************************************************************/

//new code
#define LIVE_EDGE  0x03 //loword bit 0011
#define DEAD_EDGE  0x01 //Loword bit 0001
#define LSB        0xFE	// *p ^ LSB returns the least significant bit
#define LSB_MASK(a)   (a ^ LSB)
//we'll do a right bit shift when we move from checking physical connectivity to 'live'
//connectivity (anything that's live will still have a connection, but anything that's 
//dead will no longer be connected after the shift
//end new code
//#include <stdio.h> //for bitshiftoperator

class cCH
{
public:
	cCH();
	virtual ~cCH				();
	BOOL  PushVertex			(long lPKID);
	BOOL  InitializeMatrix		();

	//new code
	//	BOOL  SetHigh				(long lRowPKID, long lColPKID);
	BOOL  SetHigh				(long lRowPKID, long lColPKID, long lDead);
	void  BitShiftRight			();
	//end new code

	void  QuickSchedule			(void);
	int   CycleMembers			(long lPredPKID, long lSucPKID,long& lNumMembers,long* &plMembers);
private:
	void  ScheduleLayer			(long lCurrentLayer);
	long  RowTotal				(long lRowIndex);
	void  ZeroColumn			(long lColumnIndex);
	BOOL  Electrify				(long lPredIndex, long lSucIndex);
	BOOL  IsPredecessor			(long lPredPKID, long lSucPKID);
	BOOL  VertexBoundsCheck		(long lRowIndex);
	long  GetVertexIndexByPKID	(long lPKID);

	BYTE *m_pbColumnBus;
	BYTE *m_pbColumnSwitchVector;
	BYTE *m_pbPredMatrix;
	long *m_plVertexVector;
	long *m_plScheduleVector;
	long m_lVertexCount;
	long m_lLayerCount;
};

#endif//AFX_cCH_H_INCLUDED	
//MEMBER VARIABLES
/*************************************************************************************
* BYTE *g_pbColumnBus
*   implements a electrical bus. 
*   if we get voltage between a predecessor and a successor, we have a connection
*   voltage is  applied row-wise at successor
**************************************************************************************/
/*************************************************************************************
* BYTE *g_pbColumnSwitchVector
*   0 or 1
*   turns a column on or off. if 0, any nonzero value in column ignored
*   
**************************************************************************************/
/*************************************************************************************
* BYTE *g_pbPredMatrix
*   square predecessor matrix of size g_lVertexCount x g_lVertex Count
*
*   
**************************************************************************************/
/*************************************************************************************
* long *g_plVertexVector
*   single dimension of all vertexs in DAG, listed by vertex PKID
*
*   
**************************************************************************************/

/*************************************************************************************
* long *g_plScheduleVector 
*   layer ordering of all vertexs in DAG. Ordinal is same as g_plVertexVector
*
*   
**************************************************************************************/

/*************************************************************************************
* long g_lVertexCount 
*   number of vertexs in DAG
*
*   
**************************************************************************************/
/*************************************************************************************
* long g_lLayerCount
*   number of schedule layers in DAG
*
*   
**************************************************************************************/

//MEMBER FUNCTIONS


/*************************************************************************************
* PushVertex: pushes a vertex onto the global Vertex Vector
*
*
**************************************************************************************/
/*************************************************************************************
* InitializeMatrix: Initializes all the globals
*
*
**************************************************************************************/
/*************************************************************************************
* void  QuickSchedule()
*   does a layer-wise scheduling of DAG vertexs. Layer 1 to g_lLayerCount
*
**************************************************************************************/
/*************************************************************************************
* void APIENTRY ScheduleLayer()
*   recursive layer-wise scheduling of DAG vertexs.
*
**************************************************************************************/
/*************************************************************************************
* long RowTotal(long lRowIndex)
*   row-wise addition of Predecessor Matrix
*
**************************************************************************************/

/*************************************************************************************
* void ZeroColumn(long lColumnIndex)
*   zeroes out the global column switch vector for the affected column
*   any subsequent bitwise &'s (as in RowTotal above) evaluate to zero
**************************************************************************************/












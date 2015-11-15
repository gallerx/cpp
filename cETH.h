//cETH.h   Header File for Event_Time_Handler Algorithm

#ifndef AFX_cETH_H_INCLUDED
#define AFX_cETH_H_INLCUDED

#include <sql.h>
//Event Scheduling Constraint ID's:  
#define CI_ASAP			1  //as soon as possible
#define CI_SDK			2  //start date known
#define CI_CE			3  //completed event

//Broken Rules, see below for discussion
#define BROKEN_RULE_ONE	  1
#define BROKEN_RULE_TWO   2
#define BROKEN_RULE_THREE 3


#define FIRST_LAYER		1
#define BUF_LEN			1024 //1024 * 4 bytes = 4096 max byte length

typedef struct tagEVENTTIME     // EVENTTIME
{
   ULONG				lPKID;
   ULARGE_INTEGER		ulStart;  
   ULARGE_INTEGER		ulFinish; 
   ULONG				lDuration;
   long					ciConstraint; 
   long					bConflict;
   ULONG				*plBrokenRules;		
   ULONG				cbBrokenRules; 
   bool					bDirty;
}  EVENTTIME, FAR *LPEVENTTIME;

/*************************************************************************************
* CALLING CYCLE FOR EVENT TIME HANDLING
*   Assume n vertexs in DAG
*	PushVertex		              x n  
*   InitializeMatrix              x 1
*	SetHigh		                  x n
*   TimeHandler                   x 1
*   TerminateMatrix               x 1
*
**************************************************************************************/

class cETH
{
public:	
	cETH();
	virtual ~cETH								();
	BOOL	PushVertex					(ULONG lPKID, TIMESTAMP_STRUCT tsStart, 
										TIMESTAMP_STRUCT tsFinish,long lDuration,
										long ciConstraint,long bConflict,ULONG* plBrokenRules,
										ULONG cbBrokenRules);
	BOOL	SetHigh						(ULONG lRowPKID, ULONG lColPKID);
	BOOL	InitializeMatrix			(void);
	void	TimeHandler					(TIMESTAMP_STRUCT dLimitDate, long& lNumDirtyET,
										 LPEVENTTIME& pETDirty);
	TIMESTAMP_STRUCT ULargeIntegerToTimestamp	(ULARGE_INTEGER ulDate);
private:
	ULARGE_INTEGER	 LatestPredecessorFinish	(long lRowIndex);
	void			 ScheduleLayer				(long lCurrentLayer);
	long			 RowTotal					(long lRowIndex);
	void			 ZeroColumn					(long lColumnIndex);
	BOOL			 VertexBoundsCheck			(long lRowIndex);
	long			 GetVertexIndexByPKID		(ULONG lPKID);
	ULARGE_INTEGER	 TimestampToULargeInteger	(TIMESTAMP_STRUCT tsDate);
	ULARGE_INTEGER   AddDays					(ULARGE_INTEGER ulDate,long nDays);
	void			 QuickSchedule();
	BOOL			 AddBrokenRule				(ULONG *&plBrokenRules,ULONG lBrokenRule,
												 ULONG &lBrokenRuleCount);
	BYTE			*m_pbColumnBus;
	BYTE			*m_pbColumnSwitchVector;
	BYTE			*m_pbPredMatrix;
	EVENTTIME		*m_pETVector;
	long			*m_plScheduleVector;
	long			m_lVertexCount;
	long			m_lLayerCount;
};

#endif //AFX_cETH_H_INCLUDED




/*BROKEN RULE COMMENTS

BROKEN RULE ONE:

INSERT INTO SCHEDULING_CONFLICT_RULE (CONSTRAINT_ID1,CONSTRAINT_ID2,ARG_1,ARG_2,OPERATOR,PERSISTABLE,CODE_1,CODE_2)
VALUES 
(1,2,'scSTARTDATE','scSTARTDATE','scGT',0,'My actual start time is earlier than Prior Event estimated start time')
(1,3,'scSTARTDATE','scSTARTDATE','scGT',0,'My actual start time is earlier than Prior Event estimated start time')


BROKEN RULE TWO:
INSERT INTO SCHEDULING_CONFLICT_RULE (CONSTRAINT_ID1,CONSTRAINT_ID2,ARG_1,ARG_2,OPERATOR,PERSISTABLE,CODE_1,CODE_2)
VALUES 
(2,2,'scSTARTDATE','scSTARTDATE','scGT',0,'My actual start time is earlier than Prior Event actual start time or Deal Announce 
(2,3,'scSTARTDATE','scSTARTDATE','scGT',0,'My actual start time is earlier than Prior Event actual start time or Deal Announce Date')

BROKEN RULE THREE:
INSERT INTO SCHEDULING_CONFLICT_RULE (CONSTRAINT_ID1,CONSTRAINT_ID2,ARG_1,ARG_2,OPERATOR,PERSISTABLE,CODE_1,CODE_2)
VALUES 
(3,2,'scFINISHDATE','scSTARTDATE','scGT',0,'My actual start time is earlier than Prior Event actual finish time')
(3,3,'scFINISHDATE','scSTARTDATE','scGT',0,'My actual start time is earlier than Prior Event actual finish time')
*/

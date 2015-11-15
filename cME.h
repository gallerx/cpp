//class cME.h Mutually Exclusive data element

//#if !defined(AFX_ME_H_INCLUDED)
//#define      AFX_ME_H_INCLUDED
/********************************************************************************************/


#include <vector>
#include <cctype>


/********************************************************************************************/
using namespace std;
/********************************************************************************************/
class cME {
public:
					cME(ULONG,const LPULONG,ULONG,const LPULONG,ULONG,ULONG,ULONG,ULONG);
	virtual		   ~cME();
	ULONG			GetExclusionSpace		(void);
	ULONG			GetActualEdge			(void);
	ULONG			GetNumPerm				(void);
	ULONG			GetNumPriorPerm			(void);
	ULONG			GetDuration				(void);
	ULONG		    GetMode					(void);
	BOOL			GetPerm					(LPULONG,ULONG);
	BOOL			GetPriorPerm			(LPULONG,ULONG);
	void			SetActualEdge			(ULONG);

	//comparison functions for STL
	friend bool     operator<				(const cME &x, const cME &y);
	friend bool		SmallerActualEdge		(const cME &x, const cME &y);

	BOOL			VectorToBuffer			(LPULONG,vector<ULONG>::iterator&,ULONG,ULONG);
	BOOL			BufferToVector			(LPULONG plBuf,vector<ULONG> *pV,ULONG lLimit);

	//members
	vector<ULONG>   m_vPerm; 
	vector<ULONG>	m_vPriorPerm;
	ULONG			m_lExclusionSpace;
	ULONG			m_lActualEdge;
	ULONG			m_lDuration;
	ULONG			m_lModeID;
	
};
/********************************************************************************************/
//#endif //AFX_ME_H_INCLUDED      
/********************************************************************************************/

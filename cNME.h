//class cNME.h Non Mutually Exclusive data element, inherits from a mutually
//exclusive data element


#if !defined(AFX_NME_H_INCLUDED)
#define		 AFX_NME_H_INCLUDED
/********************************************************************************************/
#include <vector>
#include <cctype>
#include "cME.h"
/********************************************************************************************/
using namespace std;
/********************************************************************************************/
class cNME: public cME
{
public:
					cNME(const LPULONG,ULONG,ULONG,const LPULONG,ULONG,const LPULONG,
						       ULONG,ULONG,ULONG,ULONG);
	virtual		   ~cNME					(void);
	ULONG			GetNumDJS				(void);
	BOOL			GetDJS					(LPULONG,ULONG);
	
	//comparison operator for STL
	friend bool     SmallerActualEdgeAndDJS (const cNME &x, const cNME &y);
	friend bool     SmallerDJS				(const cNME &x, const cNME &y);
	//member
	vector<ULONG>	m_vDJS;

};
/********************************************************************************************/
#endif //AFX_cNME_H_INCLUDED        

/********************************************************************************************/

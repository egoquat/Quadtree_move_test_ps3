#pragma once


#include "CommonApp.h"

#ifdef _DEBUG_NAVIGATION

#include "CommonSingleton.h"

#include <PSSG.h>

#include "Tri.h"
#include "BoundaryVolume.h"

#include "NaviCell.h"
#include "NaviCells.h"

#include "Object.h"
#include "SpaceTree.h"

namespace NAVIPART
{
#define LIMIT_LAYERCNT		1
#define NULL_NAVILAYER		-1

	typedef		vector<CNaviCells>	VEC_NAVICELLS;

	class CNaviManager : public CSingleton<CNaviManager>
	{
	public:
		CNaviCells				m_NaviCells_basement;

		VEC_NAVICELLS			m_vecNaviCells;
		CSpaceTree				*m_pSpaceTree;
		CTriContainer			*m_ptriContainer;
		
		unsigned int			m_uiCntGoal;

		void BuildupNewMappingNavi(set<int> &slistCellsGoal, set<int> &slistCellsBlock);
	public:
		// @ Get / Set
		CNaviCells &getNaviCells()
		{
			return m_NaviCells_basement;
		}

		void setClearNavi()
		{
			m_NaviCells_basement.setClearAllcells();
		}

		// @ Process
		void BuildNewNavi(	set<int> &slistCellsGoal, set<int> &slistCellsBlock );

		void Initialize(	CTriContainer	*ptriContainer,
							CSpaceTree		*pSpaceTree );

		void Release();

		// @ Function
	public:
		int getLevelofCells() {	return m_NaviCells_basement.getLevelCells();};

	public:
		CNaviManager();
		~CNaviManager();
	};
}; // NAVIPART

#endif // _DEBUG_NAVIGATION

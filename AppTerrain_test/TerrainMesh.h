#pragma once

//#include <PSSG.h>
#include <PSSGTerrain/PSSGTerrain.h>
#include <Extra/PSSGExtra.h>

#include "commonApp.h"

#include "CommonSingleton.h"

#include "CommonMath.h"

#include "TriContainer.h"
#include "SpaceTree.h"


using namespace PSSG;
using namespace Vectormath::Aos;

#ifdef _DEBUG_TERRAINTILE
class CTerrainTile
{
private:
 	PNode					*m_pNoderootTerr;

	Vector3					m_v3BoundaryMin;
	Vector3					m_v3BoundaryMax;

public:
#ifdef _DEBUG_SPACETREE
	CSpaceTree				*m_pQuadTree;
	CSpaceTree				*getSpaceTree()
	{
		return m_pQuadTree;
	}

	// @ Space Tree
	CTriContainer			*m_pContainerTri;
	CTriContainer			*getTriContainer()
	{
		return m_pContainerTri;
	}
#endif // _DEBUG_SPACETREE

private:
	float interpolateValue(int i, int maxInt, float maxFloat);
	float interpolateValueDistribute(int i, int maxInt, float maxFloat);

public:
#ifdef _DEBUG_SPACETREE 
	bool getIntersectTri(	const BV::_Ray &_ray, 
							Vector4 *pPickedTriangle_out, 
							Vector3 &v3PosIntersect_out );

	bool getIntersectTri(	const BV::_Ray &_ray, 
							int &_iTriIntersect );
#endif // _DEBUG_SPACETREE

#if defined(_DEBUG_SPACETREE) && defined(_DEBUGFOR_RENDER)
	void renderspacetree_fordebug(	const Vector3	&v3colorLine,
									const Vectormath::Aos::Matrix4 *matViewPrjCamera );
#endif // defined(_DEBUG_SPACETREE) && defined(_DEBUGFOR_RENDER)

public:
	PShaderInstance *createTestTexturedShaderInstance(	PDatabase *database,
														unsigned int width = 128, 
														unsigned int height = 128, 
														unsigned int checkerWidth = 8, 
														unsigned int checkerHeight = 8 );

	bool createTileMesh_Test(	PRootNode *pnodeRoot,
								PDatabase *pDatabase,
								float fdistHorizontal,
								float fdistVertical,
								float fMeshScale,
								float fCntWave,
								float fIntensityWave,
								unsigned int uiSubdivisions,
								const Vector3 &v3Translation );


public:
	void Release();

public:							   
	CTerrainTile();				   
	~CTerrainTile();			   
};
#endif // _DEBUG_TERRAINTILE


#ifdef _DEBUG_TERRAINSHAPE_TEST
class CTerrainShapeForSpaceTree
{
private:
	//PNode					*m_pNoderootTerr;
	PVisibleRenderNode		*m_pNoderootTerr;
	PDatabaseID				m_terrainDBID;

public:
	CSpaceTree				*m_pQuadTree;
	CSpaceTree				*getSpaceTree()
	{
		return m_pQuadTree;
	}

	// @ Space Tree
	CTriContainer			*m_pContainerTri;
	CTriContainer			*getTriContainer()
	{
		return m_pContainerTri;
	}

private:
	bool constructSpaceTree( PDatabase	*pDatabase );

public:
#ifdef _DEBUG_SPACETREE
	void renderspacetree_fordebug(	const Vector3	&v3colorLine,
									const Vectormath::Aos::Matrix4 *matViewPrjCamera );
#endif // #ifdef _DEBUG_SPACETREE

	bool getIntersectTri(const BV::_Ray &_ray, 
		Vector4 *pPickedTriangle_out, 
		Vector3 &v3PosIntersect_out);

	PResult constructSurfaceRenderDataSource(	const PSSG::PRenderDataSource	*pRenderDataSrc_in,
		PDatabase						*pdatabase,
		PRenderInterface					&renderInterface,
		PSSG::PRenderDataSource			*&pRenderDataSrc_out	 );

public:
	// @ Main Process
	bool createTerrainShape_test(	PRootNode			*pnodeRoot,
									PDatabase			*pDatabase,
									PString				szNameFileTerrain,
									float				fScaleTerrain = 1.0f );

// 	void Render(PSSG::PRenderInterface *renderInterface, 
// 				int iwidthViewPort, 
// 				int iheightViewPort, 
// 				PSSG::PCameraNode *pnodeCamera );

	void Release();

public:
	CTerrainShapeForSpaceTree();
	~CTerrainShapeForSpaceTree();
};

#endif // _DEBUG_TERRAINSHAPE_TEST



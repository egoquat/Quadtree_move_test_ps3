#include "TerrainTest.h"

#ifdef _DEBUG_TERRAINTEST

//  説明：
// テクスチャキャッシュ要素の作成方法を定義します。
PTextureCacheElementBase::PTextureCacheInitInfo	PSampleTextureCacheElement::s_initInfo[3] = {
	{"tmap1", PTexture::PE_TEXTURE_FORMAT_DXT1,		PD_TERRAIN_MAX_TEXTURE_SIZE, PD_TERRAIN_MAX_TEXTURE_MIPS},		// 拡散マップ。
	{"tmap2", PTexture::PE_TEXTURE_FORMAT_VUI8X4,	PD_TERRAIN_MAX_TEXTURE_SIZE, PD_TERRAIN_MAX_TEXTURE_MIPS},		// 法線マップ。
	{"tmap3", PTexture::PE_TEXTURE_FORMAT_VU8X4,	PD_TERRAIN_MAX_TEXTURE_SIZE, PD_TERRAIN_MAX_TEXTURE_MIPS}		// アンビエント、スペキュラ、シャドウ
};



float CTerrainTest::interpolateValue(int i, int maxInt, float maxFloat)
{
	return (i * maxFloat) / maxInt - (maxFloat*0.5f);
}

float CTerrainTest::interpolateValueDistribute(int i, int maxInt, float maxFloat)
{
	float value = (i * maxFloat * 2) / maxInt - maxFloat;

	return value * 2;
}


PShaderGroup *CTerrainTest::createTerrainShader(PDatabase &database, 
												const char *name, 
												PResult *result)
{
	PResult internalResult;
	if(!result)
		result = &internalResult;

	// Cgプログラムをロード
	//PShaderProgram *testVertexProgram = PSSG_CG_CREATE_VERTEX_PROGRAM(vtxterr.cg);
	PShaderProgram *testVertexProgram = 
			PSSG_CG_CREATE_VERTEX_PROGRAM(SampleData/Test01/Shader/vtxterr.cg);
	if(*result != PE_RESULT_NO_ERROR)
		return NULL;

// 	PShaderProgram *testFragmentProgram = 
// 			PSSG_CG_CREATE_FRAGMENT_PROGRAM(fragterr.cg);
PShaderProgram *testFragmentProgram = 
 			PSSG_CG_CREATE_FRAGMENT_PROGRAM(SampleData/Test01/Shader/fragterr.cg);
	if(*result != PE_RESULT_NO_ERROR)
		return NULL;

	// Cgプログラムを利用して新しいシェーダグループの作成を試みる
	PShaderGroup *group = PShaderGroup::createSinglePassShaderGroup(testVertexProgram, testFragmentProgram, database, name, result);
	return group;
}

bool CTerrainTest::addSpotlighttoWorld(	const Vector3	&v3ColorLight,
									   const Vector3	&v3PosLight,
									   const float		fThetaLight )
{
	//float fThetaLight = 1.4f;
	PLightNode	*pLightNew = PSSG::Extra::simpleAddLight(
		*m_prootNodetotal,	v3PosLight.getX(), 
		v3PosLight.getY(), 
		v3PosLight.getZ(), 
		v3ColorLight.getX(), 
		v3ColorLight.getY(), 
		v3ColorLight.getZ() );
	m_vecLightNodes.push_back( pLightNew );

	Vector3 lightAt = Vector3(sinf(fThetaLight), cosf(fThetaLight), 0.0f);
	lightAt = normalize(lightAt);

	Vector3 lightUp = Vector3(0.0f, 0.0f, 1.0f);
	Vector3 lightRight = normalize(cross(lightUp, lightAt));
	lightUp = normalize(cross(lightAt, lightRight));
	pLightNew->m_matrix.setCol0(Vector4(lightRight, 0.0f));
	pLightNew->m_matrix.setCol1(Vector4(lightUp, 0.0f));
	pLightNew->m_matrix.setCol2(Vector4(lightAt, 0.0f));
	pLightNew->m_matrix.setCol3(Vector4(Vector3(lightAt * 1000.0f), 1.0f));


	return true;
}



PShaderInstance *CTerrainTest::createTestTexturedShaderInstance(PDatabase *database,
																unsigned int width, 
																unsigned int height, 
																unsigned int checkerWidth, 
																unsigned int checkerHeight )
{
	PResult result;
	// 텍스쳐및 조명을 포함한 텍스쳐 쉐이더 인스턴스 생성
	PShaderInstance *texturedShaderInstance = 
		PShaderGroup::createDefaultShaderInstance<PShaderInstance>(
		PShaderGroup::PE_DEFAULT_SHADER_GROUP_PRELIT_TEXTURED, *database, PDatabaseUniqueNameHelper(*database, "sampleMaterial"), &result);
	if(result != PE_RESULT_NO_ERROR)
		return PSSG_SET_LAST_ERROR((PShaderInstance*)NULL, ("Unable to create the shader instance"));

	// 텍스처를 추가하여 만든 질감 인스턴스 설정
	// Extra 도우미 함수를 사용하여 텍스처 만들기
	PTexture *checkerTexture = PSSG::Extra::createCheckerTexture(*database, width, height, checkerWidth, checkerHeight);
	if(!checkerTexture)
		return PSSG_SET_LAST_ERROR((PShaderInstance*)NULL, ("Unable to create the checkerTexture texture"));

	// 쉐이더에 텍스쳐 매개 변수를 설정
	// 쉐이더 인스턴스 0번에 텍스쳐를 연결
	texturedShaderInstance->setTextureParameter(0U, checkerTexture);

	return texturedShaderInstance;
}

PModifierNetworkInstance *CTerrainTest::instanceSegment(
									PSSG::PDatabase &database, 
									PSSG::PRenderDataSource *source0, 
									PSSG::PRenderDataSource *source1, 
									PSSG::PModifierNetwork *network, 
									PSSG::PShaderInstance *shaderInstance )
{
	PResult result = PE_RESULT_NO_ERROR;
	PModifierNetworkInstance *instance = database.createObject<PModifierNetworkInstance>(PDatabaseUniqueNameHelper(database, "instance"), &result);

	// インスタンスを設定
	if(result == PE_RESULT_NO_ERROR)
		result = instance->addSource(*source0);					// レンダーデータソースを追加
	if(result == PE_RESULT_NO_ERROR && source1)
		result = instance->addSource(*source1);					// レンダーデータソースを追加
	if(result == PE_RESULT_NO_ERROR)
		result = instance->setModifierNetwork(network);			//  指定されたモディファイアネットワークを使います
	if(result == PE_RESULT_NO_ERROR)
		result = instance->setShaderInstance(shaderInstance);	// シェーダインスタンスを利用してレンダリング

	if(result != PE_RESULT_NO_ERROR && instance)
	{
		delete instance;
		return NULL;
	}
	return instance;
}

bool CTerrainTest::constructSpaceTree( PDatabase *pDatabase )
{
	PSegmentSet *segmentSet;
	PDatabaseListableIterator<PSegmentSet>		it1(*pDatabase);

	m_pQuadTree = m_pQuadTree->getthis();

	while (it1)
	{
		segmentSet = &(*it1);

		for(unsigned int i = 0 ; i < segmentSet->getSegmentCount() ; i++)
		{
			PRenderDataSource *source = segmentSet->getSegment(i);

			m_pContainerTri->constructAllTriangles( source );	
		}
		++it1;
	}

	m_pQuadTree->constructTree( m_pContainerTri );

	return true;
}


#if defined(_DEBUG_SPACETREE) && defined(_DEBUGFOR_RENDER)
void CTerrainTest::renderspacetree_fordebug(const Vector3	&v3colorLine,
											const Vectormath::Aos::Matrix4 *matViewPrjCamera )
{
	m_pQuadTree->renderAllNodes_debug( v3colorLine, matViewPrjCamera );
}
#endif // defined(_DEBUG_SPACETREE) && defined(_DEBUGFOR_RENDER)


bool CTerrainTest::getIntersectTri(const BV::_Ray &_ray, 
								   Vector4 *pPickedTriangle_out, 
								   Vector3 &v3PosIntersect_out )
{
	CTri tripicked;
	bool bPicked = m_pQuadTree->rayIntersectGetPos( _ray, tripicked, v3PosIntersect_out );
	if(true==bPicked)
	{
		for(int iP=0; iP<3; ++iP)
		{
			vectorCopy3to4( pPickedTriangle_out[iP] , tripicked.v3PT[iP], 1.0f );
		}
	}

	return bPicked;
}


PResult CTerrainTest::constructSurfaceRenderDataSource(	
										   const PSSG::PRenderDataSource	*pRenderDataSrc_in,
										   PDatabase						*pdatabase,
										   PRenderInterface					&renderInterface,
										   PSSG::PRenderDataSource			*&pRenderDataSrc_out	)
{

	PResult		result;

	PRenderStream *pRenderStream	= pRenderDataSrc_in->getStream(0);
	PDataBlock *vertexDatablock		= pRenderStream->getDataBlock();
	unsigned int vertexCount		= vertexDatablock->getElementCount();

	// プロセスからの出力用の領域を割り当てる
	pRenderDataSrc_out = pdatabase->createObject<PRenderDataSource>(PDatabaseUniqueNameHelper(*pdatabase, "SurfaceSource"), &result);
	if(result != PE_RESULT_NO_ERROR || !pRenderDataSrc_out)
		return result;

	pRenderDataSrc_out->setIsTransient(true);

	// インデックスソースを追加
	PRenderIndexSource *indexSource = pdatabase->createObject<PRenderIndexSource>(PDatabaseUniqueNameHelper(*pdatabase, "SurfaceIndexSource"), &result);
	indexSource->create(*PDataType::getType(PE_TYPE_UINT), vertexCount, PRenderInterface::PE_PRIMITIVE_TRIANGLES);
	indexSource->setAllocateSystem(true);
	pRenderDataSrc_out->setIndexSource(indexSource);


	// ストリームを設定
	result = pRenderDataSrc_out->setStreamCountWithUniqueBlocks(2);

	// 頂点を保持するデータブロックを追加
	PRenderStream *vertexStream = pRenderDataSrc_out->getStream(0);
	PDataBlock *vertexBlock = vertexStream->getDataBlock();
	vertexBlock->setSingleStream(vertexCount, *PDataType::getType(PE_TYPE_FLOAT3), *PSSG_GET_RENDERTYPE(Vertex));
	vertexBlock->setAllocateSystem(true);


	// 法線を保持するデータブロックを追加
	PRenderStream *normalStream = pRenderDataSrc_out->getStream(1);
	PDataBlock *normalBlock = normalStream->getDataBlock();
	normalBlock->setSingleStream(vertexCount, *PDataType::getType(PE_TYPE_FLOAT3), *PSSG_GET_RENDERTYPE(Normal));
	normalBlock->setAllocateSystem(true);


	pRenderDataSrc_out->setStream( 0, pRenderDataSrc_in->getStream(0) );
	pRenderDataSrc_out->setStream( 1, pRenderDataSrc_in->getStream(1) );


	pRenderDataSrc_out->bind( renderInterface );

	return result;
}



bool CTerrainTest::createTerrain_test(	PRootNode			*pnodeRoot,
										PDatabase			*pDatabase,
										PCameraNode			**arCamera_,
										int					icntCamera,
										PSSG::PRenderInterface *prenderInterface_,
										PString				szPathData,
										PString				szNameFileTerrain,
										PString				szNameIDShader,
										PString				szNameDiffuseFile,
										PString				szNameNormalFile,
										PString				szNameLitFile )
{
	PResult result;

	m_pNoderootTerr = pnodeRoot->createChildNode<PNode>(PDatabaseUniqueNameHelper(*pDatabase, "terrroot"), &result);
	if(result != PE_RESULT_NO_ERROR)
		return PSSG_SET_LAST_ERROR(false, ("Unable to create the screen graph root node"));	


	bool	bDefineFileDiffuse		= false,
			bDefineFileNormal		= false,
			bDefineFileLit			= false;

	if( szNameDiffuseFile.len() > 0 ) bDefineFileDiffuse = true;
	if( szNameNormalFile.len() > 0 ) bDefineFileNormal = true;
	if( szNameLitFile.len() > 0 ) bDefineFileLit = true;

#ifdef __CELLOS_LV2__

	//  有用な全ファイルをキャッシュにいれることを試みます。
	// PUtilityHDDCache::clearCache();
	const PChar *terrainDBaseName[] = {szNameFileTerrain, 
					szNameDiffuseFile, szNameNormalFile, szNameLitFile , NULL};
	PUtilityHDDCache::cacheFileSet(terrainDBaseName);

#endif // __CELLOS_LV2__


	PShaderGroup *createTerrainShaderResult = createTerrainShader(*pDatabase, szNameIDShader);

	PDatabaseID terrainDatabaseID;
	if(PLinkResolver::getDatabase(terrainDatabaseID, szPathData + szNameFileTerrain) != PE_RESULT_NO_ERROR)
		return PSSG_SET_LAST_ERROR(false, ("Unable to find terrain database"));

	if(PSSG::Extra::resolveAllLinks() != PE_RESULT_NO_ERROR)
		return PSSG_SET_LAST_ERROR(false, ("Unable to resolve terrain database"));

	PDatabaseReadLock readLock(terrainDatabaseID);
	PDatabase *terrainDatabase = readLock.getDatabase();
	if(!terrainDatabase)
		return PSSG_SET_LAST_ERROR(false, ("Unable to lock terrain database"));

	PDatabaseListableIterator<PTerrainDescriptor> tdit(*terrainDatabase);
	if(!tdit)
		return PSSG_SET_LAST_ERROR(false, ("Unable to find terrain descriptor in database"));
	PTerrainDescriptor &td = *tdit;

	unsigned int terrainWidth = td.getWidth();
	unsigned int terrainHeight = td.getHeight();
	unsigned int terrainInstanceCount = td.getCount();

#ifdef __CELLOS_LV2__
	for (int i=0; i<terrainInstanceCount; i++)
	{
		//  ブロックを処理します
		const PTerrainBlockDescriptor *tbd = td.getBlock(i);

		// 名前を生成します。
		PChar norm[128];
		PChar lit[128];
		PChar diff[128];
		PChar data[128];

		const char *name = tbd->m_name.c_str();
		PSSG_SNPRINTF(norm, sizeof(norm), "%s_norm.craw", name);
		PSSG_SNPRINTF(lit, sizeof(lit), "%s_lit.craw", name);
		PSSG_SNPRINTF(diff, sizeof(diff), "%s.craw", name);
		PSSG_SNPRINTF(data, sizeof(data), "HiTerrainPackets%dx%d.data", tbd->m_x, tbd->m_y);

		const PChar *names[] = {norm, lit, diff, data, NULL};
		PUtilityHDDCache::cacheFileSet(names);
	}
#endif //! __CELLOS_LV2__


	m_terrain.setDimensions(terrainWidth, terrainHeight, terrainInstanceCount);

	//  必要なモディファイアネットワークを構築します
	PModifierNetwork *loterrainNetwork = PSSG::Extra::simpleCreateModifierNetwork(*pDatabase, loTerrainModifier);
	PModifierNetwork *hiTerrainModifier0Network = PSSG::Extra::simpleCreateModifierNetwork(*pDatabase, hiTerrainModifier0);
	PModifierNetwork *hiTerrainModifier1Network = PSSG::Extra::simpleCreateModifierNetwork(*pDatabase, hiTerrainModifier1);
	PModifierNetwork *hiTerrainModifier2Network = PSSG::Extra::simpleCreateModifierNetwork(*pDatabase, hiTerrainModifier2);
	PModifierNetwork *hiTerrainModifier3Network = PSSG::Extra::simpleCreateModifierNetwork(*pDatabase, hiTerrainModifier3);
	if(!(loterrainNetwork && hiTerrainModifier0Network && hiTerrainModifier1Network && hiTerrainModifier2Network && hiTerrainModifier3Network))
		return PSSG_SET_LAST_ERROR(false, ("Unable to create the terrain modifier networks"));

	///////////////////////////////////////////////////////////////////////////////////////////////////
	// 低解像度バージョンの地形をインスタンス化するために、低解像度でパケット化されたバージョンの地形と、
	// 地形全体を表現するテクスチャをインスタンス化します。
	// 各地形ブロックは、m_infoを使って、モディファイアネットワークに渡された低解像度バージョンをレンダリング
	// するかどうかを指定するレンダリングデータを参照します。
	///////////////////////////////////////////////////////////////////////////////////////////////////
	PRenderDataSource *m_lores = (PRenderDataSource *)terrainDatabase->link("LoTerrainPackets");
	PLoTerrainBlockInfo *info = (PLoTerrainBlockInfo *)m_lores->getStream(3)->getData();	// ??
	PTexture *superDiffuse = 0, *superNormal = 0, *superLit = 0;

	// For TESTDEBUG
	m_pLores = m_lores;
	// For TESTDEBUG

	if( true==bDefineFileDiffuse )
	{
		superDiffuse = PTextureHandler::createTextureFromFile(szPathData + szNameDiffuseFile, *pDatabase);
	}

	if( true==bDefineFileNormal )
	{
		superNormal = PTextureHandler::createTextureFromFile(szPathData + szNameNormalFile, *pDatabase);
	}

	if( true==bDefineFileLit )
	{
		superLit = PTextureHandler::createTextureFromFile(szPathData + szNameLitFile, *pDatabase);
	}


	// Terrain에 해당하는 렌더 노드 추가.
	// 쉐이더 파라미터에 텍스쳐를 연결.
	PRenderNode *meshRenderNode = m_pNoderootTerr->createChildNode<PRenderNode>(PDatabaseUniqueNameHelper(*pDatabase, "meshRenderNode"), &result);
	if(result != PE_RESULT_NO_ERROR)
		return PSSG_SET_LAST_ERROR(false, ("Unable to create the render node"));
	PShaderInstance *loTerrainInstance = createTerrainShaderResult->createShaderInstance<PShaderInstance>(*pDatabase, PDatabaseUniqueNameHelper(*pDatabase, szNameIDShader));

	if( true==bDefineFileDiffuse )
	{
		loTerrainInstance->setTextureParameter("tmap1", superDiffuse);
	}
	
	if( true==bDefineFileNormal )
	{
		loTerrainInstance->setTextureParameter("tmap2", superNormal);
	}
	
	if( true==bDefineFileLit )
	{
		loTerrainInstance->setTextureParameter("tmap3", superLit);
	}

	PSSG::Extra::instanceSegment(*meshRenderNode, 
								*m_lores, 
								*loTerrainInstance, 
								*loterrainNetwork);

	///////////////////////////////////////////////////////////////////////////////////////////////////
	// サンプルデータベースの中にビルド済みのデータを使って、地形をシーン中にインスタンス化すます。
	// このデータには、解像度の低・中・高のバージョンがあって、詳細度の高い方の4つの表現に
	// 利用することができます。各モディファイアネットワークインスタンスは、同じシェーダインスタンスを共有しており、
	// このシェーダインスタンスは、テクスチャがテクスチャキャッシュにストリーミングされた時点で更新されます。
	///////////////////////////////////////////////////////////////////////////////////////////////////
	meshRenderNode->m_boundsMin = Vector3(FLT_MAX);
	meshRenderNode->m_boundsMax = Vector3(-FLT_MAX);

	
	// 自己記述的な地形インスタンスをすべて処理します。
	for(unsigned int i=0; i<terrainInstanceCount; i++, info++)
	{
		const PTerrainBlockDescriptor *tbd = td.getBlock(i);

		//  ブロックを処理します
		PTerrainBlock p;
		if(tbd)
		{
			p.m_x = (unsigned short)tbd->m_x;
			p.m_y = (unsigned short)tbd->m_y;
			p.m_id = tbd->m_name;
		}
		else
		{
			p.m_x = 0;
			p.m_y = 0;
			p.m_id = "A_1";
		}
		p.m_info = info;

		meshRenderNode->m_boundsMin = minPerElem(meshRenderNode->m_boundsMin, Vector3(info->m_boundsMin[0], info->m_boundsMin[1], info->m_boundsMin[2]));
		meshRenderNode->m_boundsMax = maxPerElem(meshRenderNode->m_boundsMax, Vector3(info->m_boundsMax[0], info->m_boundsMax[1], info->m_boundsMax[2]));

		PString blockPacketsName = p.getBlockPacketsName();
		PSegmentSet *segmentSet = PSSGCast<PSegmentSet>(terrainDatabase->link(blockPacketsName));
		if(!segmentSet)
			return PSSG_SET_LAST_ERROR(false, ("Unable to find the segment set for %dx%d", p.m_x, p.m_y));
		PShaderInstance *shaderInstance = createTerrainShaderResult->createShaderInstance<PShaderInstance>(*pDatabase, PDatabaseUniqueNameHelper(*pDatabase, szNameIDShader));

		//  シェーダインスタンスを使って、メッシュをノードの中にインスタンス化します
		p.m_terrainNode = m_pNoderootTerr->createChildNode<PTerrainNode>(PDatabaseUniqueNameHelper(*pDatabase, "terrainRenderNode"), &result);

		PRenderDataSource *lores = segmentSet->getSegment(0);
		PRenderDataSource *midres = segmentSet->getSegment(1);
		PRenderDataSource *hires = segmentSet->getSegment(2);
		PRenderInstance *instance0 = instanceSegment(*pDatabase, lores, midres, hiTerrainModifier0Network, shaderInstance);
		PRenderInstance *instance1 = instanceSegment(*pDatabase, lores, midres, hiTerrainModifier1Network, shaderInstance);
		PRenderInstance *instance2 = instanceSegment(*pDatabase, midres, NULL, hiTerrainModifier2Network, shaderInstance);
		PRenderInstance *instance3 = instanceSegment(*pDatabase, midres, hires, hiTerrainModifier3Network, shaderInstance);

		p.m_terrainNode->setLODCount(PD_TERRAIN_BLOCK_MAX_LOD - 1);
		p.m_terrainNode->addRenderInstance(*instance3);
		p.m_terrainNode->addRenderInstanceToLOD(0, *instance2);
		p.m_terrainNode->addRenderInstanceToLOD(1, *instance1);
		p.m_terrainNode->addRenderInstanceToLOD(2, *instance0);
		p.m_hiresBlock = hires->getStream(0)->getDataBlock();

		//  メッシュパケット情報ストリームから境界を集めます
		const PTerrainBlockInfo *infos = 
			(PTerrainBlockInfo *)midres->getStream(2)->getData();
		unsigned int infoCount = midres->getPacketCount();
		Vector3 hiBoundsMin(FLT_MAX), hiBoundsMax(-FLT_MAX);
		do
		{
			hiBoundsMin = minPerElem(hiBoundsMin, infos->m_boundsMin.getXYZ());
			hiBoundsMax = maxPerElem(hiBoundsMax, infos->m_boundsMax.getXYZ());
			infos++;
		}
		while(--infoCount);


		//m_pContainerTri->constructAllTriangles( lores );


		p.m_terrainNode->m_boundsMin = hiBoundsMin;
		p.m_terrainNode->m_boundsMax = hiBoundsMax;
		m_terrain.addBlock(p);
	}

	// Construct All Triangle and Quad Tree based on IndexBuffer, VertexBuffer 
	m_pContainerTri->constructAllTriangles( m_pLores );
	m_pQuadTree->constructTree( m_pContainerTri );

	for( int i=0; i<icntCamera; ++i )
	{
		arCamera_[i]->setLookat( meshRenderNode->m_boundsMax, (meshRenderNode->m_boundsMin + meshRenderNode->m_boundsMax) * 0.5f, Vector3::yAxis() );
	}

#if defined(__CELLOS_LV2__) && !defined(PSGL)

	//  マップされたテクスチャシステムを使ってテクスチャを更新します
	m_textureCacheRenderInterface = m_renderInterface;

#endif // defined(__CELLOS_LV2__) && !defined(PSGL)

	m_textureCache.initialize(*pDatabase, PSampleTextureCacheElement::s_initInfo, m_textureCacheRenderInterface);
	m_geometryCache.initialize(td.getGeometryCacheSize());

	m_shaderGroupBinding.bind(*createTerrainShaderResult, PSampleTextureCacheElement::s_initInfo);
	m_textureCache.addBinding(m_shaderGroupBinding);

 	// 데이터 로드 된 상태로 표시.
 	pDatabase->setLoaded();

	//  マルチスレッド化されたテクスチャストリーミングを開始します
	m_streamingThread.setUseCopyTexture((prenderInterface_->getFeatureSet() & PRenderInterface::PE_RENDERINTERFACEFEATURE_COPYTEXTURE) != 0);
	m_streamingThread.run();

	m_statusTexture.initialize(*pDatabase, td, 8);

	return true;
}

bool CTerrainTest::RenderPre(	PCameraNode	*pNodeCamera )
{
	pNodeCamera->generateGlobalTransform();
	m_terrain.updateLODs(*pNodeCamera, m_textureCache.getFreedButBusyQueue(), m_textureCacheRenderInterface);

	return true;
}

void CTerrainTest::Release()
{
	m_pQuadTree->Release();
	m_pQuadTree->destroyInstance();

	m_pContainerTri->Release();
	m_pContainerTri->destroyInstance();



	m_streamingThread.quit();
	m_geometryCache.terminate();
	m_terrain.terminate();
	m_textureCache.terminate();

	// TESTDEBUG
	vector<PLightNode*>::iterator		iter_;
	for( iter_=m_vecLightNodes.begin(); iter_!=m_vecLightNodes.end(); ++iter_ )
	{
		PLightNode		*pnodeLightCurr = (*iter_);
		pnodeLightCurr->terminate();
		delete pnodeLightCurr;
	}
	m_vecLightNodes.clear();
}


CTerrainTest::CTerrainTest() 
			: m_streamingThread(m_textureCache, m_geometryCache, m_terrain)
{
	m_wireframe = false;
	m_prootNodetotal = NULL;
	m_textureCacheRenderInterface = NULL;
	m_showTerrainStatus = false;

	PUtilityHDDCache::setTitleId("PSSG12347");

	// 必要なユーティリティの登録
	PUtility::registerUtility(PUtilityDDS::s_utilityDDS);
	PUtility::registerUtility(PUtilityHDDCache::s_utilityHDDCache);
	PUtility::registerUtility(PUtilityTerrain::s_utilityTerrain);


	m_pContainerTri	= m_pContainerTri->getthis();
	m_pQuadTree = m_pQuadTree->getthis();
}

CTerrainTest::~CTerrainTest()
{

}

#endif // _DEBUG_TERRAINTEST
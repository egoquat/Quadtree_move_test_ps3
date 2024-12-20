/* SCE CONFIDENTIAL
PhyreEngine(TM) Package 2.6.0
* Copyright (C) 2009 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/
 
#ifndef __PSSG_INTERSECTIONTRAVERSALS_H__
#define __PSSG_INTERSECTIONTRAVERSALS_H__

#include <PSSG.h>
#include <Extra/PSSGExtra.h>

#include <PSSGMesh/PSSGMesh.h>
#include <PSSGIntersectMesh/PSSGIntersectMesh.h>
#include <PSSGUtilTraversal/PSSGUtilTraversal.h>

// 解説：
// PTraversalAddIntersectionDataクラスは、シーンをトラバースして、交差のテストに利用するために、シーングラフ中のレンダーノードにPMeshインスタンスを追加する
class PTraversalAddIntersectionData : public PTraversal
{
public:
	// 解説:
	// PTraversalから派生されたクラスでオーバーライドされる仮想コールバック関数。任意の兄弟または子をトラバースする前に呼び出される。
	// 引数:
	// node		- トラバース中の現在のノード
	// 返り値:
	// 以降のトラバースを制御するための可能なトラバース継続タイプの列挙からの値
	virtual PTraversalContinueType preTraversalCallback(PNode &node, const PTraversalContext *)
	{
		PVisibleRenderNode *renderNode = PSSGCast<PVisibleRenderNode>(node);
		if(renderNode)
		{
///////////////////////////////////////////////////////////////////////////////////////////////////
// 新しいPMeshインスタンスを作成するためにレンダーノードのセグメントセットを検索する場合、頂点を表す
// 正しいレンダーデータタイプを選択する必要があります。スキンノードではSkinnableVertexレンダーデータ
// タイプが必要となり、他のほとんどのレンダーノードでは、Vertexレンダーデータタイプが必要となります。
// レンダーノードによってインスタンス化されているセグメントセットを検索するには、PSSG Extra
// ライブラリのfindSegmentSetForRenderNode()関数を利用します。失敗した場合、最初のレンダーストリーム
// インスタンスに進み（存在すれば）、特定のレンダーデータソースをインスタンス化するかチェックします。
///////////////////////////////////////////////////////////////////////////////////////////////////
			PRenderDataType *vertexRenderDataType = NULL;
			if(renderNode->isType(PSSG_TYPE(PSkinNode)))
				vertexRenderDataType = PSSG_GET_RENDERTYPE(SkinnableVertex);
			else
				vertexRenderDataType = PSSG_GET_RENDERTYPE(Vertex);
			
			PDatabase &database = node.getDBase();

			// どのセグメントセットまたはレンダーデータソースがレンダーノードによってインスタンス化されているのかを検索
			PSegmentSet *segmentSet = PSSG::Extra::findSegmentSetForRenderNode(*renderNode, vertexRenderDataType);
			PRenderDataSource *renderDataSource = NULL;
			if(!segmentSet)
			{
				PRenderStreamInstance *rsi = PSSGCast<PRenderStreamInstance>(renderNode->getFirstRenderInstance());
				renderDataSource = (rsi && rsi->getNextInNode() == NULL) ? rsi->getSource(0) : NULL;
			}

			if(segmentSet || renderDataSource)
			{
				PObject *sourceObject = segmentSet ? (PObject *)segmentSet : (PObject *)renderDataSource;

///////////////////////////////////////////////////////////////////////////////////////////////////
// セグメントセットまたはレンダーデータソースが見つかったら、PMeshクラスのインスタンスを作成する
// 必要があります。同じセグメントセットの複数のインスタンスを作成しないようにするために、
// メッシュはセグメントセットにユーザデータとして追加されるため、後から取り出すことができます。
// メッシュが見つからなかった場合、PMesh::createFromSegmentSet()またはcreateFromRenderDataSource()関数
// を利用して新しいメッシュを作成します。メッシュが正しく作成できたら、PABTriangleTreeクラスの新しい
// インスタンスを作成して、ユーザデータとしてメッシュに追加します。そうすれば、後から見つけ出せます。
// 最後に、以降の交差のテスト用にメッシュをレンダーノードと関連付けます。
///////////////////////////////////////////////////////////////////////////////////////////////////
				PMesh *mesh = (PMesh *)sourceObject->getUserData(PSSG_TYPE(PMesh));
				if(!mesh)
				{
					PResult result = PE_RESULT_NO_ERROR;
					if(segmentSet)
						mesh = PMesh::createFromSegmentSet(*segmentSet, database, PDatabaseUniqueNameHelper(database, "mesh"), vertexRenderDataType, &result, PMesh::PE_CREATE_MESH_DO_NOT_CALCULATE_ADJACENCY);
					else
						mesh = PMesh::createFromRenderDataSource(*renderDataSource, database, PDatabaseUniqueNameHelper(database, "mesh"), vertexRenderDataType, &result, PMesh::PE_CREATE_MESH_DO_NOT_CALCULATE_ADJACENCY);
					if(result == PE_RESULT_NO_ERROR)
					{
						sourceObject->addUserData(*mesh);
						PABTriangleTree *tree = database.createObject<PABTriangleTree>(PDatabaseUniqueNameHelper(database, "abTriangleTree"));
						if(tree)
						{
							tree->create(*mesh);
							mesh->addUserData(*tree);
						} 
						renderNode->addUserData(*mesh);
					}
				}
			}
		}
		return PE_TRAVERSAL_CONTINUE;
	}
};

// 解説：
// PTriangleIntersectLineTraversaクラスは、ワールド空間中の線と交差するシーンのノード中の三角形を見つけるPTraversal::preTraversalCallbackメソッドを実装する
class PTriangleIntersectLineTraversal : public PTraversal
{
protected:
	PNode *m_nearest;						// 線の開始への最も近いノード
	float m_minDistance;					// 線の開始と最も近いノードと距離

	Vectormath::Aos::Point3 m_lineStart;	// 線の開始を表すワールド空間ポイント
	Vectormath::Aos::Vector3 m_lineDir;		// 線の方向を表すワールド空間ベクトル
	Vectormath::Aos::Point3 m_lineEnd;		// 線の終了を表すワールド空間ポイント

	unsigned int m_triangleID;				// 交差した三角形のID

public:

	// 解説:
	// PTriangleIntersectLineTraversalクラスのコンストラクタ
	PTriangleIntersectLineTraversal()
	{
		m_nearest = NULL;
		m_minDistance = FLT_MAX;
		m_triangleID = ~0U;
	}

	// 解説:
	// 最も近い三角形IDのアクセッサ
	// 返り値:
	// 最も近い三角形IDへのポインタ
	unsigned int getTriangleID() const { return m_triangleID; }

	// 解説:
	// 最も近いノードのアクセッサ
	// 返り値:
	// 最も近いノードへのポインタ
	PNode *getNearest() const { return m_nearest; }

	// 解説:
	// 最も近いノードへの距離のアクセッサ
	// 返り値:
	// 最も近いノードへの距離
	float getDistance() const { return m_minDistance; }

	// 解説:
	// トラバース用に線を設定
	// 引数:
	// lineStart	- 線の開始を表すワールド空間ポイント
	// lineDir		- 線の方向を表すワールド空間ベクトル
	// lineEnd		- 線の終了を表すワールド空間ポイント
	void setLine(const Vectormath::Aos::Point3 & lineStart, const Vectormath::Aos::Vector3 & lineDir, const Vectormath::Aos::Point3 & lineEnd)
	{
		m_lineStart = lineStart;
		m_lineDir = lineDir;
		m_lineEnd = lineEnd;
	}

	// 解説:
	// PTriangleIntersectLineTraversalクラス用のPTraversal::preTraversalCallback()仮想メソッドを実装
	// 引数:
	// node - トラバース中の現在目に見えるノード
	// 返り値:
	// トラバースを続けなければならないことを示す値
	PTraversalContinueType preTraversalCallback(PNode &node, const PTraversalContext *)
	{
		PVisibleRenderNode *renderNode = PSSGCast<PVisibleRenderNode>(node);
		if(renderNode)
		{
///////////////////////////////////////////////////////////////////////////////////////////////////
// レンダーノードを交差するには、PTraversalIntersectLine::intersectsNode()メソッドを利用する前に、
// その変換が正しいことを確認しなければなりません。
///////////////////////////////////////////////////////////////////////////////////////////////////
			renderNode->generateGlobalTransform();
			renderNode->updateInverseGlobalMatrix();

			// ラインノード交差を試す
			float distance;
			if(PTraversalIntersectLine::intersectsNode(node, m_lineStart, m_lineDir, &distance))
			{
///////////////////////////////////////////////////////////////////////////////////////////////////
// 線がノードと交差している場合、交差のメッシュをテストします。関連するメッシュを取得するには、
// 検索するユーザデータタイプのPSSGオブジェクトを取るgetUserData()メソッドを利用し、
// PABTriangleTreeクラスの関連したインスタンスを見つけ出します。
///////////////////////////////////////////////////////////////////////////////////////////////////
				PMesh *mesh = (PMesh *)node.getUserData(PSSG_TYPE(PMesh));
				PABTriangleTree *tree = mesh ? (PABTriangleTree *)mesh->getUserData(PSSG_TYPE(PABTriangleTree)) : NULL;
				if(!tree)
					return PE_TRAVERSAL_CONTINUE;

///////////////////////////////////////////////////////////////////////////////////////////////////
// 線と三角形ツリーを交差させる前に、線がツリーに対してローカルな空間に位置していることを確認する
// 必要があります。線を変換した後、線を三角形ツリーに対して交差させるためにrayTest()メソッドを利用します。
// 交差が見つかった場合には、分離する部分を計算して、最も近い交差についての詳しい情報をメンバー変数に
// 保存します。
///////////////////////////////////////////////////////////////////////////////////////////////////
				Vectormath::Aos::Point3 localStart = renderNode->m_inverseGlobalMatrix * m_lineStart;
				Vectormath::Aos::Vector3 localDir = renderNode->m_inverseGlobalMatrix * m_lineDir;
				Vectormath::Aos::Point3 localEnd = renderNode->m_inverseGlobalMatrix * m_lineEnd;

				if(!tree->rayTest(m_triangleID, distance, localStart, localDir, localEnd))
				{
					// 交差のローカル空間ポイントを計算し、ワールド空間へ移動
					Vectormath::Aos::Point3 local = localStart + distance * localDir;
					Vectormath::Aos::Point3 world = node.m_globalMatrix * local;

					Vectormath::Aos::Vector3 separation = m_lineStart - world;
					float separationDistance = Vectormath::Aos::length(separation);

//					PSSG_PRINTF("Intersected triangle %d in %s at %e (current min %e)\n", m_triangleID, node.getName() ? node.getName() : "UNNAMED", separationDistance, m_minDistance);

					if(separationDistance < m_minDistance)
					{
						m_minDistance = separationDistance;
						m_nearest = &node;
					}
				}
			}
		}
		return PE_TRAVERSAL_CONTINUE;
	}
};

#endif // __PSSG_INTERSECTIONTRAVERSALS_H__

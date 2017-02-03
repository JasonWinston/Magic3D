#pragma once
#include "MyGUI.h"

namespace MagicApp
{
    class MeshShopAppUI
    {
    public:
        MeshShopAppUI();
        ~MeshShopAppUI();

        void Setup();
        void Shutdown();

        void StartProgressbar(int range);
        void SetProgressbar(int value);
        void StopProgressbar(void);
        bool IsProgressbarVisible(void);

        void SetMeshInfo(int vertexCount, int triangleCount);
        void ResetFillHole(void);

    private:
        void SwitchDisplayMode(MyGUI::Widget* pSender);
        void ImportMesh(MyGUI::Widget* pSender);
        void ConsolidateMesh(MyGUI::Widget* pSender);
        void IsManifold(MyGUI::Widget* pSender);
        void ConsolidateTopology(MyGUI::Widget* pSender);
        void ConsolidateGeometry(MyGUI::Widget* pSender);
        void ReverseDirection(MyGUI::Widget* pSender);
        void RemoveMeshIsolatePart(MyGUI::Widget* pSender);
        void CDTOptimization(MyGUI::Widget* pSender);
        void CVTOptimization(MyGUI::Widget* pSender);

        void FilterMesh(MyGUI::Widget* pSender);
        void RemoveMeshNoise(MyGUI::Widget* pSender);
        void SmoothMesh(MyGUI::Widget* pSender);
        void EnhanceMeshDetail(MyGUI::Widget* pSender);
        
        void ChangeVertexCount(MyGUI::Widget* pSender);
        void SubdivideMesh(MyGUI::Widget* pSender);
        void RefineMesh(MyGUI::Widget* pSender);
        void DoRefineMesh(MyGUI::Widget* pSender);
        void SimplifyMesh(MyGUI::Widget* pSender);
        void DoSimplifyMesh(MyGUI::Widget* pSender);
        void Remesh(MyGUI::Widget* pSender);
        void DoUniformRemesh(MyGUI::Widget* pSender);

        void FillHole(MyGUI::Widget* pSender);
        void DoFillHoleTriangulation(MyGUI::Widget* pSender);
        void DoFillHoleFlat(MyGUI::Widget* pSender);
        void DoFillHoleTangent(MyGUI::Widget* pSender);
        void DoFillHoleSmooth(MyGUI::Widget* pSender);
        void DoBridgeEdges(MyGUI::Widget* pSender);
        
        void OffsetMesh(MyGUI::Widget* pSender);
        void DoUniformOffset(MyGUI::Widget* pSender);
        
        void SelectPoint(MyGUI::Widget* pSender);
        void SelectByRectangle(MyGUI::Widget* pSender);
        void EraseByRectangle(MyGUI::Widget* pSender);
        void DeleteSelections(MyGUI::Widget* pSender);
        void SimplifySelections(MyGUI::Widget* pSender);
        void IgnoreBack(MyGUI::Widget* pSender);
        void MoveModel(MyGUI::Widget* pSender);
        
        void CutMesh(MyGUI::Widget* pSender);
        void CutMeshByXYPlane(MyGUI::Widget* pSender);
        void CutMeshByYZPlane(MyGUI::Widget* pSender);
        void CutMeshByZXPlane(MyGUI::Widget* pSender);
        void CutMeshByRandomPlane(MyGUI::Widget* pSender);
        
        void SampleMesh(MyGUI::Widget* pSender);
        void DoUniformSampling(MyGUI::Widget* pSender);
        void BackToHomepage(MyGUI::Widget* pSender);

    private:
        MyGUI::VectorWidgetPtr mRoot;
        bool mIsProgressbarVisible;
        MyGUI::TextBox* mTextInfo;
        bool mIgnoreBack;
    };
}

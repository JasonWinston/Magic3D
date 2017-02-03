#pragma once
#include "MyGUI.h"

namespace MagicApp
{
    class PointShopAppUI
    {
    public:
        PointShopAppUI();
        ~PointShopAppUI();

        void Setup();
        void Shutdown();

        void StartProgressbar(int range);
        void SetProgressbar(int value);
        void StopProgressbar(void);
        bool IsProgressbarVisible(void);

        void SetPointCloudInfo(int pointCount);

    private:
        void ImportPointCloud(MyGUI::Widget* pSender);
        
        void SamplePointCloud(MyGUI::Widget* pSender);
        void DoUniformSamplePointCloud(MyGUI::Widget* pSender);
        void DoGeometrySamplePointCloud(MyGUI::Widget* pSender);

        void SimplifyPointCloud(MyGUI::Widget* pSender);
        void DoSimplifyPointCloud(MyGUI::Widget* pSender);

        void PointCloudNormal(MyGUI::Widget* pSender);
        void CalculatePointCloudNormalFront(MyGUI::Widget* pSender);
        void CalculatePointCloudNormal(MyGUI::Widget* pSender);
        void FlipPointCloudNormal(MyGUI::Widget* pSender);
        void ReversePatchNormal(MyGUI::Widget* pSender);
        void SmoothPointCloudNormal(MyGUI::Widget* pSender);
        void UpdatePointCloudNormal(MyGUI::Widget* pSender);
        
        void ConsolidatePointCloud(MyGUI::Widget* pSender);
        void RemovePointCloudOutlier(MyGUI::Widget* pSender);
        void RemoveIsolatePart(MyGUI::Widget* pSender);
        void SmoothPointCloudGeoemtry(MyGUI::Widget* pSender);

        void SelectPoint(MyGUI::Widget* pSender);
        void SelectByRectangle(MyGUI::Widget* pSender);
        void EraseByRectangle(MyGUI::Widget* pSender);
        void DeleteSelections(MyGUI::Widget* pSender);
        void IgnoreBack(MyGUI::Widget* pSender);
        void MoveModel(MyGUI::Widget* pSender);

        void ReconstructMesh(MyGUI::Widget* pSender);
        void DoReconstructMeshOpen(MyGUI::Widget* pSender);
        void DoReconstructMeshClose(MyGUI::Widget* pSender);

        void PointCloudColor(MyGUI::Widget* pSender);
        void FusePointCloudColor(MyGUI::Widget* pSender);

        void TextureImage(MyGUI::Widget* pSender);
        void FuseTextureImage(MyGUI::Widget* pSender);
        void LoadImageColorIds(MyGUI::Widget* pSender);
        void SaveImageColorIds(MyGUI::Widget* pSender);

        void BackToHomepage(MyGUI::Widget* pSender);

    private:
        MyGUI::VectorWidgetPtr mRoot;
        bool mIsProgressbarVisible;
        MyGUI::TextBox* mTextInfo;
        bool mIgnoreBack;
    };
}

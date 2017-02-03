#pragma once
#include "MyGUI.h"

namespace MagicApp
{
    class MeasureAppUI
    {
    public:
        MeasureAppUI();
        ~MeasureAppUI();

        void Setup();
        void Shutdown();

        void StartProgressbar(int range);
        void SetProgressbar(int value);
        void StopProgressbar(void);
        bool IsProgressbarVisible(void);

        void SetModelInfo(int vertexCount, int triangleCount);
        void SetModelArea(double area);
        void SetModelVolume(double volume);
        void SetGeodesicsInfo(double distance);
        void SetDistanceInfo(int refFaceCount, bool bCalculted, double minDist, double maxDist);
        void SetThicknessInfo(bool bShow, double medianThickness = 0.0);

    private:
        void SwitchDisplayMode(MyGUI::Widget* pSender);

        void ImportModel(MyGUI::Widget* pSender);
        void ImportRefModel(MyGUI::Widget* pSender);

        void Geodesics(MyGUI::Widget* pSender);
        void DeleteMeshMark(MyGUI::Widget* pSender);
        void ComputeApproximateGeodesics(MyGUI::Widget* pSender);
        void FastComputeExactGeodesics(MyGUI::Widget* pSender);
        void ComputeExactGeodesics(MyGUI::Widget* pSender);
        void ComputeCurvatureGeodesics(MyGUI::Widget* pSender);
        void SmoothGeodesicsOnVertex(MyGUI::Widget* pSender);

        void PointsToMeshDistance(MyGUI::Widget* pSender);
        void ComputePointsToMeshDistance(MyGUI::Widget* pSender);

        void MeasureModel(MyGUI::Widget* pSender);
        void MeasureArea(MyGUI::Widget* pSender);
        void MeasureVolume(MyGUI::Widget* pSender);
        void MeasureMeanCurvature(MyGUI::Widget* pSender);
        void MeasureGaussianCurvature(MyGUI::Widget* pSender);
        void MeasurePrincipalCurvature(MyGUI::Widget* pSender);
        void MeasureThickness(MyGUI::Widget* pSender);

        void BackToHomepage(MyGUI::Widget* pSender);

        void UpdateTextInfo(void);

    private:
        MyGUI::VectorWidgetPtr mRoot;
        bool mIsProgressbarVisible;
        MyGUI::TextBox* mTextInfo;
        // Application Information
        int mVertexCount;
        int mTriangleCount;
        double mArea;
        double mVolume;
        double mGeodesicsDistance;
        int mRefMeshFaceCount;
        bool mIsShowDistance;
        double mMinDistance;
        double mMaxDistance;
        bool mIsShowThickness;
        double mMedianThickness;
    };
}

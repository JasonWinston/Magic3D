#pragma once
#include "AppBase.h"
#include <vector>
#include "GPP.h"

namespace GPP
{
    class DumpBase;
}

namespace MagicCore
{
    class ViewTool;
    class PickTool;
}

namespace MagicApp
{
    class MeasureAppUI;
    class MeasureApp : public AppBase
    {
        enum CommandType
        {
            NONE = 0,
            GEODESICS_APPROXIMATE,
            GEODESICS_FAST_EXACT,
            GEODESICS_EXACT,
            GEODESICS_CURVATURE,
            DISTANCE_POINTS_TO_MESH,
            PRINCIPAL_CURVATURE,
            THICKNESS,
            SECTION_CURVE,
            FACE_POINT_CURVE,
            SPLIT_MESH,
            DETECT_PRIMITIVE
        };

        enum RightMouseType
        {
            SELECT_VERTEX = 0,
            SELECT_FACEPOINT,
            SELECT_PRIMITIVE
        };

    public:
        MeasureApp();
        ~MeasureApp();

        virtual bool Enter(void);
        virtual bool Update(double timeElapsed);
        virtual bool Exit(void);
        virtual bool MouseMoved(const OIS::MouseEvent &arg);
        virtual bool MousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id);
        virtual bool MouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id);
        virtual bool KeyPressed(const OIS::KeyEvent &arg);
        virtual void WindowFocusChanged(Ogre::RenderWindow* rw);

        void DoCommand(bool isSubThread);

        bool ImportModel(void);
        bool ImportRefModel(void);
        void SwitchSelectionMode(void);

        void DeleteMeshMark(void);
        void ComputeApproximateGeodesics(bool isSubThread = true);
        void FastComputeExactGeodesics(double accuracy, bool isSubThread = true);
        void ComputeExactGeodesics(bool isSubThread = true);
        void ComputeCurvatureGeodesics(double curvatureWeight, bool isSubThread = true);
        void SmoothGeodesicsOnVertex(void);
        void SmoothGeodesicsCrossVertex(void);

        void ComputeSectionCurve(bool isSubThread = true);
        void ComputeFacePointCurve(bool isSubThread = true);
        void SplitMesh(bool isSubThread = true);

        void ComputeOffsetCurve(double offsetSize);
        
        void ComputePointsToMeshDistance(bool isSubThread = true);
        void ShowReferenceMesh(bool isShow);

        void MeasureArea(void);
        void MeasureVolume(void);
        void MeasureMeanCurvature(void);
        void MeasureGaussianCurvature(void);
        void MeasurePrincipalCurvature(bool isSubThread = true);
        void MeasureThickness(bool isSubThread = true);

#if DEBUGDUMPFILE
        void SetDumpInfo(GPP::DumpBase* dumpInfo);
        void RunDumpInfo(void);
#endif
        bool IsCommandInProgress(void);

        void SwitchDisplayMode(void);
        bool IsGeodesicClose();
        void SwitchGeodesicClose();

        void SetSelectPrimitiveMode();
        void SetDetectOptions(bool optionPlane, bool optionCone, bool optionSphere, bool optionCylinder);
        void DetectPrimitive(bool isSubThread = true);

    private:
        void SetupScene(void);
        void ShutdownScene(void);
        void ClearData(void);
        void ClearSelectionData(void);
        bool IsCommandAvaliable(void);

        void AddOneCurve();
        void RemoveOneCurve();

        void InitViewTool(void);
        void UpdateModelRendering(void);
        void UpdateMarkRendering(void);
        void UpdateRefModelRendering(void);

        void SelectPrimitive(int faceId);

    private:
        MeasureAppUI* mpUI;
        MagicCore::ViewTool* mpViewTool;
        MagicCore::PickTool* mpPickTool;
        int mDisplayMode;
        RightMouseType mRightMouseType;
#if DEBUGDUMPFILE
        GPP::DumpBase* mpDumpInfo;
#endif
        std::vector<GPP::Int> mMarkIds;
        std::vector<GPP::Int> mGeodesicsOnVertices;
        std::vector<GPP::Vector3> mMarkPoints;
        std::vector<GPP::PointOnFace> mMarkFacePoints;
        std::vector<GPP::PointOnFace> mGeodesicsOnPofs;

        std::vector<std::vector<GPP::PointOnFace> > mCurvesOnMesh;
        CommandType mCommandType;
        bool mIsCommandInProgress;
        bool mUpdateModelRendering;
        bool mUpdateMarkRendering;
        double mGeodesicAccuracy;
        bool mIsFlatRenderingMode;
        GPP::TriMesh* mpRefTriMesh;
        bool mUpdateRefModelRendering;
        std::vector<GPP::Real> mMinCurvature;
        std::vector<GPP::Real> mMaxCurvature;
        std::vector<GPP::Vector3> mMinCurvatureDirs;
        std::vector<GPP::Vector3> mMaxCurvatureDirs;
        std::vector<bool> mCurvatureFlags;
        int mDisplayPrincipalCurvature;
        GPP::Real mCurvatureWeight;
        bool mIsGeodesicsClose;
        bool mDetectOptions[4];
    };
}

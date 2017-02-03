#pragma once
#include "AppBase.h"
#include "MagicPointCloud.h"
#include "Gpp.h"
#if DEBUGDUMPFILE
#include "DumpBase.h"
#endif

namespace MagicCore
{
    class ViewTool;
    class PickTool;
}

namespace MagicApp
{
    class PointShopAppUI;
    class PointShopApp : public AppBase
    {
        enum CommandType
        {
            NONE = 0,
            EXPORT,
            NORMALCALCULATION,
            NORMALSMOOTH,
            NORMALUPDATE,
            OUTLIER,
            ISOLATE,
            GEOMETRYSMOOTH,
            RECONSTRUCTION,
            FUSECOLOR,
            FUSETEXTURE
        };

        enum RightMouseType
        {
            MOVE = 0,
            SELECT_ADD,
            SELECT_DELETE,
            REVERSE_NORMAL,
        };

    public:
        PointShopApp();
        ~PointShopApp();

        virtual bool Enter(void);
        virtual bool Update(double timeElapsed);
        virtual bool Exit(void);
        virtual bool MouseMoved(const OIS::MouseEvent &arg);
        virtual bool MousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id);
        virtual bool MouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id);
        virtual bool KeyPressed(const OIS::KeyEvent &arg);
        virtual void WindowFocusChanged(Ogre::RenderWindow* rw);

        void DoCommand(bool isSubThread);

        bool ImportPointCloud(void);
        void ExportPointCloud(bool isSubThread = true);
        
        void SmoothPointCloudNormal(int neighborCount, bool isSubThread = true);
        void UpdatePointCloudNormal(int neighborCount, bool isSubThread = true);
        
        void UniformSamplePointCloud(int targetPointCount);
        void GeometrySamplePointCloud(int targetPointCount);

        void SimplifyPointCloud(int resolution);

        void CalculatePointCloudNormal(bool isDepthImage, int neighborCount, bool isSubThread = true);
        void FlipPointCloudNormal(void);
        void ReversePatchNormal(int neighborCount);
        void ReconstructMesh(bool needFillHole, int quality, bool isSubThread = true);

        void RemovePointCloudOutlier(bool isSubThread = true);
        void RemoveIsolatePart(double isolateValue, bool isSubThread = true);
        void SmoothPointCloudGeoemtry(int smoothCount, bool isSubThread = true);

        void SelectByRectangle(void);
        void EraseByRectangle(void);
        void DeleteSelections(void);
        void IgnoreBack(bool ignore);
        void MoveModel(void);

        void FusePointCloudColor(int neighborCount, double sharpDiff_H, double sharpDiff_S, double sharpDiff_V, bool isSubThread = true);
        void FuseTextureImage(bool isSubThread = true);
        void LoadImageColorInfo(void);
        void SaveImageColorInfo(void);

        int GetPointCount(void);

#if DEBUGDUMPFILE
        void SetDumpInfo(GPP::DumpBase* dumpInfo);
        void RunDumpInfo(void);
#endif
        bool IsCommandInProgress(void);

    private:
        void InitViewTool(void);
        void UpdatePickTool(void);
        void UpdatePointCloudRendering(void);
        bool IsCommandAvaliable(void);
        void SelectControlPointByRectangle(int startCoordX, int startCoordY, int endCoordX, int endCoordY);
        void UpdateRectangleRendering(int startCoordX, int startCoordY, int endCoordX, int endCoordY);
        void ClearRectangleRendering(void);
        void SetupMagicPointCloud(MagicPointCloud& magicPointCloud);
        
        void PickPointCloudColorFromImages(void);
        void ConstructImageColorIdForMesh(GPP::TriMesh* triMesh, const GPP::IPointCloud* pointCloud);

    private:
        void SetupScene(void);
        void ShutdownScene(void);
        void ClearData(void);
        void ResetSelection(void);

    private:
        PointShopAppUI* mpUI;
        MagicCore::ViewTool* mpViewTool;
        MagicCore::PickTool* mpPickTool;
#if DEBUGDUMPFILE
        GPP::DumpBase* mpDumpInfo;
#endif
        CommandType mCommandType;
        bool mUpdatePointCloudRendering;
        bool mIsCommandInProgress;
        bool mIsDepthImage;
        int mReconstructionQuality;
        std::vector<bool> mPointSelectFlag;
        RightMouseType mRightMouseType;
        GPP::Vector2 mMousePressdCoord;
        bool mIgnoreBack;
        bool mNeedFillHole;
        int mResolution;
        bool mEnterMeshShop;
        int mSmoothCount;
        int mNeighborCount;
        int mColorNeighborCount;
        double mIsolateValue;
        GPP::Vector3 mSharpDiff;
    };
}

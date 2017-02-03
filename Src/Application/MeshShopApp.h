#pragma once
#include "AppBase.h"
#include "../Common/RenderSystem.h"
#include <vector>
#include "Gpp.h"
#if DEBUGDUMPFILE
#include "DumpBase.h"
#endif

namespace MagicCore
{
    class ViewTool;
}

namespace MagicApp
{
    class MeshShopAppUI;
    class MagicMesh;
    class MeshShopApp : public AppBase
    {
        enum CommandType
        {
            NONE = 0,
            EXPORTMESH,
            CONSOLIDATETOPOLOGY,
            CONSOLIDATEGEOMETRY,
            REMOVEISOLATEPART,
            REMOVEMESHNOISE,
            SMOOTHMESH,
            ENHANCEDETAIL,
            LOOPSUBDIVIDE,
            REFINE,
            SIMPLIFY,
            SIMPLIFYSELECTVERTEX,
            FILLHOLE,
            UNIFORMREMESH,
            CDTOPTIMIZATION,
            CVTOPTIMIZATION,
            RUNSCRIPT
        };

        enum RightMouseType
        {
            MOVE = 0,
            SELECT_ADD,
            SELECT_DELETE,
            SELECT_BRIDGE
        };

    public:
        enum SplitType
        {
            ST_PLANE_XY,
            ST_PLANE_YZ,
            ST_PLANE_ZX,
            ST_PLANE_RANDOM
        };

        MeshShopApp();
        ~MeshShopApp();

        virtual bool Enter(void);
        virtual bool Update(double timeElapsed);
        virtual bool Exit(void);
        virtual bool MouseMoved(const OIS::MouseEvent &arg);
        virtual bool MousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id);
        virtual bool MouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id);
        virtual bool KeyPressed(const OIS::KeyEvent &arg);
        virtual void WindowFocusChanged(Ogre::RenderWindow* rw);

        void DoCommand(bool isSubThread);

        void SwitchDisplayMode(void);
        bool ImportMesh(void);
        void IsManifold(void);
        void ConsolidateTopology(bool isSubThread = true);
        void ReverseDirection(void);
        void RemoveMeshIsolatePart(bool isSubThread = true);
        void ConsolidateGeometry(bool isSubThread = true);
        void CDTOptimization(double sharpAngle, bool isSubThread = true);
        void CVTOptimization(double sharpAngle, bool isSubThread = true);
        void RemoveMeshNoise(double positionWeight, bool isSubThread = true);
        void SmoothMesh(double positionWeight, bool isSubThread = true);
        void EnhanceMeshDetail(double intensity, bool isSubThread = true);
        void LoopSubdivide(bool isSubThread = true);
        void RefineMesh(int targetVertexCount, bool isSubThread = true);
        void SimplifyMesh(int targetVertexCount, bool isSubThread = true);
        void SimplifySelectedVertices(bool isSubThread = true);
        void UniformRemesh(int targetVertexCount, double sharpAngle, bool isSubThread = true);
        void UniformSampleMesh(int targetPointCount);
        void EnterReliefApp(void);
        void EnterTextureApp(void);
        void EnterMeasureApp(void);
        void FindHole(bool isShowHole);
        void FillHole(int type, bool isSubThread = true);
        void BridgeEdges(bool isSubThread = true);
        void UniformOffsetMesh(double offsetValue);
        void SelectByRectangle(void);
        void EraseByRectangle(void);
        void DeleteSelections(void);
        void IgnoreBack(bool ignore);
        void MoveModel(void);
        void SplitMeshByPlane(SplitType st, double offsetValue);
        void RunScript(bool isSubThread = true);

        int GetMeshVertexCount(void);
        bool IsCommandInProgress(void);
#if DEBUGDUMPFILE
        void SetDumpInfo(GPP::DumpBase* dumpInfo);
        void RunDumpInfo(void);
#endif

    private:
        void SetupScene(void);
        void ShutdownScene(void);
        void ClearData(void);
        bool IsCommandAvaliable(void);
        void ResetSelection(void);
        void SelectControlPointByRectangle(int startCoordX, int startCoordY, int endCoordX, int endCoordY);
        void UpdateRectangleRendering(int startCoordX, int startCoordY, int endCoordX, int endCoordY);
        void ClearRectangleRendering(void);
        void DoBridgeEdges();
        void ResetBridgeTags();

        void InitViewTool(void);
        void UpdateMeshRendering(void);
        void SetToShowHoleLoopVrtIds(const std::vector<std::vector<GPP::Int> >& toShowHoleLoopIds);
        void SetBoundarySeedIds(const std::vector<GPP::Int>& bounarySeedIds);
        void UpdateHoleRendering(void);
        void UpdateBridgeRendering(void);

        void SaveImageColorInfo(void);
        void LoadImageColorInfo(void);
        void PickMeshColorFromImages(void);

        void ConstructMagicMeshInfo(MagicMesh* magicMesh);
        void UpdateAddedVertexInfo(std::map<int, int>& insertVertexIdMap);

    private:
        MeshShopAppUI* mpUI;
        MagicCore::ViewTool* mpViewTool;
        int mDisplayMode;
#if DEBUGDUMPFILE
        GPP::DumpBase* mpDumpInfo;
#endif
        std::vector<std::vector<GPP::Int> > mShowHoleLoopIds;
        std::vector<GPP::Int>               mBoundarySeedIds;
        GPP::Int mTargetVertexCount;
        int mFillHoleType;
        CommandType mCommandType;
        bool mUpdateMeshRendering;
        bool mUpdateHoleRendering;
        bool mUpdateBridgeRendering;
        bool mIsCommandInProgress;
        std::vector<bool> mVertexSelectFlag;
        RightMouseType mRightMouseType;
        GPP::Vector2 mMousePressdCoord;
        bool mIgnoreBack;
        double mFilterPositionWeight;
        std::vector<GPP::Int> mBridgeEdgeVertices;
        std::vector<bool> mVertexBridgeFlag;
        bool mIsFlatRenderingMode;
        double mSharpAngle;
        double mEnhanceIntensity;
    };
}

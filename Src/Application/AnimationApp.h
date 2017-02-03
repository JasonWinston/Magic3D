#pragma once
#include "AppBase.h"
#include "Gpp.h"

namespace MagicCore
{
    class ViewTool;
    class PickTool;
}

namespace MagicApp
{
    class AnimationAppUI;
    class AnimationApp : public AppBase
    {
        enum CommandType
        {
            NONE = 0
        };

        enum RightMouseType
        {
            DEFORM = 0,
            SELECT,
            MOVE
        };

    public:

        enum DeformType
        {
            DT_NONE = 0,
            DT_CONTROL_POINT,
            DT_VERTEX
        };

        AnimationApp();
        ~AnimationApp();

        virtual bool Enter(void);
        virtual bool Update(double timeElapsed);
        virtual bool Exit(void);
        virtual bool MouseMoved(const OIS::MouseEvent &arg);
        virtual bool MousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id);
        virtual bool MouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id);
        virtual bool KeyPressed(const OIS::KeyEvent &arg);

        bool ImportModel(void);
        void SwitchDeformType(DeformType dt);

        void InitControlPoint(int controlPointCount);
        void SelectFreeControlPoint(void);
        void ClearFreeControlPoint(void);
        void MoveControlPoint(void);
        void InitControlDeformation(void);
        void DoControlDeformation(void);
        //void RealTimeDeform(void);

    private:
        void InitViewTool(void);
        
        void PickControlPoint(int mouseCoordX, int mouseCoordY);
        void DragControlPoint(int mouseCoordX, int mouseCoordY, bool mouseReleased);
        void UpdateDeformation(int mouseCoordX, int mouseCoordY, bool isAccurate);
        void SelectControlPointByRectangle(int startCoordX, int startCoordY, int endCoordX, int endCoordY);
        void UpdateRectangleRendering(int startCoordX, int startCoordY, int endCoordX, int endCoordY);
        void ClearRectangleRendering(void);

        void UpdateModelRendering(void);
        void UpdateControlRendering(void);

    private:
        void SetupScene(void);
        void ShutdownScene(void);
        void ClearData(void);
        void ClearPointCloudData(void);
        void ClearMeshData(void);

    private:
        AnimationAppUI* mpUI;
        MagicCore::ViewTool* mpViewTool;
        DeformType mDeformType;
        GPP::DeformPointList* mDeformPointList;
        GPP::DeformMesh* mDeformMesh;
        std::vector<GPP::Int> mControlIds;
        bool mIsDeformationInitialised;
        int mPickControlId;
        GPP::Vector3 mPickTargetCoord;
        GPP::Vector2 mMousePressdCoord;
        std::vector<int> mControlFlags; // 0-free, 1-fix, 2-handle
        RightMouseType mRightMouseType;
        bool mAddSelection;
        bool mFirstAlert;
        std::vector<GPP::Vector3> mTargetControlCoords;
        std::vector<int> mTargetControlIds;
    };
}

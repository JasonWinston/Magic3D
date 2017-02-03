#pragma once
#include "AppBase.h"
#if DEBUGDUMPFILE
#include "DumpBase.h"
#endif

namespace MagicCore
{
    class ViewTool;
}

namespace MagicApp
{
    class HomepageUI;
    class Homepage : public AppBase
    {
    public: 
        Homepage();
        virtual ~Homepage();

        virtual bool Enter(void);
        virtual bool Update(double timeElapsed);
        virtual bool Exit(void);
        virtual bool MouseMoved(const OIS::MouseEvent &arg);
        virtual bool MousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id);
        virtual bool MouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id);
        virtual bool KeyPressed(const OIS::KeyEvent &arg);
        virtual void WindowFocusChanged(Ogre::RenderWindow* rw);

        void SwitchDisplayMode(void);

        void ImportPointCloud(void);
        void ImportMesh(void);
        void ExportModel(bool unify);

    private:
        void SetupScene(void);
        void ShutdownScene(void);
        void ClearData(void);
        void UpdateModelRendering(void);

#if DEBUGDUMPFILE
        void LoadDumpFile(void);
        void RunDumpInfo(void);
#endif

    private:
        HomepageUI* mpUI;
        MagicCore::ViewTool* mpViewTool;
#if DEBUGDUMPFILE
        GPP::DumpBase* mpDumpInfo;
#endif
        int mDisplayMode;
    };
}

#pragma once
#include "MyGUI.h"

namespace MagicApp
{
    class HomepageUI
    {
    public:
        HomepageUI();
        ~HomepageUI();

        void Setup();
        void Shutdown();

        void SetModelInfo(int vertexCount, int triangleCount);

    private:
        void SwitchDisplayMode(MyGUI::Widget* pSender);
        void ImportModel(MyGUI::Widget* pSender);
        void ImportPointCloud(MyGUI::Widget* pSender);
        void ImportMesh(MyGUI::Widget* pSender);
        void EnterPointShopApp(MyGUI::Widget* pSender);
        void EnterMeshShopApp(MyGUI::Widget* pSender);
        void EnterRegistrationApp(MyGUI::Widget* pSender);
        void EnterMeasureApp(MyGUI::Widget* pSender);
        void EnterReliefApp(MyGUI::Widget* pSender);
        void EnterTextureApp(MyGUI::Widget* pSender);
        void EnterAnimationApp(MyGUI::Widget* pSender);
        void EnterUVUnfoldApp(MyGUI::Widget* pSender);
        void Contact(MyGUI::Widget* pSender);

    private:
        MyGUI::VectorWidgetPtr mRoot;
        MyGUI::TextBox* mTextInfo;
    };
}

#pragma once
#include "MyGUI.h"

namespace MagicApp
{
    class AnimationAppUI
    {
    public:
        AnimationAppUI();
        ~AnimationAppUI();

        void Setup();
        void Shutdown();

        void ShowControlBar(bool isVisible);
        void ShowVertexBar(bool isVisible);

    private:
        void ImportModel(MyGUI::Widget* pSender);
        
        void InitControlDeformType(MyGUI::Widget* pSender);
        void DoInitControlPoint(MyGUI::Widget* pSender);
        void SelectControlByRectangle(MyGUI::Widget* pSender);
        void ClearControlSelection(MyGUI::Widget* pSender);
        void MoveControlPoint(MyGUI::Widget* pSender);
        void InitControlDeformation(MyGUI::Widget* pSender);
        void DoControlDeformation(MyGUI::Widget* pSender);

        void InitVertexDeformType(MyGUI::Widget* pSender);
        void SelectVertexByRectangle(MyGUI::Widget* pSender);
        void ClearVertexSelection(MyGUI::Widget* pSender);
        void MoveVertex(MyGUI::Widget* pSender);
        void InitVertexDeformation(MyGUI::Widget* pSender);
        void DoVertexDeformation(MyGUI::Widget* pSender);

        void BackToHomepage(MyGUI::Widget* pSender);

    private:
        MyGUI::VectorWidgetPtr mRoot;
    };
}

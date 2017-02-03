#include "AnimationAppUI.h"
#include "../Common/ResourceManager.h"
#include "../Common/ToolKit.h"
#include "../Common/RenderSystem.h"
#include "AppManager.h"
#include "AnimationApp.h"

namespace MagicApp
{
    AnimationAppUI::AnimationAppUI()
    {
    }

    AnimationAppUI::~AnimationAppUI()
    {
    }

    void AnimationAppUI::Setup()
    {
        MagicCore::ResourceManager::LoadResource("../../Media/AnimationApp", "FileSystem", "AnimationApp");
        mRoot = MyGUI::LayoutManager::getInstance().loadLayout("AnimationApp.layout");
        mRoot.at(0)->findWidget("But_ImportModel")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &AnimationAppUI::ImportModel);
        
        mRoot.at(0)->findWidget("But_InitControlDeformType")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &AnimationAppUI::InitControlDeformType);
        mRoot.at(0)->findWidget("But_DoInitControlPoint")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &AnimationAppUI::DoInitControlPoint);
        mRoot.at(0)->findWidget("But_SelectControlByRectangle")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &AnimationAppUI::SelectControlByRectangle);
        mRoot.at(0)->findWidget("But_EraseControlSelections")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &AnimationAppUI::ClearControlSelection);
        mRoot.at(0)->findWidget("But_MoveControlPoint")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &AnimationAppUI::MoveControlPoint);
        mRoot.at(0)->findWidget("But_InitControlDeformation")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &AnimationAppUI::InitControlDeformation);
        mRoot.at(0)->findWidget("But_DoControlDeformation")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &AnimationAppUI::DoControlDeformation);
        
        mRoot.at(0)->findWidget("But_InitVertexDeformType")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &AnimationAppUI::InitVertexDeformType);
        mRoot.at(0)->findWidget("But_SelectVertexByRectangle")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &AnimationAppUI::SelectVertexByRectangle);
        mRoot.at(0)->findWidget("But_EraseVertexSelections")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &AnimationAppUI::ClearVertexSelection);
        mRoot.at(0)->findWidget("But_MoveVertex")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &AnimationAppUI::MoveVertex);
        mRoot.at(0)->findWidget("But_InitVertexDeformation")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &AnimationAppUI::InitVertexDeformation);
        mRoot.at(0)->findWidget("But_DoVertexDeformation")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &AnimationAppUI::DoVertexDeformation);
        
        mRoot.at(0)->findWidget("But_BackToHomepage")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &AnimationAppUI::BackToHomepage);
    }

    void AnimationAppUI::Shutdown()
    {
        MyGUI::LayoutManager::getInstance().unloadLayout(mRoot);
        mRoot.clear();
        MagicCore::ResourceManager::UnloadResource("AnimationApp");
    }

    void AnimationAppUI::ImportModel(MyGUI::Widget* pSender)
    {
        AnimationApp* animationApp = dynamic_cast<AnimationApp* >(AppManager::Get()->GetApp("AnimationApp"));
        if (animationApp != NULL)
        {
            animationApp->ImportModel();
        }
    }

    void AnimationAppUI::ShowControlBar(bool isVisible)
    {
        mRoot.at(0)->findWidget("But_DoInitControlPoint")->castType<MyGUI::Button>()->setVisible(isVisible);
        mRoot.at(0)->findWidget("Edit_ControlPointCount")->castType<MyGUI::EditBox>()->setVisible(isVisible);
        if (isVisible)
        {
            std::stringstream ss;
            std::string textString;
            ss << 300;
            ss >> textString;
            mRoot.at(0)->findWidget("Edit_ControlPointCount")->castType<MyGUI::EditBox>()->setOnlyText(textString);
            mRoot.at(0)->findWidget("Edit_ControlPointCount")->castType<MyGUI::EditBox>()->setTextSelectionColour(MyGUI::Colour::Black);
        }
        mRoot.at(0)->findWidget("But_SelectControlByRectangle")->castType<MyGUI::Button>()->setVisible(isVisible);
        mRoot.at(0)->findWidget("But_EraseControlSelections")->castType<MyGUI::Button>()->setVisible(isVisible);
        mRoot.at(0)->findWidget("But_MoveControlPoint")->castType<MyGUI::Button>()->setVisible(isVisible);
        mRoot.at(0)->findWidget("But_InitControlDeformation")->castType<MyGUI::Button>()->setVisible(isVisible);
        mRoot.at(0)->findWidget("But_DoControlDeformation")->castType<MyGUI::Button>()->setVisible(isVisible);
    }

    void AnimationAppUI::InitControlDeformType(MyGUI::Widget* pSender)
    {
        bool isVisible = mRoot.at(0)->findWidget("But_DoInitControlPoint")->castType<MyGUI::Button>()->isVisible();
        isVisible = !isVisible;
        ShowControlBar(isVisible);
        if (isVisible)
        {
            ShowVertexBar(false);
        }
        AnimationApp* animationApp = dynamic_cast<AnimationApp* >(AppManager::Get()->GetApp("AnimationApp"));
        if (animationApp != NULL)
        {
            if (isVisible)
            {
                animationApp->SwitchDeformType(AnimationApp::DT_CONTROL_POINT);
            }
            else
            {
                animationApp->SwitchDeformType(AnimationApp::DT_NONE);
            }
        }
    }

    void AnimationAppUI::DoInitControlPoint(MyGUI::Widget* pSender)
    {
        AnimationApp* animationApp = dynamic_cast<AnimationApp* >(AppManager::Get()->GetApp("AnimationApp"));
        if (animationApp != NULL)
        {
            std::string textString = mRoot.at(0)->findWidget("Edit_ControlPointCount")->castType<MyGUI::EditBox>()->getOnlyText();
            int controlPointCount = std::atoi(textString.c_str());
            if (controlPointCount < 1)
            {
                std::stringstream ss;
                std::string textString;
                ss << 300;
                ss >> textString;
                mRoot.at(0)->findWidget("Edit_ControlPointCount")->castType<MyGUI::EditBox>()->setOnlyText(textString);
            }
            else
            {
                animationApp->InitControlPoint(controlPointCount);
            }      
        }
    }
    
    void AnimationAppUI::SelectControlByRectangle(MyGUI::Widget* pSender)
    {
        AnimationApp* animationApp = dynamic_cast<AnimationApp* >(AppManager::Get()->GetApp("AnimationApp"));
        if (animationApp != NULL)
        {
            animationApp->SelectFreeControlPoint();
        }
    }
    
    void AnimationAppUI::ClearControlSelection(MyGUI::Widget* pSender)
    {
        AnimationApp* animationApp = dynamic_cast<AnimationApp* >(AppManager::Get()->GetApp("AnimationApp"));
        if (animationApp != NULL)
        {
            animationApp->ClearFreeControlPoint();
        }
    }

    void AnimationAppUI::MoveControlPoint(MyGUI::Widget* pSender)
    {
        AnimationApp* animationApp = dynamic_cast<AnimationApp* >(AppManager::Get()->GetApp("AnimationApp"));
        if (animationApp != NULL)
        {
            animationApp->MoveControlPoint();
        }
    }

    void AnimationAppUI::InitControlDeformation(MyGUI::Widget* pSender)
    {
        AnimationApp* animationApp = dynamic_cast<AnimationApp* >(AppManager::Get()->GetApp("AnimationApp"));
        if (animationApp != NULL)
        {
            animationApp->InitControlDeformation();
        }
    }

    void AnimationAppUI::DoControlDeformation(MyGUI::Widget* pSender)
    {
        AnimationApp* animationApp = dynamic_cast<AnimationApp* >(AppManager::Get()->GetApp("AnimationApp"));
        if (animationApp != NULL)
        {
            animationApp->DoControlDeformation();
        }
    }

    void AnimationAppUI::ShowVertexBar(bool isVisible)
    {
        mRoot.at(0)->findWidget("But_SelectVertexByRectangle")->castType<MyGUI::Button>()->setVisible(isVisible);
        mRoot.at(0)->findWidget("But_EraseVertexSelections")->castType<MyGUI::Button>()->setVisible(isVisible);
        mRoot.at(0)->findWidget("But_MoveVertex")->castType<MyGUI::Button>()->setVisible(isVisible);
        mRoot.at(0)->findWidget("But_InitVertexDeformation")->castType<MyGUI::Button>()->setVisible(isVisible);
        mRoot.at(0)->findWidget("But_DoVertexDeformation")->castType<MyGUI::Button>()->setVisible(isVisible);
    }

    void AnimationAppUI::InitVertexDeformType(MyGUI::Widget* pSender)
    {
        bool isVisible = mRoot.at(0)->findWidget("But_SelectVertexByRectangle")->castType<MyGUI::Button>()->isVisible();
        isVisible = !isVisible;
        ShowVertexBar(isVisible);
        if (isVisible)
        {
            ShowControlBar(false);
        }
        AnimationApp* animationApp = dynamic_cast<AnimationApp* >(AppManager::Get()->GetApp("AnimationApp"));
        if (animationApp != NULL)
        {
            if (isVisible)
            {
                animationApp->SwitchDeformType(AnimationApp::DT_VERTEX);
            }
            else
            {
                animationApp->SwitchDeformType(AnimationApp::DT_NONE);
            }
        }
    }

    void AnimationAppUI::SelectVertexByRectangle(MyGUI::Widget* pSender)
    {
        AnimationApp* animationApp = dynamic_cast<AnimationApp* >(AppManager::Get()->GetApp("AnimationApp"));
        if (animationApp != NULL)
        {
            animationApp->SelectFreeControlPoint();
        }
    }

    void AnimationAppUI::ClearVertexSelection(MyGUI::Widget* pSender)
    {
        AnimationApp* animationApp = dynamic_cast<AnimationApp* >(AppManager::Get()->GetApp("AnimationApp"));
        if (animationApp != NULL)
        {
            animationApp->ClearFreeControlPoint();
        }
    }

    void AnimationAppUI::MoveVertex(MyGUI::Widget* pSender)
    {
        AnimationApp* animationApp = dynamic_cast<AnimationApp* >(AppManager::Get()->GetApp("AnimationApp"));
        if (animationApp != NULL)
        {
            animationApp->MoveControlPoint();
        }
    }

    void AnimationAppUI::InitVertexDeformation(MyGUI::Widget* pSender)
    {
        AnimationApp* animationApp = dynamic_cast<AnimationApp* >(AppManager::Get()->GetApp("AnimationApp"));
        if (animationApp != NULL)
        {
            animationApp->InitControlDeformation();
        }
    }

    void AnimationAppUI::DoVertexDeformation(MyGUI::Widget* pSender)
    {
        AnimationApp* animationApp = dynamic_cast<AnimationApp* >(AppManager::Get()->GetApp("AnimationApp"));
        if (animationApp != NULL)
        {
            animationApp->DoControlDeformation();
        }
    }

    void AnimationAppUI::BackToHomepage(MyGUI::Widget* pSender)
    {
        AppManager::Get()->SwitchCurrentApp("Homepage");
    }
}

#include "HomepageUI.h"
#include "Homepage.h"
#include "../Common/ResourceManager.h"
#include "../Common/ToolKit.h"
#include "../Common/RenderSystem.h"
#include "AppManager.h"
#include "PointShopApp.h"
#include "MeshShopApp.h"
#include "RegistrationApp.h"
#include "MeasureApp.h"
#include "ReliefApp.h"
#include "TextureApp.h"
#include "AnimationApp.h"
#include "UVUnfoldApp.h"

namespace MagicApp
{
    HomepageUI::HomepageUI()
    {
    }

    HomepageUI::~HomepageUI()
    {

    }

    void HomepageUI::Setup()
    {
        MagicCore::ResourceManager::LoadResource("../../Media/Homepage", "FileSystem", "Homepage");
        mRoot = MyGUI::LayoutManager::getInstance().loadLayout("Homepage.layout");
        mRoot.at(0)->findWidget("But_DisplayMode")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &HomepageUI::SwitchDisplayMode);
        mRoot.at(0)->findWidget("But_ImportModel")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &HomepageUI::ImportModel);
        mRoot.at(0)->findWidget("But_ImportPointCloud")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &HomepageUI::ImportPointCloud);
        mRoot.at(0)->findWidget("But_ImportMesh")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &HomepageUI::ImportMesh);
        mRoot.at(0)->findWidget("But_EnterPointShopApp")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &HomepageUI::EnterPointShopApp);
        mRoot.at(0)->findWidget("But_EnterMeshShopApp")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &HomepageUI::EnterMeshShopApp);
        mRoot.at(0)->findWidget("But_EnterRegistrationApp")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &HomepageUI::EnterRegistrationApp);
        mRoot.at(0)->findWidget("But_EnterMeasureApp")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &HomepageUI::EnterMeasureApp);
        mRoot.at(0)->findWidget("But_EnterReliefApp")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &HomepageUI::EnterReliefApp);
        mRoot.at(0)->findWidget("But_EnterTextureApp")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &HomepageUI::EnterTextureApp);
        mRoot.at(0)->findWidget("But_EnterAnimationApp")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &HomepageUI::EnterAnimationApp);
        mRoot.at(0)->findWidget("But_EnterUVUnfoldApp")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &HomepageUI::EnterUVUnfoldApp);
        mRoot.at(0)->findWidget("But_Contact")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &HomepageUI::Contact);

        mTextInfo = mRoot.at(0)->findWidget("Text_Info")->castType<MyGUI::TextBox>();
        mTextInfo->setTextColour(MyGUI::Colour(75.0 / 255.0, 131.0 / 255.0, 128.0 / 255.0));
    }

    void HomepageUI::SetModelInfo(int vertexCount, int triangleCount)
    {
        std::string textString = "";
        if (vertexCount > 0)
        {
            textString += "Vertex count = ";
            std::stringstream ss;
            ss << vertexCount;
            std::string numberString;
            ss >> numberString;
            textString += numberString;
            textString += "  Triangle count = ";
            ss.clear();
            ss << triangleCount;
            numberString.clear();
            ss >> numberString;
            textString += numberString;
            textString += "\n";
        }
        mTextInfo->setCaption(textString);
    }

    void HomepageUI::SwitchDisplayMode(MyGUI::Widget* pSender)
    {
        Homepage* homepage = dynamic_cast<Homepage* >(AppManager::Get()->GetApp("Homepage"));
        if (homepage != NULL)
        {
            homepage->SwitchDisplayMode();
        }
    }

    void HomepageUI::ImportModel(MyGUI::Widget* pSender)
    {
        bool isVisible = mRoot.at(0)->findWidget("But_ImportPointCloud")->castType<MyGUI::Button>()->isVisible();
        mRoot.at(0)->findWidget("But_ImportPointCloud")->castType<MyGUI::Button>()->setVisible(!isVisible);
        mRoot.at(0)->findWidget("But_ImportMesh")->castType<MyGUI::Button>()->setVisible(!isVisible);
    }

    void HomepageUI::ImportPointCloud(MyGUI::Widget* pSender)
    {
        Homepage* homepage = dynamic_cast<Homepage* >(AppManager::Get()->GetApp("Homepage"));
        if (homepage != NULL)
        {
            homepage->ImportPointCloud();
        }
    }

    void HomepageUI::ImportMesh(MyGUI::Widget* pSender)
    {
        Homepage* homepage = dynamic_cast<Homepage* >(AppManager::Get()->GetApp("Homepage"));
        if (homepage != NULL)
        {
            homepage->ImportMesh();
        }
    }

    void HomepageUI::EnterPointShopApp(MyGUI::Widget* pSender)
    {
        AppManager::Get()->EnterApp(new PointShopApp, "PointShopApp");
    }

    void HomepageUI::EnterMeshShopApp(MyGUI::Widget* pSender)
    {
        AppManager::Get()->EnterApp(new MeshShopApp, "MeshShopApp");
    }

    void HomepageUI::EnterRegistrationApp(MyGUI::Widget* pSender)
    {
        AppManager::Get()->EnterApp(new RegistrationApp, "RegistrationApp");
    }

    void HomepageUI::EnterMeasureApp(MyGUI::Widget* pSender)
    {
        AppManager::Get()->EnterApp(new MeasureApp, "MeasureApp");
    }

    void HomepageUI::EnterReliefApp(MyGUI::Widget* pSender)
    {
        AppManager::Get()->EnterApp(new ReliefApp, "ReliefApp");
    }

    void HomepageUI::EnterTextureApp(MyGUI::Widget* pSender)
    {
        AppManager::Get()->EnterApp(new TextureApp, "TextureApp");
    }

    void HomepageUI::EnterAnimationApp(MyGUI::Widget* pSender)
    {
        AppManager::Get()->EnterApp(new AnimationApp, "AnimationApp");
    }

    void HomepageUI::EnterUVUnfoldApp(MyGUI::Widget* pSender)
    {
        AppManager::Get()->EnterApp(new UVUnfoldApp, "UVUnfoldApp");
    }

    void HomepageUI::Shutdown()
    {
        MyGUI::LayoutManager::getInstance().unloadLayout(mRoot);
        mRoot.clear();
        MagicCore::ResourceManager::UnloadResource("Homepage");
    }

    void HomepageUI::Contact(MyGUI::Widget* pSender)
    {
        MagicCore::ToolKit::OpenWebsite(std::string("http://threepark.net/magic3d"));
    }
}

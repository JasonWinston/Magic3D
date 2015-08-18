#include "MeshShopAppUI.h"
#include "../Common/ResourceManager.h"
#include "../Common/ToolKit.h"
#include "../Common/RenderSystem.h"
#include "AppManager.h"
#include "MeshShopApp.h"

namespace MagicApp
{
    MeshShopAppUI::MeshShopAppUI()
    {
    }

    MeshShopAppUI::~MeshShopAppUI()
    {
    }

    void MeshShopAppUI::Setup()
    {
        MagicCore::ResourceManager::LoadResource("../../Media/MeshShopApp", "FileSystem", "MeshShopApp");
        mRoot = MyGUI::LayoutManager::getInstance().loadLayout("MeshShopApp.layout");
        mRoot.at(0)->findWidget("But_ImportMesh")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeshShopAppUI::ImportMesh);
        mRoot.at(0)->findWidget("But_ExportMesh")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeshShopAppUI::ExportMesh);
        mRoot.at(0)->findWidget("But_BackToHomepage")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeshShopAppUI::BackToHomepage);
        mRoot.at(0)->findWidget("But_Contact")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeshShopAppUI::Contact);
    }

    void MeshShopAppUI::Shutdown()
    {
        MyGUI::LayoutManager::getInstance().unloadLayout(mRoot);
        mRoot.clear();
        MagicCore::ResourceManager::UnloadResource("MeshShopApp");
    }

    void MeshShopAppUI::ImportMesh(MyGUI::Widget* pSender)
    {
        MeshShopApp* meshShop = dynamic_cast<MeshShopApp* >(AppManager::Get()->GetApp("MeshShopApp"));
        if (meshShop != NULL)
        {
            if (meshShop->ImportMesh())
            {
                mRoot.at(0)->findWidget("But_ExportMesh")->castType<MyGUI::Button>()->setEnabled(true);
            }
        }
    }

    void MeshShopAppUI::ExportMesh(MyGUI::Widget* pSender)
    {
    }

    void MeshShopAppUI::BackToHomepage(MyGUI::Widget* pSender)
    {
        AppManager::Get()->SwitchCurrentApp("Homepage");
    }

    void MeshShopAppUI::Contact(MyGUI::Widget* pSender)
    {
        MagicCore::ToolKit::OpenWebsite(std::string("https://github.com/threepark"));
    }
}

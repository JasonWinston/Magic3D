#include "MeshShopAppUI.h"
#include "../Common/ResourceManager.h"
#include "../Common/ToolKit.h"
#include "../Common/RenderSystem.h"
#include "AppManager.h"
#include "MeshShopApp.h"

namespace MagicApp
{
    MeshShopAppUI::MeshShopAppUI() :
        mIsProgressbarVisible(false),
        mTextInfo(NULL),
        mIgnoreBack(true)
    {
    }

    MeshShopAppUI::~MeshShopAppUI()
    {
    }

    void MeshShopAppUI::Setup()
    {
        MagicCore::ResourceManager::LoadResource("../../Media/MeshShopApp", "FileSystem", "MeshShopApp");
        mRoot = MyGUI::LayoutManager::getInstance().loadLayout("MeshShopApp.layout");
        mRoot.at(0)->findWidget("But_DisplayMode")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeshShopAppUI::SwitchDisplayMode);
        mRoot.at(0)->findWidget("But_ImportMesh")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeshShopAppUI::ImportMesh);
        mRoot.at(0)->findWidget("But_ConsolidateMesh")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeshShopAppUI::ConsolidateMesh);
        mRoot.at(0)->findWidget("But_IsManifold")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeshShopAppUI::IsManifold);
        mRoot.at(0)->findWidget("But_ConsolidateTopology")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeshShopAppUI::ConsolidateTopology);
        mRoot.at(0)->findWidget("But_ReverseDirection")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeshShopAppUI::ReverseDirection);
        mRoot.at(0)->findWidget("But_RemoveMeshIsolatePart")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeshShopAppUI::RemoveMeshIsolatePart);
        mRoot.at(0)->findWidget("But_ConsolidateGeometry")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeshShopAppUI::ConsolidateGeometry);
        mRoot.at(0)->findWidget("But_CDTOptimization")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeshShopAppUI::CDTOptimization);
        mRoot.at(0)->findWidget("But_CVTOptimization")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeshShopAppUI::CVTOptimization);
        
        mRoot.at(0)->findWidget("But_FilterMesh")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeshShopAppUI::FilterMesh);
        mRoot.at(0)->findWidget("But_RemoveMeshNoise")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeshShopAppUI::RemoveMeshNoise);
        mRoot.at(0)->findWidget("But_SmoothMesh")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeshShopAppUI::SmoothMesh);
        mRoot.at(0)->findWidget("But_EnhanceMeshDetail")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeshShopAppUI::EnhanceMeshDetail);
        
        mRoot.at(0)->findWidget("But_ChangeVertexCount")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeshShopAppUI::ChangeVertexCount);
        mRoot.at(0)->findWidget("But_DoSimplifyMesh")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeshShopAppUI::DoSimplifyMesh);
        mRoot.at(0)->findWidget("But_DoRefineMesh")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeshShopAppUI::DoRefineMesh);
        mRoot.at(0)->findWidget("But_DoSubdivideMesh")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeshShopAppUI::SubdivideMesh);
        
        mRoot.at(0)->findWidget("But_ReMesh")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeshShopAppUI::Remesh);
        mRoot.at(0)->findWidget("But_DoUniformRemesh")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeshShopAppUI::DoUniformRemesh);
        mRoot.at(0)->findWidget("But_FillHole")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeshShopAppUI::FillHole);
        mRoot.at(0)->findWidget("But_FillHoleTriangulation")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeshShopAppUI::DoFillHoleTriangulation);
        mRoot.at(0)->findWidget("But_FillHoleFlat")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeshShopAppUI::DoFillHoleFlat);
        mRoot.at(0)->findWidget("But_FillHoleTangent")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeshShopAppUI::DoFillHoleTangent);
        mRoot.at(0)->findWidget("But_FillHoleSmooth")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeshShopAppUI::DoFillHoleSmooth);
        mRoot.at(0)->findWidget("But_BridgeEdges")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeshShopAppUI::DoBridgeEdges);
        mRoot.at(0)->findWidget("But_OffsetMesh")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeshShopAppUI::OffsetMesh);
        mRoot.at(0)->findWidget("But_DoUniformOffset")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeshShopAppUI::DoUniformOffset);
        mRoot.at(0)->findWidget("But_Selection")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeshShopAppUI::SelectPoint);
        mRoot.at(0)->findWidget("But_SelectByRectangle")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeshShopAppUI::SelectByRectangle);
        mRoot.at(0)->findWidget("But_EraseByRectangle")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeshShopAppUI::EraseByRectangle);
        mRoot.at(0)->findWidget("But_DeleteSelections")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeshShopAppUI::DeleteSelections);
        mRoot.at(0)->findWidget("But_SimplifySelections")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeshShopAppUI::SimplifySelections);
        mRoot.at(0)->findWidget("But_IgnoreBack")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeshShopAppUI::IgnoreBack);
        mRoot.at(0)->findWidget("But_MoveModel")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeshShopAppUI::MoveModel);        
        
        mRoot.at(0)->findWidget("But_CutMesh")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeshShopAppUI::CutMesh);        
        mRoot.at(0)->findWidget("But_CutMesh_Plane_XY")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeshShopAppUI::CutMeshByXYPlane);        
        mRoot.at(0)->findWidget("But_CutMesh_Plane_YZ")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeshShopAppUI::CutMeshByYZPlane);        
        mRoot.at(0)->findWidget("But_CutMesh_Plane_ZX")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeshShopAppUI::CutMeshByZXPlane);        
        mRoot.at(0)->findWidget("But_CutMesh_Plane_Random")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeshShopAppUI::CutMeshByRandomPlane);        

        mRoot.at(0)->findWidget("But_SampleMesh")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeshShopAppUI::SampleMesh);        
        mRoot.at(0)->findWidget("But_DoUniformSampling")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeshShopAppUI::DoUniformSampling);
        mRoot.at(0)->findWidget("But_BackToHomepage")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeshShopAppUI::BackToHomepage);

        mTextInfo = mRoot.at(0)->findWidget("Text_Info")->castType<MyGUI::TextBox>();
        mTextInfo->setTextColour(MyGUI::Colour(75.0 / 255.0, 131.0 / 255.0, 128.0 / 255.0));
    }

    void MeshShopAppUI::StartProgressbar(int range)
    {
        mRoot.at(0)->findWidget("APIProgress")->castType<MyGUI::ProgressBar>()->setVisible(true);
        mRoot.at(0)->findWidget("APIProgress")->castType<MyGUI::ProgressBar>()->setProgressRange(range);
        mRoot.at(0)->findWidget("APIProgress")->castType<MyGUI::ProgressBar>()->setProgressPosition(0);
        mIsProgressbarVisible = true;
    }

    void MeshShopAppUI::SetProgressbar(int value)
    {
        mRoot.at(0)->findWidget("APIProgress")->castType<MyGUI::ProgressBar>()->setProgressPosition(value);
    }

    void MeshShopAppUI::StopProgressbar()
    {
        mRoot.at(0)->findWidget("APIProgress")->castType<MyGUI::ProgressBar>()->setVisible(false);
        mIsProgressbarVisible = false;
    }

    bool MeshShopAppUI::IsProgressbarVisible()
    {
        return mIsProgressbarVisible;
    }

    void MeshShopAppUI::SetMeshInfo(int vertexCount, int triangleCount)
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

    void MeshShopAppUI::Shutdown()
    {
        MyGUI::LayoutManager::getInstance().unloadLayout(mRoot);
        mRoot.clear();
        MagicCore::ResourceManager::UnloadResource("MeshShopApp");
    }

    void MeshShopAppUI::SwitchDisplayMode(MyGUI::Widget* pSender)
    {
        MeshShopApp* meshShop = dynamic_cast<MeshShopApp* >(AppManager::Get()->GetApp("MeshShopApp"));
        if (meshShop != NULL)
        {
            meshShop->SwitchDisplayMode();
        }
    }

    void MeshShopAppUI::ImportMesh(MyGUI::Widget* pSender)
    {
        MeshShopApp* meshShop = dynamic_cast<MeshShopApp* >(AppManager::Get()->GetApp("MeshShopApp"));
        if (meshShop != NULL)
        {
            meshShop->ImportMesh();
        }
    }

    void MeshShopAppUI::ConsolidateMesh(MyGUI::Widget* pSender)
    {
        bool isVisible = mRoot.at(0)->findWidget("But_ConsolidateTopology")->castType<MyGUI::Button>()->isVisible();
        isVisible = !isVisible;
        mRoot.at(0)->findWidget("But_IsManifold")->castType<MyGUI::Button>()->setVisible(isVisible);
        mRoot.at(0)->findWidget("But_ConsolidateTopology")->castType<MyGUI::Button>()->setVisible(isVisible);
        mRoot.at(0)->findWidget("But_ReverseDirection")->castType<MyGUI::Button>()->setVisible(isVisible);
        mRoot.at(0)->findWidget("But_ConsolidateGeometry")->castType<MyGUI::Button>()->setVisible(isVisible);
        mRoot.at(0)->findWidget("But_RemoveMeshIsolatePart")->castType<MyGUI::Button>()->setVisible(isVisible);
        mRoot.at(0)->findWidget("But_CDTOptimization")->castType<MyGUI::Button>()->setVisible(isVisible);
        mRoot.at(0)->findWidget("Edit_CDTSharpAngle")->castType<MyGUI::EditBox>()->setVisible(isVisible);
        if (isVisible)
        {
            std::stringstream ss;
            ss << 30;
            std::string textString;
            ss >> textString;
            mRoot.at(0)->findWidget("Edit_CDTSharpAngle")->castType<MyGUI::EditBox>()->setOnlyText(textString);
        }
        mRoot.at(0)->findWidget("But_CVTOptimization")->castType<MyGUI::Button>()->setVisible(isVisible);
        mRoot.at(0)->findWidget("Edit_CVTSharpAngle")->castType<MyGUI::EditBox>()->setVisible(isVisible);
        if (isVisible)
        {
            std::stringstream ss;
            ss << 30;
            std::string textString;
            ss >> textString;
            mRoot.at(0)->findWidget("Edit_CVTSharpAngle")->castType<MyGUI::EditBox>()->setOnlyText(textString);
        }
    }

    void MeshShopAppUI::IsManifold(MyGUI::Widget* pSender)
    {
        MeshShopApp* meshShop = dynamic_cast<MeshShopApp* >(AppManager::Get()->GetApp("MeshShopApp"));
        if (meshShop != NULL)
        {
            meshShop->IsManifold();
        }
    }

    void MeshShopAppUI::ConsolidateTopology(MyGUI::Widget* pSender)
    {
        MeshShopApp* meshShop = dynamic_cast<MeshShopApp* >(AppManager::Get()->GetApp("MeshShopApp"));
        if (meshShop != NULL)
        {
            meshShop->ConsolidateTopology();
        }
    }

    void MeshShopAppUI::ReverseDirection(MyGUI::Widget* pSender)
    {
        MeshShopApp* meshShop = dynamic_cast<MeshShopApp* >(AppManager::Get()->GetApp("MeshShopApp"));
        if (meshShop != NULL)
        {
            meshShop->ReverseDirection();
        }
    }

    void MeshShopAppUI::RemoveMeshIsolatePart(MyGUI::Widget* pSender)
    {
        MeshShopApp* meshShop = dynamic_cast<MeshShopApp* >(AppManager::Get()->GetApp("MeshShopApp"));
        if (meshShop != NULL)
        {
            meshShop->RemoveMeshIsolatePart();
        }
    }

    void MeshShopAppUI::CDTOptimization(MyGUI::Widget* pSender)
    {
        MeshShopApp* meshShop = dynamic_cast<MeshShopApp* >(AppManager::Get()->GetApp("MeshShopApp"));
        if (meshShop != NULL)
        {
            std::string textString = mRoot.at(0)->findWidget("Edit_CDTSharpAngle")->castType<MyGUI::EditBox>()->getOnlyText();
            double sharpAngle = std::atof(textString.c_str());
            if (sharpAngle < 0 || sharpAngle > 180)
            {
                std::stringstream ss;
                std::string textString;
                ss << 30;
                ss >> textString;
                mRoot.at(0)->findWidget("Edit_CDTSharpAngle")->castType<MyGUI::EditBox>()->setOnlyText(textString);
            }
            else
            {
                meshShop->CDTOptimization(sharpAngle, true);
            }
        }
    }

    void MeshShopAppUI::CVTOptimization(MyGUI::Widget* pSender)
    {
        MeshShopApp* meshShop = dynamic_cast<MeshShopApp* >(AppManager::Get()->GetApp("MeshShopApp"));
        if (meshShop != NULL)
        {
            std::string textString = mRoot.at(0)->findWidget("Edit_CVTSharpAngle")->castType<MyGUI::EditBox>()->getOnlyText();
            double sharpAngle = std::atof(textString.c_str());
            if (sharpAngle < 0 || sharpAngle > 180)
            {
                std::stringstream ss;
                std::string textString;
                ss << 30;
                ss >> textString;
                mRoot.at(0)->findWidget("Edit_CVTSharpAngle")->castType<MyGUI::EditBox>()->setOnlyText(textString);
            }
            else
            {
                meshShop->CVTOptimization(sharpAngle, true);
            }
        }
    }

    void MeshShopAppUI::ConsolidateGeometry(MyGUI::Widget* pSender)
    {
        MeshShopApp* meshShop = dynamic_cast<MeshShopApp* >(AppManager::Get()->GetApp("MeshShopApp"));
        if (meshShop != NULL)
        {
            meshShop->ConsolidateGeometry();
        }
    }

    void MeshShopAppUI::FilterMesh(MyGUI::Widget* pSender)
    {
        bool isVisible = mRoot.at(0)->findWidget("But_SmoothMesh")->castType<MyGUI::Button>()->isVisible();
        isVisible = !isVisible;
        mRoot.at(0)->findWidget("Edit_FilterPositionWeight")->castType<MyGUI::EditBox>()->setVisible(isVisible);
        mRoot.at(0)->findWidget("But_SmoothMesh")->castType<MyGUI::Button>()->setVisible(isVisible);
        mRoot.at(0)->findWidget("But_RemoveMeshNoise")->castType<MyGUI::Button>()->setVisible(isVisible);
        mRoot.at(0)->findWidget("But_EnhanceMeshDetail")->castType<MyGUI::Button>()->setVisible(isVisible);
        mRoot.at(0)->findWidget("Edit_EnhanceIntensity")->castType<MyGUI::EditBox>()->setVisible(isVisible);
        if (isVisible)
        {
            std::stringstream ss;
            std::string textString;
            ss << 1;
            ss >> textString;
            mRoot.at(0)->findWidget("Edit_FilterPositionWeight")->castType<MyGUI::EditBox>()->setOnlyText(textString);

            ss.clear();
            ss << 2;
            textString.clear();
            ss >> textString;
            mRoot.at(0)->findWidget("Edit_EnhanceIntensity")->castType<MyGUI::EditBox>()->setOnlyText(textString);
        }
    }

    void MeshShopAppUI::RemoveMeshNoise(MyGUI::Widget* pSender)
    {
        MeshShopApp* meshShop = dynamic_cast<MeshShopApp* >(AppManager::Get()->GetApp("MeshShopApp"));
        if (meshShop != NULL)
        {
            std::string textString = mRoot.at(0)->findWidget("Edit_FilterPositionWeight")->castType<MyGUI::EditBox>()->getOnlyText();
            double positionWeight = std::atof(textString.c_str());
            if (positionWeight > GPP::REAL_TOL)
            {
                meshShop->RemoveMeshNoise(positionWeight);
            }
            else
            {
                std::stringstream ss;
                std::string textString;
                ss << 1;
                ss >> textString;
                mRoot.at(0)->findWidget("Edit_FilterPositionWeight")->castType<MyGUI::EditBox>()->setOnlyText(textString);
            }          
        }
    }

    void MeshShopAppUI::SmoothMesh(MyGUI::Widget* pSender)
    {
        MeshShopApp* meshShop = dynamic_cast<MeshShopApp* >(AppManager::Get()->GetApp("MeshShopApp"));
        if (meshShop != NULL)
        {
            std::string textString = mRoot.at(0)->findWidget("Edit_FilterPositionWeight")->castType<MyGUI::EditBox>()->getOnlyText();
            double positionWeight = std::atof(textString.c_str());
            if (positionWeight > GPP::REAL_TOL)
            {
                meshShop->SmoothMesh(positionWeight);
            }
            else
            {
                std::stringstream ss;
                std::string textString;
                ss << 1;
                ss >> textString;
                mRoot.at(0)->findWidget("Edit_FilterPositionWeight")->castType<MyGUI::EditBox>()->setOnlyText(textString);
            }          
        }
    }

    void MeshShopAppUI::EnhanceMeshDetail(MyGUI::Widget* pSender)
    {
        MeshShopApp* meshShop = dynamic_cast<MeshShopApp* >(AppManager::Get()->GetApp("MeshShopApp"));
        if (meshShop != NULL)
        {
            std::string textString = mRoot.at(0)->findWidget("Edit_EnhanceIntensity")->castType<MyGUI::EditBox>()->getOnlyText();
            double intensity = std::atof(textString.c_str());
            if (intensity < 1.0)
            {
                std::stringstream ss;
                std::string textString;
                ss << 2;
                ss >> textString;
                mRoot.at(0)->findWidget("Edit_EnhanceIntensity")->castType<MyGUI::EditBox>()->setOnlyText(textString);
            }
            else
            {
                meshShop->EnhanceMeshDetail(intensity, true);
            }          
        }
    }

    void MeshShopAppUI::ChangeVertexCount(MyGUI::Widget* pSender)
    {
        bool isVisible = mRoot.at(0)->findWidget("But_DoSimplifyMesh")->castType<MyGUI::Button>()->isVisible();
        isVisible = !isVisible;
        mRoot.at(0)->findWidget("But_DoSimplifyMesh")->castType<MyGUI::Button>()->setVisible(isVisible);
        mRoot.at(0)->findWidget("But_DoRefineMesh")->castType<MyGUI::Button>()->setVisible(isVisible);
        mRoot.at(0)->findWidget("But_DoSubdivideMesh")->castType<MyGUI::Button>()->setVisible(isVisible);
        mRoot.at(0)->findWidget("Edit_RefineTargetVertexCount")->castType<MyGUI::EditBox>()->setVisible(isVisible);
        if (isVisible)
        {
            int meshVertexCount = 0;
            MeshShopApp* meshShop = dynamic_cast<MeshShopApp* >(AppManager::Get()->GetApp("MeshShopApp"));
            if (meshShop != NULL)
            {
                meshVertexCount = meshShop->GetMeshVertexCount();
            }
            std::stringstream ss;
            std::string textString;
            ss << meshVertexCount;
            ss >> textString;
            mRoot.at(0)->findWidget("Edit_RefineTargetVertexCount")->castType<MyGUI::EditBox>()->setOnlyText(textString);
        }
    }

    void MeshShopAppUI::SubdivideMesh(MyGUI::Widget* pSender)
    {
        MeshShopApp* meshShop = dynamic_cast<MeshShopApp* >(AppManager::Get()->GetApp("MeshShopApp"));
        if (meshShop != NULL)
        {
            meshShop->LoopSubdivide();
        }        
    }

    void MeshShopAppUI::DoRefineMesh(MyGUI::Widget* pSender)
    {
        MeshShopApp* meshShop = dynamic_cast<MeshShopApp* >(AppManager::Get()->GetApp("MeshShopApp"));
        if (meshShop != NULL)
        {
            int meshVertexCount = meshShop->GetMeshVertexCount();
            std::string textString = mRoot.at(0)->findWidget("Edit_RefineTargetVertexCount")->castType<MyGUI::EditBox>()->getOnlyText();
            int targetVertexCount = std::atoi(textString.c_str());
            if (targetVertexCount < meshVertexCount)
            {
                std::stringstream ss;
                std::string textString;
                ss << meshVertexCount;
                ss >> textString;
                mRoot.at(0)->findWidget("Edit_RefineTargetVertexCount")->castType<MyGUI::EditBox>()->setOnlyText(textString);
            }
            else
            {
                meshShop->RefineMesh(targetVertexCount);
            }            
        }
    }

    void MeshShopAppUI::DoSimplifyMesh(MyGUI::Widget* pSender)
    {
        MeshShopApp* meshShop = dynamic_cast<MeshShopApp* >(AppManager::Get()->GetApp("MeshShopApp"));
        if (meshShop != NULL)
        {
            int meshVertexCount = meshShop->GetMeshVertexCount();
            std::string textString = mRoot.at(0)->findWidget("Edit_RefineTargetVertexCount")->castType<MyGUI::EditBox>()->getOnlyText();
            int targetVertexCount = std::atoi(textString.c_str());
            if (targetVertexCount > meshVertexCount || targetVertexCount < 3)
            {
                std::stringstream ss;
                std::string textString;
                ss << meshVertexCount;
                ss >> textString;
                mRoot.at(0)->findWidget("Edit_RefineTargetVertexCount")->castType<MyGUI::EditBox>()->setOnlyText(textString);
            }
            else
            {
                meshShop->SimplifyMesh(targetVertexCount);
            }            
        }
    }

    void MeshShopAppUI::Remesh(MyGUI::Widget* pSender)
    {
        bool isVisible = mRoot.at(0)->findWidget("But_DoUniformRemesh")->castType<MyGUI::Button>()->isVisible();
        isVisible = !isVisible;
        mRoot.at(0)->findWidget("But_DoUniformRemesh")->castType<MyGUI::Button>()->setVisible(isVisible);
        mRoot.at(0)->findWidget("Edit_RemeshTargetVertexCount")->castType<MyGUI::EditBox>()->setVisible(isVisible);
        mRoot.at(0)->findWidget("Edit_RemeshSharpAngle")->castType<MyGUI::EditBox>()->setVisible(isVisible);
        if (isVisible)
        {
            int meshVertexCount = 0;
            MeshShopApp* meshShop = dynamic_cast<MeshShopApp* >(AppManager::Get()->GetApp("MeshShopApp"));
            if (meshShop != NULL)
            {
                meshVertexCount = meshShop->GetMeshVertexCount();
            }
            std::stringstream ss;
            std::string textString;
            ss << meshVertexCount;
            ss >> textString;
            mRoot.at(0)->findWidget("Edit_RemeshTargetVertexCount")->castType<MyGUI::EditBox>()->setOnlyText(textString);

            ss.clear();
            ss << 30;
            textString.clear();
            ss >> textString;
            mRoot.at(0)->findWidget("Edit_RemeshSharpAngle")->castType<MyGUI::EditBox>()->setOnlyText(textString);
        }
    }

    void MeshShopAppUI::DoUniformRemesh(MyGUI::Widget* pSender)
    {
        MeshShopApp* meshShop = dynamic_cast<MeshShopApp* >(AppManager::Get()->GetApp("MeshShopApp"));
        if (meshShop != NULL)
        {
            bool isValid = true;
            int meshVertexCount = meshShop->GetMeshVertexCount();
            std::string textString = mRoot.at(0)->findWidget("Edit_RemeshTargetVertexCount")->castType<MyGUI::EditBox>()->getOnlyText();
            int targetVertexCount = std::atoi(textString.c_str());
            if (targetVertexCount < 3)
            {
                isValid = false;
                std::stringstream ss;
                std::string textString;
                ss << meshVertexCount;
                ss >> textString;
                mRoot.at(0)->findWidget("Edit_RemeshTargetVertexCount")->castType<MyGUI::EditBox>()->setOnlyText(textString);
            }
            textString = mRoot.at(0)->findWidget("Edit_RemeshSharpAngle")->castType<MyGUI::EditBox>()->getOnlyText();
            double sharpAngle = std::atof(textString.c_str());
            if (sharpAngle < 0 || sharpAngle > 180)
            {
                isValid = false;
                std::stringstream ss;
                std::string textString;
                ss << 30;
                ss >> textString;
                mRoot.at(0)->findWidget("Edit_RemeshSharpAngle")->castType<MyGUI::EditBox>()->setOnlyText(textString);
            }
            if (isValid)
            {
                meshShop->UniformRemesh(targetVertexCount, sharpAngle);
            }            
        }
    }

    void MeshShopAppUI::SampleMesh(MyGUI::Widget* pSender)
    {
        bool isVisible = mRoot.at(0)->findWidget("But_DoUniformSampling")->castType<MyGUI::Button>()->isVisible();
        isVisible = !isVisible;
        mRoot.at(0)->findWidget("But_DoUniformSampling")->castType<MyGUI::Button>()->setVisible(isVisible);
        mRoot.at(0)->findWidget("Edit_SampleCount")->castType<MyGUI::EditBox>()->setVisible(isVisible);
        if (isVisible)
        {
            int meshVertexCount = 0;
            MeshShopApp* meshShop = dynamic_cast<MeshShopApp* >(AppManager::Get()->GetApp("MeshShopApp"));
            if (meshShop != NULL)
            {
                meshVertexCount = meshShop->GetMeshVertexCount();
            }
            std::stringstream ss;
            std::string textString;
            ss << meshVertexCount;
            ss >> textString;
            mRoot.at(0)->findWidget("Edit_SampleCount")->castType<MyGUI::EditBox>()->setOnlyText(textString);
        }
    }

    void MeshShopAppUI::DoUniformSampling(MyGUI::Widget* pSender)
    {
        MeshShopApp* meshShop = dynamic_cast<MeshShopApp* >(AppManager::Get()->GetApp("MeshShopApp"));
        if (meshShop != NULL)
        {
            int meshVertexCount = meshShop->GetMeshVertexCount();
            std::string textString = mRoot.at(0)->findWidget("Edit_SampleCount")->castType<MyGUI::EditBox>()->getOnlyText();
            int targetVertexCount = std::atoi(textString.c_str());
            if (targetVertexCount < 1)
            {
                std::stringstream ss;
                std::string textString;
                ss << meshVertexCount;
                ss >> textString;
                mRoot.at(0)->findWidget("Edit_SampleCount")->castType<MyGUI::EditBox>()->setOnlyText(textString);
            }
            else
            {
                meshShop->UniformSampleMesh(targetVertexCount);
            }            
        }
    }

    void MeshShopAppUI::ResetFillHole()
    {
        mRoot.at(0)->findWidget("But_FillHoleTriangulation")->castType<MyGUI::Button>()->setVisible(false);
        mRoot.at(0)->findWidget("But_FillHoleFlat")->castType<MyGUI::Button>()->setVisible(false);
        mRoot.at(0)->findWidget("But_FillHoleTangent")->castType<MyGUI::Button>()->setVisible(false);
        mRoot.at(0)->findWidget("But_FillHoleSmooth")->castType<MyGUI::Button>()->setVisible(false);
        mRoot.at(0)->findWidget("But_BridgeEdges")->castType<MyGUI::Button>()->setVisible(false);
    }

    void MeshShopAppUI::FillHole(MyGUI::Widget* pSender)
    {
        bool isVisible = mRoot.at(0)->findWidget("But_FillHoleFlat")->castType<MyGUI::Button>()->getVisible();
        isVisible = !isVisible;
        mRoot.at(0)->findWidget("But_FillHoleTriangulation")->castType<MyGUI::Button>()->setVisible(isVisible);
        mRoot.at(0)->findWidget("But_FillHoleFlat")->castType<MyGUI::Button>()->setVisible(isVisible);
        mRoot.at(0)->findWidget("But_FillHoleTangent")->castType<MyGUI::Button>()->setVisible(isVisible);
        mRoot.at(0)->findWidget("But_FillHoleSmooth")->castType<MyGUI::Button>()->setVisible(isVisible);
        mRoot.at(0)->findWidget("But_BridgeEdges")->castType<MyGUI::Button>()->setVisible(isVisible);
        MeshShopApp* meshShop = dynamic_cast<MeshShopApp* >(AppManager::Get()->GetApp("MeshShopApp"));
        if (meshShop != NULL)
        {
            meshShop->FindHole(isVisible);
        }
    }

    void MeshShopAppUI::DoFillHoleTriangulation(MyGUI::Widget* pSender)
    {
        MeshShopApp* meshShop = dynamic_cast<MeshShopApp* >(AppManager::Get()->GetApp("MeshShopApp"));
        if (meshShop != NULL)
        {
            meshShop->FillHole(3);
        }
    }

    void MeshShopAppUI::DoFillHoleFlat(MyGUI::Widget* pSender)
    {
        MeshShopApp* meshShop = dynamic_cast<MeshShopApp* >(AppManager::Get()->GetApp("MeshShopApp"));
        if (meshShop != NULL)
        {
            meshShop->FillHole(0);
        }
    }

    void MeshShopAppUI::DoFillHoleTangent(MyGUI::Widget* pSender)
    {
        MeshShopApp* meshShop = dynamic_cast<MeshShopApp* >(AppManager::Get()->GetApp("MeshShopApp"));
        if (meshShop != NULL)
        {
            meshShop->FillHole(1);
        }
    }

    void MeshShopAppUI::DoFillHoleSmooth(MyGUI::Widget* pSender)
    {
        MeshShopApp* meshShop = dynamic_cast<MeshShopApp* >(AppManager::Get()->GetApp("MeshShopApp"));
        if (meshShop != NULL)
        {
            meshShop->FillHole(2);
        }
    }

    void MeshShopAppUI::DoBridgeEdges(MyGUI::Widget* pSender)
    {
        MeshShopApp* meshShop = dynamic_cast<MeshShopApp* >(AppManager::Get()->GetApp("MeshShopApp"));
        if (meshShop != NULL)
        {
            meshShop->BridgeEdges();
        }
    }

    void MeshShopAppUI::OffsetMesh(MyGUI::Widget* pSender)
    {
        bool isVisible = mRoot.at(0)->findWidget("But_DoUniformOffset")->castType<MyGUI::Button>()->isVisible();
        isVisible = !isVisible;
        mRoot.at(0)->findWidget("But_DoUniformOffset")->castType<MyGUI::Button>()->setVisible(isVisible);
        mRoot.at(0)->findWidget("Edit_OffsetValue")->castType<MyGUI::EditBox>()->setVisible(isVisible);
        if (isVisible)
        {
            std::stringstream ss;
            std::string textString;
            ss << 0.025;
            ss >> textString;
            mRoot.at(0)->findWidget("Edit_OffsetValue")->castType<MyGUI::EditBox>()->setOnlyText(textString);
        }
    }

    void MeshShopAppUI::DoUniformOffset(MyGUI::Widget* pSender)
    {
        MeshShopApp* meshShop = dynamic_cast<MeshShopApp* >(AppManager::Get()->GetApp("MeshShopApp"));
        if (meshShop != NULL)
        {
            std::string textString = mRoot.at(0)->findWidget("Edit_OffsetValue")->castType<MyGUI::EditBox>()->getOnlyText();
            double offsetValue = std::atof(textString.c_str());
            meshShop->UniformOffsetMesh(offsetValue);         
        }
    }

    void MeshShopAppUI::SelectPoint(MyGUI::Widget* pSender)
    {
        bool isVisible = mRoot.at(0)->findWidget("But_SelectByRectangle")->castType<MyGUI::Button>()->isVisible();
        isVisible = !isVisible;
        mRoot.at(0)->findWidget("But_SelectByRectangle")->castType<MyGUI::Button>()->setVisible(isVisible);
        mRoot.at(0)->findWidget("But_EraseByRectangle")->castType<MyGUI::Button>()->setVisible(isVisible);
        mRoot.at(0)->findWidget("But_DeleteSelections")->castType<MyGUI::Button>()->setVisible(isVisible);
        mRoot.at(0)->findWidget("But_SimplifySelections")->castType<MyGUI::Button>()->setVisible(isVisible);
        mRoot.at(0)->findWidget("But_IgnoreBack")->castType<MyGUI::Button>()->setVisible(isVisible);
        mRoot.at(0)->findWidget("But_MoveModel")->castType<MyGUI::Button>()->setVisible(isVisible);
    }

    void MeshShopAppUI::SelectByRectangle(MyGUI::Widget* pSender)
    {
        MeshShopApp* meshShop = dynamic_cast<MeshShopApp* >(AppManager::Get()->GetApp("MeshShopApp"));
        if (meshShop != NULL)
        {
            meshShop->SelectByRectangle();
        }
    }

    void MeshShopAppUI::EraseByRectangle(MyGUI::Widget* pSender)
    {
        MeshShopApp* meshShop = dynamic_cast<MeshShopApp* >(AppManager::Get()->GetApp("MeshShopApp"));
        if (meshShop != NULL)
        {
            meshShop->EraseByRectangle();
        }
    }

    void MeshShopAppUI::DeleteSelections(MyGUI::Widget* pSender)
    {
        MeshShopApp* meshShop = dynamic_cast<MeshShopApp* >(AppManager::Get()->GetApp("MeshShopApp"));
        if (meshShop != NULL)
        {
            meshShop->DeleteSelections();
        }
    }

    void MeshShopAppUI::SimplifySelections(MyGUI::Widget* pSender)
    {
        MeshShopApp* meshShop = dynamic_cast<MeshShopApp* >(AppManager::Get()->GetApp("MeshShopApp"));
        if (meshShop != NULL)
        {
            meshShop->SimplifySelectedVertices(true);
        }
    }

    void MeshShopAppUI::IgnoreBack(MyGUI::Widget* pSender)
    {
        mIgnoreBack = !mIgnoreBack;
        if (mIgnoreBack)
        {
            mRoot.at(0)->findWidget("But_IgnoreBack")->castType<MyGUI::Button>()->changeWidgetSkin("But_Front");
        }
        else
        {
            mRoot.at(0)->findWidget("But_IgnoreBack")->castType<MyGUI::Button>()->changeWidgetSkin("But_Back");
        }
        MeshShopApp* meshShop = dynamic_cast<MeshShopApp* >(AppManager::Get()->GetApp("MeshShopApp"));
        if (meshShop != NULL)
        {
            meshShop->IgnoreBack(mIgnoreBack);
        }
    }

    void MeshShopAppUI::MoveModel(MyGUI::Widget* pSender)
    {
        MeshShopApp* meshShop = dynamic_cast<MeshShopApp* >(AppManager::Get()->GetApp("MeshShopApp"));
        if (meshShop != NULL)
        {
            meshShop->MoveModel();
        }
    }

    void MeshShopAppUI::CutMesh(MyGUI::Widget* pSender)
    {
        bool isVisible = mRoot.at(0)->findWidget("But_CutMesh_Plane_XY")->castType<MyGUI::Button>()->isVisible();
        isVisible = !isVisible;
        mRoot.at(0)->findWidget("Edit_PlaneOffsetValue")->castType<MyGUI::EditBox>()->setVisible(isVisible);
        mRoot.at(0)->findWidget("But_CutMesh_Plane_XY")->castType<MyGUI::Button>()->setVisible(isVisible);
        mRoot.at(0)->findWidget("But_CutMesh_Plane_YZ")->castType<MyGUI::Button>()->setVisible(isVisible);
        mRoot.at(0)->findWidget("But_CutMesh_Plane_ZX")->castType<MyGUI::Button>()->setVisible(isVisible);
        mRoot.at(0)->findWidget("But_CutMesh_Plane_Random")->castType<MyGUI::Button>()->setVisible(isVisible);
        if (isVisible)
        {
            std::stringstream ss;
            std::string textString;
            ss << 0;
            ss >> textString;
            mRoot.at(0)->findWidget("Edit_PlaneOffsetValue")->castType<MyGUI::EditBox>()->setOnlyText(textString);
        }
    }

    void MeshShopAppUI::CutMeshByXYPlane(MyGUI::Widget* pSender)
    {
        MeshShopApp* meshShop = dynamic_cast<MeshShopApp* >(AppManager::Get()->GetApp("MeshShopApp"));
        if (meshShop != NULL)
        {
            std::string textString = mRoot.at(0)->findWidget("Edit_PlaneOffsetValue")->castType<MyGUI::EditBox>()->getOnlyText();
            double offsetValue = std::atof(textString.c_str());
            if (offsetValue > 1.0 || offsetValue < -1.0)
            {
                std::stringstream ss;
                std::string textString;
                ss << 0;
                ss >> textString;
                mRoot.at(0)->findWidget("Edit_PlaneOffsetValue")->castType<MyGUI::EditBox>()->setOnlyText(textString);
            }
            else
            {
                meshShop->SplitMeshByPlane(MeshShopApp::ST_PLANE_XY, offsetValue);
            }
        }
    }

    void MeshShopAppUI::CutMeshByYZPlane(MyGUI::Widget* pSender)
    {
        MeshShopApp* meshShop = dynamic_cast<MeshShopApp* >(AppManager::Get()->GetApp("MeshShopApp"));
        if (meshShop != NULL)
        {
            std::string textString = mRoot.at(0)->findWidget("Edit_PlaneOffsetValue")->castType<MyGUI::EditBox>()->getOnlyText();
            double offsetValue = std::atof(textString.c_str());
            if (offsetValue > 1.0 || offsetValue < -1.0)
            {
                std::stringstream ss;
                std::string textString;
                ss << 0;
                ss >> textString;
                mRoot.at(0)->findWidget("Edit_PlaneOffsetValue")->castType<MyGUI::EditBox>()->setOnlyText(textString);
            }
            else
            {
                meshShop->SplitMeshByPlane(MeshShopApp::ST_PLANE_YZ, offsetValue);
            }
        }
    }

    void MeshShopAppUI::CutMeshByZXPlane(MyGUI::Widget* pSender)
    {
        MeshShopApp* meshShop = dynamic_cast<MeshShopApp* >(AppManager::Get()->GetApp("MeshShopApp"));
        if (meshShop != NULL)
        {
            std::string textString = mRoot.at(0)->findWidget("Edit_PlaneOffsetValue")->castType<MyGUI::EditBox>()->getOnlyText();
            double offsetValue = std::atof(textString.c_str());
            if (offsetValue > 1.0 || offsetValue < -1.0)
            {
                std::stringstream ss;
                std::string textString;
                ss << 0;
                ss >> textString;
                mRoot.at(0)->findWidget("Edit_PlaneOffsetValue")->castType<MyGUI::EditBox>()->setOnlyText(textString);
            }
            else
            {
                meshShop->SplitMeshByPlane(MeshShopApp::ST_PLANE_ZX, offsetValue);
            }
        }
    }

    void MeshShopAppUI::CutMeshByRandomPlane(MyGUI::Widget* pSender)
    {
        MeshShopApp* meshShop = dynamic_cast<MeshShopApp* >(AppManager::Get()->GetApp("MeshShopApp"));
        if (meshShop != NULL)
        {
            std::string textString = mRoot.at(0)->findWidget("Edit_PlaneOffsetValue")->castType<MyGUI::EditBox>()->getOnlyText();
            double offsetValue = std::atof(textString.c_str());
            if (offsetValue > 1.0 || offsetValue < -1.0)
            {
                std::stringstream ss;
                std::string textString;
                ss << 0;
                ss >> textString;
                mRoot.at(0)->findWidget("Edit_PlaneOffsetValue")->castType<MyGUI::EditBox>()->setOnlyText(textString);
            }
            else
            {
                meshShop->SplitMeshByPlane(MeshShopApp::ST_PLANE_RANDOM, offsetValue);
            }
        }
    }

    void MeshShopAppUI::BackToHomepage(MyGUI::Widget* pSender)
    {
        MeshShopApp* meshShop = dynamic_cast<MeshShopApp* >(AppManager::Get()->GetApp("MeshShopApp"));
        if (meshShop != NULL)
        {
            if (meshShop->IsCommandInProgress())
            {
                return;
            }
            AppManager::Get()->SwitchCurrentApp("Homepage");
        }
    }
}

#include "MeasureAppUI.h"
#include "../Common/ResourceManager.h"
#include "../Common/ToolKit.h"
#include "../Common/RenderSystem.h"
#include "AppManager.h"
#include "MeasureApp.h"

namespace MagicApp
{
    MeasureAppUI::MeasureAppUI() :
        mIsProgressbarVisible(false),
        mTextInfo(NULL),
        mVertexCount(0),
        mTriangleCount(0),
        mArea(0),
        mVolume(0),
        mGeodesicsDistance(0),
        mRefMeshFaceCount(0),
        mIsShowDistance(false),
        mMinDistance(0.0),
        mMaxDistance(0.0),
        mIsShowThickness(false),
        mMedianThickness(0.0),
        mOptionPlane(true),
        mOptionCone(true),
        mOptionSphere(true),
        mOptionCylinder(true)
    {
    }

    MeasureAppUI::~MeasureAppUI()
    {
    }

    void MeasureAppUI::Setup()
    {
        MagicCore::ResourceManager::LoadResource("../../Media/MeasureApp", "FileSystem", "MeasureApp");
        mRoot = MyGUI::LayoutManager::getInstance().loadLayout("MeasureApp.layout");
        mRoot.at(0)->findWidget("But_DisplayMode")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeasureAppUI::SwitchDisplayMode);
        mRoot.at(0)->findWidget("But_SelectMode")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeasureAppUI::SwitchSelectMode);
        
        mRoot.at(0)->findWidget("But_ImportModelRef")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeasureAppUI::ImportModel);

        mRoot.at(0)->findWidget("But_Geodesics")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeasureAppUI::Geodesics);
        mRoot.at(0)->findWidget("But_DeleteMeshRef")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeasureAppUI::DeleteMeshMark);
        mRoot.at(0)->findWidget("But_ApproximateGeodesics")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeasureAppUI::ComputeApproximateGeodesics);
        mRoot.at(0)->findWidget("But_QuickExactGeodesics")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeasureAppUI::FastComputeExactGeodesics);
        mRoot.at(0)->findWidget("But_ExactGeodesics")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeasureAppUI::ComputeExactGeodesics);
        mRoot.at(0)->findWidget("But_CurvatureGeodesics")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeasureAppUI::ComputeCurvatureGeodesics);
        mRoot.at(0)->findWidget("But_SmoothGeodesicsOnVertex")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeasureAppUI::SmoothGeodesicsOnVertex);

        mRoot.at(0)->findWidget("But_SectionCurve")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeasureAppUI::SectionCurve);
        mRoot.at(0)->findWidget("But_FacePointCurve")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeasureAppUI::FacePointCurve);
        mRoot.at(0)->findWidget("But_Cut")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeasureAppUI::SplitMesh);

        mRoot.at(0)->findWidget("But_MeasureRef")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeasureAppUI::MeasureModel);
        mRoot.at(0)->findWidget("But_AreaRef")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeasureAppUI::MeasureArea);
        mRoot.at(0)->findWidget("But_VolumeRef")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeasureAppUI::MeasureVolume);
        mRoot.at(0)->findWidget("But_MeanCurvature")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeasureAppUI::MeasureMeanCurvature);
        mRoot.at(0)->findWidget("But_GaussianCurvature")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeasureAppUI::MeasureGaussianCurvature);
        mRoot.at(0)->findWidget("But_PrincipalCurvature")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeasureAppUI::MeasurePrincipalCurvature);
        mRoot.at(0)->findWidget("But_Thickness")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeasureAppUI::MeasureThickness);

        mRoot.at(0)->findWidget("But_PointsToMeshDist")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeasureAppUI::PointsToMeshDistance);
        mRoot.at(0)->findWidget("But_ImportMeasureData")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeasureAppUI::ImportRefModel);
        mRoot.at(0)->findWidget("But_ComputeDistance")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeasureAppUI::ComputePointsToMeshDistance);

        mRoot.at(0)->findWidget("But_DetectPrimitive")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeasureAppUI::DetectPrimitive);
        mRoot.at(0)->findWidget("CB_Plane")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeasureAppUI::DetectOptionPlane);
        mRoot.at(0)->findWidget("CB_Cone")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeasureAppUI::DetectOptionCone);
        mRoot.at(0)->findWidget("CB_Sphere")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeasureAppUI::DetectOptionSphere);
        mRoot.at(0)->findWidget("CB_Cylinder")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeasureAppUI::DetectOptionCylinder);
        mRoot.at(0)->findWidget("But_DoDetectPrimitive")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeasureAppUI::DoDetectPrimitive);
        mRoot.at(0)->findWidget("But_SelectPrimitive")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeasureAppUI::SelectPrimitive);

        mRoot.at(0)->findWidget("But_BackToHomepage")->castType<MyGUI::Button>()->eventMouseButtonClick += MyGUI::newDelegate(this, &MeasureAppUI::BackToHomepage);

        mTextInfo = mRoot.at(0)->findWidget("Text_Info")->castType<MyGUI::TextBox>();
        mTextInfo->setTextColour(MyGUI::Colour(75.0 / 255.0, 131.0 / 255.0, 128.0 / 255.0));

        MyGUI::Button* cbClosed = mRoot.at(0)->findWidget("CB_Closed")->castType<MyGUI::Button>();
        cbClosed->eventMouseButtonClick += MyGUI::newDelegate(this, &MeasureAppUI::UpdateClosedInfo);
        MeasureApp* measureShop = dynamic_cast<MeasureApp* >(AppManager::Get()->GetApp("MeasureApp"));
        if (measureShop != NULL)
        {
            cbClosed->setStateCheck(measureShop->IsGeodesicClose());
        }
    }

    void MeasureAppUI::Shutdown()
    {
        MyGUI::LayoutManager::getInstance().unloadLayout(mRoot);
        mRoot.clear();
        MagicCore::ResourceManager::UnloadResource("MeasureApp");
    }

    void MeasureAppUI::StartProgressbar(int range)
    {
        mRoot.at(0)->findWidget("APIProgress")->castType<MyGUI::ProgressBar>()->setVisible(true);
        mRoot.at(0)->findWidget("APIProgress")->castType<MyGUI::ProgressBar>()->setProgressRange(range);
        mRoot.at(0)->findWidget("APIProgress")->castType<MyGUI::ProgressBar>()->setProgressPosition(0);
        mIsProgressbarVisible = true;
    }

    void MeasureAppUI::SetProgressbar(int value)
    {
        mRoot.at(0)->findWidget("APIProgress")->castType<MyGUI::ProgressBar>()->setProgressPosition(value);
    }

    void MeasureAppUI::StopProgressbar()
    {
        mRoot.at(0)->findWidget("APIProgress")->castType<MyGUI::ProgressBar>()->setVisible(false);
        mIsProgressbarVisible = false;
    }

    bool MeasureAppUI::IsProgressbarVisible()
    {
        return mIsProgressbarVisible;
    }

    void MeasureAppUI::SetModelInfo(int vertexCount, int triangleCount)
    {
        mVertexCount = vertexCount;
        mTriangleCount = triangleCount;
        UpdateTextInfo();
    }

    void MeasureAppUI::SetDistanceInfo(int refFaceCount, bool bCalculted, double minDist, double maxDist)
    {
        mIsShowDistance = bCalculted;
        mRefMeshFaceCount = refFaceCount;
        mMinDistance = minDist;
        mMaxDistance = maxDist;
        if (mIsShowDistance)
        {
            mIsShowThickness = false;
        }
        UpdateTextInfo();
    }

    void MeasureAppUI::SetThicknessInfo(bool bShow, double medianThickness)
    {
        mIsShowThickness = bShow;
        mMedianThickness = medianThickness;
        if (mIsShowThickness)
        {
            mIsShowDistance = false;
        }
        UpdateTextInfo();
    }

    void MeasureAppUI::SetModelArea(double area)
    {
        mArea = area;
        UpdateTextInfo();
    }

    void MeasureAppUI::SetModelVolume(double volume)
    {
        mVolume = volume;
        UpdateTextInfo();
    }

    void MeasureAppUI::SetGeodesicsInfo(double distance)
    {
        mGeodesicsDistance = distance;
        UpdateTextInfo();
    }

    void MeasureAppUI::SwitchDisplayMode(MyGUI::Widget* pSender)
    {
        MeasureApp* measureShop = dynamic_cast<MeasureApp* >(AppManager::Get()->GetApp("MeasureApp"));
        if (measureShop != NULL)
        {
            measureShop->SwitchDisplayMode();
        }
    }

    void MeasureAppUI::SwitchSelectMode(MyGUI::Widget* pSender)
    {
        MeasureApp* measureShop = dynamic_cast<MeasureApp* >(AppManager::Get()->GetApp("MeasureApp"));
        if (measureShop != NULL)
        {
            measureShop->SwitchSelectionMode();
        }
    }

    void MeasureAppUI::ImportModel(MyGUI::Widget* pSender)
    {
        MeasureApp* measureShop = dynamic_cast<MeasureApp* >(AppManager::Get()->GetApp("MeasureApp"));
        if (measureShop != NULL)
        {
            measureShop->ImportModel();
        }
    }

    void MeasureAppUI::Geodesics(MyGUI::Widget* pSender)
    {
        bool isVisible = mRoot.at(0)->findWidget("But_DeleteMeshRef")->castType<MyGUI::Button>()->isVisible();
        isVisible = !isVisible;
        mRoot.at(0)->findWidget("But_DeleteMeshRef")->castType<MyGUI::Button>()->setVisible(isVisible);
        mRoot.at(0)->findWidget("But_ApproximateGeodesics")->castType<MyGUI::Button>()->setVisible(isVisible);
        mRoot.at(0)->findWidget("But_QuickExactGeodesics")->castType<MyGUI::Button>()->setVisible(isVisible);
        mRoot.at(0)->findWidget("But_ExactGeodesics")->castType<MyGUI::Button>()->setVisible(isVisible);
        mRoot.at(0)->findWidget("But_CurvatureGeodesics")->castType<MyGUI::Button>()->setVisible(isVisible);
        mRoot.at(0)->findWidget("Edit_GeodesicAccuracy")->castType<MyGUI::EditBox>()->setVisible(isVisible);
        mRoot.at(0)->findWidget("But_SmoothGeodesicsOnVertex")->castType<MyGUI::Button>()->setVisible(isVisible);
        if (isVisible)
        {
            std::string textString = "0.5";
            mRoot.at(0)->findWidget("Edit_GeodesicAccuracy")->castType<MyGUI::EditBox>()->setOnlyText(textString);
            mRoot.at(0)->findWidget("Edit_GeodesicAccuracy")->castType<MyGUI::EditBox>()->setTextSelectionColour(MyGUI::Colour::Black);
        }
        mRoot.at(0)->findWidget("Edit_CurvtureWeight")->castType<MyGUI::EditBox>()->setVisible(isVisible);
        if (isVisible)
        {
            std::string textString = "0.25";
            mRoot.at(0)->findWidget("Edit_CurvtureWeight")->castType<MyGUI::EditBox>()->setOnlyText(textString);
            mRoot.at(0)->findWidget("Edit_CurvtureWeight")->castType<MyGUI::EditBox>()->setTextSelectionColour(MyGUI::Colour::Black);
        }
    }

    void MeasureAppUI::DeleteMeshMark(MyGUI::Widget* pSender)
    {
        MeasureApp* measureShop = dynamic_cast<MeasureApp* >(AppManager::Get()->GetApp("MeasureApp"));
        if (measureShop != NULL)
        {
            measureShop->DeleteMeshMark();
        }
    }

    void MeasureAppUI::ComputeApproximateGeodesics(MyGUI::Widget* pSender)
    {
        MeasureApp* measureShop = dynamic_cast<MeasureApp* >(AppManager::Get()->GetApp("MeasureApp"));
        if (measureShop != NULL)
        {
            measureShop->ComputeApproximateGeodesics();
        }
    }

    void MeasureAppUI::FastComputeExactGeodesics(MyGUI::Widget* pSender)
    {
        MeasureApp* measureShop = dynamic_cast<MeasureApp* >(AppManager::Get()->GetApp("MeasureApp"));
        if (measureShop != NULL)
        {
            std::string textString = mRoot.at(0)->findWidget("Edit_GeodesicAccuracy")->castType<MyGUI::EditBox>()->getOnlyText();
            double accuary = std::atof(textString.c_str());
            if (accuary <= 0 || accuary > 1)
            {
                std::string newStr = "0.5";
                mRoot.at(0)->findWidget("Edit_GeodesicAccuracy")->castType<MyGUI::EditBox>()->setOnlyText(newStr);
                return;
            }
            measureShop->FastComputeExactGeodesics(accuary);
        }
    }

    void MeasureAppUI::ComputeExactGeodesics(MyGUI::Widget* pSender)
    {
        MeasureApp* measureShop = dynamic_cast<MeasureApp* >(AppManager::Get()->GetApp("MeasureApp"));
        if (measureShop != NULL)
        {
            measureShop->ComputeExactGeodesics();
        }
    }

    void MeasureAppUI::ComputeCurvatureGeodesics(MyGUI::Widget* pSender)
    {
        MeasureApp* measureShop = dynamic_cast<MeasureApp* >(AppManager::Get()->GetApp("MeasureApp"));
        if (measureShop != NULL)
        {
            std::string textString = mRoot.at(0)->findWidget("Edit_CurvtureWeight")->castType<MyGUI::EditBox>()->getOnlyText();
            double weight = std::atof(textString.c_str());
            if (weight < 0)
            {
                std::string newStr = "0.25";
                mRoot.at(0)->findWidget("Edit_CurvtureWeight")->castType<MyGUI::EditBox>()->setOnlyText(newStr);
                return;
            }
            measureShop->ComputeCurvatureGeodesics(weight, true);
        }
    }

    void MeasureAppUI::SmoothGeodesicsOnVertex(MyGUI::Widget* pSender)
    {
        MeasureApp* measureShop = dynamic_cast<MeasureApp* >(AppManager::Get()->GetApp("MeasureApp"));
        if (measureShop != NULL)
        {
            measureShop->SmoothGeodesicsOnVertex();
        }
    }

    void MeasureAppUI::SectionCurve(MyGUI::Widget* pSender)
    {
        MeasureApp* measureShop = dynamic_cast<MeasureApp* >(AppManager::Get()->GetApp("MeasureApp"));
        if (measureShop != NULL)
        {
            measureShop->ComputeSectionCurve();
        }
    }

    void MeasureAppUI::FacePointCurve(MyGUI::Widget* pSender)
    {
        MeasureApp* measureShop = dynamic_cast<MeasureApp* >(AppManager::Get()->GetApp("MeasureApp"));
        if (measureShop != NULL)
        {
            measureShop->ComputeFacePointCurve();
        }
    }

    void MeasureAppUI::SplitMesh(MyGUI::Widget* pSender)
    {
        MeasureApp* measureShop = dynamic_cast<MeasureApp* >(AppManager::Get()->GetApp("MeasureApp"));
        if (measureShop != NULL)
        {
            measureShop->SplitMesh();
        }
    }

    void MeasureAppUI::MeasureModel(MyGUI::Widget* pSender)
    {
        bool isVisible = mRoot.at(0)->findWidget("But_AreaRef")->castType<MyGUI::Button>()->isVisible();
        mRoot.at(0)->findWidget("But_AreaRef")->castType<MyGUI::Button>()->setVisible(!isVisible);
        mRoot.at(0)->findWidget("But_VolumeRef")->castType<MyGUI::Button>()->setVisible(!isVisible);
        mRoot.at(0)->findWidget("But_MeanCurvature")->castType<MyGUI::Button>()->setVisible(!isVisible);
        mRoot.at(0)->findWidget("But_GaussianCurvature")->castType<MyGUI::Button>()->setVisible(!isVisible);
        mRoot.at(0)->findWidget("But_PrincipalCurvature")->castType<MyGUI::Button>()->setVisible(!isVisible);
        mRoot.at(0)->findWidget("But_Thickness")->castType<MyGUI::Button>()->setVisible(!isVisible);
    }

    void MeasureAppUI::MeasureArea(MyGUI::Widget* pSender)
    {
        MeasureApp* measureShop = dynamic_cast<MeasureApp* >(AppManager::Get()->GetApp("MeasureApp"));
        if (measureShop != NULL)
        {
            measureShop->MeasureArea();
        }
    }

    void MeasureAppUI::MeasureVolume(MyGUI::Widget* pSender)
    {
        MeasureApp* measureShop = dynamic_cast<MeasureApp* >(AppManager::Get()->GetApp("MeasureApp"));
        if (measureShop != NULL)
        {
            measureShop->MeasureVolume();
        }
    }

    void MeasureAppUI::MeasureMeanCurvature(MyGUI::Widget* pSender)
    {
        MeasureApp* measureShop = dynamic_cast<MeasureApp* >(AppManager::Get()->GetApp("MeasureApp"));
        if (measureShop != NULL)
        {
            measureShop->MeasureMeanCurvature();
        }
    }

    void MeasureAppUI::MeasureGaussianCurvature(MyGUI::Widget* pSender)
    {
        MeasureApp* measureShop = dynamic_cast<MeasureApp* >(AppManager::Get()->GetApp("MeasureApp"));
        if (measureShop != NULL)
        {
            measureShop->MeasureGaussianCurvature();
        }
    }

    void MeasureAppUI::MeasurePrincipalCurvature(MyGUI::Widget* pSender)
    {
        MeasureApp* measureShop = dynamic_cast<MeasureApp* >(AppManager::Get()->GetApp("MeasureApp"));
        if (measureShop != NULL)
        {
            measureShop->MeasurePrincipalCurvature(true);
        }
    }

    void MeasureAppUI::MeasureThickness(MyGUI::Widget* pSender)
    {
        MeasureApp* measureShop = dynamic_cast<MeasureApp* >(AppManager::Get()->GetApp("MeasureApp"));
        if (measureShop != NULL)
        {
            measureShop->MeasureThickness(true);
        }
    }

    void MeasureAppUI::DetectPrimitive(MyGUI::Widget* pSender)
    {
        bool isVisible = mRoot.at(0)->findWidget("But_DoDetectPrimitive")->castType<MyGUI::Button>()->isVisible();
        isVisible = !isVisible;
        mRoot.at(0)->findWidget("CB_Plane")->castType<MyGUI::Button>()->setVisible(isVisible);
        mRoot.at(0)->findWidget("CB_Cone")->castType<MyGUI::Button>()->setVisible(isVisible);
        mRoot.at(0)->findWidget("CB_Sphere")->castType<MyGUI::Button>()->setVisible(isVisible);
        mRoot.at(0)->findWidget("CB_Cylinder")->castType<MyGUI::Button>()->setVisible(isVisible);
        mRoot.at(0)->findWidget("But_DoDetectPrimitive")->castType<MyGUI::Button>()->setVisible(isVisible);
        mRoot.at(0)->findWidget("But_SelectPrimitive")->castType<MyGUI::Button>()->setVisible(isVisible);
        if (isVisible)
        {
            mOptionPlane = true;
            mOptionCone = true;
            mOptionSphere = true;
            mOptionCylinder = true;
            mRoot.at(0)->findWidget("CB_Plane")->castType<MyGUI::Button>()->setStateCheck(true);
            mRoot.at(0)->findWidget("CB_Cone")->castType<MyGUI::Button>()->setStateCheck(true);
            mRoot.at(0)->findWidget("CB_Sphere")->castType<MyGUI::Button>()->setStateCheck(true);
            mRoot.at(0)->findWidget("CB_Cylinder")->castType<MyGUI::Button>()->setStateCheck(true);
        }
    }

    void MeasureAppUI::DetectOptionPlane(MyGUI::Widget* pSender)
    {
        mOptionPlane = !mOptionPlane;
        mRoot.at(0)->findWidget("CB_Plane")->castType<MyGUI::Button>()->setStateCheck(mOptionPlane);
        MeasureApp* pointShop = dynamic_cast<MeasureApp* >(AppManager::Get()->GetApp("MeasureApp"));
        if (pointShop != NULL)
        {
            pointShop->SetDetectOptions(mOptionPlane, mOptionCone, mOptionSphere, mOptionCylinder);
        }
    }

    void MeasureAppUI::DetectOptionCone(MyGUI::Widget* pSender)
    {
        mOptionCone = !mOptionCone;
        mRoot.at(0)->findWidget("CB_Cone")->castType<MyGUI::Button>()->setStateCheck(mOptionCone);
        MeasureApp* measureApp = dynamic_cast<MeasureApp* >(AppManager::Get()->GetApp("MeasureApp"));
        if (measureApp != NULL)
        {
            measureApp->SetDetectOptions(mOptionPlane, mOptionCone, mOptionSphere, mOptionCylinder);
        }
    }

    void MeasureAppUI::DetectOptionSphere(MyGUI::Widget* pSender)
    {
        mOptionSphere = !mOptionSphere;
        mRoot.at(0)->findWidget("CB_Sphere")->castType<MyGUI::Button>()->setStateCheck(mOptionSphere);
        MeasureApp* measureApp = dynamic_cast<MeasureApp* >(AppManager::Get()->GetApp("MeasureApp"));
        if (measureApp != NULL)
        {
            measureApp->SetDetectOptions(mOptionPlane, mOptionCone, mOptionSphere, mOptionCylinder);
        }
    }

    void MeasureAppUI::DetectOptionCylinder(MyGUI::Widget* pSender)
    {
        mOptionCylinder = !mOptionCylinder;
        mRoot.at(0)->findWidget("CB_Cylinder")->castType<MyGUI::Button>()->setStateCheck(mOptionCylinder);
        MeasureApp* measureApp = dynamic_cast<MeasureApp* >(AppManager::Get()->GetApp("MeasureApp"));
        if (measureApp != NULL)
        {
            measureApp->SetDetectOptions(mOptionPlane, mOptionCone, mOptionSphere, mOptionCylinder);
        }
    }

    void MeasureAppUI::DoDetectPrimitive(MyGUI::Widget* pSender)
    {
        MeasureApp* measureApp = dynamic_cast<MeasureApp* >(AppManager::Get()->GetApp("MeasureApp"));
        if (measureApp != NULL)
        {
            measureApp->SetDetectOptions(mOptionPlane, mOptionCone, mOptionSphere, mOptionCylinder);
            measureApp->DetectPrimitive(true);
        }
    }

    void MeasureAppUI::SelectPrimitive(MyGUI::Widget* pSender)
    {
        MeasureApp* measureApp = dynamic_cast<MeasureApp* >(AppManager::Get()->GetApp("MeasureApp"));
        if (measureApp != NULL)
        {
            measureApp->SetDetectOptions(mOptionPlane, mOptionCone, mOptionSphere, mOptionCylinder);
            measureApp->SetSelectPrimitiveMode();
        }
    }

    void MeasureAppUI::PointsToMeshDistance(MyGUI::Widget*)
    {
        bool isVisible = mRoot.at(0)->findWidget("But_ImportMeasureData")->castType<MyGUI::Button>()->isVisible();
        mRoot.at(0)->findWidget("But_ImportMeasureData")->castType<MyGUI::Button>()->setVisible(!isVisible);
        mRoot.at(0)->findWidget("But_ComputeDistance")->castType<MyGUI::Button>()->setVisible(!isVisible);
        MeasureApp* measureShop = dynamic_cast<MeasureApp* >(AppManager::Get()->GetApp("MeasureApp"));
        if (measureShop)
        {
            measureShop->ShowReferenceMesh(!isVisible);
        }
    }
    
    void MeasureAppUI::ImportRefModel(MyGUI::Widget* pSender)
    {
        MeasureApp* measureShop = dynamic_cast<MeasureApp* >(AppManager::Get()->GetApp("MeasureApp"));
        if (measureShop != NULL)
        {
            measureShop->ImportRefModel();
        }
    }

    void MeasureAppUI::ComputePointsToMeshDistance(MyGUI::Widget* pSender)
    {
        MeasureApp* measureShop = dynamic_cast<MeasureApp* >(AppManager::Get()->GetApp("MeasureApp"));
        if (measureShop != NULL)
        {
            measureShop->ComputePointsToMeshDistance();
        }
    }

    void MeasureAppUI::BackToHomepage(MyGUI::Widget* pSender)
    {
        MeasureApp* measureShop = dynamic_cast<MeasureApp* >(AppManager::Get()->GetApp("MeasureApp"));
        if (measureShop != NULL)
        {
            if (measureShop->IsCommandInProgress())
            {
                return;
            }
            AppManager::Get()->SwitchCurrentApp("Homepage");
        }
    }

    void MeasureAppUI::UpdateTextInfo()
    {
        std::string textString = "";
        if (mVertexCount > 0)
        {
            textString += "Vertex count = ";
            std::stringstream ss;
            ss << mVertexCount;
            std::string numberString;
            ss >> numberString;
            textString += numberString;
            textString += " ";
            if (mTriangleCount > 0)
            {
                textString += " Triangle count = ";
                std::stringstream ss;
                ss << mTriangleCount;
                std::string numberString;
                ss >> numberString;
                textString += numberString;
                textString += "\n";
            }
            else
            {
                textString += "\n";
            }
        }

        if (mGeodesicsDistance > 0)
        {
            textString += "Geodesics distance = ";
            std::stringstream ss;
            ss << mGeodesicsDistance;
            std::string numberString;
            ss >> numberString;
            textString += numberString;
            textString += "\n";
        }

        if (mArea > 0)
        {
            textString += "Area = ";
            std::stringstream ss;
            ss << mArea;
            std::string numberString;
            ss >> numberString;
            textString += numberString;
            textString += "\n";
        }

        if (mVolume > 0)
        {
            textString += "Volume = ";
            std::stringstream ss;
            ss << mVolume;
            std::string numberString;
            ss >> numberString;
            textString += numberString;
            textString += "\n";
        }

        if (mRefMeshFaceCount > 0)
        {
            textString += "ReferenceMesh Face count = ";
            std::stringstream ss;
            ss << mRefMeshFaceCount;
            std::string numberPointCount;
            ss >> numberPointCount;
            textString += numberPointCount;
            textString += "\n";
        }

        if (mIsShowDistance)
        {
            textString += "Maximum distance = ";
            std::stringstream ss;
            ss << mMaxDistance;
            std::string distanceString;
            ss >> distanceString;
            textString += distanceString;
            textString += "\n";
            textString += "Minimum distance = ";
            std::stringstream ss2;
            ss2 << mMinDistance;
            std::string distanceString2;
            ss2 >> distanceString2;
            textString += distanceString2;
            textString += "\n";
        }

        if (mIsShowThickness)
        {
            textString += "Median thickness = ";
            std::stringstream ss;
            ss << mMedianThickness;
            std::string thicknessString;
            ss >> thicknessString;
            textString += thicknessString;
            textString += "\n";
        }

        mTextInfo->setCaption(textString);
    }

    void MeasureAppUI::UpdateIsClosedInfo()
    {
        UpdateClosedInfo(NULL);
    }

    void MeasureAppUI::UpdateClosedInfo(MyGUI::Widget* pSender)
    {
        MyGUI::Button* closedCB = mRoot.at(0)->findWidget("CB_Closed")->castType<MyGUI::Button>();
        MeasureApp* measureShop = dynamic_cast<MeasureApp* >(AppManager::Get()->GetApp("MeasureApp"));
        if (measureShop != NULL)
        {
            measureShop->SwitchGeodesicClose();
            bool isClosed = measureShop->IsGeodesicClose();
            closedCB->setStateCheck(isClosed);
        }
    }
}

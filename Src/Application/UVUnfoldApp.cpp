#include "stdafx.h"
#include <process.h>
#include "UVUnfoldApp.h"
#include "UVUnfoldAppUI.h"
#include "AppManager.h"
#include "ModelManager.h"
#include "../Common/LogSystem.h"
#include "../Common/ToolKit.h"
#include "../Common/ViewTool.h"
#include "../Common/PickTool.h"
#include "../Common/RenderSystem.h"
#include "GPP.h"

namespace MagicApp
{
    static unsigned __stdcall RunThread(void *arg)
    {
        UVUnfoldApp* app = (UVUnfoldApp*)arg;
        if (app == NULL)
        {
            return 0;
        }
        app->DoCommand(false);
        return 1;
    }

    UVUnfoldApp::UVUnfoldApp() :
        mpUI(NULL),
        mpImageFrameMesh(NULL),
        mDistortionImage(),
        mpTriMeshTexture(NULL),
        mpViewTool(NULL),
        mDisplayMode(TRIMESH_SOLID),
        mCommandType(NONE),
        mIsCommandInProgress(false),
        mUpdateDisplay(false),
        mHideMarks(false),
        mpPickTool(NULL),
        mLastCutVertexId(-1),
        mCurPointsOnVertex(),
        mCurPointsOnEdge(),
        mCurMarkCoords(),
        mCutLineList(),
        mInitChartCount(1),
        mSnapIds(),
        mTargetVertexCount(0),
        mIsCutLineAccurate(false)
    {
    }

    UVUnfoldApp::~UVUnfoldApp()
    {
        GPPFREEPOINTER(mpUI);
        GPPFREEPOINTER(mpImageFrameMesh);
        GPPFREEPOINTER(mpViewTool);
        GPPFREEPOINTER(mpPickTool);
    }

    void UVUnfoldApp::ClearData()
    {
        GPPFREEPOINTER(mpUI);
        GPPFREEPOINTER(mpViewTool);
        GPPFREEPOINTER(mpImageFrameMesh);
        GPPFREEPOINTER(mpPickTool);
        mDisplayMode = TRIMESH_SOLID;
        mDistortionImage.release();
        ClearSplitData();
    }

    void UVUnfoldApp::ClearSplitData()
    {
        mLastCutVertexId = -1;
        mCurPointsOnVertex.clear();
        mCurPointsOnEdge.clear();
        mCurMarkCoords.clear();
        mCutLineList.clear();
        mSnapIds.clear();
        mHideMarks = false;
    }

    bool UVUnfoldApp::IsCommandAvaliable()
    {
        if (mIsCommandInProgress)
        {
            MessageBox(NULL, "请等待当前命令执行完", "温馨提示", MB_OK);
            return false;
        }
        return true;
    }

#if DEBUGDUMPFILE
    void UVUnfoldApp::SetDumpInfo(GPP::DumpBase* dumpInfo)
    {
    }

    void UVUnfoldApp::RunDumpInfo()
    {
    }
#endif

    bool UVUnfoldApp::Enter()
    {
        InfoLog << "Enter UVUnfoldApp" << std::endl; 
        if (mpUI == NULL)
        {
            mpUI = new UVUnfoldAppUI;
        }
        mpUI->Setup();
        SetupScene();
        UpdateDisplay();
        return true;
    }

    bool UVUnfoldApp::Update(double timeElapsed)
    {
        if (mpUI && mpUI->IsProgressbarVisible())
        {
            int progressValue = int(GPP::GetApiProgress() * 100.0);
            mpUI->SetProgressbar(progressValue);
        }
        if (mUpdateDisplay)
        {
            mUpdateDisplay = false;
            UpdateDisplay();
            UpdateMarkDisplay();
            InfoLog << "Update::UpdateDisplay" << std::endl;
        }
        return true;
    }

    bool UVUnfoldApp::Exit()
    {
        InfoLog << "Exit UVUnfoldApp" << std::endl; 
        ShutdownScene();
        if (mpUI != NULL)
        {
            mpUI->Shutdown();
        }
        ClearData();
        return true;
    }

    bool UVUnfoldApp::MouseMoved( const OIS::MouseEvent &arg )
    {
        if (arg.state.buttonDown(OIS::MB_Middle) && mpViewTool != NULL)
        {
            mpViewTool->MouseMoved(arg.state.X.abs, arg.state.Y.abs, MagicCore::ViewTool::MM_MIDDLE_DOWN);
        }
        else if (arg.state.buttonDown(OIS::MB_Left) && arg.state.buttonDown(OIS::MB_Right) && mpViewTool != NULL)
        {
            mpViewTool->MouseMoved(arg.state.X.abs, arg.state.Y.abs, MagicCore::ViewTool::MM_RIGHT_DOWN);
        }
        else if (arg.state.buttonDown(OIS::MB_Left) && mpViewTool != NULL)
        {
            mpViewTool->MouseMoved(arg.state.X.abs, arg.state.Y.abs, MagicCore::ViewTool::MM_LEFT_DOWN);
        }   
        
        return true;
    }

    bool UVUnfoldApp::MousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
    {
        if ((id == OIS::MB_Middle || id == OIS::MB_Left) && mpViewTool != NULL)
        {
            mpViewTool->MousePressed(arg.state.X.abs, arg.state.Y.abs);
        }
        else if (!arg.state.buttonDown(OIS::MB_Left) && arg.state.buttonDown(OIS::MB_Right) && mIsCommandInProgress == false && mpPickTool)
        {
            mpPickTool->MousePressed(arg.state.X.abs, arg.state.Y.abs);
        }
        return true;
    }

    bool UVUnfoldApp::MouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
    {
        if (mpViewTool)
        {
            mpViewTool->MouseReleased();
        }
        if (!arg.state.buttonDown(OIS::MB_Left) && mIsCommandInProgress == false && id == OIS::MB_Right && mpPickTool)
        {
            mpPickTool->MouseReleased(arg.state.X.abs, arg.state.Y.abs);
            GPP::Int pickedId = mpPickTool->GetPickVertexId();;
            mpPickTool->ClearPickedIds();
            if (pickedId != -1)
            {
                InfoLog << "pickId " << pickedId << std::endl;
                if (mLastCutVertexId == -1)
                {
                    mLastCutVertexId = pickedId;
                }
                else
                {
                    std::vector<GPP::Int> sectionVertexIds;
                    sectionVertexIds.push_back(mLastCutVertexId);
                    sectionVertexIds.push_back(pickedId);
                    if (mIsCutLineAccurate)
                    {
                        std::vector<GPP::Vector3> pathCoords;
                        std::vector<GPP::PointOnEdge> pathInfos;
                        GPP::Real distance = 0;
                        GPP::ErrorCode res = GPP::MeasureMesh::FastComputeExactGeodesics(ModelManager::Get()->GetMesh(), 
                            sectionVertexIds, false, pathCoords, distance, &pathInfos, 0.5);
                        if (res != GPP_NO_ERROR)
                        {
                            MessageBox(NULL, "测量失败", "温馨提示", MB_OK);
                            return true;
                        }
                        int startId = 1;
                        if (mCurPointsOnEdge.empty())
                        {
                            startId = 0;
                        }
                        for (int pvid = startId; pvid < pathInfos.size(); pvid++)
                        {
                            mCurPointsOnEdge.push_back(pathInfos.at(pvid));
                        }
                        for (int pvid = startId; pvid < pathCoords.size(); pvid++)
                        {
                            mCurMarkCoords.push_back(pathCoords.at(pvid));
                        }
                    }
                    else
                    {
                        std::vector<int> pathVertexIds;
                        GPP::Real distance = 0;
                        GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
                        GPP::ErrorCode res = GPP::MeasureMesh::ComputeApproximateGeodesics(triMesh, sectionVertexIds, false, 
                            pathVertexIds, distance);
                        if (res != GPP_NO_ERROR)
                        {
                            MessageBox(NULL, "测量失败", "温馨提示", MB_OK);
                            return true;
                        }
                        int startId = 1;
                        if (mCurPointsOnVertex.empty())
                        {
                            startId = 0;
                        }
                        for (int pvid = startId; pvid < pathVertexIds.size(); pvid++)
                        {
                            mCurPointsOnVertex.push_back(pathVertexIds.at(pvid));
                            mCurMarkCoords.push_back(triMesh->GetVertexCoord(pathVertexIds.at(pvid)));
                        }
                    }
                    mLastCutVertexId = pickedId;
                }
                UpdateMarkDisplay();
            }
        }
        return true;
    }

    bool UVUnfoldApp::KeyPressed( const OIS::KeyEvent &arg )
    {
        if (arg.key == OIS::KC_E)
        {
            MagicCore::RenderSystem::Get()->GetMainCamera()->setPolygonMode(Ogre::PolygonMode::PM_WIREFRAME);
        }
        return true;
    }

    void UVUnfoldApp::SetupScene()
    {
        Ogre::SceneManager* sceneManager = MagicCore::RenderSystem::Get()->GetSceneManager();
        sceneManager->setAmbientLight(Ogre::ColourValue(0.3, 0.3, 0.3));
        Ogre::Light* light = sceneManager->createLight("UVUnfoldApp_SimpleLight");
        light->setPosition(0, 0, 20);
        light->setDiffuseColour(0.8, 0.8, 0.8);
        light->setSpecularColour(0.5, 0.5, 0.5);

        if (mpImageFrameMesh == NULL)
        {
            mpImageFrameMesh = GPP::Parser::ImportTriMesh("../../Media/UVUnfoldApp/ImageMesh.obj");
            if (mpImageFrameMesh == NULL)
            {
                MessageBox(NULL, "纹理网格导入失败", "温馨提示", MB_OK);
                return;
            }
            mpImageFrameMesh->UpdateNormal();
        }

        mDistortionImage.release();
        mDistortionImage = cv::imread("../../Media/UVUnfoldApp/grid.png");
        if (mDistortionImage.data == NULL)
        {
            MessageBox(NULL, "纹理图片打开失败", "温馨提示", MB_OK);
        }
        InitTriMeshTexture();

        InitViewTool();

        // set up pick tool
        GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
        if (triMesh)
        {
            GPPFREEPOINTER(mpPickTool);
            mpPickTool = new MagicCore::PickTool;
            mpPickTool->SetPickParameter(MagicCore::PM_POINT, true, NULL, triMesh, "ModelNode");
            mpUI->SetMeshInfo(triMesh->GetVertexCount(), triMesh->GetTriangleCount());
            ClearSplitData();
            InsertHolesToSnapIds();
            UpdateMarkDisplay();
        }
    }

    void UVUnfoldApp::ShutdownScene()
    {
        Ogre::SceneManager* sceneManager = MagicCore::RenderSystem::Get()->GetSceneManager();
        sceneManager->setAmbientLight(Ogre::ColourValue::Black);
        sceneManager->destroyLight("UVUnfoldApp_SimpleLight");        
        /*MagicCore::RenderSystem::Get()->SetupCameraDefaultParameter();
        if (MagicCore::RenderSystem::Get()->GetSceneManager()->hasSceneNode("ModelNode"))
        {
            MagicCore::RenderSystem::Get()->GetSceneManager()->getSceneNode("ModelNode")->resetToInitialState();
        }*/
        MagicCore::RenderSystem::Get()->HideRenderingObject("TriMesh_UVUnfoldApp");
        MagicCore::RenderSystem::Get()->HideRenderingObject("CutVertexRed_UVUnfoldApp");
        MagicCore::RenderSystem::Get()->HideRenderingObject("CutVertexYellow_UVUnfoldApp");
    }

    void UVUnfoldApp::ImportTriMesh()
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        std::string fileName;
        char filterName[] = "OBJ Files(*.obj)\0*.obj\0STL Files(*.stl)\0*.stl\0OFF Files(*.off)\0*.off\0PLY Files(*.ply)\0*.ply\0GPT Files(*.gpt)\0*.gpt\0";
        if (MagicCore::ToolKit::FileOpenDlg(fileName, filterName))
        {
            ModelManager::Get()->ClearPointCloud();
            if (ModelManager::Get()->ImportMesh(fileName) == false)
            {
                MessageBox(NULL, "网格导入失败", "温馨提示", MB_OK);
                return;
            }
            GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
            SwitchDisplayMode(TRIMESH_SOLID);
            UpdateDisplay();
            // set up pick tool
            GPPFREEPOINTER(mpPickTool);
            mpPickTool = new MagicCore::PickTool;
            mpPickTool->SetPickParameter(MagicCore::PM_POINT, true, NULL, triMesh, "ModelNode");
            // Clear data
            ClearSplitData();
            InsertHolesToSnapIds();
            UpdateMarkDisplay();
            mpUI->SetMeshInfo(triMesh->GetVertexCount(), triMesh->GetTriangleCount());
        }
    }

    void UVUnfoldApp::ConfirmGeodesics()
    {
        if (mCurPointsOnVertex.size() > 0 && mCurPointsOnEdge.size() > 0)
        {
            MessageBox(NULL, "割线信息有错", "温馨提示", MB_OK);
            return;
        }
        if (mCurPointsOnEdge.size() > 0)
        {
            GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
            mCurMarkCoords.clear();
            mLastCutVertexId = -1;
            std::vector<GPP::Int> newSplitLineIds;
            GPP::ErrorCode res = GPP::SplitMesh::InsertSplitLineOnTriMesh(triMesh, mCurPointsOnEdge, &newSplitLineIds);
            mCurPointsOnEdge.clear();
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "割线不能自交", "温馨提示", MB_OK);
                return;
            }
            triMesh->UpdateNormal();
            mCutLineList.push_back(newSplitLineIds);
            mpUI->SetMeshInfo(triMesh->GetVertexCount(), triMesh->GetTriangleCount());
            if (mSnapIds.empty())
            {
                InsertHolesToSnapIds();
            }
            for (std::vector<GPP::Int>::iterator itr = newSplitLineIds.begin(); itr != newSplitLineIds.end(); ++itr)
            {
                mSnapIds.push_back(*itr);
            }
            UpdateMarkDisplay();
            UpdateDisplay();
        }
        else if (mCurPointsOnVertex.size() > 0)
        {
            mCurMarkCoords.clear();
            mLastCutVertexId = -1;
            mCutLineList.push_back(mCurPointsOnVertex);
            if (mSnapIds.empty())
            {
                InsertHolesToSnapIds();
            }
            for (std::vector<GPP::Int>::iterator itr = mCurPointsOnVertex.begin(); itr != mCurPointsOnVertex.end(); ++itr)
            {
                mSnapIds.push_back(*itr);
            }
            mCurPointsOnVertex.clear();
            UpdateMarkDisplay();
            UpdateDisplay();
        }
    }

    void UVUnfoldApp::CloseGeodesics()
    {
        if (mLastCutVertexId == -1)
        {
            return;
        }
        if (mIsCutLineAccurate)
        {
            if (mCurPointsOnEdge.empty())
            {
                return;
            }
            int pickedId = mCurPointsOnEdge.at(0).mVertexIdStart;
            if (mLastCutVertexId == pickedId)
            {
                return;
            }
            std::vector<GPP::Int> sectionVertexIds;
            sectionVertexIds.push_back(mLastCutVertexId);
            sectionVertexIds.push_back(pickedId);
            std::vector<GPP::Vector3> pathCoords;
            std::vector<GPP::PointOnEdge> pathInfos;
            GPP::Real distance = 0;
            GPP::ErrorCode res = GPP::MeasureMesh::FastComputeExactGeodesics(ModelManager::Get()->GetMesh(), sectionVertexIds, false, 
                pathCoords, distance, &pathInfos, 0.5);
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "测量失败", "温馨提示", MB_OK);
                return;
            }
            int startId = 1;
            for (int pvid = startId; pvid < pathInfos.size(); pvid++)
            {
                mCurPointsOnEdge.push_back(pathInfos.at(pvid));
            }
            for (int pvid = startId; pvid < pathCoords.size(); pvid++)
            {
                mCurMarkCoords.push_back(pathCoords.at(pvid));
            }
            mLastCutVertexId = pickedId;
            UpdateMarkDisplay();
        }
        else
        {
            if (mCurPointsOnVertex.empty())
            {
                return;
            }
            int pickedId = mCurPointsOnVertex.at(0);
            if (mLastCutVertexId == pickedId)
            {
                return;
            }
            std::vector<GPP::Int> sectionVertexIds;
            sectionVertexIds.push_back(mLastCutVertexId);
            sectionVertexIds.push_back(pickedId);
            std::vector<int> pathVertexIds;
            GPP::Real distance = 0;
            GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
            GPP::ErrorCode res = GPP::MeasureMesh::ComputeApproximateGeodesics(triMesh, sectionVertexIds, false, pathVertexIds, distance);
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "测量失败", "温馨提示", MB_OK);
                return;
            }
            int startId = 1;
            for (int pvid = startId; pvid < pathVertexIds.size(); pvid++)
            {
                mCurPointsOnVertex.push_back(pathVertexIds.at(pvid));
                mCurMarkCoords.push_back(triMesh->GetVertexCoord(pathVertexIds.at(pvid)));
            }
            mLastCutVertexId = pickedId;
            UpdateMarkDisplay();
        }
    }

    void UVUnfoldApp::SnapFrontGeodesics()
    {
        GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
        if (mLastCutVertexId == -1 || triMesh == NULL)
        {
            return;
        }
        if ((mIsCutLineAccurate && mCurPointsOnEdge.empty()) || (!mIsCutLineAccurate && mCurPointsOnVertex.empty()))
        {
            return;
        }
        if (mSnapIds.empty())
        {
            InsertHolesToSnapIds();
        }
        int pickId = -1;
        double minDist = GPP::REAL_LARGE;
        GPP::Vector3 headCoord = triMesh->GetVertexCoord(mLastCutVertexId);
        for (std::vector<int>::iterator sitr = mSnapIds.begin(); sitr != mSnapIds.end(); ++sitr)
        {
            double curDist = (headCoord - triMesh->GetVertexCoord(*sitr)).LengthSquared();
            if (curDist < minDist)
            {
                minDist = curDist;
                pickId = *sitr;
            }
        }
        if (pickId == -1 || pickId == mLastCutVertexId)
        {
            return;
        }
        std::vector<GPP::Int> sectionVertexIds;
        sectionVertexIds.push_back(mLastCutVertexId);
        sectionVertexIds.push_back(pickId);
        if (mIsCutLineAccurate && mCurPointsOnEdge.size() > 0)
        {
            std::vector<GPP::Vector3> pathCoords;
            std::vector<GPP::PointOnEdge> pathInfos;
            GPP::Real distance = 0;
            GPP::ErrorCode res = GPP::MeasureMesh::FastComputeExactGeodesics(triMesh, sectionVertexIds, false, 
                pathCoords, distance, &pathInfos, 0.5);
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "测量失败", "温馨提示", MB_OK);
                return;
            }
            int startId = 1;
            for (int pvid = startId; pvid < pathInfos.size(); pvid++)
            {
                mCurPointsOnEdge.push_back(pathInfos.at(pvid));
            }
            for (int pvid = startId; pvid < pathCoords.size(); pvid++)
            {
                mCurMarkCoords.push_back(pathCoords.at(pvid));
            }
            mLastCutVertexId = pickId;
            UpdateMarkDisplay();
        }
        else if (!mIsCutLineAccurate && mCurPointsOnVertex.size() > 0)
        {
            std::vector<int> pathVertexIds;
            GPP::Real distance = 0;
            GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
            GPP::ErrorCode res = GPP::MeasureMesh::ComputeApproximateGeodesics(triMesh, sectionVertexIds, false, pathVertexIds, distance);
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "测量失败", "温馨提示", MB_OK);
                return;
            }
            int startId = 1;
            for (int pvid = startId; pvid < pathVertexIds.size(); pvid++)
            {
                mCurPointsOnVertex.push_back(pathVertexIds.at(pvid));
                mCurMarkCoords.push_back(triMesh->GetVertexCoord(pathVertexIds.at(pvid)));
            }
            mLastCutVertexId = pickId;
            UpdateMarkDisplay();
        }
    }
    
    void UVUnfoldApp::SnapBackGeodesics()
    {
        GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
        if (mLastCutVertexId == -1 || triMesh == NULL)
        {
            return;
        }
        if ((mIsCutLineAccurate && mCurPointsOnEdge.empty()) || (!mIsCutLineAccurate && mCurPointsOnVertex.empty()))
        {
            return;
        }
        if (mSnapIds.empty())
        {
            InsertHolesToSnapIds();
        }
        int pickId = -1;
        double minDist = GPP::REAL_LARGE;
        GPP::Vector3 headCoord;
        if (mIsCutLineAccurate)
        {
            headCoord = triMesh->GetVertexCoord(mCurPointsOnEdge.at(0).mVertexIdStart);
        }
        else
        {
            headCoord = triMesh->GetVertexCoord(mCurPointsOnVertex.at(0));
        }
        for (std::vector<int>::iterator sitr = mSnapIds.begin(); sitr != mSnapIds.end(); ++sitr)
        {
            double curDist = (headCoord - triMesh->GetVertexCoord(*sitr)).LengthSquared();
            if (curDist < minDist)
            {
                minDist = curDist;
                pickId = *sitr;
            }
        }
        if (pickId == -1)
        {
            return;
        }
        if (mIsCutLineAccurate && mCurPointsOnEdge.size() > 0)
        {
            if (pickId == mCurPointsOnEdge.at(0).mVertexIdStart)
            {
                return;
            }
            std::vector<GPP::Int> sectionVertexIds;
            sectionVertexIds.push_back(pickId);
            sectionVertexIds.push_back(mCurPointsOnEdge.at(0).mVertexIdStart);
            std::vector<GPP::Vector3> pathCoords;
            std::vector<GPP::PointOnEdge> pathInfos;
            GPP::Real distance = 0;
            GPP::ErrorCode res = GPP::MeasureMesh::FastComputeExactGeodesics(triMesh, sectionVertexIds, false, 
                pathCoords, distance, &pathInfos, 0.5);
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "测量失败", "温馨提示", MB_OK);
                return;
            }
            std::vector<GPP::PointOnEdge> newPonitsOnEdge;
            for (int pvid = 0; pvid < pathInfos.size() - 1; pvid++)
            {
                newPonitsOnEdge.push_back(pathInfos.at(pvid));
            }
            newPonitsOnEdge.insert(newPonitsOnEdge.end(), mCurPointsOnEdge.begin(), mCurPointsOnEdge.end());
            mCurPointsOnEdge.swap(newPonitsOnEdge);
            for (int pvid = 0; pvid < pathCoords.size(); pvid++)
            {
                mCurMarkCoords.push_back(pathCoords.at(pvid));
            }
            UpdateMarkDisplay();
        }
        else if (!mIsCutLineAccurate && mCurPointsOnVertex.size() > 0)
        {
            if (pickId == mCurPointsOnVertex.at(0))
            {
                return;
            }
            std::vector<GPP::Int> sectionVertexIds;
            sectionVertexIds.push_back(pickId);
            sectionVertexIds.push_back(mCurPointsOnVertex.at(0));
            std::vector<int> pathVertexIds;
            GPP::Real distance = 0;
            GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
            GPP::ErrorCode res = GPP::MeasureMesh::ComputeApproximateGeodesics(triMesh, sectionVertexIds, false, pathVertexIds, distance);
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "测量失败", "温馨提示", MB_OK);
                return;
            }
            for (int pvid = 0; pvid < pathVertexIds.size(); pvid++)
            {
                mCurMarkCoords.push_back(triMesh->GetVertexCoord(pathVertexIds.at(pvid)));
            }
            pathVertexIds.pop_back();
            pathVertexIds.insert(pathVertexIds.end(), mCurPointsOnVertex.begin(), mCurPointsOnVertex.end());
            mCurPointsOnVertex.swap(pathVertexIds);
            UpdateMarkDisplay();
        }
    }

    void UVUnfoldApp::InsertHolesToSnapIds()
    {
        GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
        if (triMesh == NULL)
        {
            return;
        }
        std::vector<std::vector<GPP::Int> > holeIds;
        GPP::ErrorCode res = GPP::FillMeshHole::FindHoles(triMesh, &holeIds);
        if (res != GPP_NO_ERROR)
        {
            return;
        }
        for (std::vector<std::vector<GPP::Int> >::iterator holeItr = holeIds.begin(); holeItr != holeIds.end(); ++holeItr)
        {
            for (std::vector<GPP::Int>::iterator lineItr = holeItr->begin(); lineItr != holeItr->end(); ++lineItr)
            {
                mSnapIds.push_back(*lineItr);
            }
        }
    }

    void UVUnfoldApp::DeleteGeodesics()
    {
        mLastCutVertexId = -1;
        if (!mCurMarkCoords.empty())
        {
            mCurMarkCoords.clear();
        }
        if (!mCurPointsOnVertex.empty())
        {
            mCurPointsOnVertex.clear();
        }
        if (!mCurPointsOnEdge.empty())
        {
            mCurPointsOnEdge.clear();
        }
        UpdateMarkDisplay();
    }

    void UVUnfoldApp::SwitchMarkDisplay()
    {
        mHideMarks = !mHideMarks;
        UpdateMarkDisplay();
    }

    void UVUnfoldApp::SetCutLineType(bool isAccurate)
    {
        mIsCutLineAccurate = isAccurate;
        if (isAccurate && mCurPointsOnVertex.size() > 0)
        {
            mLastCutVertexId = -1;
            mCurMarkCoords.clear();
            mCurPointsOnVertex.clear();
            UpdateMarkDisplay();
        }
        else if (!isAccurate &&  mCurPointsOnEdge.size() > 0)
        {
            mLastCutVertexId = -1;
            mCurMarkCoords.clear();
            mCurPointsOnEdge.clear();
            UpdateMarkDisplay();
        }
    }

    void UVUnfoldApp::SmoothCutLine()
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        if (mCurPointsOnVertex.empty())
        {
            return;
        }
        GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
        if (triMesh == NULL)
        {
            MessageBox(NULL, "请导入需要UV展开的网格", "温馨提示", MB_OK);
            return;
        }
#if MAKEDUMPFILE
        GPP::DumpOnce();
#endif
        GPP::ErrorCode res = GPP::OptimiseCurve::SmoothCurveOnMesh(triMesh, mCurPointsOnVertex, false, GPP::ONE_RADIAN * 60, 0.2, 10);
        if (res == GPP_API_IS_NOT_AVAILABLE)
        {
            MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
            MagicCore::ToolKit::Get()->SetAppRunning(false);
        }
        if (res != GPP_NO_ERROR)
        {
            MessageBox(NULL, "割线优化失败", "温馨提示", MB_OK);
            return;
        }
        mCurMarkCoords.clear();
        mCurMarkCoords.reserve(mCurPointsOnVertex.size());
        for (std::vector<GPP::Int>::iterator vitr = mCurPointsOnVertex.begin(); vitr != mCurPointsOnVertex.end(); ++vitr)
        {
            mCurMarkCoords.push_back(triMesh->GetVertexCoord(*vitr));
        }
        UpdateMarkDisplay();
        UpdateDisplay();
    }

    void UVUnfoldApp::GenerateSplitLines(int splitChartCount)
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
        if (triMesh == NULL)
        {
            MessageBox(NULL, "请导入需要UV展开的网格", "温馨提示", MB_OK);
            return;
        }
        std::vector<std::vector<GPP::Int> > splitLines;
        GPP::ErrorCode res = GPP::SplitMesh::GenerateAtlasSplitLines(triMesh, splitChartCount, splitLines);
        if (res == GPP_API_IS_NOT_AVAILABLE)
        {
            MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
            MagicCore::ToolKit::Get()->SetAppRunning(false);
        }
        if (res != GPP_NO_ERROR)
        {
            MessageBox(NULL, "自动割线失败", "温馨提示", MB_OK);
            return;
        }
        ClearSplitData();
        InsertHolesToSnapIds();
        mCutLineList.swap(splitLines);
        for (std::vector<std::vector<GPP::Int> >::iterator itr = mCutLineList.begin(); itr != mCutLineList.end(); ++itr)
        {
            for (std::vector<GPP::Int>::iterator lineItr = itr->begin(); lineItr != itr->end(); ++lineItr)
            {
                mSnapIds.push_back(*lineItr);
            }
        }
        UpdateMarkDisplay();
    }

    void UVUnfoldApp::DoCommand(bool isSubThread)
    {
        if (isSubThread)
        {
            GPP::ResetApiProgress();
            mpUI->StartProgressbar(100);
            _beginthreadex(NULL, 0, RunThread, (void *)this, 0, NULL);
        }
        else
        {
            switch (mCommandType)
            {
            case MagicApp::UVUnfoldApp::NONE:
                break;
            case MagicApp::UVUnfoldApp::UNFOLD_INITIAL:
                UnfoldTriMesh(false);
                break;
            case MagicApp::UVUnfoldApp::GENERATE_UV_ATLAS:
                GenerateUVAtlas(mInitChartCount, false);
                break;
            case MagicApp::UVUnfoldApp::UNFOLD_DISC:
                Unfold2Disc(false);
                break;
            default:
                break;
            }
            mpUI->StopProgressbar();
        }
    }

    void UVUnfoldApp::UnfoldTriMesh(bool isSubThread)
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
        if (triMesh == NULL)
        {
            MessageBox(NULL, "请导入需要UV展开的网格", "温馨提示", MB_OK);
            return;
        }
        if (isSubThread)
        {
            mCommandType = UNFOLD_INITIAL;
            DoCommand(true);
        }
        else
        {
            if (GPP::ConsolidateMesh::_IsTriMeshManifold(triMesh) == false)
            {
                MessageBox(NULL, "网格展开失败：网格有非流形结构，请先拓扑修复", "温馨提示", MB_OK);
                return;
            }
            if (GPP::ConsolidateMesh::_IsGeometryDegenerate(triMesh) == false)
            {
                if (MessageBox(NULL, "警告：网格有退化几何，继续计算可能得到无效结果，可以先几何修复，是否继续？", "温馨提示", MB_OKCANCEL) != IDOK)
                {
                    return;
                }
            }
            GenerateSplitMesh();
            bool isMultiPatchCase = false;
            // Generate fixed vertex
            std::vector<std::vector<GPP::Int> > holeIds;
            GPP::ErrorCode res = GPP::FillMeshHole::FindHoles(triMesh, &holeIds);
            if (res == GPP_API_IS_NOT_AVAILABLE)
            {
                MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
                MagicCore::ToolKit::Get()->SetAppRunning(false);
            }
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "找网格边界失败", "温馨提示", MB_OK);
                return;
            }
            if (holeIds.empty())
            {
                isMultiPatchCase = true;
            }
            if (!isMultiPatchCase)
            {
                // Is single connected region
                std::vector<GPP::Real> isolation;
                GPP::Int uvVertexCount = triMesh->GetVertexCount();
                res = GPP::ConsolidateMesh::CalculateIsolation(triMesh, &isolation);
                for (GPP::Int vid = 0; vid < uvVertexCount; vid++)
                {
                    if (isolation.at(vid) < 1)
                    {
                        isMultiPatchCase = true;
                        break;
                    }
                }
                if (res == GPP_API_IS_NOT_AVAILABLE)
                {
                    MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
                    MagicCore::ToolKit::Get()->SetAppRunning(false);
                }
                if (res != GPP_NO_ERROR)
                {
                    MessageBox(NULL, "网格展开失败：网格连通区域检测失败", "温馨提示", MB_OK);
                    return;
                }
                if (!isMultiPatchCase)
                {
                    std::vector<GPP::Int> fixedVertexIndices(2);
                    std::vector<GPP::Real> fixedVertexCoords(4);
                    fixedVertexIndices.at(0) = holeIds.at(0).at(0);
                    fixedVertexIndices.at(1) = holeIds.at(0).at(holeIds.at(0).size() / 2);
                    fixedVertexCoords.at(0) = -1.0;
                    fixedVertexCoords.at(1) = -1.0;
                    fixedVertexCoords.at(2) = 1.0;
                    fixedVertexCoords.at(3) = 1.0;
                    std::vector<GPP::Real> texCoords;
                    mIsCommandInProgress = true;
#if MAKEDUMPFILE
                    GPP::DumpOnce();
#endif
                    res = GPP::UnfoldMesh::ConformalMap(triMesh, &fixedVertexIndices, &fixedVertexCoords, &texCoords);
                    if (res == GPP_NO_ERROR)
                    {
#if MAKEDUMPFILE
                        GPP::DumpOnce();
#endif
                        res = GPP::UnfoldMesh::OptimizeIsometric(triMesh, &texCoords, 10, NULL);
                    }
                    mIsCommandInProgress = false;
                    if (res == GPP_API_IS_NOT_AVAILABLE)
                    {
                        MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
                        MagicCore::ToolKit::Get()->SetAppRunning(false);
                    }
                    if (res != GPP_NO_ERROR)
                    {
                        MessageBox(NULL, "网格展开失败", "温馨提示", MB_OK);
                        return;
                    }
                    UnifyTextureCoords(texCoords, 0.9);
                    triMesh->SetHasVertexTexCoord(true);
                    GPP::Int vertexCount = triMesh->GetVertexCount();
                    for (GPP::Int vid = 0; vid < vertexCount; vid++)
                    {
                        triMesh->SetVertexTexcoord(vid, GPP::Vector3(texCoords.at(vid * 2), texCoords.at(vid * 2 + 1), 0));
                    }
                    triMesh->SetHasTriangleTexCoord(true);
                    GPP::Int faceCount = triMesh->GetTriangleCount();
                    int vertexIds[3] = {-1};
                    for (int fid = 0; fid < faceCount; fid++)
                    {
                        triMesh->GetTriangleVertexIds(fid, vertexIds);
                        for (int lid = 0; lid < 3; lid++)
                        {
                            triMesh->SetTriangleTexcoord(fid, lid, triMesh->GetVertexTexcoord(vertexIds[lid]));
                        }
                    }
                }
            }
            if (isMultiPatchCase)
            {
                std::vector<GPP::Real> texCoords;
                std::vector<GPP::Int> faceTexIds;
                mIsCommandInProgress = true;
                GPP::ErrorCode res = GPP::UnfoldMesh::GenerateUVAtlas(triMesh, 1, &texCoords, &faceTexIds, false, false, false);
                mIsCommandInProgress = false;
                if (res == GPP_API_IS_NOT_AVAILABLE)
                {
                    MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
                    MagicCore::ToolKit::Get()->SetAppRunning(false);
                }
                if (res != GPP_NO_ERROR)
                {
                    MessageBox(NULL, "UV Atlas 生成失败", "温馨提示", MB_OK);
                    return;
                }
                UnifyTextureCoords(texCoords, 0.9);
                triMesh->SetHasTriangleTexCoord(true);
                int uvFaceCount = faceTexIds.size() / 3;
                for (int fid = 0; fid < uvFaceCount; fid++)
                {
                    for (int localId = 0; localId < 3; localId++)
                    {
                        GPP::Int tid = faceTexIds.at(fid * 3 + localId);
                        triMesh->SetTriangleTexcoord(fid, localId, GPP::Vector3(texCoords.at(tid * 2), texCoords.at(tid * 2 + 1), 0));
                    }
                }
            }
            
            mDisplayMode = UVMESH_WIREFRAME;
            mUpdateDisplay = true;
            mHideMarks = true;
        }
    }

    void UVUnfoldApp::Unfold2Disc(bool isSubThread)
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
        if (triMesh == NULL)
        {
            MessageBox(NULL, "请导入需要UV展开的网格", "温馨提示", MB_OK);
            return;
        }
        if (isSubThread)
        {
            mCommandType = UNFOLD_DISC;
            DoCommand(true);
        }
        else
        {
            if (GPP::ConsolidateMesh::_IsTriMeshManifold(triMesh) == false)
            {
                MessageBox(NULL, "网格展开失败：网格有非流形结构，请先拓扑修复", "温馨提示", MB_OK);
                return;
            }
            if (GPP::ConsolidateMesh::_IsGeometryDegenerate(triMesh) == false)
            {
                if (MessageBox(NULL, "警告：网格有退化几何，继续计算可能得到无效结果，可以先几何修复，是否继续？", "温馨提示", MB_OKCANCEL) != IDOK)
                {
                    return;
                }
            }
            GenerateSplitMesh();
            bool isMultiPatchCase = false;
            // Generate fixed vertex
            std::vector<std::vector<GPP::Int> > holeIds;
            GPP::ErrorCode res = GPP::FillMeshHole::FindHoles(triMesh, &holeIds);
            if (res == GPP_API_IS_NOT_AVAILABLE)
            {
                MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
                MagicCore::ToolKit::Get()->SetAppRunning(false);
            }
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "找网格边界失败", "温馨提示", MB_OK);
                return;
            }
            if (holeIds.empty())
            {
                MessageBox(NULL, "网格必须为单连通原盘拓扑结构", "温馨提示", MB_OK);
                return;
            }

            // Is single connected region
            std::vector<GPP::Real> isolation;
            GPP::Int uvVertexCount = triMesh->GetVertexCount();
            res = GPP::ConsolidateMesh::CalculateIsolation(triMesh, &isolation);
            for (GPP::Int vid = 0; vid < uvVertexCount; vid++)
            {
                if (isolation.at(vid) < 1)
                {
                    MessageBox(NULL, "网格必须为单连通结构", "温馨提示", MB_OK);
                    return;
                }
            }
            if (res == GPP_API_IS_NOT_AVAILABLE)
            {
                MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
                MagicCore::ToolKit::Get()->SetAppRunning(false);
            }
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "网格展开失败：网格连通区域检测失败", "温馨提示", MB_OK);
                return;
            }

            int maxLengthBoundaryId = -1;
            double maxLength = -1;
            for (int bid = 0; bid < holeIds.size(); bid++)
            {
                double curLength = 0;
                for (int vid = 0; vid < holeIds.at(bid).size() - 1; vid++)
                {
                    curLength += (triMesh->GetVertexCoord(holeIds.at(bid).at(vid)) - triMesh->GetVertexCoord(holeIds.at(bid).at(vid + 1))).Length();
                }
                if (curLength > maxLength)
                {
                    maxLength = curLength;
                    maxLengthBoundaryId = bid;
                }
            }
            std::vector<GPP::Int> fixedVertexIndices = holeIds.at(maxLengthBoundaryId);
            int boundaryVertexSize = fixedVertexIndices.size();
            double theta = -2.0 * GPP::GPP_PI / double(boundaryVertexSize);
            std::vector<GPP::Real> fixedVertexCoords;
            fixedVertexCoords.reserve(boundaryVertexSize * 2);
            for (int pid = 0; pid < boundaryVertexSize; pid++)
            {
                fixedVertexCoords.push_back(sin(pid * theta));
                fixedVertexCoords.push_back(cos(pid * theta));
            }
            std::vector<GPP::Real> texCoords;
            mIsCommandInProgress = true;
#if MAKEDUMPFILE
            GPP::DumpOnce();
#endif
            res = GPP::UnfoldMesh::ConformalMap(triMesh, &fixedVertexIndices, &fixedVertexCoords, &texCoords);
            mIsCommandInProgress = false;
            if (res == GPP_API_IS_NOT_AVAILABLE)
            {
                MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
                MagicCore::ToolKit::Get()->SetAppRunning(false);
            }
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "网格展开失败", "温馨提示", MB_OK);
                return;
            }
            UnifyTextureCoords(texCoords, 0.9);
            triMesh->SetHasVertexTexCoord(true);
            GPP::Int vertexCount = triMesh->GetVertexCount();
            for (GPP::Int vid = 0; vid < vertexCount; vid++)
            {
                triMesh->SetVertexTexcoord(vid, GPP::Vector3(texCoords.at(vid * 2), texCoords.at(vid * 2 + 1), 0));
            }
            triMesh->SetHasTriangleTexCoord(true);
            GPP::Int faceCount = triMesh->GetTriangleCount();
            int vertexIds[3] = {-1};
            for (int fid = 0; fid < faceCount; fid++)
            {
                triMesh->GetTriangleVertexIds(fid, vertexIds);
                for (int lid = 0; lid < 3; lid++)
                {
                    triMesh->SetTriangleTexcoord(fid, lid, triMesh->GetVertexTexcoord(vertexIds[lid]));
                }
            }

            mDisplayMode = UVMESH_WIREFRAME;
            mUpdateDisplay = true;
            mHideMarks = true;
        }
    }

    void UVUnfoldApp::GenerateUVAtlas(int initChartCount, bool isSubThread)
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
        if (triMesh == NULL)
        {
            MessageBox(NULL, "请导入需要UV展开的网格", "温馨提示", MB_OK);
            return;
        }
        if (isSubThread)
        {
            mInitChartCount = initChartCount;
            mCommandType = GENERATE_UV_ATLAS;
            DoCommand(true);
        }
        else
        {
            if (GPP::ConsolidateMesh::_IsTriMeshManifold(triMesh) == false)
            {
                MessageBox(NULL, "网格展开失败：网格有非流形结构，请先拓扑修复", "温馨提示", MB_OK);
                return;
            }
            if (GPP::ConsolidateMesh::_IsGeometryDegenerate(triMesh) == false)
            {
                if (MessageBox(NULL, "警告：网格有退化几何，继续计算可能得到无效结果，可以先几何修复，是否继续？", "温馨提示", MB_OKCANCEL) != IDOK)
                {
                    return;
                }
            }
            GenerateSplitMesh();
            std::vector<GPP::Real> texCoords;
            std::vector<GPP::Int> faceTexIds;
            mIsCommandInProgress = true;
#if MAKEDUMPFILE
            GPP::DumpOnce();
#endif
            GPP::ErrorCode res = GPP::UnfoldMesh::GenerateUVAtlas(triMesh, initChartCount, &texCoords, &faceTexIds, true, true, true);
            mIsCommandInProgress = false;
            if (res == GPP_API_IS_NOT_AVAILABLE)
            {
                MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
                MagicCore::ToolKit::Get()->SetAppRunning(false);
            }
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "UV Atlas 生成失败", "温馨提示", MB_OK);
                return;
            }
            UnifyTextureCoords(texCoords, 0.9);
            triMesh->SetHasTriangleTexCoord(true);
            int uvFaceCount = faceTexIds.size() / 3;
            for (int fid = 0; fid < uvFaceCount; fid++)
            {
                for (int localId = 0; localId < 3; localId++)
                {
                    GPP::Int tid = faceTexIds.at(fid * 3 + localId);
                    triMesh->SetTriangleTexcoord(fid, localId, GPP::Vector3(texCoords.at(tid * 2), texCoords.at(tid * 2 + 1), 0));
                }
            }

            mDisplayMode = UVMESH_WIREFRAME;
            mUpdateDisplay = true;
            mHideMarks = true;
        }
    }

    int UVUnfoldApp::GetMeshVertexCount()
    {
        if (ModelManager::Get()->GetMesh() != NULL)
        {
            return ModelManager::Get()->GetMesh()->GetVertexCount();
        }
        else
        {
            return 0;
        }
    }

    void UVUnfoldApp::SwitchDisplayMode(DisplayMode dm)
    {
        if (dm == AUTO)
        {
            if (mDisplayMode == TRIMESH_SOLID)
            {
                mDisplayMode = TRIMESH_WIREFRAME;
            }
            else if (mDisplayMode == TRIMESH_WIREFRAME)
            {
                mDisplayMode = UVMESH_WIREFRAME;
            }
            else if (mDisplayMode == UVMESH_WIREFRAME)
            {
                mDisplayMode = TRIMESH_TEXTURE;
            }
            else if (mDisplayMode == TRIMESH_TEXTURE)
            {
                mDisplayMode = TRIMESH_SOLID;
            }
        }
        else
        {
            mDisplayMode = dm;
        }

        if (mDisplayMode == TRIMESH_SOLID || mDisplayMode == UVMESH_WIREFRAME)
        {
            Ogre::Material* material = dynamic_cast<Ogre::Material*>(Ogre::MaterialManager::getSingleton().getByName("CookTorrance").getPointer());
            if (material)
            {
                material->setCullingMode(Ogre::CullingMode::CULL_NONE);
            }
        }
        else if (mDisplayMode == TRIMESH_WIREFRAME)
        {
            Ogre::Material* material = dynamic_cast<Ogre::Material*>(Ogre::MaterialManager::getSingleton().getByName("CookTorrance").getPointer());
            if (material)
            {
                material->setCullingMode(Ogre::CullingMode::CULL_CLOCKWISE);
            }
        }
        else if (mDisplayMode == TRIMESH_TEXTURE)
        {
            Ogre::Material* material = dynamic_cast<Ogre::Material*>(Ogre::MaterialManager::getSingleton().getByName("UVUnfoldGridMaterial").getPointer());
            if (material)
            {
                material->setCullingMode(Ogre::CullingMode::CULL_NONE);
            }
        }
        UpdateDisplay();
    }

    void UVUnfoldApp::InitViewTool()
    {
        if (mpViewTool == NULL)
        {
            mpViewTool = new MagicCore::ViewTool;
            mpViewTool->SetRightNodeFixed(true);
        }
    }

    void UVUnfoldApp::UpdateDisplay()
    {
        GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
        if (mDisplayMode == TRIMESH_SOLID)
        {
            InfoLog << "Display TRIMESH_SOLID" << std::endl;
            if (triMesh == NULL)
            {
                MagicCore::RenderSystem::Get()->HideRenderingObject("TriMesh_UVUnfoldApp");
                return;
            }
            MagicCore::RenderSystem::Get()->GetMainCamera()->setPolygonMode(Ogre::PolygonMode::PM_SOLID);
            MagicCore::RenderSystem::Get()->RenderMesh("TriMesh_UVUnfoldApp", "CookTorrance", triMesh, 
                MagicCore::RenderSystem::MODEL_NODE_CENTER, NULL, NULL, false);
            InfoLog << "Display done" << std::endl;
        }
        else if (mDisplayMode == TRIMESH_WIREFRAME)
        {
            InfoLog << "Display TRIMESH_WIREFRAME" << std::endl;
            if (triMesh == NULL)
            {
                MagicCore::RenderSystem::Get()->HideRenderingObject("TriMesh_UVUnfoldApp");
                return;
            }
            MagicCore::RenderSystem::Get()->GetMainCamera()->setPolygonMode(Ogre::PolygonMode::PM_WIREFRAME);
            MagicCore::RenderSystem::Get()->RenderMesh("TriMesh_UVUnfoldApp", "CookTorrance", triMesh, 
                MagicCore::RenderSystem::MODEL_NODE_CENTER);
            InfoLog << "Display done" << std::endl;
        }
        else if (mDisplayMode == TRIMESH_TEXTURE)
        {
            InfoLog << "Display TRIMESH_TEXTURE" << std::endl;
            if (triMesh == NULL || (!triMesh->HasTriangleTexCoord()))
            {
                MagicCore::RenderSystem::Get()->HideRenderingObject("TriMesh_UVUnfoldApp");
                return;
            }
            MagicCore::RenderSystem::Get()->GetMainCamera()->setPolygonMode(Ogre::PolygonMode::PM_SOLID);
            MagicCore::RenderSystem::Get()->RenderTextureMesh("TriMesh_UVUnfoldApp", "UVUnfoldGridMaterial", 
                triMesh, MagicCore::RenderSystem::MODEL_NODE_CENTER);
            InfoLog << "Display done" << std::endl;
        }
        else if (mDisplayMode == UVMESH_WIREFRAME)
        {
            InfoLog << "Display UVMESH_WIREFRAME" << std::endl;
            if (triMesh == NULL || (!triMesh->HasTriangleTexCoord()))
            {
                MagicCore::RenderSystem::Get()->HideRenderingObject("TriMesh_UVUnfoldApp");
                return;
            }
            MagicCore::RenderSystem::Get()->GetMainCamera()->setPolygonMode(Ogre::PolygonMode::PM_SOLID);
            MagicCore::RenderSystem::Get()->RenderUVMesh("TriMesh_UVUnfoldApp", "CookTorrance", triMesh, 
                MagicCore::RenderSystem::MODEL_NODE_CENTER);
            InfoLog << "Display done" << std::endl;
        }
    }

    void UVUnfoldApp::UpdateMarkDisplay()
    {
        if (!mHideMarks && ModelManager::Get()->GetMesh() && (mSnapIds.size() > 0 || mCurMarkCoords.size() > 0 || mLastCutVertexId != -1))
        {
            GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
            
            std::vector<GPP::Vector3> cutVerticesYellow;
            cutVerticesYellow.reserve(mCurMarkCoords.size() + 1);
            for (std::vector<GPP::Vector3>::iterator coordItr = mCurMarkCoords.begin(); coordItr != mCurMarkCoords.end(); ++coordItr)
            {
                cutVerticesYellow.push_back(*coordItr);
            }
            if (mLastCutVertexId != -1)
            {
                cutVerticesYellow.push_back(triMesh->GetVertexCoord(mLastCutVertexId));
            }
            MagicCore::RenderSystem::Get()->RenderPointList("CutVertexYellow_UVUnfoldApp", "SimplePoint_Large", 
                GPP::Vector3(0, 1, 0), cutVerticesYellow, MagicCore::RenderSystem::MODEL_NODE_CENTER);

            std::vector<GPP::Vector3> cutVerticesRed;
            cutVerticesRed.reserve(mSnapIds.size());
            for (std::vector<int>::iterator vitr = mSnapIds.begin(); vitr != mSnapIds.end(); ++vitr)
            {
                cutVerticesRed.push_back(triMesh->GetVertexCoord(*vitr));
            }
            MagicCore::RenderSystem::Get()->RenderPointList("CutVertexRed_UVUnfoldApp", "SimplePoint_Large", 
                GPP::Vector3(1, 0, 0), cutVerticesRed, MagicCore::RenderSystem::MODEL_NODE_CENTER);
        }
        else
        {
            MagicCore::RenderSystem::Get()->HideRenderingObject("CutVertexRed_UVUnfoldApp");
            MagicCore::RenderSystem::Get()->HideRenderingObject("CutVertexYellow_UVUnfoldApp");
        }
    }

    void UVUnfoldApp::InitTriMeshTexture()
    {
        if (mDistortionImage.data == NULL)
        {
            return;
        }
        
        if (mpTriMeshTexture.isNull())
        {
            int textureImageSize = mDistortionImage.rows;
            mpTriMeshTexture = Ogre::TextureManager::getSingleton().createManual(  
                "UVGridTexture",      // name  
                Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,  
                Ogre::TEX_TYPE_2D,   // type  
                textureImageSize,  // width  
                textureImageSize, // height  
                0,                   // number of mipmaps  
                Ogre::PF_B8G8R8A8,   // pixel format  
                Ogre::TU_DYNAMIC_WRITE_ONLY_DISCARDABLE // usage, for textures updated very often  
                );

            // Get the pixel buffer  
            Ogre::HardwarePixelBufferSharedPtr pixelBuffer = mpTriMeshTexture->getBuffer();  
            cv::Mat textureImage = mDistortionImage;;
            // Lock the pixel buffer and get a pixel box  
            unsigned char* buffer = static_cast<unsigned char*>(  
                pixelBuffer->lock(0, textureImageSize * textureImageSize * 4, Ogre::HardwareBuffer::HBL_DISCARD) ); 
            int imgW = textureImage.cols;
            int imgH = textureImage.rows;
            for(int y = 0; y < textureImageSize; ++y)  
            {  
                for(int x = 0; x < textureImageSize; ++x)  
                {
                    if (x < imgW && y < imgH)
                    {
                        const unsigned char* pixel = textureImage.ptr(imgH - y - 1, x);
                        *buffer++ = pixel[0];
                        *buffer++ = pixel[1];
                        *buffer++ = pixel[2];
                        *buffer++ = 255;
                    }
                    else
                    {
                        *buffer++ = 255;
                        *buffer++ = 255;
                        *buffer++ = 255;
                        *buffer++ = 255;
                    } 
                }  
            }  
            // Unlock the pixel buffer  
            pixelBuffer->unlock();
        }
  
        if (Ogre::MaterialManager::getSingleton().getByName("UVUnfoldGridMaterial", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME).isNull())
        {
            Ogre::MaterialPtr material = Ogre::MaterialManager::getSingleton().create("UVUnfoldGridMaterial", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME); 
            material->getTechnique(0)->getPass(0)->createTextureUnitState("UVGridTexture");
            material->getTechnique(0)->getPass(0)->setDiffuse(0.7, 0.7, 0.7, 1.0);
            material->getTechnique(0)->getPass(0)->setSpecular(0.55, 0.55, 0.55, 1.0);
            material->getTechnique(0)->getPass(0)->setShininess(100);
        }
    }

    void UVUnfoldApp::GenerateSplitMesh()
    {
        if (!mCutLineList.empty())
        {
            GPP::SplitMesh::SplitByLines(ModelManager::Get()->GetMesh(), mCutLineList);
            ModelManager::Get()->GetMesh()->UpdateNormal();
            ClearSplitData();
            InsertHolesToSnapIds();
        }
    }

    void UVUnfoldApp::UnifyTextureCoords(std::vector<double>& texCoords, double scaleValue)
    {
        int vertexCount = texCoords.size() / 2;
        GPP::Real texCoordMax_X = texCoords.at(0);
        GPP::Real texCoordMax_Y = texCoords.at(1);
        GPP::Real texCoordMin_X = texCoords.at(0);
        GPP::Real texCoordMin_Y = texCoords.at(1);
        for (GPP::Int vid = 1; vid < vertexCount; vid++)
        {
            if (texCoords.at(vid * 2) > texCoordMax_X)
            {
                texCoordMax_X = texCoords.at(vid * 2);
            }
            else if (texCoords.at(vid * 2) < texCoordMin_X)
            {
                texCoordMin_X = texCoords.at(vid * 2);
            }
            if (texCoords.at(vid * 2 + 1) > texCoordMax_Y)
            {
                texCoordMax_Y = texCoords.at(vid * 2 + 1);
            }
            else if (texCoords.at(vid * 2 + 1) < texCoordMin_Y)
            {
                texCoordMin_Y = texCoords.at(vid * 2 + 1);
            }
        }

        GPP::Real range_X = texCoordMax_X - texCoordMin_X;
        GPP::Real range_Y = texCoordMax_Y - texCoordMin_Y;
        GPP::Real range_max = range_X > range_Y ? range_X : range_Y;
        if (range_max > GPP::REAL_TOL)
        {
            GPP::Real scaleV = scaleValue / range_max;
            GPP::Real center_X = (texCoordMax_X + texCoordMin_X) / 2.0;
            GPP::Real center_Y = (texCoordMax_Y + texCoordMin_Y) / 2.0;
            for (GPP::Int vid = 0; vid < vertexCount; vid++)
            {
                texCoords.at(vid * 2) = (texCoords.at(vid * 2) - center_X) * scaleV + 0.5;
                texCoords.at(vid * 2 + 1) = (texCoords.at(vid * 2 + 1) - center_Y) * scaleV + 0.5;
            }
        }
    }
}

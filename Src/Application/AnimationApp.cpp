#include "stdafx.h"
#include <process.h>
#include "AnimationApp.h"
#include "AnimationAppUI.h"
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
    AnimationApp::AnimationApp() :
        mpUI(NULL),
        mpViewTool(NULL),
        mDeformType(DT_NONE),
        mDeformPointList(NULL),
        mDeformMesh(NULL),
        mControlIds(),
        mIsDeformationInitialised(false),
        mPickControlId(-1),
        mPickTargetCoord(),
        mMousePressdCoord(),
        mControlFlags(),
        mRightMouseType(DEFORM),
        mAddSelection(true),
        mFirstAlert(true),
        mTargetControlCoords(),
        mTargetControlIds()
    {
    }

    AnimationApp::~AnimationApp()
    {
        GPPFREEPOINTER(mpUI);
        GPPFREEPOINTER(mpViewTool);
        GPPFREEPOINTER(mDeformPointList);
        GPPFREEPOINTER(mDeformMesh);
    }

    void AnimationApp::ClearData()
    {
        GPPFREEPOINTER(mpUI);
        GPPFREEPOINTER(mpViewTool);
        ClearMeshData();
        ClearPointCloudData();
        mRightMouseType = DEFORM;
        GPPFREEPOINTER(mDeformPointList);
        GPPFREEPOINTER(mDeformMesh);
        std::vector<GPP::Int>().swap(mControlIds);
        std::vector<int>().swap(mControlFlags);
        std::vector<GPP::Vector3>().swap(mTargetControlCoords);
        std::vector<int>().swap(mTargetControlIds);
    }

    void AnimationApp::ClearPointCloudData()
    {
        mIsDeformationInitialised = false;
        mPickControlId = -1;
        mTargetControlCoords.clear();
        mTargetControlIds.clear();
        mTargetControlCoords.clear();
        mControlIds.clear();
        GPPFREEPOINTER(mDeformPointList);
    }
    
    void AnimationApp::ClearMeshData()
    {
        mIsDeformationInitialised = false;
        mDeformType = DT_NONE;
        mPickControlId = -1;
        mTargetControlCoords.clear();
        mTargetControlIds.clear();
        mTargetControlCoords.clear();
        mControlIds.clear();
        GPPFREEPOINTER(mDeformMesh);
    }

    bool AnimationApp::Enter()
    {
        InfoLog << "Enter AnimationApp" << std::endl; 
        if (mpUI == NULL)
        {
            mpUI = new AnimationAppUI;
        }
        mpUI->Setup();
        SetupScene();
        UpdateModelRendering();
        return true;
    }

    bool AnimationApp::Update(double timeElapsed)
    {
        return true;
    }

    bool AnimationApp::Exit()
    {
        InfoLog << "Exit AnimationApp" << std::endl; 
        if (mpUI != NULL)
        {
            mpUI->Shutdown();
        }
        ClearData();
        ShutdownScene();
        return true;
    }

    void AnimationApp::PickControlPoint(int mouseCoordX, int mouseCoordY)
    {
        GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
        GPP::PointCloud* pointCloud = ModelManager::Get()->GetPointCloud();

        GPP::Vector2 mouseCoord(mouseCoordX * 2.0 / MagicCore::RenderSystem::Get()->GetRenderWindow()->getWidth() - 1.0, 
                    1.0 - mouseCoordY * 2.0 / MagicCore::RenderSystem::Get()->GetRenderWindow()->getHeight());
        if (MagicCore::RenderSystem::Get()->GetSceneManager()->hasSceneNode("ModelNode") == false || mControlIds.empty())
        {
            return;
        }
        double pointSizeSquared = 0.01 * 0.01;
        Ogre::Matrix4 worldM = MagicCore::RenderSystem::Get()->GetSceneManager()->getSceneNode("ModelNode")->_getFullTransform();
        Ogre::Matrix4 viewM  = MagicCore::RenderSystem::Get()->GetMainCamera()->getViewMatrix();
        Ogre::Matrix4 projM  = MagicCore::RenderSystem::Get()->GetMainCamera()->getProjectionMatrix();
        Ogre::Matrix4 wvpM   = projM * viewM * worldM;
        double minZ = 1.0e10;
        mPickControlId = -1;
        GPP::Int pointCount = mControlIds.size();
        for (GPP::Int pid = 0; pid < pointCount; pid++)
        {
            if (mControlFlags.at(pid) == 1)
            {
                continue;
            }
            GPP::Vector3 coord;
            if (triMesh)
            {
                coord = triMesh->GetVertexCoord(mControlIds.at(pid));
            }
            else
            {
                coord = pointCloud->GetPointCoord(mControlIds.at(pid));
            }
            Ogre::Vector3 ogreCoord(coord[0], coord[1], coord[2]);
            ogreCoord = wvpM * ogreCoord;
            GPP::Vector2 screenCoord(ogreCoord.x, ogreCoord.y);
            if ((screenCoord - mouseCoord).LengthSquared() < pointSizeSquared)
            {
                if (ogreCoord.z < minZ)
                {
                    minZ = ogreCoord.z;
                    mPickControlId = pid;
                }
            }
        }
        if (mPickControlId != -1)
        {
            /*InfoLog << "mControlNeighbors.at(mPickControlId).size = " << mControlNeighbors.at(mPickControlId).size() << std::endl;
            std::vector<GPP::Vector3> startCoords, endCoords;
            if (mpTriMesh)
            {
                for (std::set<int>::iterator itr = mControlNeighbors.at(mPickControlId).begin(); itr != mControlNeighbors.at(mPickControlId).end(); ++itr)
                {
                    startCoords.push_back(mpTriMesh->GetVertexCoord(mControlIds.at(mPickControlId)));
                    endCoords.push_back(mpTriMesh->GetVertexCoord(mControlIds.at(*itr)));
                }
            }
            else
            {
                for (std::set<int>::iterator itr = mControlNeighbors.at(mPickControlId).begin(); itr != mControlNeighbors.at(mPickControlId).end(); ++itr)
                {
                    startCoords.push_back(mpPointCloud->GetPointCoord(mControlIds.at(mPickControlId)));
                    endCoords.push_back(mpPointCloud->GetPointCoord(mControlIds.at(*itr)));
                }
            }
            MagicCore::RenderSystem::Get()->RenderLineSegments("Test", "Simple_Line", startCoords, endCoords);*/
            if (triMesh)
            {
                mPickTargetCoord = triMesh->GetVertexCoord(mControlIds.at(mPickControlId));
            }
            else
            {
                mPickTargetCoord = pointCloud->GetPointCoord(mControlIds.at(mPickControlId));
            }
        }
    }

    void AnimationApp::DragControlPoint(int mouseCoordX, int mouseCoordY, bool mouseReleased)
    {
        if (mPickControlId == -1)
        {
            return;
        }
        GPP::Vector2 mouseCoord(mouseCoordX * 2.0 / MagicCore::RenderSystem::Get()->GetRenderWindow()->getWidth() - 1.0, 
                    1.0 - mouseCoordY * 2.0 / MagicCore::RenderSystem::Get()->GetRenderWindow()->getHeight());
        Ogre::Ray ray = MagicCore::RenderSystem::Get()->GetMainCamera()->getCameraToViewportRay((mouseCoord[0] + 1.0) / 2.0, (1.0 - mouseCoord[1]) / 2.0);
        Ogre::Vector3 originCoord = ray.getOrigin();
        Ogre::Vector3 dir = ray.getDirection();
        Ogre::Matrix4 worldM = MagicCore::RenderSystem::Get()->GetSceneManager()->getSceneNode("ModelNode")->_getFullTransform();
        Ogre::Vector3 ogrePickCoord(mPickTargetCoord[0], mPickTargetCoord[1], mPickTargetCoord[2]);
        ogrePickCoord = worldM * ogrePickCoord;
        double t = dir.dotProduct(ogrePickCoord - originCoord);
        Ogre::Vector3 targetCoord = originCoord + dir * t;
        Ogre::Matrix4 worldMInverse = worldM.inverse();
        targetCoord = worldMInverse * targetCoord;
        mPickTargetCoord = GPP::Vector3(targetCoord[0], targetCoord[1], targetCoord[2]);
        if (mouseReleased)
        {
            int targetIndex = -1;
            for (int tid = 0; tid < mTargetControlIds.size(); tid++)
            {
                if (mTargetControlIds.at(tid) == mPickControlId)
                {
                    targetIndex = tid;
                }
            }
            if (targetIndex == -1)
            {
                mTargetControlIds.push_back(mPickControlId);
                mTargetControlCoords.push_back(mPickTargetCoord);
                mControlFlags.at(mPickControlId) = 2;
                if (mDeformMesh)
                {
                    mIsDeformationInitialised = false;
                }
            }
            else
            {
                mTargetControlCoords.at(targetIndex) = mPickTargetCoord;
            }
        }
        UpdateControlRendering();
    }

    void AnimationApp::UpdateDeformation(int mouseCoordX, int mouseCoordY, bool isAccurate)
    {
        if (mPickControlId == -1 || mControlFlags.at(mPickControlId) != 2)
        {
            return;
        }
        GPP::Vector2 mouseCoord(mouseCoordX * 2.0 / MagicCore::RenderSystem::Get()->GetRenderWindow()->getWidth() - 1.0, 
                    1.0 - mouseCoordY * 2.0 / MagicCore::RenderSystem::Get()->GetRenderWindow()->getHeight());
        Ogre::Ray ray = MagicCore::RenderSystem::Get()->GetMainCamera()->getCameraToViewportRay((mouseCoord[0] + 1.0) / 2.0, (1.0 - mouseCoord[1]) / 2.0);
        Ogre::Vector3 originCoord = ray.getOrigin();
        Ogre::Vector3 dir = ray.getDirection();
        Ogre::Matrix4 worldM = MagicCore::RenderSystem::Get()->GetSceneManager()->getSceneNode("ModelNode")->_getFullTransform();
        Ogre::Vector3 ogrePickCoord(mPickTargetCoord[0], mPickTargetCoord[1], mPickTargetCoord[2]);
        ogrePickCoord = worldM * ogrePickCoord;
        double t = dir.dotProduct(ogrePickCoord - originCoord);
        Ogre::Vector3 targetCoord = originCoord + dir * t;
        Ogre::Matrix4 worldMInverse = worldM.inverse();
        targetCoord = worldMInverse * targetCoord;
        mPickTargetCoord = GPP::Vector3(targetCoord[0], targetCoord[1], targetCoord[2]);
        std::vector<GPP::Vector3> targetCoords;
        targetCoords.push_back(mPickTargetCoord);  
        GPP::ErrorCode res = GPP_NO_ERROR;
        if (mDeformPointList)
        {
            std::vector<int> controlIndex;
            controlIndex.push_back(mPickControlId);
            std::vector<bool> controlFixFlags;
            controlFixFlags.reserve(mControlFlags.size());
            for (std::vector<int>::iterator itr = mControlFlags.begin(); itr != mControlFlags.end(); ++itr)
            {
                if (*itr == 1)
                {
                    controlFixFlags.push_back(1);
                }
                else
                {
                    controlFixFlags.push_back(0);
                }
            }
            res = mDeformPointList->Deform(controlIndex, targetCoords, controlFixFlags);
        }
        else if (mDeformMesh)
        {
            std::vector<int> targetVertexIds;
            targetVertexIds.push_back(mControlIds.at(mPickControlId));
            if (isAccurate)
            {
                res = mDeformMesh->Deform(targetVertexIds, targetCoords, GPP::DEFORM_MESH_TYPE_ACCURATE);
            }
            else
            {
                res = mDeformMesh->Deform(targetVertexIds, targetCoords, GPP::DEFORM_MESH_TYPE_FAST);
            }
            ModelManager::Get()->GetMesh()->UpdateNormal();
        }
        if (res != GPP_NO_ERROR)
        {
            MessageBox(NULL, "三维模型变形失败", "温馨提示", MB_OK);
            mPickControlId = -1;
            return;
        }
        UpdateModelRendering();
        UpdateControlRendering();
    }

    void AnimationApp::SelectControlPointByRectangle(int startCoordX, int startCoordY, int endCoordX, int endCoordY)
    {
        GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
        GPP::PointCloud* pointCloud = ModelManager::Get()->GetPointCloud();

        GPP::Vector2 pos0(startCoordX * 2.0 / MagicCore::RenderSystem::Get()->GetRenderWindow()->getWidth() - 1.0, 
                    1.0 - startCoordY * 2.0 / MagicCore::RenderSystem::Get()->GetRenderWindow()->getHeight());
        GPP::Vector2 pos1(endCoordX * 2.0 / MagicCore::RenderSystem::Get()->GetRenderWindow()->getWidth() - 1.0, 
                    1.0 - endCoordY * 2.0 / MagicCore::RenderSystem::Get()->GetRenderWindow()->getHeight());
        double minX = (pos0[0] < pos1[0]) ? pos0[0] : pos1[0];
        double maxX = (pos0[0] > pos1[0]) ? pos0[0] : pos1[0];
        double minY = (pos0[1] < pos1[1]) ? pos0[1] : pos1[1];
        double maxY = (pos0[1] > pos1[1]) ? pos0[1] : pos1[1];
        Ogre::Matrix4 worldM = MagicCore::RenderSystem::Get()->GetSceneManager()->getSceneNode("ModelNode")->_getFullTransform();
        Ogre::Matrix4 viewM  = MagicCore::RenderSystem::Get()->GetMainCamera()->getViewMatrix();
        Ogre::Matrix4 projM  = MagicCore::RenderSystem::Get()->GetMainCamera()->getProjectionMatrix();
        Ogre::Matrix4 wvpM   = projM * viewM * worldM;
        GPP::Int pointCount = mControlIds.size();
        int controlFlag = mAddSelection ? 0 : 1;
        for (GPP::Int pid = 0; pid < pointCount; pid++)
        {
            GPP::Vector3 coord;
            if (triMesh)
            {
                coord = triMesh->GetVertexCoord(mControlIds.at(pid));
            }
            else
            {
                coord = pointCloud->GetPointCoord(mControlIds.at(pid));
            }
            Ogre::Vector3 ogreCoord(coord[0], coord[1], coord[2]);
            ogreCoord = wvpM * ogreCoord;
            if (ogreCoord.x > minX && ogreCoord.x < maxX && ogreCoord.y > minY && ogreCoord.y < maxY)
            {
                mControlFlags.at(pid) = controlFlag;
                if (mDeformMesh)
                {
                    mIsDeformationInitialised = false;
                }
            }
        }
    }

    void AnimationApp::UpdateRectangleRendering(int startCoordX, int startCoordY, int endCoordX, int endCoordY)
    {
        Ogre::ManualObject* pMObj = NULL;
        Ogre::SceneManager* pSceneMgr = MagicCore::RenderSystem::Get()->GetSceneManager();
        if (pSceneMgr->hasManualObject("PickRectangleObj"))
        {
            pMObj = pSceneMgr->getManualObject("PickRectangleObj");
            pMObj->clear();
        }
        else
        {
            pMObj = pSceneMgr->createManualObject("PickRectangleObj");
            pMObj->setRenderQueueGroup(Ogre::RENDER_QUEUE_OVERLAY);
            pMObj->setUseIdentityProjection(true);
            pMObj->setUseIdentityView(true);
            if (pSceneMgr->hasSceneNode("PickRectangleNode"))
            {
                pSceneMgr->getSceneNode("PickRectangleNode")->attachObject(pMObj);
            }
            else
            {
                pSceneMgr->getRootSceneNode()->createChildSceneNode("PickRectangleNode")->attachObject(pMObj);
            }
        }
        GPP::Vector2 pos0(startCoordX * 2.0 / MagicCore::RenderSystem::Get()->GetRenderWindow()->getWidth() - 1.0, 
                    1.0 - startCoordY * 2.0 / MagicCore::RenderSystem::Get()->GetRenderWindow()->getHeight());
        GPP::Vector2 pos1(endCoordX * 2.0 / MagicCore::RenderSystem::Get()->GetRenderWindow()->getWidth() - 1.0, 
                    1.0 - endCoordY * 2.0 / MagicCore::RenderSystem::Get()->GetRenderWindow()->getHeight());
        pMObj->begin("SimpleLine", Ogre::RenderOperation::OT_LINE_STRIP);
        pMObj->position(pos0[0], pos0[1], -1);
        pMObj->colour(0.09, 0.48627, 0.69);
        pMObj->position(pos0[0], pos1[1], -1);
        pMObj->colour(0.09, 0.48627, 0.69);
        pMObj->position(pos1[0], pos1[1], -1);
        pMObj->colour(0.09, 0.48627, 0.69);
        pMObj->position(pos1[0], pos0[1], -1);
        pMObj->colour(0.09, 0.48627, 0.69);
        pMObj->position(pos0[0], pos0[1], -1);
        pMObj->colour(0.09, 0.48627, 0.69);
        pMObj->end();
    }

    void AnimationApp::ClearRectangleRendering()
    {
        MagicCore::RenderSystem::Get()->HideRenderingObject("PickRectangleObj");
    }

    bool AnimationApp::MouseMoved( const OIS::MouseEvent &arg )
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
        else if (arg.state.buttonDown(OIS::MB_Right) && mControlIds.size() > 0)
        {
            if (mRightMouseType == SELECT)
            {
                UpdateRectangleRendering(mMousePressdCoord[0], mMousePressdCoord[1], arg.state.X.abs, arg.state.Y.abs);
            }
            else if (mRightMouseType == DEFORM && mPickControlId >= 0)
            {
                UpdateDeformation(arg.state.X.abs, arg.state.Y.abs, false);
            }
            else if (mRightMouseType == MOVE && mPickControlId >= 0)
            {
                DragControlPoint(arg.state.X.abs, arg.state.Y.abs, false);
            }
        }
        
        return true;
    }

    bool AnimationApp::MousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
    {
        if ((id == OIS::MB_Middle || id == OIS::MB_Left) && mpViewTool != NULL)
        {
            mpViewTool->MousePressed(arg.state.X.abs, arg.state.Y.abs);
        }
        if ((id == OIS::MB_Right) && mControlIds.size() > 0)
        {
            if (mRightMouseType == SELECT)
            {
                mMousePressdCoord[0] = arg.state.X.abs;
                mMousePressdCoord[1] = arg.state.Y.abs;
            }
            else
            {
                PickControlPoint(arg.state.X.abs, arg.state.Y.abs);
            }
        }
        return true;
    }

    bool AnimationApp::MouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
    {
        if (mpViewTool)
        {
            mpViewTool->MouseReleased();
        }
        if ((id == OIS::MB_Right) && mControlIds.size() > 0)
        {
            if (mRightMouseType == SELECT)
            {
                SelectControlPointByRectangle(mMousePressdCoord[0], mMousePressdCoord[1], arg.state.X.abs, arg.state.Y.abs);
                ClearRectangleRendering();
                UpdateControlRendering();
            }
            else if (mRightMouseType == DEFORM && mPickControlId >= 0)
            {
                UpdateDeformation(arg.state.X.abs, arg.state.Y.abs, true);
                mPickControlId = -1;
            }
            else if (mRightMouseType == MOVE && mPickControlId >= 0)
            {
                DragControlPoint(arg.state.X.abs, arg.state.Y.abs, true);
                mPickControlId = -1;
            }
        }
        return true;
    }

    bool AnimationApp::KeyPressed( const OIS::KeyEvent &arg )
    {
        if (arg.key == OIS::KC_F)
        {
            MagicCore::RenderSystem::Get()->GetMainCamera()->setPolygonMode(Ogre::PolygonMode::PM_SOLID);
            Ogre::Material* material = dynamic_cast<Ogre::Material*>(Ogre::MaterialManager::getSingleton().getByName("CookTorrance").getPointer());
            if (material)
            {
                material->setCullingMode(Ogre::CullingMode::CULL_NONE);
            }
        }
        else if (arg.key == OIS::KC_E)
        {
            MagicCore::RenderSystem::Get()->GetMainCamera()->setPolygonMode(Ogre::PolygonMode::PM_WIREFRAME);
            Ogre::Material* material = dynamic_cast<Ogre::Material*>(Ogre::MaterialManager::getSingleton().getByName("CookTorrance").getPointer());
            if (material)
            {
                material->setCullingMode(Ogre::CullingMode::CULL_CLOCKWISE);
            }
        }
        else if (arg.key == OIS::KC_V)
        {
            MagicCore::RenderSystem::Get()->GetMainCamera()->setPolygonMode(Ogre::PolygonMode::PM_POINTS);
            Ogre::Material* material = dynamic_cast<Ogre::Material*>(Ogre::MaterialManager::getSingleton().getByName("CookTorrance").getPointer());
            if (material)
            {
                material->setCullingMode(Ogre::CullingMode::CULL_CLOCKWISE);
            }
        }
        return true;
    }

    bool AnimationApp::ImportModel()
    {
        std::string fileName;
        char filterName[] = "OBJ Files(*.obj)\0*.obj\0STL Files(*.stl)\0*.stl\0OFF Files(*.off)\0*.off\0PLY Files(*.ply)\0*.ply\0ASC Files(*.asc)\0*.asc\0Geometry++ Point Cloud(*.gpc)\0*.gpc\0XYZ Files(*.xyz)\0*.xyz\0";
        if (MagicCore::ToolKit::FileOpenDlg(fileName, filterName))
        {
            size_t dotPos = fileName.rfind('.');
            if (dotPos == std::string::npos)
            {
                return false;
            }
            std::string extName = fileName;
            extName = extName.substr(dotPos + 1);
            bool updateMark = false;
            if (extName == std::string("obj") || extName == std::string("stl") || extName == std::string("off") || extName == std::string("ply"))
            {
                ModelManager::Get()->ClearPointCloud();
                if (ModelManager::Get()->ImportMesh(fileName) == false)
                {
                    MessageBox(NULL, "网格导入失败", "温馨提示", MB_OK);
                    return false;
                }
                    
                // Clear Data
                ClearMeshData();
                ClearPointCloudData();

                mpUI->ShowControlBar(false);
                mpUI->ShowVertexBar(false);
                // Update
                UpdateModelRendering();
                UpdateControlRendering();

                return true;
            }
            else if (extName == std::string("asc") || extName == std::string("gpc") || extName == std::string("xyz"))
            {
                ModelManager::Get()->ClearMesh();
                if (ModelManager::Get()->ImportPointCloud(fileName) == false)
                {
                    MessageBox(NULL, "点云导入失败", "温馨提示", MB_OK);
                    return false;
                }

                // Clear Data
                ClearMeshData();
                ClearPointCloudData();

                mpUI->ShowControlBar(false);
                mpUI->ShowVertexBar(false);
                // Update
                UpdateModelRendering();
                UpdateControlRendering();

                return true;
            }
        }
        return false;
    }

    void AnimationApp::SwitchDeformType(DeformType dt)
    {
        if (dt == DT_NONE)
        {
            mDeformType = DT_NONE;
            ClearMeshData();
            ClearPointCloudData();
            UpdateControlRendering();
        }
        else if (dt == DT_VERTEX)
        {
            if (mDeformType == DT_CONTROL_POINT)
            {
                ClearMeshData();
                ClearPointCloudData();
                UpdateControlRendering();
            }
            mDeformType = DT_VERTEX;
            if (ModelManager::Get()->GetPointCloud() != NULL)
            {
                MessageBox(NULL, "顶点变形只支持网格类型", "温馨提示", MB_OK);
                mpUI->ShowVertexBar(false);
                mDeformType = DT_NONE;
            }
            else if (ModelManager::Get()->GetMesh() == NULL)
            {
                MessageBox(NULL, "请导入网格", "温馨提示", MB_OK);
                mpUI->ShowVertexBar(false);
                mDeformType = DT_NONE;
            }
            else
            {
                mDeformMesh = new GPP::DeformMesh;
                int vertexCount = ModelManager::Get()->GetMesh()->GetVertexCount();
                InitControlPoint(vertexCount);
            }
        }
        else if (dt == DT_CONTROL_POINT)
        {
            if (mDeformType == DT_VERTEX)
            {
                ClearMeshData();
                UpdateControlRendering();
            }
            mDeformType = DT_CONTROL_POINT;
            if (ModelManager::Get()->GetPointCloud() != NULL)
            {
                mDeformPointList = new GPP::DeformPointList;
            }
            else if (ModelManager::Get()->GetMesh() != NULL)
            {
                mDeformMesh = new GPP::DeformMesh;
            }
            else
            {
                MessageBox(NULL, "请导入网格", "温馨提示", MB_OK);
                mpUI->ShowControlBar(false);
                mDeformType = DT_NONE;
            }
        }
    }

    void AnimationApp::InitControlPoint(int controlPointCount)
    {
        GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
        GPP::PointCloud* pointCloud = ModelManager::Get()->GetPointCloud();

        if (pointCloud == NULL && triMesh == NULL)
        {
            MessageBox(NULL, "请先导入点云或者网格模型", "温馨提示", MB_OK);
            return;
        }
        GPP::ErrorCode res = GPP_NO_ERROR;
        if (pointCloud)
        {
            res = mDeformPointList->Init(pointCloud, controlPointCount);
            if (res == GPP_NO_ERROR)
            {
                mDeformPointList->GetControlIds(mControlIds);
            }
            mIsDeformationInitialised = true;
        }
        else if (triMesh)
        {
            int vertexCount = triMesh->GetVertexCount();
            int sampleCount = vertexCount > controlPointCount ? controlPointCount : vertexCount;
            GPP::TriMeshPointList pointList(triMesh);
            int* sampleIndex = new int[sampleCount];
            res = GPP::SamplePointCloud::_UniformSamplePointList(&pointList, sampleCount, sampleIndex, 0, GPP::SAMPLE_QUALITY_HIGH);
            if (res == GPP_NO_ERROR)
            {
                mControlIds.clear();
                mControlIds.reserve(sampleCount);
                for (int sid = 0; sid < sampleCount; sid++)
                {
                    mControlIds.push_back(sampleIndex[sid]);
                }
            }
            GPPFREEARRAY(sampleIndex);
            mIsDeformationInitialised = false;
        }
        if (res == GPP_API_IS_NOT_AVAILABLE)
        {
            MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
            MagicCore::ToolKit::Get()->SetAppRunning(false);
        }
        if (res != GPP_NO_ERROR)
        {
            MessageBox(NULL, "变形初始化失败", "温馨提示", MB_OK);
            return;
        }
        mControlFlags = std::vector<int>(mControlIds.size(), 1);
        UpdateControlRendering();
    }

    void AnimationApp::InitControlDeformation()
    {
        if (mDeformMesh)
        {
            GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
            int vertexCount = triMesh->GetVertexCount();
            std::vector<bool> vertexFixFlags(vertexCount, 0);
            int controlSize = mControlFlags.size();
            for (int cid = 0; cid < controlSize; cid++)
            {
                if (mControlFlags.at(cid) != 0)
                {
                    vertexFixFlags.at(mControlIds.at(cid)) = 1;
                }
            }
#if MAKEDUMPFILE
            GPP::DumpOnce();
#endif
            GPP::ErrorCode res = mDeformMesh->Init(triMesh, vertexFixFlags);
            if (res == GPP_API_IS_NOT_AVAILABLE)
            {
                MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
                MagicCore::ToolKit::Get()->SetAppRunning(false);
            }
            if (res != GPP_NO_ERROR)
            {
                if (res == GPP_INVALID_INPUT)
                {
                    MessageBox(NULL, "网格有连通区域没有固定点", "温馨提示", MB_OK);
                }
                else
                {
                    MessageBox(NULL, "变形初始化失败", "温馨提示", MB_OK);
                }
                return;
            }
            mIsDeformationInitialised = true;
        }
    }

    void AnimationApp::DoControlDeformation()
    {
        if (mTargetControlCoords.empty())
        {
            return;
        }
        if (!mIsDeformationInitialised)
        {
            MessageBox(NULL, "变形没有初始化", "温馨提示", MB_OK);
            return;
        }
        GPP::ErrorCode res = GPP_NO_ERROR;
        if (mDeformPointList)
        {
            std::vector<bool> controlFixFlags;
            controlFixFlags.reserve(mControlFlags.size());
            for (std::vector<int>::iterator itr = mControlFlags.begin(); itr != mControlFlags.end(); ++itr)
            {
                if (*itr == 1)
                {
                    controlFixFlags.push_back(1);
                }
                else
                {
                    controlFixFlags.push_back(0);
                }
            }
            res = mDeformPointList->Deform(mTargetControlIds, mTargetControlCoords, controlFixFlags);
        }
        else if (mDeformMesh)
        {
            std::vector<int> targetVertexIds;
            for (std::vector<int>::iterator itr = mTargetControlIds.begin(); itr != mTargetControlIds.end(); ++itr)
            {
                targetVertexIds.push_back(mControlIds.at(*itr));
            }
#if MAKEDUMPFILE
            GPP::DumpOnce();
#endif
            res = mDeformMesh->Deform(targetVertexIds, mTargetControlCoords, GPP::DEFORM_MESH_TYPE_ACCURATE);
            ModelManager::Get()->GetMesh()->UpdateNormal();
        }
        if (res != GPP_NO_ERROR)
        {
            MessageBox(NULL, "三维模型变形失败", "温馨提示", MB_OK);
            return;
        }
        UpdateModelRendering();
        UpdateControlRendering();
    }

    void AnimationApp::SelectFreeControlPoint()
    {
        mRightMouseType = SELECT;
        mAddSelection = true;
        if (mFirstAlert)
        {
            mFirstAlert = false;
            MessageBox(NULL, "鼠标右键进入框选控制点模式(+)，点击选择按钮返回移动控制点模式", "温馨提示", MB_OK);
        }
    }
        
    void AnimationApp::ClearFreeControlPoint()
    {
        mRightMouseType = SELECT;
        mAddSelection = false;
        if (mFirstAlert)
        {
            mFirstAlert = false;
            MessageBox(NULL, "鼠标右键进入框选控制点模式(-)，点击选择按钮返回移动控制点模式", "温馨提示", MB_OK);
        }
    }

    //void AnimationApp::RealTimeDeform()
    //{
    //    if (!mIsDeformationInitialised)
    //    {
    //        MessageBox(NULL, "变形没有初始化", "温馨提示", MB_OK);
    //        return;
    //    }
    //    mRightMouseType = DEFORM;
    //    mTargetControlCoords.clear();
    //    mTargetControlIds.clear();
    //    UpdateModelRendering();
    //}

    void AnimationApp::MoveControlPoint()
    {
        mRightMouseType = MOVE;
    }

    void AnimationApp::SetupScene()
    {
        Ogre::SceneManager* sceneManager = MagicCore::RenderSystem::Get()->GetSceneManager();
        sceneManager->setAmbientLight(Ogre::ColourValue(0.3, 0.3, 0.3));
        Ogre::Light* light = sceneManager->createLight("AnimationApp_SimpleLight");
        light->setPosition(0, 0, 20);
        light->setDiffuseColour(0.8, 0.8, 0.8);
        light->setSpecularColour(0.5, 0.5, 0.5);

        InitViewTool();

        if (ModelManager::Get()->GetMesh() != NULL)
        {
            ClearPointCloudData();
            GPPFREEPOINTER(mDeformMesh);
            mDeformMesh = new GPP::DeformMesh;
        }
        else if (ModelManager::Get()->GetPointCloud() != NULL)
        {
            ClearMeshData();
            GPPFREEPOINTER(mDeformPointList);
            mDeformPointList = new GPP::DeformPointList;
        }
    }

    void AnimationApp::ShutdownScene()
    {
        Ogre::SceneManager* sceneManager = MagicCore::RenderSystem::Get()->GetSceneManager();
        sceneManager->setAmbientLight(Ogre::ColourValue::Black);
        sceneManager->destroyLight("AnimationApp_SimpleLight");
        //MagicCore::RenderSystem::Get()->SetupCameraDefaultParameter();
        /*if (MagicCore::RenderSystem::Get()->GetSceneManager()->hasSceneNode("ModelNode"))
        {
            MagicCore::RenderSystem::Get()->GetSceneManager()->getSceneNode("ModelNode")->resetToInitialState();
        }*/
        MagicCore::RenderSystem::Get()->HideRenderingObject("Model_AnimationApp");
        MagicCore::RenderSystem::Get()->HideRenderingObject("ControlPoint_Free_AnimationApp");
        MagicCore::RenderSystem::Get()->HideRenderingObject("ControlPoint_Fix_AnimationApp");
        MagicCore::RenderSystem::Get()->HideRenderingObject("ControlPoint_Handle_AnimationApp");
        MagicCore::RenderSystem::Get()->HideRenderingObject("ControlPoint_Target_AnimationApp");

        GPPFREEPOINTER(mDeformPointList);
    }

    void AnimationApp::InitViewTool()
    {
        if (mpViewTool == NULL)
        {
            mpViewTool = new MagicCore::ViewTool;
        }
    }

    void AnimationApp::UpdateModelRendering()
    {
        GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
        GPP::PointCloud* pointCloud = ModelManager::Get()->GetPointCloud();

        if (triMesh)
        {
            MagicCore::RenderSystem::Get()->RenderMesh("Model_AnimationApp", "CookTorrance", triMesh, 
                MagicCore::RenderSystem::MODEL_NODE_CENTER, NULL, NULL, true);
        }
        else if (pointCloud)
        {
            if (pointCloud->HasNormal())
            {
                MagicCore::RenderSystem::Get()->RenderPointCloud("Model_AnimationApp", "CookTorrancePoint", pointCloud);
            }
            else
            {
                MagicCore::RenderSystem::Get()->RenderPointCloud("Model_AnimationApp", "SimplePoint", pointCloud);
            }
        }
        else
        {
            MagicCore::RenderSystem::Get()->HideRenderingObject("Model_AnimationApp");
        }
    }

    void AnimationApp::UpdateControlRendering()
    {
        if (mControlIds.empty())
        {
            MagicCore::RenderSystem::Get()->HideRenderingObject("ControlPoint_Free_AnimationApp");
            MagicCore::RenderSystem::Get()->HideRenderingObject("ControlPoint_Fix_AnimationApp");
            MagicCore::RenderSystem::Get()->HideRenderingObject("ControlPoint_Handle_AnimationApp");
            MagicCore::RenderSystem::Get()->HideRenderingObject("ControlPoint_Target_AnimationApp");
        }
        else
        {
            GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
            GPP::PointCloud* pointCloud = ModelManager::Get()->GetPointCloud();

            std::vector<GPP::Vector3> fixCoords;
            std::vector<GPP::Vector3> freeCoords;
            std::vector<GPP::Vector3> handleCoords;
            std::vector<GPP::Vector3> targetCoords;
            if (triMesh)
            {
                for (int cid = 0; cid < mControlIds.size(); cid++)
                {
                    if (mControlFlags.at(cid) == 0)
                    {
                        freeCoords.push_back(triMesh->GetVertexCoord(mControlIds.at(cid)));
                    }
                    else if (mControlFlags.at(cid) == 1)
                    {
                        fixCoords.push_back(triMesh->GetVertexCoord(mControlIds.at(cid)));
                    }
                    else if (mControlFlags.at(cid) == 2)
                    {
                        handleCoords.push_back(triMesh->GetVertexCoord(mControlIds.at(cid)));
                    }
                }
            }
            else if (pointCloud)
            {
                for (int cid = 0; cid < mControlIds.size(); cid++)
                {
                    if (mControlFlags.at(cid) == 0)
                    {
                        freeCoords.push_back(pointCloud->GetPointCoord(mControlIds.at(cid)));
                    }
                    else if (mControlFlags.at(cid) == 1)
                    {
                        fixCoords.push_back(pointCloud->GetPointCoord(mControlIds.at(cid)));
                    }
                    else if (mControlFlags.at(cid) == 2)
                    {
                        handleCoords.push_back(pointCloud->GetPointCoord(mControlIds.at(cid)));
                    }
                }
            }            
            MagicCore::RenderSystem::Get()->RenderPointList("ControlPoint_Free_AnimationApp", "SimplePoint_Large", GPP::Vector3(0, 0, 1), freeCoords, MagicCore::RenderSystem::MODEL_NODE_CENTER);
            MagicCore::RenderSystem::Get()->RenderPointList("ControlPoint_Handle_AnimationApp", "SimplePoint_Large", GPP::Vector3(0, 1, 0), handleCoords, MagicCore::RenderSystem::MODEL_NODE_CENTER);
            if (mDeformType == AnimationApp::DT_CONTROL_POINT)
            {
                MagicCore::RenderSystem::Get()->RenderPointList("ControlPoint_Fix_AnimationApp", "SimplePoint_Large", GPP::Vector3(1, 0, 0), fixCoords, MagicCore::RenderSystem::MODEL_NODE_CENTER);
            }

            if (mTargetControlCoords.size() > 0)
            {
                MagicCore::RenderSystem::Get()->RenderPointList("ControlPoint_Target_AnimationApp", "SimplePoint_Large", GPP::Vector3(0.5, 0.5, 0.5), mTargetControlCoords, MagicCore::RenderSystem::MODEL_NODE_CENTER);
            }
            else
            {
                MagicCore::RenderSystem::Get()->HideRenderingObject("ControlPoint_Target_AnimationApp");
            }
        }
    }
}

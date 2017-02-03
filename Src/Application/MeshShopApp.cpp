#include "stdafx.h"
#include <process.h>
#include "MeshShopApp.h"
#include "MeshShopAppUI.h"
#include "PointShopApp.h"
#include "ReliefApp.h"
#include "TextureApp.h"
#include "MeasureApp.h"
#include "AppManager.h"
#include "ModelManager.h"
#include "../Common/LogSystem.h"
#include "../Common/ToolKit.h"
#include "../Common/ViewTool.h"
#include "../Common/ScriptSystem.h"
#include "MagicMesh.h"
#if DEBUGDUMPFILE
#include "DumpFillMeshHole.h"
#endif

namespace MagicApp
{
    static unsigned __stdcall RunThread(void *arg)
    {
        MeshShopApp* app = (MeshShopApp*)arg;
        if (app == NULL)
        {
            return 0;
        }
        app->DoCommand(false);
        return 1;
    }

    MeshShopApp::MeshShopApp() :
        mpUI(NULL),
        mpViewTool(NULL),
        mDisplayMode(0),
#if DEBUGDUMPFILE
        mpDumpInfo(NULL),
#endif
        mShowHoleLoopIds(),
        mBoundarySeedIds(),
        mTargetVertexCount(0),
        mFillHoleType(0),
        mCommandType(NONE),
        mUpdateMeshRendering(false),
        mUpdateHoleRendering(false),
        mIsCommandInProgress(false),
        mVertexSelectFlag(),
        mRightMouseType(MOVE),
        mMousePressdCoord(),
        mIgnoreBack(true),
        mFilterPositionWeight(1.0),
        mIsFlatRenderingMode(true),
        mSharpAngle(0),
        mEnhanceIntensity(0)
    {
    }

    MeshShopApp::~MeshShopApp()
    {
        GPPFREEPOINTER(mpUI);
        GPPFREEPOINTER(mpViewTool);
#if DEBUGDUMPFILE
        GPPFREEPOINTER(mpDumpInfo);
#endif
    }

    bool MeshShopApp::Enter()
    {
        InfoLog << "Enter MeshShopApp" << std::endl; 
        if (mpUI == NULL)
        {
            mpUI = new MeshShopAppUI;
        }
        mpUI->Setup();
        SetupScene();
        ResetSelection();
        mRightMouseType = MOVE;
        InitViewTool();
        UpdateMeshRendering();
        return true;
    }

    bool MeshShopApp::Update(double timeElapsed)
    {
        if (MagicCore::ScriptSystem::Get()->IsOnRunningScript())
        {
            return false;
        }

        if (mpUI && mpUI->IsProgressbarVisible())
        {
            int progressValue = int(GPP::GetApiProgress() * 100.0);
            mpUI->SetProgressbar(progressValue);
        }
        if (mUpdateMeshRendering)
        {
            UpdateMeshRendering();
            mUpdateMeshRendering = false;
        }
        if (mUpdateHoleRendering)
        {
            UpdateHoleRendering();
            mUpdateHoleRendering = false;
        }
        if (mUpdateBridgeRendering)
        {
            UpdateBridgeRendering();
            mUpdateBridgeRendering = false;
        }
        return true;
    }

    bool MeshShopApp::Exit()
    {
        InfoLog << "Exit MeshShopApp" << std::endl; 
        ShutdownScene();
        if (mpUI != NULL)
        {
            mpUI->Shutdown();
        }
        ClearData();
        return true;
    }

    bool MeshShopApp::MouseMoved( const OIS::MouseEvent &arg )
    {
        if (arg.state.buttonDown(OIS::MB_Middle) && mpViewTool != NULL)
        {
            mpViewTool->MouseMoved(arg.state.X.abs, arg.state.Y.abs, MagicCore::ViewTool::MM_MIDDLE_DOWN);
        }
        else if (arg.state.buttonDown(OIS::MB_Right) && mRightMouseType == MOVE && mpViewTool != NULL)
        {
            mpViewTool->MouseMoved(arg.state.X.abs, arg.state.Y.abs, MagicCore::ViewTool::MM_RIGHT_DOWN);
        }
        else if (arg.state.buttonDown(OIS::MB_Left) && mpViewTool != NULL)
        {
            mpViewTool->MouseMoved(arg.state.X.abs, arg.state.Y.abs, MagicCore::ViewTool::MM_LEFT_DOWN);
        } 
        else if ((mRightMouseType == SELECT_ADD || mRightMouseType == SELECT_DELETE || mRightMouseType == SELECT_BRIDGE) && arg.state.buttonDown(OIS::MB_Right))
        {
            UpdateRectangleRendering(mMousePressdCoord[0], mMousePressdCoord[1], arg.state.X.abs, arg.state.Y.abs);
        }

        return true;
    }

    bool MeshShopApp::MousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
    {
        if ((id == OIS::MB_Middle || id == OIS::MB_Left || (id == OIS::MB_Right && mRightMouseType == MOVE)) && mpViewTool != NULL)
        {
            mpViewTool->MousePressed(arg.state.X.abs, arg.state.Y.abs);
        }
        else if (arg.state.buttonDown(OIS::MB_Right) && ModelManager::Get()->GetMesh() && (mRightMouseType == SELECT_ADD || mRightMouseType == SELECT_DELETE || mRightMouseType == SELECT_BRIDGE))
        {
            mMousePressdCoord[0] = arg.state.X.abs;
            mMousePressdCoord[1] = arg.state.Y.abs;
        }
        return true;
    }

    bool MeshShopApp::MouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
    {
        if ((id == OIS::MB_Middle || id == OIS::MB_Left || (id == OIS::MB_Right && mRightMouseType == MOVE)) && mpViewTool != NULL)
        {
            mpViewTool->MouseReleased(); 
        }
        else if (id == OIS::MB_Right && ModelManager::Get()->GetMesh() && (mRightMouseType == SELECT_ADD || mRightMouseType == SELECT_DELETE))
        {
            SelectControlPointByRectangle(mMousePressdCoord[0], mMousePressdCoord[1], arg.state.X.abs, arg.state.Y.abs);
            ClearRectangleRendering();
            UpdateMeshRendering();
        }
        else if (id == OIS::MB_Right && ModelManager::Get()->GetMesh() && (mRightMouseType == SELECT_BRIDGE))
        {
            SelectControlPointByRectangle(mMousePressdCoord[0], mMousePressdCoord[1], arg.state.X.abs, arg.state.Y.abs);
            ClearRectangleRendering();
            DoBridgeEdges();
            UpdateMeshRendering();
            UpdateBridgeRendering();
        }

        return true;
    }

    static void CollectTriMeshVerticesColorFields(const GPP::TriMesh* triMesh, std::vector<GPP::Real> *vertexColorFields)
    {
        if (triMesh == NULL || vertexColorFields == NULL)
        {
            return;
        }
        GPP::Int verticeSize = triMesh->GetVertexCount();
        GPP::Int dim = 3;
        vertexColorFields->resize(verticeSize * dim, 0.0);
        for (GPP::Int vid = 0; vid < verticeSize; ++vid)
        {
            GPP::Vector3 color = triMesh->GetVertexColor(vid);
            GPP::Int offset = vid * 3;
            vertexColorFields->at(offset) = color[0];
            vertexColorFields->at(offset+1) = color[1];
            vertexColorFields->at(offset+2) = color[2];
        }
    }

    static GPP::Vector3 TrimVector(const GPP::Vector3& vec, GPP::Real minValue, GPP::Real maxValue)
    {
        GPP::Vector3 out(vec);
        for (GPP::Int ii = 0; ii < 3; ++ii)
        {
            if (out[ii] < minValue)
            {
                out[ii] = minValue;
            }
            else if (out[ii] > maxValue)
            {
                out[ii] = maxValue;
            }
        }
        return out;
    }

    static void UpdateTriMeshVertexColors(GPP::TriMesh *triMesh, GPP::Int oldTriMeshVertexSize, const std::vector<GPP::Real>& insertedVertexFields)
    {
        if (triMesh == NULL || insertedVertexFields.empty())
        {
            return;
        }
        GPP::Int insertedVertexSize = (triMesh->GetVertexCount() - oldTriMeshVertexSize);
        if (insertedVertexFields.size() != insertedVertexSize * 3)
        {
            return;
        }
        for (GPP::Int vid = 0; vid < insertedVertexSize; ++vid)
        {
            GPP::Int newVertexId = vid + oldTriMeshVertexSize;
            GPP::Vector3 color(insertedVertexFields[vid * 3 + 0], insertedVertexFields[vid * 3 + 1], insertedVertexFields[vid * 3 + 2]);
            triMesh->SetVertexColor(newVertexId, TrimVector(color, 0.0, 1.0));
        }
    }

    bool MeshShopApp::KeyPressed( const OIS::KeyEvent &arg )
    {
        if (arg.key == OIS::KC_D)
        {
#if DEBUGDUMPFILE
            RunDumpInfo();
#endif
        }
        else if (arg.key == OIS::KC_N)
        {
            RunScript(true);
        }
        return true;
    }
    
    void MeshShopApp::WindowFocusChanged( Ogre::RenderWindow* rw)
    {
        if (mpViewTool)
        {
            mpViewTool->MouseReleased();
        }
    }

    void MeshShopApp::DoCommand(bool isSubThread)
    {
        if (isSubThread)
        {
            if (MagicCore::ScriptSystem::Get()->IsOnRunningScript())
            {
                DoCommand(false);
                return;
            }
            GPP::ResetApiProgress();
            mpUI->StartProgressbar(100);
            _beginthreadex(NULL, 0, RunThread, (void *)this, 0, NULL);
        }
        else
        {
            switch (mCommandType)
            {
            case MagicApp::MeshShopApp::NONE:
                break;
            case MagicApp::MeshShopApp::CONSOLIDATETOPOLOGY:
                ConsolidateTopology(false);
                break;
            case MagicApp::MeshShopApp::CONSOLIDATEGEOMETRY:
                ConsolidateGeometry(false);
                break;
            case MagicApp::MeshShopApp::REMOVEISOLATEPART:
                RemoveMeshIsolatePart(false);
                break;
            case MagicApp::MeshShopApp::CDTOPTIMIZATION:
                CDTOptimization(mSharpAngle, false);
                break;
            case MagicApp::MeshShopApp::CVTOPTIMIZATION:
                CVTOptimization(mSharpAngle, false);
                break;
            case MagicApp::MeshShopApp::REMOVEMESHNOISE:
                RemoveMeshNoise(mFilterPositionWeight, false);
                break;
            case MagicApp::MeshShopApp::SMOOTHMESH:
                SmoothMesh(mFilterPositionWeight, false);
                break;
            case MagicApp::MeshShopApp::ENHANCEDETAIL:
                EnhanceMeshDetail(mEnhanceIntensity, false);
                break;
            case MagicApp::MeshShopApp::LOOPSUBDIVIDE:
                LoopSubdivide(false);
                break;
            case MagicApp::MeshShopApp::REFINE:
                RefineMesh(mTargetVertexCount, false);
                break;
            case MagicApp::MeshShopApp::SIMPLIFY:
                SimplifyMesh(mTargetVertexCount, false);
                break;
            case MagicApp::MeshShopApp::SIMPLIFYSELECTVERTEX:
                SimplifySelectedVertices(false);
                break;
            case MagicApp::MeshShopApp::UNIFORMREMESH:
                UniformRemesh(mTargetVertexCount, mSharpAngle, false);
                break;
            case MagicApp::MeshShopApp::FILLHOLE:
                FillHole(mFillHoleType, false);
                break;
            case MagicApp::MeshShopApp::RUNSCRIPT:
                RunScript(false);
                break;
            default:
                break;
            }
            if (!MagicCore::ScriptSystem::Get()->IsOnRunningScript())
            {
                mpUI->StopProgressbar();
            }
        }
    }

    void MeshShopApp::SetupScene()
    {
        Ogre::SceneManager* sceneManager = MagicCore::RenderSystem::Get()->GetSceneManager();
        sceneManager->setAmbientLight(Ogre::ColourValue(0.3, 0.3, 0.3));
        Ogre::Light* light = sceneManager->createLight("MeshShop_SimpleLight");
        light->setPosition(-5, 5, 20);
        light->setDiffuseColour(0.8, 0.8, 0.8);
        light->setSpecularColour(0.5, 0.5, 0.5);
        InitViewTool();
        if (ModelManager::Get()->GetMesh())
        {
            mpUI->SetMeshInfo(ModelManager::Get()->GetMesh()->GetVertexCount(), ModelManager::Get()->GetMesh()->GetTriangleCount());
        }
    }

    void MeshShopApp::ShutdownScene()
    {
        Ogre::SceneManager* sceneManager = MagicCore::RenderSystem::Get()->GetSceneManager();
        sceneManager->setAmbientLight(Ogre::ColourValue::Black);
        sceneManager->destroyLight("MeshShop_SimpleLight");
        //MagicCore::RenderSystem::Get()->SetupCameraDefaultParameter();
        MagicCore::RenderSystem::Get()->HideRenderingObject("Mesh_MeshShop");
        MagicCore::RenderSystem::Get()->HideRenderingObject("MeshShop_Holes");
        MagicCore::RenderSystem::Get()->HideRenderingObject("MeshShop_HoleSeeds");
        /*if (MagicCore::RenderSystem::Get()->GetSceneManager()->hasSceneNode("ModelNode"))
        {
            MagicCore::RenderSystem::Get()->GetSceneManager()->getSceneNode("ModelNode")->resetToInitialState();
        } */
    }

    void MeshShopApp::ClearData()
    {
        GPPFREEPOINTER(mpUI);
        GPPFREEPOINTER(mpViewTool);
#if DEBUGDUMPFILE
        GPPFREEPOINTER(mpDumpInfo);
#endif
        mShowHoleLoopIds.clear();
        mVertexSelectFlag.clear();
        mRightMouseType = MOVE;
    }

    bool MeshShopApp::IsCommandAvaliable()
    {
        if (ModelManager::Get()->GetMesh() == NULL)
        {
            MessageBox(NULL, "请先导入网格", "温馨提示", MB_OK);
            return false;
        }
        if (mIsCommandInProgress)
        {
            MessageBox(NULL, "请等待当前命令执行完", "温馨提示", MB_OK);
            return false;
        }
        return true;
    }

    void MeshShopApp::ResetSelection()
    {
        if (ModelManager::Get()->GetMesh())
        {
            mVertexSelectFlag = std::vector<bool>(ModelManager::Get()->GetMesh()->GetVertexCount(), 0);
        }
        else
        {
            mVertexSelectFlag.clear();
        }
    }

    void MeshShopApp::SelectControlPointByRectangle(int startCoordX, int startCoordY, int endCoordX, int endCoordY)
    {
        GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
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
        GPP::Int vertexCount = triMesh->GetVertexCount();
        GPP::Vector3 coord;
        GPP::Vector3 normal;
        bool ignoreBack = mIgnoreBack;
        for (GPP::Int vid = 0; vid < vertexCount; vid++)
        {
            if (mRightMouseType == SELECT_ADD && mVertexSelectFlag.at(vid))
            {
                continue;
            }
            else if (mRightMouseType == SELECT_DELETE && mVertexSelectFlag.at(vid) == 0)
            {
                continue;
            }
            GPP::Vector3 coord = triMesh->GetVertexCoord(vid);
            Ogre::Vector3 ogreCoord(coord[0], coord[1], coord[2]);
            ogreCoord = wvpM * ogreCoord;
            if (ogreCoord.x > minX && ogreCoord.x < maxX && ogreCoord.y > minY && ogreCoord.y < maxY)
            {
                if (ignoreBack)
                {
                    normal = triMesh->GetVertexNormal(vid);
                    Ogre::Vector4 ogreNormal(normal[0], normal[1], normal[2], 0);
                    ogreNormal = worldM * ogreNormal;
                    if (ogreNormal.z > 0)
                    {
                        if (mRightMouseType == SELECT_BRIDGE)
                        {
                            mVertexBridgeFlag.at(vid) = 1;
                        }
                        else if (mRightMouseType == SELECT_ADD)
                        {
                            mVertexSelectFlag.at(vid) = 1;
                            //InfoLog << vid << std::endl;
                        }
                        else
                        {
                            mVertexSelectFlag.at(vid) = 0;
                        }
                    }
                }
                else
                {
                    if (mRightMouseType == SELECT_BRIDGE)
                    {
                        mVertexBridgeFlag.at(vid) = 1;
                    }
                    else if (mRightMouseType == SELECT_ADD)
                    {
                        mVertexSelectFlag.at(vid) = 1;
                    }
                    else
                    {
                        mVertexSelectFlag.at(vid) = 0;
                    }
                }
            }
        }
    }

    void MeshShopApp::UpdateRectangleRendering(int startCoordX, int startCoordY, int endCoordX, int endCoordY)
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
        pMObj->colour(1, 0, 0);
        pMObj->position(pos0[0], pos1[1], -1);
        pMObj->colour(1, 0, 0);
        pMObj->position(pos1[0], pos1[1], -1);
        pMObj->colour(1, 0, 0);
        pMObj->position(pos1[0], pos0[1], -1);
        pMObj->colour(1, 0, 0);
        pMObj->position(pos0[0], pos0[1], -1);
        pMObj->colour(1, 0, 0);
        pMObj->end();
    }

    void MeshShopApp::ClearRectangleRendering()
    {
        MagicCore::RenderSystem::Get()->HideRenderingObject("PickRectangleObj");
    }

    void MeshShopApp::SwitchDisplayMode()
    {
        mDisplayMode++;
        if (mDisplayMode > 3)
        {
            mDisplayMode = 0;
        }
        if (mDisplayMode == 0)
        {
            MagicCore::RenderSystem::Get()->GetMainCamera()->setPolygonMode(Ogre::PolygonMode::PM_SOLID);
            mIsFlatRenderingMode = true;
        }
        else if (mDisplayMode == 1)
        {
            MagicCore::RenderSystem::Get()->GetMainCamera()->setPolygonMode(Ogre::PolygonMode::PM_SOLID);
            mIsFlatRenderingMode = false;
        }
        else if (mDisplayMode == 2)
        {
            MagicCore::RenderSystem::Get()->GetMainCamera()->setPolygonMode(Ogre::PolygonMode::PM_WIREFRAME);
            mIsFlatRenderingMode = false;
        }
        else if (mDisplayMode == 3)
        {
            MagicCore::RenderSystem::Get()->GetMainCamera()->setPolygonMode(Ogre::PolygonMode::PM_POINTS);
            mIsFlatRenderingMode = false;
        }
        Ogre::Material* material = dynamic_cast<Ogre::Material*>(Ogre::MaterialManager::getSingleton().getByName("CookTorrance").getPointer());
        if (material)
        {
            if (mDisplayMode == 0 || mDisplayMode == 1)
            {
                material->setCullingMode(Ogre::CullingMode::CULL_NONE);
            }
            else
            {
                material->setCullingMode(Ogre::CullingMode::CULL_CLOCKWISE);
            }
        }
        if (!mIsCommandInProgress)
        {
            mUpdateMeshRendering = true;
        }
    }

    bool MeshShopApp::ImportMesh()
    {
        if (mIsCommandInProgress)
        {
            MessageBox(NULL, "请等待当前命令执行完", "温馨提示", MB_OK);
            return false;
        }
        std::string fileName;
        char filterName[] = "OBJ Files(*.obj)\0*.obj\0STL Files(*.stl)\0*.stl\0OFF Files(*.off)\0*.off\0PLY Files(*.ply)\0*.ply\0GPT Files(*.gpt)\0*.gpt\0";
        if (MagicCore::ToolKit::FileOpenDlg(fileName, filterName))
        {
            ModelManager::Get()->ClearPointCloud();
            if (ModelManager::Get()->ImportMesh(fileName) == false)
            {
                MessageBox(NULL, "网格导入失败", "温馨提示", MB_OK);
                return false;
            }
            GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
            ResetSelection();
            mRightMouseType = MOVE;
            mShowHoleLoopIds.clear();
            UpdateHoleRendering();
            UpdateMeshRendering();
            mpUI->SetMeshInfo(triMesh->GetVertexCount(), triMesh->GetTriangleCount());
            mpUI->ResetFillHole();
            return true;
        }
        return false;
    }

#if DEBUGDUMPFILE
    void MeshShopApp::SetDumpInfo(GPP::DumpBase* dumpInfo)
    {
    }

    void MeshShopApp::RunDumpInfo()
    {
    }
#endif

    bool MeshShopApp::IsCommandInProgress(void)
    {
        return mIsCommandInProgress;
    }

    void MeshShopApp::UpdateAddedVertexInfo(std::map<int, int>& insertVertexIdMap)
    {
        GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
        if (triMesh && insertVertexIdMap.size() > 0)
        {
            int curVertexCount = triMesh->GetVertexCount();
            std::vector<GPP::ImageColorId>* imageColorIds = ModelManager::Get()->GetImageColorIdsPointer();
            if (imageColorIds)
            {
                imageColorIds->resize(curVertexCount);
                for (std::map<int, int>::iterator itr = insertVertexIdMap.begin(); itr != insertVertexIdMap.end(); ++itr)
                {
                    imageColorIds->at(itr->first) = imageColorIds->at(itr->second);
                }
            }
            std::vector<int>* imageColorIdFlags = ModelManager::Get()->GetImageColorIdFlagsPointer();
            if (imageColorIdFlags)
            {
                imageColorIdFlags->resize(curVertexCount);
                for (std::map<int, int>::iterator itr = insertVertexIdMap.begin(); itr != insertVertexIdMap.end(); ++itr)
                {
                    imageColorIdFlags->at(itr->first) = imageColorIdFlags->at(itr->second);
                }
            }
            std::vector<int>* colorIds = ModelManager::Get()->GetColorIdsPointer();
            if (colorIds)
            {
                colorIds->resize(curVertexCount);
                for (std::map<int, int>::iterator itr = insertVertexIdMap.begin(); itr != insertVertexIdMap.end(); ++itr)
                {
                    colorIds->at(itr->first) = colorIds->at(itr->second);
                }
            }
            if (triMesh->HasVertexColor())
            {
                for (std::map<int, int>::iterator itr = insertVertexIdMap.begin(); itr != insertVertexIdMap.end(); ++itr)
                {
                    triMesh->SetVertexColor(itr->first, triMesh->GetVertexColor(itr->second));
                }
            }
        }
    }

    void MeshShopApp::IsManifold()
    {
        GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
        if (triMesh)
        {
            int invalidVertexId = -1;
            if (GPP::ConsolidateMesh::_IsTriMeshManifold(triMesh, &invalidVertexId) == false)
            {
                MessageBox(NULL, "网格有非流形结构", "温馨提示", MB_OK);
                if (mVertexSelectFlag.size() == triMesh->GetVertexCount() && invalidVertexId != -1)
                {
                    mVertexSelectFlag.at(invalidVertexId) = 1;
                    UpdateMeshRendering();
                }
            }
            else
            {
                MessageBox(NULL, "网格是流形结构", "温馨提示", MB_OK);
            }
        }
        else
        {
            MessageBox(NULL, "请导入网格", "温馨提示", MB_OK);
        }
    }

    void MeshShopApp::ConsolidateTopology(bool isSubThread)
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        if (isSubThread)
        {
            mCommandType = CONSOLIDATETOPOLOGY;
            DoCommand(true);
        }
        else
        {
            GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
            int originVertexCount = triMesh->GetVertexCount();
            MagicMesh magicMesh(triMesh);
            ConstructMagicMeshInfo(&magicMesh);
            mIsCommandInProgress = true;
#if MAKEDUMPFILE
            GPP::DumpOnce();
#endif
            std::map<int, int> insertVertexIdMap;
            GPP::ErrorCode res = GPP::ConsolidateMesh::MakeTriMeshManifold(&magicMesh, &insertVertexIdMap);
            mIsCommandInProgress = false;
            if (res == GPP_API_IS_NOT_AVAILABLE)
            {
                MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
                MagicCore::ToolKit::Get()->SetAppRunning(false);
            }
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "拓扑修复失败", "温馨提示", MB_OK);
                return;
            }
            UpdateAddedVertexInfo(insertVertexIdMap);
            ResetSelection();
            bool isManifold = GPP::ConsolidateMesh::_IsTriMeshManifold(triMesh);
            if (!isManifold)
            {
                MessageBox(NULL, "拓扑修复后，网格仍然是非流形结构", "温馨提示", MB_OK);
                return;
            }
            triMesh->UpdateNormal();
            mUpdateMeshRendering= true;
            mpUI->SetMeshInfo(triMesh->GetVertexCount(), triMesh->GetTriangleCount());
            mpUI->ResetFillHole();
            FindHole(false);
        }
    }

    void MeshShopApp::ReverseDirection()
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
        GPP::Int faceCount = triMesh->GetTriangleCount();
        GPP::Int vertexIds[3];
        for (GPP::Int fid = 0; fid < faceCount; fid++)
        {
            triMesh->GetTriangleVertexIds(fid, vertexIds);
            triMesh->SetTriangleVertexIds(fid, vertexIds[1], vertexIds[0], vertexIds[2]);
        }
        triMesh->UpdateNormal();
        UpdateMeshRendering();
    }

    void MeshShopApp::RemoveMeshIsolatePart(bool isSubThread)
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        if (isSubThread)
        {
            mCommandType = REMOVEISOLATEPART;
            DoCommand(true);
        }
        else
        {
            GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
            GPP::Int vertexCount = triMesh->GetVertexCount();
            if (vertexCount < 1)
            {
                MessageBox(NULL, "网格顶点个数小于1，操作失败", "温馨提示", MB_OK);
                return;
            }
            std::vector<GPP::Real> isolation;
            mIsCommandInProgress = true;
            GPP::ErrorCode res = GPP::ConsolidateMesh::CalculateIsolation(triMesh, &isolation);
            mIsCommandInProgress = false;
            if (res == GPP_API_IS_NOT_AVAILABLE)
            {
                MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
                MagicCore::ToolKit::Get()->SetAppRunning(false);
            }
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "网格光顺失败", "温馨提示", MB_OK);
                return;
            }
            GPP::Real cutValue = 0.10;
            std::vector<GPP::Int> deleteIndex;
            for (GPP::Int vid = 0; vid < vertexCount; vid++)
            {
                if (isolation[vid] < cutValue)
                {
                    deleteIndex.push_back(vid);
                }
            }
            DebugLog << "MeshShopApp::RemoveOutlier deleteIndex size=" << deleteIndex.size() << std::endl;
            MagicMesh magicMesh(triMesh);
            ConstructMagicMeshInfo(&magicMesh);
            //res = GPP::DeleteTriMeshVertices(triMesh, deleteIndex);
            res = GPP::DeleteTriMeshVertices(&magicMesh, deleteIndex);
            if (res != GPP_NO_ERROR)
            {
                return;
            }
            triMesh->UpdateNormal();
            ResetSelection();
            mUpdateMeshRendering = true;
            mpUI->SetMeshInfo(triMesh->GetVertexCount(), triMesh->GetTriangleCount());
            mpUI->ResetFillHole();
            FindHole(false);
        }
    }

    void MeshShopApp::ConsolidateGeometry(bool isSubThread)
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        if (isSubThread)
        {
            mCommandType = CONSOLIDATEGEOMETRY;
            DoCommand(true);
        }
        else
        {
            GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
            mIsCommandInProgress = true;
#if MAKEDUMPFILE
            GPP::DumpOnce();
#endif
            GPP::ErrorCode res = GPP::ConsolidateMesh::ConsolidateGeometry(triMesh, GPP::ONE_RADIAN * 5.0, 
                GPP::REAL_TOL, GPP::ONE_RADIAN * 170.0);
            mIsCommandInProgress = false;
            if (res == GPP_API_IS_NOT_AVAILABLE)
            {
                MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
                MagicCore::ToolKit::Get()->SetAppRunning(false);
            }
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "几何修复失败", "温馨提示", MB_OK);
                return;
            }
            triMesh->UpdateNormal();
            ResetSelection();
            mUpdateMeshRendering = true;
        }
    }

    void MeshShopApp::CVTOptimization(double sharpAngle, bool isSubThread)
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        if (isSubThread)
        {
            mCommandType = CVTOPTIMIZATION;
            mSharpAngle = sharpAngle;
            DoCommand(true);
        }
        else
        {
            GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
            if (GPP::ConsolidateMesh::_IsTriMeshManifold(triMesh) == false)
            {
                MessageBox(NULL, "警告：网格有非流形结构，请先拓扑修复，否则程序会出错", "温馨提示", MB_OK);
                return;
            }
            mIsCommandInProgress = true;
            sharpAngle *= GPP::ONE_RADIAN;
            if (triMesh->HasVertexColor())
            {
                std::vector<GPP::Real> vertexFields;
                CollectTriMeshVerticesColorFields(triMesh, &vertexFields);
#if MAKEDUMPFILE
                GPP::DumpOnce();
#endif
                GPP::ErrorCode res = GPP::Triangulation::CentroidVoronoiOptimization(triMesh, &sharpAngle, 
                    &vertexFields, NULL, NULL, NULL);
                mIsCommandInProgress = false;
                if (res == GPP_API_IS_NOT_AVAILABLE)
                {
                    MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
                    MagicCore::ToolKit::Get()->SetAppRunning(false);
                }
                if (res != GPP_NO_ERROR)
                {
                    MessageBox(NULL, "网格优化失败", "温馨提示", MB_OK);
                    return;
                }
                GPP::Int vertexCount = triMesh->GetVertexCount();
                for (GPP::Int vid = 0; vid < vertexCount; vid++)
                {
                    GPP::Int baseIndex = vid * 3;
                    triMesh->SetVertexColor(vid, GPP::Vector3(vertexFields.at(baseIndex), vertexFields.at(baseIndex + 1), vertexFields.at(baseIndex + 2)));
                }
            }
            else
            {
#if MAKEDUMPFILE
                GPP::DumpOnce();
#endif
                GPP::ErrorCode res = GPP::Triangulation::CentroidVoronoiOptimization(triMesh, &sharpAngle, NULL, NULL, NULL, NULL);
                mIsCommandInProgress = false;
                if (res == GPP_API_IS_NOT_AVAILABLE)
                {
                    MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
                    MagicCore::ToolKit::Get()->SetAppRunning(false);
                }
                if (res != GPP_NO_ERROR)
                {
                    MessageBox(NULL, "网格优化失败", "温馨提示", MB_OK);
                    return;
                }
            }        
            triMesh->UpdateNormal();
            mUpdateMeshRendering = true;
        }
    }

    void MeshShopApp::CDTOptimization(double sharpAngle, bool isSubThread)
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        if (isSubThread)
        {
            mCommandType = CDTOPTIMIZATION;
            mSharpAngle = sharpAngle;
            DoCommand(true);
        }
        else
        {
            GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
            if (GPP::ConsolidateMesh::_IsTriMeshManifold(triMesh) == false)
            {
                MessageBox(NULL, "警告：网格有非流形结构，请先拓扑修复，否则程序会出错", "温馨提示", MB_OK);
                return;
            }
            mIsCommandInProgress = true;
            sharpAngle *= GPP::ONE_RADIAN;
#if MAKEDUMPFILE
            GPP::DumpOnce();
#endif
            GPP::ErrorCode res = GPP::Triangulation::ConstrainedDelaunayOptimization(triMesh, &sharpAngle, 
                NULL, NULL);
            mIsCommandInProgress = false;
            if (res == GPP_API_IS_NOT_AVAILABLE)
            {
                MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
                MagicCore::ToolKit::Get()->SetAppRunning(false);
            }
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "网格优化失败", "温馨提示", MB_OK);
                return;
            }       
            triMesh->UpdateNormal();
            mUpdateMeshRendering = true;
        }
    }

    void MeshShopApp::RemoveMeshNoise(double positionWeight, bool isSubThread)
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        if (isSubThread)
        {
            mCommandType = REMOVEMESHNOISE;
            mFilterPositionWeight = positionWeight;
            DoCommand(true);
        }
        else
        {
            GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
            mIsCommandInProgress = true;
            bool isVertexSelected = false;
            for (std::vector<bool>::iterator itr = mVertexSelectFlag.begin(); itr != mVertexSelectFlag.end(); ++itr)
            {
                if (*itr)
                {
                    isVertexSelected = true;
                    break;
                }
            }
            GPP::ErrorCode res = GPP_NO_ERROR;
#if MAKEDUMPFILE
            GPP::DumpOnce();
#endif
            if (isVertexSelected)
            {
                GPP::SubTriMesh subTriMesh(triMesh, mVertexSelectFlag, GPP::SubTriMesh::BUILD_SUBTRIMESH_TYPE_BY_VERTICES);
                res = GPP::ConsolidateMesh::RemoveGeometryNoise(&subTriMesh, 70.0 * GPP::ONE_RADIAN, positionWeight);
            }
            else
            {
                res = GPP::ConsolidateMesh::RemoveGeometryNoise(triMesh, 70.0 * GPP::ONE_RADIAN, positionWeight);
            }
            mIsCommandInProgress = false;
            if (res == GPP_API_IS_NOT_AVAILABLE)
            {
                MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
                MagicCore::ToolKit::Get()->SetAppRunning(false);
            }
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "网格光顺失败", "温馨提示", MB_OK);
                return;
            }
            triMesh->UpdateNormal();
            mUpdateMeshRendering = true;
        }
    }

    void MeshShopApp::SmoothMesh(double positionWeight, bool isSubThread)
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        if (isSubThread)
        {
            mCommandType = SMOOTHMESH;
            mFilterPositionWeight = positionWeight;
            DoCommand(true);
        }
        else
        {
            GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
            mIsCommandInProgress = true;
            bool isVertexSelected = false;
            for (std::vector<bool>::iterator itr = mVertexSelectFlag.begin(); itr != mVertexSelectFlag.end(); ++itr)
            {
                if (*itr)
                {
                    isVertexSelected = true;
                    break;
                }
            }
            GPP::ErrorCode res = GPP_NO_ERROR;
#if MAKEDUMPFILE
            GPP::DumpOnce();
#endif
            if (isVertexSelected)
            {
                GPP::SubTriMesh subTriMesh(triMesh, mVertexSelectFlag, GPP::SubTriMesh::BUILD_SUBTRIMESH_TYPE_BY_VERTICES);
                res = GPP::FilterMesh::LaplaceSmooth(&subTriMesh, true, positionWeight);
            }
            else
            {
                res = GPP::FilterMesh::LaplaceSmooth(triMesh, true, positionWeight);
            }
            mIsCommandInProgress = false;
            if (res == GPP_API_IS_NOT_AVAILABLE)
            {
                MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
                MagicCore::ToolKit::Get()->SetAppRunning(false);
            }
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "网格光顺失败", "温馨提示", MB_OK);
                return;
            }
            triMesh->UpdateNormal();
            mUpdateMeshRendering = true;
        }
    }

    void MeshShopApp::EnhanceMeshDetail(double intensity, bool isSubThread)
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        if (isSubThread)
        {
            mCommandType = ENHANCEDETAIL;
            mEnhanceIntensity = intensity;
            DoCommand(true);
        }
        else
        {
            GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
            mIsCommandInProgress = true;
            bool isVertexSelected = false;
            for (std::vector<bool>::iterator itr = mVertexSelectFlag.begin(); itr != mVertexSelectFlag.end(); ++itr)
            {
                if (*itr)
                {
                    isVertexSelected = true;
                    break;
                }
            }
            GPP::ErrorCode res = GPP_NO_ERROR;
#if MAKEDUMPFILE
            GPP::DumpOnce();
#endif
            if (isVertexSelected)
            {
                GPP::SubTriMesh subTriMesh(triMesh, mVertexSelectFlag, GPP::SubTriMesh::BUILD_SUBTRIMESH_TYPE_BY_VERTICES);
                res = GPP::FilterMesh::EnhanceDetail(&subTriMesh, intensity);
            }
            else
            {
                res = GPP::FilterMesh::EnhanceDetail(triMesh, intensity);
            }
            mIsCommandInProgress = false;
            if (res == GPP_API_IS_NOT_AVAILABLE)
            {
                MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
                MagicCore::ToolKit::Get()->SetAppRunning(false);
            }
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "几何细节增强失败", "温馨提示", MB_OK);
                return;
            }
            triMesh->UpdateNormal();
            mUpdateMeshRendering = true;
        }
    }

    void MeshShopApp::LoopSubdivide(bool isSubThread)
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        if (isSubThread)
        {
            mCommandType = LOOPSUBDIVIDE;
            DoCommand(true);
        }
        else
        {
            GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
            GPP::Int vertexCount = triMesh->GetVertexCount();
            if (vertexCount >= 250000)
            {
                if (MessageBox(NULL, "细分后网格顶点数量可能会大于1M，是否继续", "温馨提示", MB_OKCANCEL) != IDOK)
                {
                    return;
                }
            }
            if (triMesh->HasVertexColor())
            {
                std::vector<GPP::Real> vertexFields(vertexCount * 3);
                for (GPP::Int vid = 0; vid < vertexCount; vid++)
                {
                    GPP::Vector3 color = triMesh->GetVertexColor(vid);
                    GPP::Int baseId = vid * 3;
                    vertexFields.at(baseId) = color[0];
                    vertexFields.at(baseId + 1) = color[1];
                    vertexFields.at(baseId + 2) = color[2];
                }
                std::vector<GPP::Real> insertedVertexFields;
                mIsCommandInProgress = true;
#if MAKEDUMPFILE
                GPP::DumpOnce();
#endif
                GPP::ErrorCode res = GPP::SubdivideMesh::LoopSubdivideMesh(triMesh, &vertexFields, &insertedVertexFields);
                mIsCommandInProgress = false;
                if (res == GPP_API_IS_NOT_AVAILABLE)
                {
                    MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
                    MagicCore::ToolKit::Get()->SetAppRunning(false);
                }
                if (res != GPP_NO_ERROR)
                {
                    MessageBox(NULL, "Loop细分失败", "温馨提示", MB_OK);
                    return;
                }               
                GPP::Int insertedVertexCount = triMesh->GetVertexCount() - vertexCount;
                for (GPP::Int vid = 0; vid < insertedVertexCount; vid++)
                {
                    GPP::Int baseId = vid * 3;
                    triMesh->SetVertexColor(vertexCount + vid, GPP::Vector3(insertedVertexFields.at(baseId), insertedVertexFields.at(baseId + 1), insertedVertexFields.at(baseId + 2)));
                }
            }
            else
            {
                mIsCommandInProgress = true;
#if MAKEDUMPFILE
                GPP::DumpOnce();
#endif
                GPP::ErrorCode res = GPP::SubdivideMesh::LoopSubdivideMesh(triMesh, NULL, NULL);
                mIsCommandInProgress = false;
                if (res == GPP_API_IS_NOT_AVAILABLE)
                {
                    MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
                    MagicCore::ToolKit::Get()->SetAppRunning(false);
                }
                if (res != GPP_NO_ERROR)
                {
                    MessageBox(NULL, "Loop细分失败", "温馨提示", MB_OK);
                    return;
                }
            }
            triMesh->UpdateNormal();
            ResetSelection();
            mUpdateMeshRendering = true;
            mpUI->SetMeshInfo(triMesh->GetVertexCount(), triMesh->GetTriangleCount());
        }
    }

    void MeshShopApp::RefineMesh(int targetVertexCount, bool isSubThread)
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        if (isSubThread)
        {
            mTargetVertexCount = targetVertexCount;
            mCommandType = REFINE;
            DoCommand(true);
        }
        else
        {
            GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
            if (triMesh->GetVertexCount() >= targetVertexCount)
            {
                MessageBox(NULL, "警告：目标顶点数必须大于网格点数", "温馨提示", MB_OK);
                return;
            }
            if (targetVertexCount > 1000000)
            {
                if (MessageBox(NULL, "加密后网格顶点数量会大于1M，是否继续", "温馨提示", MB_OKCANCEL) != IDOK)
                {
                    return;
                }
            }
            GPP::Int vertexCount = triMesh->GetVertexCount();
            if (triMesh->HasVertexColor())
            {
                std::vector<GPP::Real> vertexFields(vertexCount * 3);
                for (GPP::Int vid = 0; vid < vertexCount; vid++)
                {
                    GPP::Vector3 color = triMesh->GetVertexColor(vid);
                    GPP::Int baseId = vid * 3;
                    vertexFields.at(baseId) = color[0];
                    vertexFields.at(baseId + 1) = color[1];
                    vertexFields.at(baseId + 2) = color[2];
                }
                std::vector<GPP::Real> insertedVertexFields;
                mIsCommandInProgress = true;
#if MAKEDUMPFILE
                GPP::DumpOnce();
#endif
                GPP::ErrorCode res = GPP::SubdivideMesh::DensifyMesh(triMesh, targetVertexCount, &vertexFields, &insertedVertexFields);
                mIsCommandInProgress = false;
                if (res == GPP_API_IS_NOT_AVAILABLE)
                {
                    MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
                    MagicCore::ToolKit::Get()->SetAppRunning(false);
                }
                if (res != GPP_NO_ERROR)
                {
                    MessageBox(NULL, "网格加密失败", "温馨提示", MB_OK);
                    return;
                }
                GPP::Int insertedVertexCount = triMesh->GetVertexCount() - vertexCount;
                for (GPP::Int vid = 0; vid < insertedVertexCount; vid++)
                {
                    GPP::Int baseId = vid * 3;
                    triMesh->SetVertexColor(vertexCount + vid, GPP::Vector3(insertedVertexFields.at(baseId), insertedVertexFields.at(baseId + 1), insertedVertexFields.at(baseId + 2)));
                }
            }
            else
            {
                mIsCommandInProgress = true;
#if MAKEDUMPFILE
                GPP::DumpOnce();
#endif
                GPP::ErrorCode res = GPP::SubdivideMesh::DensifyMesh(triMesh, targetVertexCount, NULL, NULL);
                mIsCommandInProgress = false;
                if (res == GPP_API_IS_NOT_AVAILABLE)
                {
                    MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
                    MagicCore::ToolKit::Get()->SetAppRunning(false);
                }
                if (res != GPP_NO_ERROR)
                {
                    MessageBox(NULL, "网格加密失败", "温馨提示", MB_OK);
                    return;
                }
            }
            triMesh->UpdateNormal();
            ResetSelection();
            mUpdateMeshRendering = true;
            mpUI->SetMeshInfo(triMesh->GetVertexCount(), triMesh->GetTriangleCount());
        }
    }

    void MeshShopApp::SimplifyMesh(int targetVertexCount, bool isSubThread)
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        if (isSubThread)
        {
            mTargetVertexCount = targetVertexCount;
            mCommandType = SIMPLIFY;
            DoCommand(true);
        }
        else
        {
            GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
            if (GPP::ConsolidateMesh::_IsTriMeshManifold(triMesh) == false)
            {
                MessageBox(NULL, "警告：网格有非流形结构，请先拓扑修复，否则程序会出错", "温馨提示", MB_OK);
                return;
            }
            if (triMesh->GetVertexCount() <= targetVertexCount)
            {
                MessageBox(NULL, "警告：目标顶点数必须小于网格点数", "温馨提示", MB_OK);
                return;
            }
            if (triMesh->HasVertexColor())
            {
                std::vector<GPP::Real> vertexFields;
                CollectTriMeshVerticesColorFields(triMesh, &vertexFields);
                std::vector<GPP::Real> simplifiedVertexFields;
                //GPP::DumpOnce();
                mIsCommandInProgress = true;
#if MAKEDUMPFILE
                GPP::DumpOnce();
#endif
                GPP::ErrorCode res = GPP::SimplifyMesh::QuadricSimplify(triMesh, targetVertexCount, false, &vertexFields, &simplifiedVertexFields);
                mIsCommandInProgress = false;
                if (res == GPP_API_IS_NOT_AVAILABLE)
                {
                    MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
                    MagicCore::ToolKit::Get()->SetAppRunning(false);
                }
                if (res != GPP_NO_ERROR)
                {
                    MessageBox(NULL, "网格简化失败", "温馨提示", MB_OK);
                    return;
                }               
                GPP::Int vertexCount = triMesh->GetVertexCount();
                for (GPP::Int vid = 0; vid < vertexCount; vid++)
                {
                    GPP::Int baseIndex = vid * 3;
                    triMesh->SetVertexColor(vid, GPP::Vector3(simplifiedVertexFields.at(baseIndex), simplifiedVertexFields.at(baseIndex + 1), simplifiedVertexFields.at(baseIndex + 2)));
                }
            }
            else
            {
                mIsCommandInProgress = true;
#if MAKEDUMPFILE
                GPP::DumpOnce();
#endif
                GPP::ErrorCode res = GPP::SimplifyMesh::QuadricSimplify(triMesh, targetVertexCount, false, NULL, NULL);
                mIsCommandInProgress = false;
                if (res == GPP_API_IS_NOT_AVAILABLE)
                {
                    MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
                    MagicCore::ToolKit::Get()->SetAppRunning(false);
                }
                if (res != GPP_NO_ERROR)
                {
                    MessageBox(NULL, "网格简化失败", "温馨提示", MB_OK);
                    return;
                }
            }
            ResetSelection();
            mUpdateMeshRendering = true;
            mpUI->SetMeshInfo(triMesh->GetVertexCount(), triMesh->GetTriangleCount());
            mpUI->ResetFillHole();
            FindHole(false);
        }
    }

    void MeshShopApp::SimplifySelectedVertices(bool isSubThread)
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        if (isSubThread)
        {
            mCommandType = SIMPLIFYSELECTVERTEX;
            DoCommand(true);
        }
        else
        {
            GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
            if (GPP::ConsolidateMesh::_IsTriMeshManifold(triMesh) == false)
            {
                MessageBox(NULL, "警告：网格有非流形结构，请先拓扑修复，否则程序会出错", "温馨提示", MB_OK);
                return;
            }
            std::vector<GPP::Int> removingVertices;
            GPP::Int originVertexCount = triMesh->GetVertexCount();
            for (GPP::Int vid = 0; vid < originVertexCount; vid++)
            {
                if (mVertexSelectFlag.at(vid))
                {
                    removingVertices.push_back(vid);
                }
            }
            if (removingVertices.empty())
            {
                return;
            }
            if (triMesh->HasVertexColor())
            {
                GPP::Int originVertexCount = triMesh->GetVertexCount();
                std::vector<GPP::Vector3> vertexColors(originVertexCount);
                for (GPP::Int vid = 0; vid < originVertexCount; vid++)
                {
                    vertexColors.at(vid) = triMesh->GetVertexColor(vid);
                }
                mIsCommandInProgress = true;
#if MAKEDUMPFILE
                GPP::DumpOnce();
#endif
                std::vector<GPP::Int> vertexMap;
                GPP::ErrorCode res = GPP::SimplifyMesh::SimplifyByRemovingVertex(triMesh, removingVertices, &vertexMap, NULL, NULL, NULL);
                mIsCommandInProgress = false;
                if (res == GPP_API_IS_NOT_AVAILABLE)
                {
                    MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
                    MagicCore::ToolKit::Get()->SetAppRunning(false);
                }
                if (res != GPP_NO_ERROR)
                {
                    MessageBox(NULL, "网格简化失败", "温馨提示", MB_OK);
                    return;
                }               
                GPP::Int vertexCount = triMesh->GetVertexCount();
                for (GPP::Int vid = 0; vid < vertexCount; vid++)
                {
                    triMesh->SetVertexColor(vid, vertexColors.at(vertexMap.at(vid)));
                }
            }
            else
            {
                mIsCommandInProgress = true;
#if MAKEDUMPFILE
                GPP::DumpOnce();
#endif
                GPP::ErrorCode res = GPP::SimplifyMesh::SimplifyByRemovingVertex(triMesh, removingVertices, NULL, NULL, NULL, NULL);
                mIsCommandInProgress = false;
                if (res == GPP_API_IS_NOT_AVAILABLE)
                {
                    MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
                    MagicCore::ToolKit::Get()->SetAppRunning(false);
                }
                if (res != GPP_NO_ERROR)
                {
                    MessageBox(NULL, "网格简化失败", "温馨提示", MB_OK);
                    return;
                }          
            }
            ResetSelection();
            mUpdateMeshRendering = true;
            mpUI->SetMeshInfo(triMesh->GetVertexCount(), triMesh->GetTriangleCount());
            mpUI->ResetFillHole();
            FindHole(false);
        }
    }

    void MeshShopApp::UniformRemesh(int targetVertexCount, double sharpAngle, bool isSubThread)
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        if (isSubThread)
        {
            mTargetVertexCount = targetVertexCount;
            mSharpAngle = sharpAngle;
            mCommandType = UNIFORMREMESH;
            DoCommand(true);
        }
        else
        {
            GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
            if (GPP::ConsolidateMesh::_IsTriMeshManifold(triMesh) == false)
            {
                MessageBox(NULL, "警告：网格有非流形结构，请先拓扑修复，否则程序会出错", "温馨提示", MB_OK);
                return;
            }
            sharpAngle *= GPP::ONE_RADIAN;
            if (triMesh->HasVertexColor())
            {
                std::vector<GPP::Real> vertexFields;
                CollectTriMeshVerticesColorFields(triMesh, &vertexFields);
                std::vector<GPP::Real> remeshVertexFields;
#if MAKEDUMPFILE
        GPP::DumpOnce();
#endif
                mIsCommandInProgress = true;
                GPP::ErrorCode res = GPP::Remesh::UniformRemesh(triMesh, targetVertexCount, sharpAngle, 2, &vertexFields, &remeshVertexFields);
                mIsCommandInProgress = false;
                if (res == GPP_API_IS_NOT_AVAILABLE)
                {
                    MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
                    MagicCore::ToolKit::Get()->SetAppRunning(false);
                }
                if (res != GPP_NO_ERROR)
                {
                    MessageBox(NULL, "Remesh失败", "温馨提示", MB_OK);
                    return;
                }               
                GPP::Int vertexCount = triMesh->GetVertexCount();
                for (GPP::Int vid = 0; vid < vertexCount; vid++)
                {
                    GPP::Int baseIndex = vid * 3;
                    triMesh->SetVertexColor(vid, GPP::Vector3(remeshVertexFields.at(baseIndex), remeshVertexFields.at(baseIndex + 1), remeshVertexFields.at(baseIndex + 2)));
                }
            }
            else
            {
#if MAKEDUMPFILE
                GPP::DumpOnce();
#endif
                mIsCommandInProgress = true;
                GPP::ErrorCode res = GPP::Remesh::UniformRemesh(triMesh, targetVertexCount, sharpAngle, 2, NULL, NULL);
                mIsCommandInProgress = false;
                if (res == GPP_API_IS_NOT_AVAILABLE)
                {
                    MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
                    MagicCore::ToolKit::Get()->SetAppRunning(false);
                }
                if (res != GPP_NO_ERROR)
                {
                    MessageBox(NULL, "Remesh失败", "温馨提示", MB_OK);
                    return;
                }
            }
            triMesh->UpdateNormal();
            ResetSelection();
            mUpdateMeshRendering = true;
            mpUI->SetMeshInfo(triMesh->GetVertexCount(), triMesh->GetTriangleCount());
            mpUI->ResetFillHole();
            FindHole(false);
        }
    }

    void MeshShopApp::UniformSampleMesh(int targetPointCount)
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
        std::vector<GPP::PointOnEdge> pointsOnEdge;
        std::vector<GPP::PointOnFace> pointsOnFace;
        std::vector<GPP::Int> pointsOnVertex;
#if MAKEDUMPFILE
        GPP::DumpOnce();
#endif
        GPP::ErrorCode res = GPP::SampleMesh::UniformSample(triMesh, targetPointCount, 70 * GPP::ONE_RADIAN, 
            pointsOnFace, pointsOnEdge, pointsOnVertex, false); 
        if (res == GPP_API_IS_NOT_AVAILABLE)
        {
            MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
            MagicCore::ToolKit::Get()->SetAppRunning(false);
        }
        if (res != GPP_NO_ERROR)
        {
            MessageBox(NULL, "采样失败", "温馨提示", MB_OK);
            return;
        }

        bool hasColor = triMesh->HasVertexColor();
        GPP::PointCloud* pointCloud = new GPP::PointCloud(true, hasColor);
        for (std::vector<GPP::PointOnEdge>::iterator itr = pointsOnEdge.begin(); itr != pointsOnEdge.end(); ++itr)
        {
            GPP::Vector3 coord = triMesh->GetVertexCoord(itr->mVertexIdStart) * itr->mWeight + triMesh->GetVertexCoord(itr->mVertexIdEnd) * (1.0 - itr->mWeight);
            GPP::Vector3 normal = triMesh->GetVertexNormal(itr->mVertexIdStart) * itr->mWeight + triMesh->GetVertexNormal(itr->mVertexIdEnd) * (1.0 - itr->mWeight);
            normal.Normalise();
            GPP::Int pointId = pointCloud->InsertPoint(coord, normal);
            if (hasColor)
            {
                GPP::Vector3 color = triMesh->GetVertexColor(itr->mVertexIdStart) * itr->mWeight + triMesh->GetVertexColor(itr->mVertexIdEnd) * (1.0 - itr->mWeight);
                pointCloud->SetPointColor(pointId, color);
            }
        }
        GPP::Int vertexIds[3] = {-1};
        for (std::vector<GPP::PointOnFace>::iterator itr = pointsOnFace.begin(); itr != pointsOnFace.end(); ++itr)
        {
            triMesh->GetTriangleVertexIds(itr->mFaceId, vertexIds);
            GPP::Vector3 coord(0, 0, 0);
            for (int fvid = 0; fvid < 3; fvid++)
            {
                coord += triMesh->GetVertexCoord(vertexIds[fvid]) * itr->mCoord[fvid];
            }
            GPP::Int pointId = pointCloud->InsertPoint(coord, triMesh->GetTriangleNormal(itr->mFaceId));
            if (hasColor)
            {
                GPP::Vector3 color(0, 0, 0);
                for (int fvid = 0; fvid < 3; fvid++)
                {
                    color += triMesh->GetVertexColor(vertexIds[fvid]) * itr->mCoord[fvid];
                }
                pointCloud->SetPointColor(pointId, color);
            }
        }
        for (std::vector<GPP::Int>::iterator itr = pointsOnVertex.begin(); itr != pointsOnVertex.end(); ++itr)
        {
            GPP::Int pointId = pointCloud->InsertPoint(triMesh->GetVertexCoord(*itr), triMesh->GetVertexNormal(*itr));
            if (hasColor)
            {
                pointCloud->SetPointColor(pointId, triMesh->GetVertexColor(*itr));
            }
        }
        ModelManager::Get()->ClearMesh();
        ModelManager::Get()->SetPointCloud(pointCloud);
        AppManager::Get()->EnterApp(new PointShopApp, "PointShopApp");
    }

    void MeshShopApp::EnterReliefApp()
    {
        if (mIsCommandInProgress)
        {
            MessageBox(NULL, "请等待当前命令执行完", "温馨提示", MB_OK);
            return;
        }
        AppManager::Get()->EnterApp(new ReliefApp, "ReliefApp");
    }

    void MeshShopApp::EnterTextureApp()
    {
        if (mIsCommandInProgress)
        {
            MessageBox(NULL, "请等待当前命令执行完", "温馨提示", MB_OK);
            return;
        }
        AppManager::Get()->EnterApp(new TextureApp, "TextureApp");
    }

    void MeshShopApp::EnterMeasureApp()
    {
        if (mIsCommandInProgress)
        {
            MessageBox(NULL, "请等待当前命令执行完", "温馨提示", MB_OK);
            return;
        }
        AppManager::Get()->EnterApp(new MeasureApp, "MeasureApp");
    }

    void MeshShopApp::ResetBridgeTags()
    {
        if (ModelManager::Get()->GetMesh())
        {
            mVertexBridgeFlag = std::vector<bool>(ModelManager::Get()->GetMesh()->GetVertexCount(), 0);
        }
        else
        {
            mVertexBridgeFlag.clear();
        }
    }

    void MeshShopApp::FindHole(bool isShowHoles)
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        std::vector<std::vector<GPP::Int> > holeIds;
        if (!isShowHoles)
        {
            // clear the holes
            mRightMouseType = MOVE; 
            ResetBridgeTags();
            mBridgeEdgeVertices.clear();
            SetToShowHoleLoopVrtIds(holeIds);
            SetBoundarySeedIds(std::vector<GPP::Int>());
            mUpdateBridgeRendering = true;
            mUpdateHoleRendering = true;
            //UpdateBridgeRendering();
            //UpdateHoleRendering();
            return;
        }
        GPP::ErrorCode res = GPP::FillMeshHole::FindHoles(ModelManager::Get()->GetMesh(), &holeIds);
        if (res == GPP_API_IS_NOT_AVAILABLE)
        {
            MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
            MagicCore::ToolKit::Get()->SetAppRunning(false);
        }
        if (res != GPP_NO_ERROR)
        {
            return;
        }
        SetToShowHoleLoopVrtIds(holeIds);
        SetBoundarySeedIds(std::vector<GPP::Int>());

        ModelManager::Get()->GetMesh()->UpdateNormal();
        UpdateHoleRendering();
        UpdateMeshRendering();
    }

    void MeshShopApp::FillHole(int type, bool isSubThread)
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        if (isSubThread)
        {
            mFillHoleType = type;
            mCommandType = FILLHOLE;
            DoCommand(true);
        }
        else
        {
            mIsCommandInProgress = true;
            
            std::vector<GPP::Int> holeSeeds;            
            for (GPP::Int vLoop = 0; vLoop < mShowHoleLoopIds.size(); ++vLoop)
            {
                bool needFill = true;
                for (std::vector<GPP::Int>::iterator loopItr = mShowHoleLoopIds.at(vLoop).begin(); loopItr != mShowHoleLoopIds.at(vLoop).end(); ++loopItr)
                {
                    if (mVertexSelectFlag.at(*loopItr))
                    {
                        needFill = false;
                        break;
                    }
                }
                if (mShowHoleLoopIds.at(vLoop).size() > 0 && needFill)
                {
                    holeSeeds.push_back(mShowHoleLoopIds.at(vLoop).at(0));
                }
            }
            
            GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
            int originVertexCount = triMesh->GetVertexCount();
#if MAKEDUMPFILE
            GPP::DumpOnce();
#endif
            GPP::ErrorCode res = GPP_NO_ERROR;
            if (triMesh->HasVertexColor())
            {
                std::vector<GPP::Real> vertexScaleFields, outputScaleFields;
                CollectTriMeshVerticesColorFields(triMesh, &vertexScaleFields);
                res = GPP::FillMeshHole::FillHoles(triMesh, &holeSeeds, GPP::FillMeshHoleType(mFillHoleType),
                    &vertexScaleFields, &outputScaleFields);
                if (res == GPP_NO_ERROR)
                {
                    UpdateTriMeshVertexColors(triMesh, originVertexCount, outputScaleFields);
                }
            }
            else
            {
                res = GPP::FillMeshHole::FillHoles(triMesh, &holeSeeds, GPP::FillMeshHoleType(mFillHoleType), NULL, NULL);
            }
            mIsCommandInProgress = false;
            if (res == GPP_API_IS_NOT_AVAILABLE)
            {
                MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
                MagicCore::ToolKit::Get()->SetAppRunning(false);
            }
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "网格补洞失败", "温馨提示", MB_OK);
                return;
            }
            std::vector<GPP::ImageColorId>* imageColorIds = ModelManager::Get()->GetImageColorIdsPointer();
            if (imageColorIds && imageColorIds->size() == originVertexCount)
            {
                int curVertexCount = triMesh->GetVertexCount();
                if (curVertexCount > originVertexCount)
                {
                    imageColorIds->resize(curVertexCount, GPP::ImageColorId(-1, 0, 0));
                    std::vector<int>* colorIds = ModelManager::Get()->GetColorIdsPointer();
                    if (colorIds && colorIds->size() == originVertexCount)
                    {
                        colorIds->resize(curVertexCount, -1);
                    }
                    std::vector<int>* imageColorIdFlags = ModelManager::Get()->GetImageColorIdFlagsPointer();
                    if (imageColorIdFlags && imageColorIdFlags->size() == originVertexCount)
                    {
                        imageColorIdFlags->resize(curVertexCount, 0);
                    }
                }
            }
            ResetSelection();           
            SetToShowHoleLoopVrtIds(std::vector<std::vector<GPP::Int> >());
            SetBoundarySeedIds(std::vector<GPP::Int>());
            triMesh->UpdateNormal();
            mUpdateMeshRendering = true;
            mUpdateHoleRendering = true;
            mpUI->SetMeshInfo(triMesh->GetVertexCount(), triMesh->GetTriangleCount());
            mpUI->ResetFillHole();
        }
    }

    void MeshShopApp::DoBridgeEdges()
    {
        if (ModelManager::Get()->GetMesh() == NULL)
        {
            return;
        }
        
        if (mBridgeEdgeVertices.size() == 0 || mBridgeEdgeVertices.size() == 2)
        {
            for (int holeLoopId = 0; holeLoopId < mShowHoleLoopIds.size(); ++holeLoopId)
            {
                bool isHoleSelected = false;
                std::vector<bool> isSelectedTags(mShowHoleLoopIds.at(holeLoopId).size(), false);
                GPP::Int oneSelectedId = -1;
                for (int vid = 0; vid < mShowHoleLoopIds.at(holeLoopId).size(); ++vid)
                {
                    GPP::Int vertexId = mShowHoleLoopIds.at(holeLoopId).at(vid);
                    if (mVertexBridgeFlag.at(vertexId))
                    {
                        isSelectedTags.at(vid) = true;
                        isHoleSelected = true;
                        oneSelectedId = vid;
                    }
                }
                if (isHoleSelected)
                {
                    // find the "most center" id
                    int allCount = isSelectedTags.size();
                    int longestStartId = -1, longestEndId = -1;
                    int tmpStartId = -1;
                    int longestSize = 0;
                    int currentId = 0;
                    bool bFind = false;
                    for (; currentId < allCount; ++currentId)
                    {
                        int nextId = (currentId + 1) % allCount;
                        if (!isSelectedTags.at(currentId) && isSelectedTags.at(nextId))
                        {
                            bFind = true;
                            break;
                        }
                    }
                    if (!bFind)
                    {
                        GPP::Int nextId = (oneSelectedId + 1) % allCount;
                        mBridgeEdgeVertices.push_back(mShowHoleLoopIds.at(holeLoopId).at(oneSelectedId));
                        mBridgeEdgeVertices.push_back(mShowHoleLoopIds.at(holeLoopId).at(nextId));
                        break;
                    }

                    //
                    currentId ++;
                    if (currentId >= allCount)
                    {
                        currentId -= allCount;
                    }
                    int startId = currentId;
                    do 
                    {
                        if (!isSelectedTags.at(currentId))
                        {
                            currentId ++;
                            if (currentId >= allCount)
                            {
                                currentId = 0;
                            }
                            continue;
                        }

                        tmpStartId = currentId;
                        do 
                        {
                            int nextId = currentId + 1;
                            if (nextId >= allCount)
                            {
                                nextId = 0;
                            }
                            if (!isSelectedTags.at(nextId))
                            {
                                int count = (nextId - tmpStartId + allCount) % allCount;
                                if (count > longestSize)
                                {
                                    longestStartId = tmpStartId;
                                    longestEndId = currentId;
                                    longestSize = count;
                                }
                                break;
                            }

                            currentId ++;
                            if (currentId >= allCount)
                            {
                                currentId = 0;
                            }
                        } while (currentId != tmpStartId);
                        currentId ++;
                        if (currentId >= allCount)
                        {
                            currentId = 0;
                        }
                    } while (currentId != startId);
                    if (longestSize <= 2)
                    {
                        mBridgeEdgeVertices.push_back(mShowHoleLoopIds.at(holeLoopId).at(longestStartId));
                        mBridgeEdgeVertices.push_back(mShowHoleLoopIds.at(holeLoopId).at((longestStartId + 1) % allCount));
                    }
                    else
                    {
                        int centerId = (longestStartId + longestEndId) / 2;
                        if (longestStartId > longestEndId)
                        {
                            centerId = (longestStartId + longestEndId + allCount) / 2;
                            centerId %= allCount;
                        }
                        int nextId = (centerId + 1) % allCount;
                        mBridgeEdgeVertices.push_back(mShowHoleLoopIds.at(holeLoopId).at(centerId));
                        mBridgeEdgeVertices.push_back(mShowHoleLoopIds.at(holeLoopId).at(nextId));
                    }
                    break;
                }
            }
            mUpdateBridgeRendering = true;
        }
        else
        {
            ResetBridgeTags();
        }

        if (mBridgeEdgeVertices.size() == 2)
        {
            ResetBridgeTags();
        }
        else if (mBridgeEdgeVertices.size() == 4)
        {
            GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
            GPP::Int edgeVertex1[2] = {mBridgeEdgeVertices[0], mBridgeEdgeVertices[1]};
            GPP::Int edgeVertex2[2] = {mBridgeEdgeVertices[2], mBridgeEdgeVertices[3]};
            if (triMesh->HasVertexColor())
            {
                std::vector<GPP::Real> vertexScaleFields, outputScaleFields;
                CollectTriMeshVerticesColorFields(triMesh, &vertexScaleFields);
                GPP::Int oldTriMeshVertexSize = triMesh->GetVertexCount();
#if MAKEDUMPFILE
                GPP::DumpOnce();
#endif
                GPP::ErrorCode res = GPP::FillMeshHole::BridgeEdges(triMesh, edgeVertex1, edgeVertex2, &vertexScaleFields, &outputScaleFields);
                if (res == GPP_API_IS_NOT_AVAILABLE)
                {
                    MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
                    MagicCore::ToolKit::Get()->SetAppRunning(false);
                }
                if (res != GPP_NO_ERROR)
                {
                    MessageBox(NULL, "网格搭桥失败", "温馨提示", MB_OK);
                }
                UpdateTriMeshVertexColors(triMesh, oldTriMeshVertexSize, outputScaleFields);
            }
            else
            {
#if MAKEDUMPFILE
                GPP::DumpOnce();
#endif
                GPP::ErrorCode res = GPP::FillMeshHole::BridgeEdges(triMesh, edgeVertex1, edgeVertex2, NULL, NULL);
                if (res == GPP_API_IS_NOT_AVAILABLE)
                {
                    MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
                    MagicCore::ToolKit::Get()->SetAppRunning(false);
                }
                if (res != GPP_NO_ERROR)
                {
                    MessageBox(NULL, "网格搭桥失败", "温馨提示", MB_OK);
                }
            }          
            ResetSelection();
            FindHole(false);
            triMesh->UpdateNormal();
            mUpdateMeshRendering = true;
            mUpdateHoleRendering = true;
            mUpdateBridgeRendering = true;
            mpUI->SetMeshInfo(triMesh->GetVertexCount(), triMesh->GetTriangleCount());
            mpUI->ResetFillHole();
            mRightMouseType = MOVE;
        }
    }

    void MeshShopApp::BridgeEdges(bool isSubThread)
    {
        if (ModelManager::Get()->GetMesh() == NULL)
        {
            return;
        }
        mRightMouseType = SELECT_BRIDGE;
        ResetBridgeTags();
        if (mBridgeEdgeVertices.empty())
        {
            MessageBox(NULL, "进入搭桥模式: 请使用鼠标右键依次框选需要进行搭桥的两对边", "温馨提示", MB_OK);
        }
        else
        {
            MessageBox(NULL, "搭桥模式: 请使用鼠标右键再选择一条边进行搭桥", "温馨提示", MB_OK);
        }
    }

    void MeshShopApp::UniformOffsetMesh(double offsetValue)
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
#if MAKEDUMPFILE
        GPP::DumpOnce();
#endif
        GPP::ErrorCode res = GPP::OffsetMesh::UniformApproximate(triMesh, offsetValue);
        if (res == GPP_API_IS_NOT_AVAILABLE)
        {
            MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
            MagicCore::ToolKit::Get()->SetAppRunning(false);
        }
        if (res != GPP_NO_ERROR)
        {
            MessageBox(NULL, "网格抽壳失败", "温馨提示", MB_OK);
            return;
        }
        ResetSelection();
        mpUI->SetMeshInfo(triMesh->GetVertexCount(), triMesh->GetTriangleCount());
        UpdateMeshRendering();
    }

    void MeshShopApp::SelectByRectangle()
    {
        mRightMouseType = SELECT_ADD;
        ResetBridgeTags();
        mBridgeEdgeVertices.clear();
        UpdateBridgeRendering();
    }

    void MeshShopApp::EraseByRectangle()
    {
        mRightMouseType = SELECT_DELETE;
        ResetBridgeTags();
        mBridgeEdgeVertices.clear();
        UpdateBridgeRendering();
    }
 
    void MeshShopApp::DeleteSelections()
    {
        GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
        if (triMesh == NULL)
        {
            return;
        }
        std::vector<GPP::Int> deleteIndex;
        GPP::Int vertexCount = triMesh->GetVertexCount();
        for (GPP::Int vid = 0; vid < vertexCount; vid++)
        {
            if (mVertexSelectFlag.at(vid))
            {
                deleteIndex.push_back(vid);
            }
        }
        MagicMesh magicMesh(triMesh);
        ConstructMagicMeshInfo(&magicMesh);
        //res = GPP::DeleteTriMeshVertices(triMesh, deleteIndex);
        GPP::ErrorCode res = GPP::DeleteTriMeshVertices(&magicMesh, deleteIndex);
        if (res != GPP_NO_ERROR)
        {
            MessageBox(NULL, "删点失败", "温馨提示", MB_OK);
            return;
        }
        if (GPP::ConsolidateMesh::_IsTriMeshManifold(triMesh) == false)
        {
            if (MessageBox(NULL, "警告：删除面片后的网格有非流形结构，是否需要修复？", "温馨提示", MB_OKCANCEL) == IDOK)
            {
                std::map<int, int> insertVertexIdMap;
                GPP::ErrorCode res = GPP::ConsolidateMesh::MakeTriMeshManifold(&magicMesh, &insertVertexIdMap);
                if (res == GPP_API_IS_NOT_AVAILABLE)
                {
                    MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
                    MagicCore::ToolKit::Get()->SetAppRunning(false);
                }
                if (res != GPP_NO_ERROR)
                {
                    MessageBox(NULL, "拓扑修复失败", "温馨提示", MB_OK);
                    return;
                }
                UpdateAddedVertexInfo(insertVertexIdMap);
            }
        }
        ResetSelection();
        triMesh->UpdateNormal();
        mUpdateMeshRendering = true;
        mpUI->SetMeshInfo(triMesh->GetVertexCount(), triMesh->GetTriangleCount());
        mpUI->ResetFillHole();
        FindHole(false);
    }

    void MeshShopApp::IgnoreBack(bool ignore)
    {
        mIgnoreBack = ignore;
    }

    void MeshShopApp::MoveModel(void)
    {
        mRightMouseType = MOVE;
    }

    void MeshShopApp::SplitMeshByPlane(SplitType st, double offsetValue)
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
        GPP::Vector3 planeCenter(0, 0, 0);
        GPP::Vector3 planeNormal(0, 0, 0);
        if (st == ST_PLANE_XY)
        {
            planeCenter[2] = offsetValue;
            planeNormal = GPP::Vector3(0, 0, 1);
        }
        else if (st == ST_PLANE_YZ)
        {
            planeCenter[0] = offsetValue;
            planeNormal = GPP::Vector3(1, 0, 0);
        }
        else if (st == ST_PLANE_ZX)
        {
            planeCenter[1] = offsetValue;
            planeNormal = GPP::Vector3(0, 1, 0);
        }
        else if (st == ST_PLANE_RANDOM)
        {
            int resolution = 1000;
            double randRange = 0.2; 
            planeNormal[0] = (rand() % resolution) / double(resolution) * randRange - randRange / 2.0; 
            planeNormal[1] = (rand() % resolution) / double(resolution) * randRange - randRange / 2.0;  
            planeNormal[2] = (rand() % resolution) / double(resolution) * randRange - randRange / 2.0; 
            if (planeNormal.Normalise() < GPP::REAL_TOL)
            {
                planeNormal = GPP::Vector3(0, 0, 1);
            }
            planeCenter += planeNormal * offsetValue;
        }
        else
        {
            return;
        }
        
        GPP::Plane3 plane(planeCenter, planeNormal);
        std::vector<bool> triangleFlags;
#if MAKEDUMPFILE
        GPP::DumpOnce();
#endif
        GPP::ErrorCode res = GPP_NO_ERROR;
        if (triMesh->HasVertexColor())
        {
            std::vector<GPP::Real> vertexFields, insertedFields;
            CollectTriMeshVerticesColorFields(triMesh, &vertexFields);
            int originVertexCount = triMesh->GetVertexCount();
            res = GPP::SplitMesh::SplitByPlane(triMesh, &plane, &triangleFlags, &vertexFields, &insertedFields);
            if (res == GPP_NO_ERROR)
            {
                UpdateTriMeshVertexColors(triMesh, originVertexCount, insertedFields);
            }
        }
        else
        {
            res = GPP::SplitMesh::SplitByPlane(triMesh, &plane, &triangleFlags, NULL, NULL);
        }
        if (res == GPP_API_IS_NOT_AVAILABLE)
        {
            MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
            MagicCore::ToolKit::Get()->SetAppRunning(false);
        }
        if (res != GPP_NO_ERROR)
        {
            MessageBox(NULL, "SplitByPlane failed", "温馨提示", MB_OK);
            return;
        }
        std::vector<int> deleteIndex;
        int faceCount = triMesh->GetTriangleCount();
        for (int fid = 0; fid < faceCount; fid++)
        {
            if (!triangleFlags.at(fid))
            {
                deleteIndex.push_back(fid);
            }
        }
        res = GPP::DeleteTriMeshTriangles(triMesh, deleteIndex, true);
        if (res != GPP_NO_ERROR)
        {
            MessageBox(NULL, "DeleteTriMeshTriangles failed", "温馨提示", MB_OK);
            return;
        }
        triMesh->UpdateNormal();
        mpUI->SetMeshInfo(triMesh->GetVertexCount(), triMesh->GetTriangleCount());
        ResetSelection();
        UpdateMeshRendering();
        if (triMesh->GetVertexCount() < 3 || triMesh->GetTriangleCount() < 1)
        {
            MessageBox(NULL, "网格已空", "温馨提示", MB_OK);
            return;
        }
    }

    int MeshShopApp::GetMeshVertexCount()
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

    void MeshShopApp::UpdateMeshRendering()
    {
        if (MagicCore::ScriptSystem::Get()->IsOnRunningScript())
        {
            return ;
        }
        if (ModelManager::Get()->GetMesh() == NULL)
        {
            MagicCore::RenderSystem::Get()->HideRenderingObject("Mesh_MeshShop");
            return;
        }
        GPP::Vector3 selectColor(1, 0, 0);
        MagicCore::RenderSystem::Get()->RenderMesh("Mesh_MeshShop", "CookTorrance", ModelManager::Get()->GetMesh(), 
            MagicCore::RenderSystem::MODEL_NODE_CENTER, &mVertexSelectFlag, &selectColor, mIsFlatRenderingMode);
    }

    void MeshShopApp::UpdateHoleRendering()
    {
        if (MagicCore::ScriptSystem::Get()->IsOnRunningScript())
        {
            return;
        }

        GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
        if (triMesh == NULL)
        {
            return;
        }
        // reset the render object
        MagicCore::RenderSystem::Get()->RenderPolyline("MeshShop_Holes", "SimpleLine", GPP::Vector3(1.0,0.0,0.0), std::vector<GPP::Vector3>(), true);
        // append each polyline to the render object
        for (GPP::Int vLoop = 0; vLoop < mShowHoleLoopIds.size(); ++vLoop)
        {
            int vSize = mShowHoleLoopIds.at(vLoop).size();
            std::vector<GPP::Vector3> positions(vSize + 1);
            for (GPP::Int vId = 0; vId < vSize; ++vId)
            {
                positions.at(vId) = triMesh->GetVertexCoord(mShowHoleLoopIds[vLoop][vId]);
            }
            positions.at(vSize) = triMesh->GetVertexCoord(mShowHoleLoopIds[vLoop][0]);
            MagicCore::RenderSystem::Get()->RenderPolyline("MeshShop_Holes", "SimpleLine", GPP::Vector3(1.0,0.0,0.0), positions, false);
        }
        // show seeds
        GPP::Int boundarySeedSize = mBoundarySeedIds.size();
        std::vector<GPP::Vector3> holeSeeds(boundarySeedSize, GPP::Vector3());
        for (GPP::Int vId = 0; vId < boundarySeedSize; ++vId)
        {
            holeSeeds.at(vId) = triMesh->GetVertexCoord(mBoundarySeedIds[vId]);
        }
        MagicCore::RenderSystem::Get()->RenderPointList("MeshShop_HoleSeeds", "SimplePoint", GPP::Vector3(1.0, 0.0, 0.0), holeSeeds);
    }

    void MeshShopApp::UpdateBridgeRendering()
    {
        if (MagicCore::ScriptSystem::Get()->IsOnRunningScript())
        {
            return;
        }
        GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
        if (triMesh == NULL)
        {
            return;
        }
        MagicCore::RenderSystem::Get()->RenderPointList("MeshShop_Bridge", "CookTorrancePointLarge", 
            GPP::Vector3(0.0, 1.0, 1.0), std::vector<GPP::Vector3>());
        std::vector<GPP::Vector3> bridgeVertices;
        if (mBridgeEdgeVertices.size() < 2)
        {
            return;
        }
        for (int vid = 0; vid < mBridgeEdgeVertices.size(); ++vid)
        {
            bridgeVertices.push_back(triMesh->GetVertexCoord(mBridgeEdgeVertices.at(vid)));
        }
        bridgeVertices.push_back(bridgeVertices.front());
        MagicCore::RenderSystem::Get()->RenderPointList("MeshShop_Bridge", "CookTorrancePointLarge", 
            GPP::Vector3(0.0, 1.0, 1.0), bridgeVertices);
    }

    void MeshShopApp::SaveImageColorInfo()
    {
        std::string fileName;
        char filterName[] = "Support format(*.gii)\0*.*\0";
        if (MagicCore::ToolKit::FileSaveDlg(fileName, filterName))
        {
            std::ofstream fout(fileName);
            std::vector<GPP::ImageColorId> imageColorIds = ModelManager::Get()->GetImageColorIds();
            int imageIdCount = imageColorIds.size();
            fout << imageIdCount << " ";
            GPP::ImageColorId curId;
            for (int iid = 0; iid < imageIdCount; iid++)
            {
                curId = imageColorIds.at(iid);
                fout << curId.GetImageIndex() << " " << curId.GetLocalX() << " " << curId.GetLocalY() << " ";
            }
            std::vector<std::string> textureImageFiles = ModelManager::Get()->GetTextureImageFiles();
            int imageCount = textureImageFiles.size();
            fout << imageCount << "\n";
            for (int fid = 0; fid < imageCount; fid++)
            {
                fout << textureImageFiles.at(fid) << "\n";
            }
            fout << std::endl;
            fout.close();
        }
    }
    
    void MeshShopApp::LoadImageColorInfo()
    {
        std::string fileName;
        char filterName[] = "Geometry++ Image Info(*.gii)\0*.gii\0";
        if (MagicCore::ToolKit::FileOpenDlg(fileName, filterName))
        {
            std::ifstream fin(fileName);
            if (!fin)
            {
                MessageBox(NULL, "GII导入失败", "温馨提示", MB_OK);
                return;
            }
            int imageIdCount = 0;
            fin >> imageIdCount;
            int imageId, posX, posY;
            std::vector<GPP::ImageColorId> imageColorIds;
            imageColorIds.reserve(imageIdCount);
            for (int iid = 0; iid < imageIdCount; iid++)
            {
                fin >> imageId >> posX >> posY;
                imageColorIds.push_back(GPP::ImageColorId(imageId, posX, posY));
            }
            ModelManager::Get()->SetImageColorIds(imageColorIds);

            int imageCount = 0;
            fin >> imageCount;
            std::vector<std::string> textureImageFiles;
            textureImageFiles.reserve(imageCount);
            for (int fid = 0; fid < imageCount; fid++)
            {
                std::string filePath;
                fin >> filePath;
                textureImageFiles.push_back(filePath);
            }
            fin.close();

            std::vector<std::string> fileNames;
            char filterName[] = "JPG Files(*.jpg)\0*.jpg\0PNG Files(*.png)\0*.png\0";
            if (MagicCore::ToolKit::MultiFileOpenDlg(fileNames, filterName))
            {
                textureImageFiles = fileNames;
            }
            ModelManager::Get()->SetTextureImageFiles(textureImageFiles);
        }
    }

    void MeshShopApp::RunScript(bool isSubThread)
    {
        if (MagicCore::ScriptSystem::Get()->IsOnRunningScript())
        {
            MessageBox(NULL, "请等待前一脚本执行完毕", "温馨提示", MB_OK);
            return;
        }
        if (IsCommandInProgress())
        {
            return;
        }
        if (isSubThread)
        {
            mCommandType = RUNSCRIPT;
            DoCommand(true);
        }
        else
        {
            std::string fileName;
            char filterName[] = "GPP Script File(*.gsf)\0*.gsf\0";
            if (MagicCore::ToolKit::FileOpenDlg(fileName, filterName))
            {
                InfoLog << "Run Script file: " << fileName.c_str() << std::endl;
                if (MagicCore::ScriptSystem::Get()->RunScriptFile(fileName.c_str()))
                {
                    GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
                    if (triMesh)
                    {
                        if (mpUI)
                        {
                            mpUI->SetMeshInfo(triMesh->GetVertexCount(), triMesh->GetTriangleCount());
                            mpUI->ResetFillHole();
                            FindHole(false);
                            mpUI->StopProgressbar();
                        }
                    }
                    ResetSelection();
                    mUpdateMeshRendering = true;
                    mUpdateBridgeRendering = true;
                    mUpdateHoleRendering = true;
                    mUpdateMeshRendering = true;
                    MessageBox(NULL, "脚本执行完毕", "温馨提示", MB_OK);
                }
            }
        }
    }

    void MeshShopApp::PickMeshColorFromImages()
    {
        GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
        if (triMesh == NULL)
        {
            MessageBox(NULL, "mpTriMesh == NULL", "温馨提示", MB_OK);
            return;
        }
        std::vector<std::string> textureImageFiles = ModelManager::Get()->GetTextureImageFiles();
        if (textureImageFiles.empty())
        {
            MessageBox(NULL, "mTextureImageFiles is empty", "温馨提示", MB_OK);
            return;
        }
        std::vector<GPP::ImageColorId> imageColorIds = ModelManager::Get()->GetImageColorIds();
        if (imageColorIds.size() != triMesh->GetVertexCount())
        {
            MessageBox(NULL, "mImageColorIds.size() != mpTriMesh->GetVertexCount()", "温馨提示", MB_OK);
            return;
        }
        int imageCount = textureImageFiles.size();
        std::vector<cv::Mat> imageList(imageCount);
        std::vector<int> imageHList(imageCount);
        for (int fid = 0; fid < imageCount; fid++)
        {
            imageList.at(fid) = cv::imread(textureImageFiles.at(fid));
            if (imageList.at(fid).data == NULL)
            {
                MessageBox(NULL, "Image导入失败", "温馨提示", MB_OK);
                return;
            }
            imageHList.at(fid) = imageList.at(fid).rows;
        }
        int vertexCount = triMesh->GetVertexCount();
        triMesh->SetHasVertexColor(true);
        for (int vid = 0; vid < vertexCount; vid++)
        {
            GPP::ImageColorId colorId = imageColorIds.at(vid);
            int imageIndex = colorId.GetImageIndex();
            const unsigned char* pixel = imageList.at(imageIndex).ptr(imageHList.at(imageIndex) - colorId.GetLocalY() - 1,
                colorId.GetLocalX());
            triMesh->SetVertexColor(vid, GPP::Vector3(double(pixel[2]) / 255.0, double(pixel[1]) / 255.0, double(pixel[0]) / 255.0));
        }
        for (int fid = 0; fid < imageCount; fid++)
        {
            imageList.at(fid).release();
        }

        UpdateMeshRendering();
    }

    void MeshShopApp::ConstructMagicMeshInfo(MagicMesh* magicMesh)
    {
        std::vector<GPP::ImageColorId>* imageColorIds = ModelManager::Get()->GetImageColorIdsPointer();
        if (imageColorIds && imageColorIds->size() == magicMesh->GetVertexCount())
        {
            magicMesh->SetImageColorIds(imageColorIds);
        }
        std::vector<int>* imageColorIdFlags = ModelManager::Get()->GetImageColorIdFlagsPointer();
        if (imageColorIdFlags && imageColorIdFlags->size() == magicMesh->GetVertexCount())
        {
            magicMesh->SetImageColorIdFlags(imageColorIdFlags);
        }
        std::vector<int>* colorIds = ModelManager::Get()->GetColorIdsPointer();
        if (colorIds && colorIds->size() == magicMesh->GetVertexCount())
        {
            magicMesh->SetColorIds(colorIds);
        }
    }

    void MeshShopApp::SetToShowHoleLoopVrtIds(const std::vector<std::vector<GPP::Int> >& toShowHoleLoopVrtIds)
    {
        if (ModelManager::Get()->GetMesh() == NULL || toShowHoleLoopVrtIds.empty())
        {
            mShowHoleLoopIds.clear();
            return;
        }

        mShowHoleLoopIds = toShowHoleLoopVrtIds;
    }

    void MeshShopApp::SetBoundarySeedIds(const std::vector<GPP::Int>& holeSeedIds)
    {
        mBoundarySeedIds = holeSeedIds;
    }

    void MeshShopApp::InitViewTool()
    {
        if (mpViewTool == NULL)
        {
            mpViewTool = new MagicCore::ViewTool;
        }
    }
}

#include "stdafx.h"
#include "Homepage.h"
#include "HomepageUI.h"
#include "AppManager.h"
#include "ModelManager.h"
#include "PointShopApp.h"
#include "MeshShopApp.h"
#include "RegistrationApp.h"
#include "MeasureApp.h"
#include "ReliefApp.h"
#include "DepthVideoApp.h"
#include "../Common/LogSystem.h"
#include "../Common/ToolKit.h"
#include "../Common/ViewTool.h"
#include "DumpInfo.h"

namespace MagicApp
{
    Homepage::Homepage() :
        mpUI(NULL),
        mpViewTool(NULL),
#if DEBUGDUMPFILE
        mpDumpInfo(NULL),
#endif
        mDisplayMode(0)
    {
    }

    Homepage::~Homepage()
    {
        if (mpUI != NULL)
        {
            delete mpUI;
            mpUI = NULL;
        }
    }

    bool Homepage::Enter()
    {
        InfoLog << "Enter Homepage" << std::endl;
        if (mpUI == NULL)
        {
            mpUI = new HomepageUI;
        }
        mpUI->Setup();
        SetupScene();
        UpdateModelRendering();
        return true;
    }

    bool Homepage::Update(double timeElapsed)
    {
        return true;
    }

    bool Homepage::Exit()
    {
        InfoLog << "Exit Homepage" << std::endl;
        ShutdownScene();
        if (mpUI != NULL)
        {
            mpUI->Shutdown();
        }
        ClearData();
        return true;
    }

    bool Homepage::MouseMoved( const OIS::MouseEvent &arg )
    {
        if (arg.state.buttonDown(OIS::MB_Middle) && mpViewTool != NULL)
        {
            mpViewTool->MouseMoved(arg.state.X.abs, arg.state.Y.abs, MagicCore::ViewTool::MM_MIDDLE_DOWN);
        }
        else if (arg.state.buttonDown(OIS::MB_Right) && mpViewTool != NULL)
        {
            mpViewTool->MouseMoved(arg.state.X.abs, arg.state.Y.abs, MagicCore::ViewTool::MM_RIGHT_DOWN);
        }
        else if (arg.state.buttonDown(OIS::MB_Left) && mpViewTool != NULL)
        {
            mpViewTool->MouseMoved(arg.state.X.abs, arg.state.Y.abs, MagicCore::ViewTool::MM_LEFT_DOWN);
        } 
        return true;
    }

    bool Homepage::MousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
    {
        if (mpViewTool != NULL)
        {
            mpViewTool->MousePressed(arg.state.X.abs, arg.state.Y.abs);
        }
        return true;
    }

    bool Homepage::MouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
    {
        if (mpViewTool != NULL)
        {
            mpViewTool->MouseReleased(); 
        }
        return true;
    }

    void Homepage::WindowFocusChanged( Ogre::RenderWindow* rw)
    {
        if (mpViewTool)
        {
            mpViewTool->MouseReleased();
        }
    }

    bool Homepage::KeyPressed( const OIS::KeyEvent &arg )
    {
        if (arg.key == OIS::KC_D)
        {
#if DEBUGDUMPFILE
            LoadDumpFile();
#endif
        }
        if (arg.key == OIS::KC_R)
        {
#if DEBUGDUMPFILE
            RunDumpInfo();
#endif
        }
        else if (arg.key == OIS::KC_V)
        {
            AppManager::Get()->EnterApp(new DepthVideoApp, "DepthVideoApp");
        }
        else if (arg.key == OIS::KC_U)
        {
#if DEVELOPSTATE
            ExportModel(true);
#endif
        }
        else if (arg.key == OIS::KC_S)
        {
#if DEVELOPSTATE
            ExportModel(false);
#endif
        }
        return true;
    }

#if DEBUGDUMPFILE
    void Homepage::LoadDumpFile(void)
    {
        std::string fileName;
        char filterName[] = "Dump Files(*.dump)\0*.dump\0";
        if (MagicCore::ToolKit::FileOpenDlg(fileName, filterName))
        {
            std::ifstream fin(fileName.c_str());
            int dumpApiName;
            fin >> dumpApiName;
            fin.close();
            GPPFREEPOINTER(mpDumpInfo);
            mpDumpInfo = GPP::DumpManager::Get()->GetDumpInstance(dumpApiName);
            if (mpDumpInfo == NULL)
            {
                MessageBox(NULL, "dump文件打开失败", "温馨提示", MB_OK);
                return;
            }
            mpDumpInfo->LoadDumpFile(fileName);
            if (dumpApiName == GPP::POINT_REGISTRATION_ICP || dumpApiName == GPP::POINT_REGISTRATION_GLOBAL ||
                dumpApiName == GPP::POINT_REGISTRATION_ALIGNPOINTCLOUD || dumpApiName == GPP::POINT_REGISTRATION_ALIGNPOINTCLOUD_MARK ||
                dumpApiName == GPP::INTRINSICCOLOR_TUNE_COLOR_SINGLE_LIGHT)
            {
                AppManager::Get()->EnterApp(new RegistrationApp, "RegistrationApp");
                RegistrationApp* registrationApp = dynamic_cast<RegistrationApp*>(AppManager::Get()->GetApp("RegistrationApp"));
                if (registrationApp)
                {
                    registrationApp->SetDumpInfo(mpDumpInfo);
                    mpDumpInfo = NULL;
                }
                else
                {
                    GPPFREEPOINTER(mpDumpInfo);
                }
            }
            else if (dumpApiName == GPP::MESH_MEASURE_SECTION_APPROXIMATE || dumpApiName == GPP::MESH_MEASURE_SECTION_EXACT ||
                dumpApiName == GPP::MESH_MEASURE_SECTION_FAST_EXACT)
            {
                AppManager::Get()->EnterApp(new MeasureApp, "MeasureApp");
                MeasureApp* measureApp = dynamic_cast<MeasureApp*>(AppManager::Get()->GetApp("MeasureApp"));
                if (measureApp)
                {
                    measureApp->SetDumpInfo(mpDumpInfo);
                    mpDumpInfo = NULL;
                }
                else
                {
                    GPPFREEPOINTER(mpDumpInfo);
                }
            }
            else
            {
                if (mpDumpInfo->GetPointCloud(0) != NULL)
                {
                    GPP::PointCloud* copiedPointCloud = GPP::CopyPointCloud(mpDumpInfo->GetPointCloud(0));
                    copiedPointCloud->UnifyCoords(2.0);
                    ModelManager::Get()->SetPointCloud(copiedPointCloud);
                    ModelManager::Get()->ClearMesh();
                    UpdateModelRendering();
                }
                else if (mpDumpInfo->GetTriMesh(0) != NULL)
                {
                    GPP::TriMesh* copiedTriMesh = GPP::CopyTriMesh(mpDumpInfo->GetTriMesh(0));
                    copiedTriMesh->UnifyCoords(2.0);
                    copiedTriMesh->UpdateNormal();
                    ModelManager::Get()->SetMesh(copiedTriMesh);
                    ModelManager::Get()->ClearPointCloud();
                    UpdateModelRendering();
                }
            }
        }
    }

    void Homepage::RunDumpInfo()
    {
        if (mpDumpInfo == NULL)
        {
            MessageBox(NULL, "请先导入dump文件", "温馨提示", MB_OK);
            return;
        }
        GPP::ErrorCode res = mpDumpInfo->Run();
        if (res != GPP_NO_ERROR)
        {
            MessageBox(NULL, "dump文件执行失败", "温馨提示", MB_OK);
            //return;
        }
        else
        {
            MessageBox(NULL, "dump文件执行成功", "温馨提示", MB_OK);
        }

        if (mpDumpInfo->GetPointCloud(0) != NULL)
        {
            GPP::PointCloud* copiedPointCloud = GPP::CopyPointCloud(mpDumpInfo->GetPointCloud(0));
            copiedPointCloud->UnifyCoords(2.0);
            ModelManager::Get()->SetPointCloud(copiedPointCloud);
            ModelManager::Get()->ClearMesh();
            UpdateModelRendering();
        }
        else if (mpDumpInfo->GetTriMesh(0) != NULL)
        {
            GPP::TriMesh* copiedTriMesh = GPP::CopyTriMesh(mpDumpInfo->GetTriMesh(0));
            copiedTriMesh->UnifyCoords(2.0);
            copiedTriMesh->UpdateNormal();
            ModelManager::Get()->SetMesh(copiedTriMesh);
            ModelManager::Get()->ClearPointCloud();
            UpdateModelRendering();
            if (GPP::ConsolidateMesh::_IsTriMeshManifold(copiedTriMesh) == false)
            {
                MessageBox(NULL, "网格有非流形结构", "温馨提示", MB_OK);
            }
        }
    }
#endif

    void Homepage::SwitchDisplayMode()
    {
        mDisplayMode++;
        if (mDisplayMode > 2)
        {
            mDisplayMode = 0;
        }
        if (mDisplayMode == 0)
        {
            MagicCore::RenderSystem::Get()->GetMainCamera()->setPolygonMode(Ogre::PolygonMode::PM_SOLID);
        }
        else if (mDisplayMode == 1)
        {
            MagicCore::RenderSystem::Get()->GetMainCamera()->setPolygonMode(Ogre::PolygonMode::PM_WIREFRAME);
        }
        else if (mDisplayMode == 2)
        {
            MagicCore::RenderSystem::Get()->GetMainCamera()->setPolygonMode(Ogre::PolygonMode::PM_POINTS);
        }
        Ogre::Material* material = dynamic_cast<Ogre::Material*>(Ogre::MaterialManager::getSingleton().getByName("CookTorrance").getPointer());
        if (material)
        {
            if (mDisplayMode == 0)
            {
                material->setCullingMode(Ogre::CullingMode::CULL_NONE);
            }
            else
            {
                material->setCullingMode(Ogre::CullingMode::CULL_CLOCKWISE);
            }
        }
        UpdateModelRendering();
    }

    void Homepage::ImportPointCloud()
    {
        std::string fileName;
        char filterName[] = "ASC Files(*.asc)\0*.asc\0OBJ Files(*.obj)\0*.obj\0PLY Files(*.ply)\0*.ply\0Geometry++ Point Cloud(*.gpc)\0*.gpc\0XYZ Files(*.xyz)\0*.xyz\0";
        if (MagicCore::ToolKit::FileOpenDlg(fileName, filterName))
        {
            ModelManager::Get()->ClearMesh();
            if (ModelManager::Get()->ImportPointCloud(fileName) == false)
            {
                MessageBox(NULL, "点云导入失败", "温馨提示", MB_OK);
                return;
            }
            mpUI->SetModelInfo(ModelManager::Get()->GetPointCloud()->GetPointCount(), 0);
            UpdateModelRendering();
        }
    }

    void Homepage::ImportMesh()
    {
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
            mpUI->SetModelInfo(triMesh->GetVertexCount(), triMesh->GetTriangleCount());
            UpdateModelRendering();
        }
    }

    void Homepage::ExportModel(bool unify)
    {
        GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
        if (triMesh)
        {
            std::string fileName;
            char filterName[] = "OBJ Files(*.obj)\0*.obj\0STL Files(*.stl)\0*.stl\0PLY Files(*.ply)\0*.ply\0OFF Files(*.off)\0*.off\0GPT Files(*.gpt)\0*.gpt\0";
            if (MagicCore::ToolKit::FileSaveDlg(fileName, filterName))
            {
                size_t dotPos = fileName.rfind('.');
                if (dotPos == std::string::npos)
                {
                    MessageBox(NULL, "请输入文件后缀名", "温馨提示", MB_OK);
                    return;
                }
                GPP::ErrorCode res = GPP_NO_ERROR;
                if (unify)
                {
                    GPP::Real scaleValue = ModelManager::Get()->GetScaleValue();
                    GPP::Vector3 objCenterCoord = ModelManager::Get()->GetObjCenterCoord();
                    triMesh->UnifyCoords(1.0 / scaleValue, objCenterCoord * (-scaleValue));
                    res = GPP::Parser::ExportTriMesh(fileName, triMesh);
                    triMesh->UnifyCoords(scaleValue, objCenterCoord);
                }
                else
                {
                    res = GPP::Parser::ExportTriMesh(fileName, triMesh);
                }
                if (res != GPP_NO_ERROR)
                {
                    MessageBox(NULL, "网格导出失败", "温馨提示", MB_OK);
                }
            }
        }
        else
        {
            GPP::PointCloud* pointCloud = ModelManager::Get()->GetPointCloud();
            if (pointCloud)
            {
                std::string fileName;
                char filterName[] = "Support format(*.obj, *.ply, *.asc, *.gpc)\0*.*\0";
                if (MagicCore::ToolKit::FileSaveDlg(fileName, filterName))
                {
                    size_t dotPos = fileName.rfind('.');
                    if (dotPos == std::string::npos)
                    {
                        MessageBox(NULL, "请输入文件后缀名", "温馨提示", MB_OK);
                        return;
                    }
                    GPP::ErrorCode res = GPP_NO_ERROR;
                    if (unify)
                    {
                        GPP::Real scaleValue = ModelManager::Get()->GetScaleValue();
                        GPP::Vector3 objCenterCoord = ModelManager::Get()->GetObjCenterCoord();
                        pointCloud->UnifyCoords(1.0 / scaleValue, objCenterCoord * (-scaleValue));
                        res = GPP::Parser::ExportPointCloud(fileName, pointCloud);
                        pointCloud->UnifyCoords(scaleValue, objCenterCoord);
                    }
                    else
                    {
                        res = GPP::Parser::ExportPointCloud(fileName, pointCloud);
                    }
                    if (res != GPP_NO_ERROR)
                    {
                        MessageBox(NULL, "导出点云失败", "温馨提示", MB_OK);
                    }
                }
            }
        }
    }

    void Homepage::SetupScene()
    {
        Ogre::SceneManager* sceneManager = MagicCore::RenderSystem::Get()->GetSceneManager();
        sceneManager->setAmbientLight(Ogre::ColourValue(0.3, 0.3, 0.3));
        Ogre::Light* light = sceneManager->createLight("Homepage_SimpleLight");
        light->setPosition(-5, 5, 20);
        light->setDiffuseColour(0.8, 0.8, 0.8);
        light->setSpecularColour(0.5, 0.5, 0.5);
        if (mpViewTool == NULL)
        {
            mpViewTool = new MagicCore::ViewTool;
        }
        GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
        if (triMesh)
        {
            mpUI->SetModelInfo(triMesh->GetVertexCount(), triMesh->GetTriangleCount());
        }
        GPP::PointCloud* pointCloud = ModelManager::Get()->GetPointCloud();
        if (pointCloud)
        {
            mpUI->SetModelInfo(pointCloud->GetPointCount(), 0);
        }
    }

    void Homepage::ShutdownScene()
    {
        Ogre::SceneManager* sceneManager = MagicCore::RenderSystem::Get()->GetSceneManager();
        sceneManager->setAmbientLight(Ogre::ColourValue::Black);
        sceneManager->destroyLight("Homepage_SimpleLight");
        //MagicCore::RenderSystem::Get()->SetupCameraDefaultParameter();
        MagicCore::RenderSystem::Get()->HideRenderingObject("Mesh_Homepage");
        MagicCore::RenderSystem::Get()->HideRenderingObject("PointCloud_Homepage");
        /*if (MagicCore::RenderSystem::Get()->GetSceneManager()->hasSceneNode("ModelNode"))
        {
            MagicCore::RenderSystem::Get()->GetSceneManager()->getSceneNode("ModelNode")->resetToInitialState();
        } */
    }

    void Homepage::ClearData()
    {
        GPPFREEPOINTER(mpViewTool);
    }

    void Homepage::UpdateModelRendering()
    {
        GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
        if (triMesh)
        {
            MagicCore::RenderSystem::Get()->RenderMesh("Mesh_Homepage", "CookTorrance", triMesh, 
                MagicCore::RenderSystem::MODEL_NODE_CENTER, NULL, NULL, true);
        }
        else
        {
            MagicCore::RenderSystem::Get()->HideRenderingObject("Mesh_Homepage");
        }

        GPP::PointCloud* pointCloud = ModelManager::Get()->GetPointCloud();
        if (pointCloud)
        {
            if (pointCloud->HasNormal())
            {
                MagicCore::RenderSystem::Get()->RenderPointCloud("PointCloud_Homepage", "CookTorrancePoint", pointCloud, 
                    MagicCore::RenderSystem::MODEL_NODE_CENTER, NULL, NULL);
            }
            else
            {
                MagicCore::RenderSystem::Get()->RenderPointCloud("PointCloud_Homepage", "SimplePoint", pointCloud, 
                    MagicCore::RenderSystem::MODEL_NODE_CENTER, NULL, NULL);
            }
        }
        else
        {
            MagicCore::RenderSystem::Get()->HideRenderingObject("PointCloud_Homepage");
        }
    }
}

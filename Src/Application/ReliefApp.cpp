#include "stdafx.h"
#include "ReliefApp.h"
#include "ReliefAppUI.h"
#include "AppManager.h"
#include "ModelManager.h"
#include "../Common/LogSystem.h"
#include "../Common/ToolKit.h"
#include "../Common/ViewTool.h"
#include "../Common/RenderSystem.h"
#include "../Common/ScriptSystem.h"
#include "Gpp.h"

namespace MagicApp
{
    ReliefApp::ReliefApp() :
        mpUI(NULL),
        mpViewTool(NULL),
#if DEBUGDUMPFILE
        mpDumpInfo(NULL),
#endif
        mpReliefMesh(NULL),
        mDisplayMode(TRIMESH),
        mpDepthPointCloud(NULL),
        mDepthImage(),
        mScanResolution(0),
        mImageResolution(0),
        mImageStartId(0),
        mImageSegCount(3)
    {
    }

    ReliefApp::~ReliefApp()
    {
        GPPFREEPOINTER(mpUI);
        GPPFREEPOINTER(mpReliefMesh);
        GPPFREEPOINTER(mpViewTool);
#if DEBUGDUMPFILE
        GPPFREEPOINTER(mpDumpInfo);
#endif
        GPPFREEPOINTER(mpReliefMesh);
        GPPFREEPOINTER(mpDepthPointCloud);
        mDepthImage.release();
    }

    bool ReliefApp::Enter()
    {
        InfoLog << "Enter ReliefApp" << std::endl; 
        if (mpUI == NULL)
        {
            mpUI = new ReliefAppUI;
        }
        mpUI->Setup();
        SetupScene();
        UpdateModelRendering();
        return true;
    }

    bool ReliefApp::Update(double timeElapsed)
    {
        return true;
    }

    bool ReliefApp::Exit()
    {
        InfoLog << "Exit ReliefApp" << std::endl; 
        ShutdownScene();
        if (mpUI != NULL)
        {
            mpUI->Shutdown();
        }
        ClearData();
        return true;
    }

    bool ReliefApp::MouseMoved( const OIS::MouseEvent &arg )
    {
        if (mpViewTool != NULL)
        {
            MagicCore::ViewTool::MouseMode mm;
            if (arg.state.buttonDown(OIS::MB_Left))
            {
                mm = MagicCore::ViewTool::MM_LEFT_DOWN;
            }
            else if (arg.state.buttonDown(OIS::MB_Middle))
            {
                mm = MagicCore::ViewTool::MM_MIDDLE_DOWN;
            }
            else if (arg.state.buttonDown(OIS::MB_Right))
            {
                mm = MagicCore::ViewTool::MM_RIGHT_DOWN;
            }
            else
            {
                mm = MagicCore::ViewTool::MM_NONE;
            }
            mpViewTool->MouseMoved(arg.state.X.abs, arg.state.Y.abs, mm);
        }
        
        return true;
    }

    bool ReliefApp::MousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
    {
        if (mpViewTool != NULL)
        {
            mpViewTool->MousePressed(arg.state.X.abs, arg.state.Y.abs);
        }
        return true;
    }

    bool ReliefApp::MouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
    {        
        if (mpViewTool)
        {
            mpViewTool->MouseReleased();
        }
        return  true;
    }

    bool ReliefApp::KeyPressed( const OIS::KeyEvent &arg )
    {
        if (arg.key == OIS::KC_C)
        {
            GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
            if (triMesh)
            {
                int vertexCount = triMesh->GetVertexCount();
                GPP::Vector3 minCoord(-1.5, -1.5, -1.5);
                double deltaLength = 0.3;
                int maxColorId = 6;
                triMesh->SetHasVertexColor(true);
                for (int vid = 0; vid < vertexCount; vid++)
                {
                    GPP::Vector3 deltaCoord = triMesh->GetVertexCoord(vid) - minCoord;
                    int colorId = int(deltaCoord[0] / deltaLength) + int(deltaCoord[1] / deltaLength) + int(deltaCoord[2] / deltaLength);
                    //int colorId = int((deltaCoord[0] + deltaCoord[1] + deltaCoord[2]) / deltaLength);
                    triMesh->SetVertexColor(vid, MagicCore::ToolKit::ColorCoding(double(colorId % maxColorId + 1) / double(maxColorId)));
                }
                UpdateModelRendering();
            }
        }
        else if (arg.key == OIS::KC_D)
        {
#if DEBUGDUMPFILE
            RunDumpInfo();
#endif
        }
        else if (arg.key == OIS::KC_S)
        {
            mImageStartId = 0;
        }
        else if (arg.key == OIS::KC_N)
        {
            RunScript();
        }
        return true;
    }

    void ReliefApp::WindowFocusChanged(Ogre::RenderWindow* rw)
    {
        if (mpViewTool)
        {
            mpViewTool->MouseReleased();
        }
    }

    void ReliefApp::SetupScene()
    {
        Ogre::SceneManager* sceneManager = MagicCore::RenderSystem::Get()->GetSceneManager();
        sceneManager->setAmbientLight(Ogre::ColourValue(0.5, 0.5, 0.5));
        Ogre::Light* light = sceneManager->createLight("ReliefApp_SimpleLight");
        light->setPosition(0, 0, 20);
        light->setDiffuseColour(0.7, 0.7, 0.7);
        light->setSpecularColour(0.1, 0.1, 0.1);
        InitViewTool();
        mImageStartId = 0;
    }

    void ReliefApp::ShutdownScene()
    {
        Ogre::SceneManager* sceneManager = MagicCore::RenderSystem::Get()->GetSceneManager();
        sceneManager->setAmbientLight(Ogre::ColourValue::Black);
        sceneManager->destroyLight("ReliefApp_SimpleLight");
        //MagicCore::RenderSystem::Get()->SetupCameraDefaultParameter();
        MagicCore::RenderSystem::Get()->HideRenderingObject("Mesh_ReliefApp");
        /*if (MagicCore::RenderSystem::Get()->GetSceneManager()->hasSceneNode("ModelNode"))
        {
            MagicCore::RenderSystem::Get()->GetSceneManager()->getSceneNode("ModelNode")->resetToInitialState();
        } */
    }

    void ReliefApp::ClearData()
    {
        GPPFREEPOINTER(mpUI);
        GPPFREEPOINTER(mpReliefMesh);
        GPPFREEPOINTER(mpViewTool);
#if DEBUGDUMPFILE
        GPPFREEPOINTER(mpDumpInfo);
#endif
        mDisplayMode = TRIMESH;
        GPPFREEPOINTER(mpDepthPointCloud);
        mDisplayMode = TRIMESH;
        mImageStartId = 0;
    }

#if DEBUGDUMPFILE
    void ReliefApp::SetDumpInfo(GPP::DumpBase* dumpInfo)
    {
        /*if (dumpInfo == NULL)
        {
            return;
        }
        GPPFREEPOINTER(mpDumpInfo);
        mpDumpInfo = dumpInfo;
        GPP::TriMesh* triMesh = mpDumpInfo->GetTriMesh();
        if (triMesh == NULL)
        {
            return;
        }
        GPPFREEPOINTER(mpTriMesh);
        mpTriMesh = triMesh;
        mpTriMesh->UpdateNormal();
        InitViewTool();
        UpdateModelRendering();*/
    }

    void ReliefApp::RunDumpInfo()
    {
        //if (mpDumpInfo == NULL)
        //{
        //    return;
        //}
        //GPP::ErrorCode res = mpDumpInfo->Run();
        //if (res != GPP_NO_ERROR)
        //{
        //    MagicCore::ToolKit::Get()->SetAppRunning(false);
        //    return;
        //}
        ////Copy result
        //GPPFREEPOINTER(mpTriMesh);
        //mpTriMesh = mpDumpInfo->GetTriMesh();
        //if (mpTriMesh != NULL)
        //{
        //    mpTriMesh->UnifyCoords(2.0);
        //    mpTriMesh->UpdateNormal();
        //}
        //UpdateModelRendering();
        //GPPFREEPOINTER(mpDumpInfo);
    }
#endif

    void ReliefApp::SwitchDisplayMode()
    {
        if (mDisplayMode == TRIMESH)
        {
            if (mpReliefMesh)
            {
                mDisplayMode = RELIEF;
            }
            else if (mpDepthPointCloud)
            {
                mDisplayMode = POINTCLOUD;
            }
            else
            {
                return;
            }
        }
        else if (mDisplayMode == RELIEF)
        {
            if (mpDepthPointCloud)
            {
                mDisplayMode = POINTCLOUD;
            }
            else
            {
                mDisplayMode = TRIMESH;
            }
        }
        else if (mDisplayMode == POINTCLOUD)
        {
            mDisplayMode = TRIMESH;
        }
        UpdateModelRendering();
    }

    bool ReliefApp::ImportModel()
    {
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
            GPPFREEPOINTER(mpReliefMesh);   
            UpdateModelRendering();
            return true;
        }
        return false;
    }

    void ReliefApp::GenerateRelief(double compressRatio, int resolution)
    {
        GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
        if (triMesh == NULL)
        {
            MessageBox(NULL, "请先导入网格", "温馨提示", MB_OK);
            return;
        }
        //Get depth data
        Ogre::TexturePtr depthTex = Ogre::TextureManager::getSingleton().createManual(  
            "DepthTexture",      // name   
            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,  
            Ogre::TEX_TYPE_2D,   // type   
            resolution,  // width   
            resolution,  // height   
            0,                   // number of mipmaps   
            //Ogre::PF_B8G8R8A8,   // pixel format
            Ogre::PF_FLOAT32_R,
            Ogre::TU_RENDERTARGET
            ); 
        Ogre::RenderTarget* target = depthTex->getBuffer()->getRenderTarget();
        Ogre::Camera* orthCam = MagicCore::RenderSystem::Get()->GetMainCamera();
        orthCam->setProjectionType(Ogre::PT_ORTHOGRAPHIC);
        orthCam->setOrthoWindow(3, 3);
        orthCam->setPosition(0, 0, 3);
        orthCam->lookAt(0, 0, 0);
        orthCam->setAspectRatio(1.0);
        orthCam->setNearClipDistance(0.5);
        orthCam->setFarClipDistance(5);
        Ogre::Viewport* viewport = target->addViewport(orthCam);
        viewport->setDimensions(0, 0, 1, 1);
        MagicCore::RenderSystem::Get()->RenderMesh("Mesh_ReliefApp", "Depth", triMesh);
        MagicCore::RenderSystem::Get()->Update();
        Ogre::Image img;
        depthTex->convertToImage(img);
        std::vector<double> compressedHeightField(resolution * resolution);
        for(int x = 0; x < resolution; x++)  
        {  
            int baseIndex = x * resolution;
            for(int y = 0; y < resolution; y++)  
            {
                compressedHeightField.at(baseIndex + y) = (img.getColourAt(x, resolution - 1 - y, 0))[1];
            }
        }
        Ogre::TextureManager::getSingleton().remove("DepthTexture");
        MagicCore::RenderSystem::Get()->SetupCameraDefaultParameter();

        GPP::ErrorCode res = GPP::DigitalRelief::CompressHeightField(&compressedHeightField, resolution, resolution, compressRatio);
        if (res == GPP_API_IS_NOT_AVAILABLE)
        {
            MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
            MagicCore::ToolKit::Get()->SetAppRunning(false);
        }
        if (res != GPP_NO_ERROR)
        {
            MessageBox(NULL, "浮雕生成失败", "温馨提示", MB_OK);
            return;
        }
        GPP::TriMesh* reliefMesh = GenerateTriMeshFromHeightField(compressedHeightField, resolution, resolution);
        if (reliefMesh != NULL)
        {
            GPPFREEPOINTER(mpReliefMesh);
            mpReliefMesh = reliefMesh;
            mpReliefMesh->UnifyCoords(2.0);
            mpReliefMesh->UpdateNormal();
            mDisplayMode = RELIEF;
        }
        else
        {
            MessageBox(NULL, "浮雕生成失败", "温馨提示", MB_OK);
        }
        UpdateModelRendering();
    }

    void ReliefApp::ExportReliefMesh()
    {
        GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
        if (triMesh == NULL || mpReliefMesh == NULL)
        {
            return;
        }
        GPP::TriMesh* copiedTriMesh = GPP::CopyTriMesh(mpReliefMesh);
        ModelManager::Get()->SetMesh(copiedTriMesh);
    }

    void ReliefApp::CaptureDepthPointCloud(int scanResolution, int imageResolution, const char* shadeName)
    {
        GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
        if (triMesh == NULL)
        {
            MessageBox(NULL, "请先导入网格", "温馨提示", MB_OK);
            return;
        }
        if (scanResolution < 0 || imageResolution < 0)
        {
            MessageBox(NULL, "请设置合适的分辨率", "温馨提示", MB_OK);
            return;
        }

        Ogre::Camera* orthCam = MagicCore::RenderSystem::Get()->GetMainCamera();
        orthCam->setProjectionType(Ogre::PT_ORTHOGRAPHIC);
        orthCam->setOrthoWindow(3, 3);
        orthCam->setPosition(0, 0, 3);
        orthCam->lookAt(0, 0, 0);
        orthCam->setAspectRatio(1.0);
        orthCam->setNearClipDistance(0.5);
        orthCam->setFarClipDistance(5);

        //Get color data
        Ogre::TexturePtr colorTex = Ogre::TextureManager::getSingleton().createManual(  
            "ColorTexture",      // name   
            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,  
            Ogre::TEX_TYPE_2D,   // type   
            imageResolution,  // width   
            imageResolution,  // height   
            0,                   // number of mipmaps   
            Ogre::PF_R8G8B8A8,   // pixel format
            Ogre::TU_RENDERTARGET
            ); 
        Ogre::RenderTarget* colorTarget = colorTex->getBuffer()->getRenderTarget();
        Ogre::Viewport* colorViewport = colorTarget->addViewport(orthCam);
        colorViewport->setDimensions(0, 0, 1, 1);

        MagicCore::RenderSystem::Get()->RenderMesh("Mesh_ReliefApp", shadeName, triMesh);
        MagicCore::RenderSystem::Get()->Update();

        Ogre::Image colorImg;
        colorTex->convertToImage(colorImg);
        mDepthImage.release();
        mDepthImage = cv::Mat(imageResolution, imageResolution, CV_8UC4);
        for (int x = 0; x < imageResolution; ++x)
        {
            for (int y = 0; y < imageResolution; ++y)
            {
                unsigned char* pixel = mDepthImage.ptr(y, x);
                pixel[0] = colorImg.getColourAt(x, y, 0)[2] * 255;
                pixel[1] = colorImg.getColourAt(x, y, 0)[1] * 255;
                pixel[2] = colorImg.getColourAt(x, y, 0)[0] * 255;
                pixel[3] = 255;
            }
        }
        Ogre::TextureManager::getSingleton().remove("ColorTexture");

        //Get depth data
        Ogre::TexturePtr depthTex = Ogre::TextureManager::getSingleton().createManual(  
            "DepthTexture",      // name   
            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,  
            Ogre::TEX_TYPE_2D,   // type   
            scanResolution,  // width   
            scanResolution,  // height   
            0,                   // number of mipmaps   
            Ogre::PF_FLOAT32_R,
            Ogre::TU_RENDERTARGET
            ); 
        Ogre::RenderTarget* depthTarget = depthTex->getBuffer()->getRenderTarget();
        Ogre::Viewport* depthViewport = depthTarget->addViewport(orthCam);
        depthViewport->setDimensions(0, 0, 1, 1);

        MagicCore::RenderSystem::Get()->RenderMesh("Mesh_ReliefApp", "Depth", triMesh);
        MagicCore::RenderSystem::Get()->Update();
        
        Ogre::Image depthImg;
        depthTex->convertToImage(depthImg);
        GPPFREEPOINTER(mpDepthPointCloud);
        mpDepthPointCloud = new GPP::PointCloud;
        double scaleValue = 2.0 / 3.0;
        double minX = -1.0 * scaleValue;
        double maxX = 1.0 * scaleValue;
        double minY = -1.0 * scaleValue;
        double maxY = 1.0 * scaleValue;
        double deltaX = (maxX - minX) / scanResolution;
        double deltaY = (maxY - minY) / scanResolution;
        for (int xid = 0; xid < scanResolution; xid++)
        {
            for (int yid = 0; yid < scanResolution; yid++)
            {
                mpDepthPointCloud->InsertPoint(GPP::Vector3(minX + deltaX * xid, minY + deltaY * yid, 
                    (depthImg.getColourAt(xid, scanResolution - 1 - yid, 0))[1]));
            }
        }
        GPP::ErrorCode res = GPP::ConsolidatePointCloud::ConsolidateRawScanData(mpDepthPointCloud, scanResolution, 
            scanResolution, false, true, 75.0 * GPP::ONE_RADIAN);
        if (res == GPP_API_IS_NOT_AVAILABLE)
        {
            MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
            MagicCore::ToolKit::Get()->SetAppRunning(false);
        }
        if (res != GPP_NO_ERROR)
        {
            MessageBox(NULL, "浮雕生成失败", "温馨提示", MB_OK);
            return;
        }
        Ogre::TextureManager::getSingleton().remove("DepthTexture");

        // point to color
        if (triMesh->HasVertexColor())
        {
            mpDepthPointCloud->SetHasColor(true);
            int pointCount = mpDepthPointCloud->GetPointCount();
            for (int pid = 0; pid < pointCount; pid++)
            {
                GPP::Vector3 coord = mpDepthPointCloud->GetPointCoord(pid);
                int imageX = floor((coord[0] - minX) / deltaX * imageResolution / scanResolution + 0.5);
                int imageY = floor((coord[1] - minY) / deltaY * imageResolution / scanResolution + 0.5);
                const unsigned char* pixel = mDepthImage.ptr(imageResolution - imageY - 1, imageX);
                mpDepthPointCloud->SetPointColor(pid, GPP::Vector3(pixel[2] / 255.0, pixel[1] / 255.0, pixel[0] / 255.0));
            }
        }

        mScanResolution = scanResolution;
        mImageResolution = imageResolution;

        MagicCore::RenderSystem::Get()->SetupCameraDefaultParameter();
        mDisplayMode = POINTCLOUD;
        UpdateModelRendering();
    }

    void ReliefApp::SaveDepthPointCloud(const char* fileNameStr)
    {
        if (fileNameStr == NULL || mpDepthPointCloud == NULL)
        {
            return;
        }
        std::string fileName(fileNameStr);
        if (fileName.empty())
        {
            return;
        }
        size_t dotPos = fileName.rfind('.');
        if (dotPos == std::string::npos)
        {
            MessageBox(NULL, "请输入文件后缀名", "温馨提示", MB_OK);
            return;
        }
        mpDepthPointCloud->SetHasColor(false);
        GPP::ErrorCode res = GPP::Parser::ExportPointCloud(fileName, mpDepthPointCloud);
        if (res != GPP_NO_ERROR)
        {
            MessageBox(NULL, "导出点云失败", "温馨提示", MB_OK);
        }

        dotPos = fileName.rfind('.');
        std::stringstream dumpStream;
        dumpStream << fileName.substr(0, dotPos) << ".jpg";
        std::string imageName;
        dumpStream >> imageName;
        cv::imwrite(imageName, mDepthImage);

        std::vector<int> hStartList(mImageSegCount);
        std::vector<int> hEndList(mImageSegCount);
        if (mImageSegCount > 1)
        {
            int imageW = mDepthImage.cols;
            int imageH = mDepthImage.rows;
            int deltaH = imageH / mImageSegCount;
                
            for (int segId = 0; segId < mImageSegCount; segId++)
            {
                hStartList.at(segId) = segId * deltaH;
                if (segId == mImageSegCount - 1)
                {
                    hEndList.at(segId) = imageH - 1;
                }
                else
                {
                    hEndList.at(segId) = (segId + 1) * deltaH - 1;
                }
                int segHeight = hEndList.at(segId) - hStartList.at(segId) + 1;
                cv::Mat segImage = cv::Mat(segHeight, imageW, CV_8UC4);
                for (int x = 0; x < imageW; ++x)
                {
                    for (int y = 0; y < segHeight; ++y)
                    {
                        unsigned char* originPixel = mDepthImage.ptr(imageH - (y + hStartList.at(segId)) - 1, x);
                        unsigned char* segPixel = segImage.ptr(segHeight - y - 1, x);
                        segPixel[0] = originPixel[0];
                        segPixel[1] = originPixel[1];
                        segPixel[2] = originPixel[2];
                        segPixel[3] = originPixel[3];
                    }
                }
                std::stringstream ss;
                ss << fileName.substr(0, dotPos) << "_" << segId << ".jpg";
                std::string segImageName;
                ss >> segImageName;
                cv::imwrite(segImageName, segImage);
                segImage.release();
            }
        }
        double scaleValue = 2.0 / 3.0;
        double minX = -1.0 * scaleValue;
        double maxX = 1.0 * scaleValue;
        double minY = -1.0 * scaleValue;
        double maxY = 1.0 * scaleValue;
        double deltaX = (maxX - minX) / mScanResolution;
        double deltaY = (maxY - minY) / mScanResolution;
        dotPos = fileName.rfind('.');
        std::stringstream pairStream;
        pairStream << fileName.substr(0, dotPos) << ".map";
        std::string pairName;
        pairStream >> pairName;
        std::ofstream pairOut(pairName);
        int pointCount = mpDepthPointCloud->GetPointCount();
        std::vector<int> coordXList(pointCount);
        std::vector<int> coordYList(pointCount);
        for (int pid = 0; pid < pointCount; pid++)
        {
            GPP::Vector3 coord = mpDepthPointCloud->GetPointCoord(pid);
            int imageX = floor((coord[0] - minX) / deltaX * mImageResolution / mScanResolution + 0.5);
            int imageY = floor((coord[1] - minY) / deltaY * mImageResolution / mScanResolution + 0.5);
            pairOut << imageX << " " << imageY << std::endl;
            coordXList.at(pid) = imageX;
            coordYList.at(pid) = imageY;
        }
        pairOut.close();
        if (mImageSegCount > 1)
        {
            std::stringstream ss;
            ss << fileName.substr(0, dotPos) << ".mmap";
            std::string mmapName;
            ss >> mmapName;
            std::ofstream mmapOut(mmapName);
            mmapOut << pointCount << std::endl;;
            for (int pid = 0; pid < pointCount; pid++)
            {
                for (int sid = 0; sid < mImageSegCount; sid++)
                {
                    if (coordYList.at(pid) >= hStartList.at(sid) && coordYList.at(pid) <= hEndList.at(sid))
                    {
                        mmapOut << mImageStartId + sid << " " << coordXList.at(pid) << " " << coordYList.at(pid) - hStartList.at(sid) << std::endl;
                        break;
                    }
                }
            }
            mmapOut.close();
            mImageStartId += mImageSegCount;
        }
    }

    void ReliefApp::SavePointCloud()
    {
        if (mpDepthPointCloud == NULL)
        {
            MessageBox(NULL, "请先生成点云数据", "温馨提示", MB_OK);
            return;
        }
        std::string fileName;
        char filterName[] = "Support format(*.obj, *.ply, *.asc, *.gpc)\0*.*\0";
        if (MagicCore::ToolKit::FileSaveDlg(fileName, filterName))
        {
            SaveDepthPointCloud(fileName.c_str());
        }
    }

    void ReliefApp::InitViewTool()
    {
        if (mpViewTool == NULL)
        {
            mpViewTool = new MagicCore::ViewTool;
        }
    }

    void ReliefApp::RotateView(double axisX, double axisY, double axisZ, double angle)
    {
        if (mpViewTool)
        {
            mpViewTool->Rotate(axisX, axisY, axisZ, angle);
        }
    }

    void ReliefApp::UpdateModelRendering()
    {
        if (mDisplayMode == TRIMESH)
        {
            GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
            if (triMesh != NULL)
            {
                MagicCore::RenderSystem::Get()->RenderMesh("Mesh_ReliefApp", "CookTorranceShade", triMesh);
            }
        }
        else if (mDisplayMode == RELIEF)
        {
            if (mpReliefMesh != NULL)
            {
                MagicCore::RenderSystem::Get()->RenderMesh("Mesh_ReliefApp", "CookTorranceShade", mpReliefMesh);
            }
        }
        else if (mDisplayMode == POINTCLOUD)
        {
            if (mpDepthPointCloud)
            {
                MagicCore::RenderSystem::Get()->RenderPointCloud("Mesh_ReliefApp", "CookTorrancePoint", mpDepthPointCloud);
            }
        }
    }

    GPP::TriMesh* ReliefApp::GenerateTriMeshFromHeightField(const std::vector<GPP::Real>& heightField, int resolutionX, int resolutionY)
    {
        GPP::TriMesh* triMesh = new GPP::TriMesh;
        double scaleValue = 1.0;
        double minX = -1.0 * scaleValue;
        double maxX = 1.0 * scaleValue;
        double minY = -1.0 * scaleValue;
        double maxY = 1.0 * scaleValue;
        double deltaX = (maxX - minX) / resolutionX;
        double deltaY = (maxY - minY) / resolutionY;
        for (int xid = 0; xid < resolutionX; xid++)
        {
            for (int yid = 0; yid < resolutionY; yid++)
            {
                int baseIndex = xid * resolutionY;
                triMesh->InsertVertex(GPP::Vector3(minX + deltaX * xid, minY + deltaY * yid, heightField.at(baseIndex + yid)));
            }
        }
        for (int xid = 0; xid < resolutionX - 1; xid++)
        {
            for (int yid = 0; yid < resolutionY - 1; yid++)
            {
                int index = xid * resolutionY + yid;
                int indexRight = index + resolutionY;
                int indexDown = index + 1;
                int indexDiag = indexRight + 1;
                triMesh->InsertTriangle(index, indexRight, indexDiag);
                triMesh->InsertTriangle(index, indexDiag, indexDown);
            }
        }
        return triMesh;
    }

    void ReliefApp::RunScript()
    {
        if (MagicCore::ScriptSystem::Get()->IsOnRunningScript())
        {
            MessageBox(NULL, "请等待前一脚本执行完毕", "温馨提示", MB_OK);
            return;
        }
        std::string fileName;
        char filterName[] = "GPP Script File(*.gsf)\0*.gsf\0";
        if (MagicCore::ToolKit::FileOpenDlg(fileName, filterName))
        {
            InfoLog << "Run Script file: " << fileName.c_str() << std::endl;
            if (MagicCore::ScriptSystem::Get()->RunScriptFile(fileName.c_str()))
            {
                UpdateModelRendering();
                MessageBox(NULL, "脚本执行完毕", "温馨提示", MB_OK);
            }
        }
    }
}

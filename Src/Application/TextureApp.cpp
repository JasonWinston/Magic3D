#include "stdafx.h"
#include <process.h>
#include "TextureApp.h"
#include "TextureAppUI.h"
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
        TextureApp* app = (TextureApp*)arg;
        if (app == NULL)
        {
            return 0;
        }
        app->DoCommand(false);
        return 1;
    }

    TextureApp::TextureApp() :
        mpUI(NULL),
        mpImageFrameMesh(NULL),
        mDistortionImage(),
        mTextureImageSize(4096),
        mpTriMeshTexture(NULL),
        mpViewTool(NULL),
        mDisplayMode(TRIMESH_SOLID),
        mCommandType(NONE),
        mIsCommandInProgress(false),
        mUpdateDisplay(false),
        mTextureImageNames(),
        mCurrentTextureImageId(0),
        mTextureType(TT_NONE),
        mTextureImageName("../../Media/TextureApp/texture.png"),
        mTextureImageMasks(),
        mSharpDiff()
    {
    }

    TextureApp::~TextureApp()
    {
        GPPFREEPOINTER(mpUI);
        GPPFREEPOINTER(mpImageFrameMesh);
        GPPFREEPOINTER(mpViewTool);
    }

    void TextureApp::ClearData()
    {
        GPPFREEPOINTER(mpUI);
        GPPFREEPOINTER(mpViewTool);
        ClearMeshData();
    }

    void TextureApp::ClearMeshData()
    {
        GPPFREEPOINTER(mpImageFrameMesh);
        mDisplayMode = TRIMESH_SOLID;
        mDistortionImage.release();
        mTextureType = TT_NONE;
        mTextureImageMasks.clear();
    }

    bool TextureApp::IsCommandAvaliable()
    {
        if (mIsCommandInProgress)
        {
            MessageBox(NULL, "请等待当前命令执行完", "温馨提示", MB_OK);
            return false;
        }
        return true;
    }

#if DEBUGDUMPFILE
    void TextureApp::SetDumpInfo(GPP::DumpBase* dumpInfo)
    {
    }

    void TextureApp::RunDumpInfo()
    {
    }
#endif

    bool TextureApp::Enter()
    {
        InfoLog << "Enter TextureApp" << std::endl; 
        if (mpUI == NULL)
        {
            mpUI = new TextureAppUI;
        }
        mpUI->Setup();
        SetupScene();
        UpdateDisplay();
        return true;
    }

    bool TextureApp::Update(double timeElapsed)
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
            InfoLog << "Update::UpdateDisplay" << std::endl;
        }
        return true;
    }

    bool TextureApp::Exit()
    {
        InfoLog << "Exit TextureApp" << std::endl; 
        ShutdownScene();
        if (mpUI != NULL)
        {
            mpUI->Shutdown();
        }
        ClearData();
        return true;
    }

    bool TextureApp::MouseMoved( const OIS::MouseEvent &arg )
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

    bool TextureApp::MousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
    {
        if ((id == OIS::MB_Middle || id == OIS::MB_Left) && mpViewTool != NULL)
        {
            mpViewTool->MousePressed(arg.state.X.abs, arg.state.Y.abs);
        }
        return true;
    }

    bool TextureApp::MouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
    {
        if (mpViewTool)
        {
            mpViewTool->MouseReleased();
        }
        return true;
    }

    bool TextureApp::KeyPressed( const OIS::KeyEvent &arg )
    {
        if (arg.key == OIS::KC_P)
        {
            PickMeshColorFromImages();
        }
        else if (arg.key == OIS::KC_O)
        {
#if DEVELOPSTATE
            ExportObjFile();
#endif
        }
        else if (arg.key == OIS::KC_I)
        {
            GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
            std::vector<GPP::ImageColorId> imageColorIds = ModelManager::Get()->GetImageColorIds();
            triMesh->SetHasVertexColor(true);
            int maxColorId = 10;
            double deltaColor = 0.1;
            int vertexCount = triMesh->GetVertexCount();
            for (int vid = 0; vid < vertexCount; vid++)
            {
                int imageIndex = imageColorIds.at(vid).GetImageIndex();
                if (imageIndex < 0)
                {
                    triMesh->SetVertexColor(vid, MagicCore::ToolKit::Get()->ColorCoding(0));
                }
                else
                {
                    triMesh->SetVertexColor(vid, MagicCore::ToolKit::Get()->ColorCoding(0.2 + imageIndex % maxColorId * deltaColor));
                }
            }
            UpdateDisplay();
        }

        return true;
    }

    void TextureApp::SetupScene()
    {
        Ogre::SceneManager* sceneManager = MagicCore::RenderSystem::Get()->GetSceneManager();
        sceneManager->setAmbientLight(Ogre::ColourValue(0.3, 0.3, 0.3));
        Ogre::Light* light = sceneManager->createLight("TextureApp_SimpleLight");
        light->setPosition(0, 0, 20);
        light->setDiffuseColour(0.8, 0.8, 0.8);
        light->setSpecularColour(0.5, 0.5, 0.5);

        if (mpImageFrameMesh == NULL)
        {
            mpImageFrameMesh = GPP::Parser::ImportTriMesh("../../Media/TextureApp/ImageMesh.obj");
            if (mpImageFrameMesh == NULL)
            {
                MessageBox(NULL, "纹理网格导入失败", "温馨提示", MB_OK);
                return;
            }
            mpImageFrameMesh->UpdateNormal();
        }

        mTextureImageNames.clear();
        mTextureImageNames.push_back("../../Media/TextureApp/grid.png");
        SwitchTextureImage();

        GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
        if (triMesh)
        {
            mpUI->SetMeshInfo(triMesh->GetVertexCount(), triMesh->GetTriangleCount());
            mTextureType = TT_NONE;
            std::vector<std::string>::iterator itr = std::find(mTextureImageNames.begin(), mTextureImageNames.end(), mTextureImageName);
            if (itr != mTextureImageNames.end())
            {
                mTextureImageNames.erase(itr);
            }
            mCurrentTextureImageId = 0;
            UpdatetextureImage();
        }
        
        InitViewTool();
    }

    void TextureApp::ShutdownScene()
    {
        Ogre::SceneManager* sceneManager = MagicCore::RenderSystem::Get()->GetSceneManager();
        sceneManager->setAmbientLight(Ogre::ColourValue::Black);
        sceneManager->destroyLight("TextureApp_SimpleLight");
        //MagicCore::RenderSystem::Get()->SetupCameraDefaultParameter();
        /*if (MagicCore::RenderSystem::Get()->GetSceneManager()->hasSceneNode("ModelNode"))
        {
            MagicCore::RenderSystem::Get()->GetSceneManager()->getSceneNode("ModelNode")->resetToInitialState();
        }*/
        MagicCore::RenderSystem::Get()->HideRenderingObject("TriMesh_TextureApp");
        MagicCore::RenderSystem::Get()->HideRenderingObject("PointCloudList_TextureApp");
    }

    void TextureApp::ImportTriMesh()
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
            mDisplayMode = TRIMESH_SOLID;
            UpdateDisplay();
            mpUI->SetMeshInfo(triMesh->GetVertexCount(), triMesh->GetTriangleCount());
            mTextureType = TT_NONE;
            std::vector<std::string>::iterator itr = std::find(mTextureImageNames.begin(), mTextureImageNames.end(), mTextureImageName);
            if (itr != mTextureImageNames.end())
            {
                mTextureImageNames.erase(itr);
            }
            mCurrentTextureImageId = 0;
            UpdatetextureImage();
        }
    }

    void TextureApp::DoCommand(bool isSubThread)
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
            case MagicApp::TextureApp::NONE:
                break;
            case MagicApp::TextureApp::FUSEMESHCOLOR:
                FuseMeshColor(mSharpDiff[0], mSharpDiff[1], mSharpDiff[2], false);
                break;
            case MagicApp::TextureApp::OPTIMISE_IMAGE_COLOR_ID:
                ComputeImageColorIds(false);
                break;
            case MagicApp::TextureApp::TUNE_TEXTURE_IMAGE_BY_VERTEX_COLOR:
                TuneTextureImageByVertexColor(false);
                break;
            default:
                break;
            }
            mpUI->StopProgressbar();
        }
    }

    int TextureApp::GetMeshVertexCount()
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

    void TextureApp::SwitchDisplayMode()
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
        
        UpdateDisplay();
    }

    void TextureApp::SwitchTextureImage()
    {
        mCurrentTextureImageId = (mCurrentTextureImageId + 1) % mTextureImageNames.size();
        mDistortionImage.release();
        mDistortionImage = cv::imread(mTextureImageNames.at(mCurrentTextureImageId));
        if (mDistortionImage.data != NULL)
        {
            mTextureImageSize = mDistortionImage.rows;
        }
        else
        {
            MessageBox(NULL, "图片打开失败", "温馨提示", MB_OK);
        }
        UpdateDisplay();
    }

    void TextureApp::UpdatetextureImage()
    {
        mDistortionImage.release();
        mDistortionImage = cv::imread(mTextureImageNames.at(mCurrentTextureImageId));
        if (mDistortionImage.data != NULL)
        {
            mTextureImageSize = mDistortionImage.rows;
        }
        else
        {
            MessageBox(NULL, "图片打开失败", "温馨提示", MB_OK);
        }
        mUpdateDisplay = true;
    }

    void TextureApp::InitViewTool()
    {
        if (mpViewTool == NULL)
        {
            mpViewTool = new MagicCore::ViewTool;
            mpViewTool->SetRightNodeFixed(true);
        }
    }

    void TextureApp::UpdateDisplay()
    {
        GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
        if (mDisplayMode == TRIMESH_SOLID)
        {
            if (triMesh == NULL)
            {
                MagicCore::RenderSystem::Get()->HideRenderingObject("TriMesh_TextureApp");
                return;
            }
            MagicCore::RenderSystem::Get()->GetMainCamera()->setPolygonMode(Ogre::PolygonMode::PM_SOLID);
            MagicCore::RenderSystem::Get()->RenderMesh("TriMesh_TextureApp", "CookTorrance", triMesh, 
                MagicCore::RenderSystem::MODEL_NODE_CENTER, NULL, NULL, false);
        }
        else if (mDisplayMode == TRIMESH_WIREFRAME)
        {
            if (triMesh == NULL)
            {
                MagicCore::RenderSystem::Get()->HideRenderingObject("TriMesh_TextureApp");
                return;
            }
            MagicCore::RenderSystem::Get()->GetMainCamera()->setPolygonMode(Ogre::PolygonMode::PM_WIREFRAME);
            MagicCore::RenderSystem::Get()->RenderMesh("TriMesh_TextureApp", "CookTorrance", triMesh, 
                MagicCore::RenderSystem::MODEL_NODE_CENTER);
        }
        else if (mDisplayMode == TRIMESH_TEXTURE)
        {
            if (triMesh == NULL || triMesh->HasTriangleTexCoord() == false)
            {
                MagicCore::RenderSystem::Get()->HideRenderingObject("TriMesh_TextureApp");
                return;
            }
            MagicCore::RenderSystem::Get()->GetMainCamera()->setPolygonMode(Ogre::PolygonMode::PM_SOLID);
            UpdateTriMeshTexture();
            MagicCore::RenderSystem::Get()->RenderTextureMesh("TriMesh_TextureApp", "TextureMeshMaterial", triMesh, 
                MagicCore::RenderSystem::MODEL_NODE_CENTER);
        }
        else if (mDisplayMode == UVMESH_WIREFRAME)
        {
            if (triMesh == NULL || triMesh->HasTriangleTexCoord() == false)
            {
                MagicCore::RenderSystem::Get()->HideRenderingObject("TriMesh_TextureApp");
                return;
            }
            MagicCore::RenderSystem::Get()->GetMainCamera()->setPolygonMode(Ogre::PolygonMode::PM_SOLID);
            MagicCore::RenderSystem::Get()->RenderUVMesh("TriMesh_TextureApp", "CookTorrance", triMesh, 
                MagicCore::RenderSystem::MODEL_NODE_CENTER);
        }
    }

    void TextureApp::UpdateTriMeshTexture()
    {
        if (mpTriMeshTexture.isNull())
        {
            mpTriMeshTexture = Ogre::TextureManager::getSingleton().createManual(  
                "TriMeshTexture",      // name  
                Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,  
                Ogre::TEX_TYPE_2D,   // type  
                mTextureImageSize,  // width  
                mTextureImageSize, // height  
                0,                   // number of mipmaps  
                Ogre::PF_B8G8R8A8,   // pixel format  
                Ogre::TU_DYNAMIC_WRITE_ONLY_DISCARDABLE // usage, for textures updated very often  
                );
        }

        if (mDistortionImage.data != NULL)
        {
            // Get the pixel buffer  
            Ogre::HardwarePixelBufferSharedPtr pixelBuffer = mpTriMeshTexture->getBuffer();  
            cv::Mat textureImage = mDistortionImage;;
            // Lock the pixel buffer and get a pixel box  
            unsigned char* buffer = static_cast<unsigned char*>(  
                pixelBuffer->lock(0, mTextureImageSize * mTextureImageSize * 4, Ogre::HardwareBuffer::HBL_DISCARD) ); 
            int imgW = textureImage.cols;
            int imgH = textureImage.rows;
            for(int y = 0; y < mTextureImageSize; ++y)  
            {  
                for(int x = 0; x < mTextureImageSize; ++x)  
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
          
        if (Ogre::MaterialManager::getSingleton().getByName("TextureMeshMaterial", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME).isNull())
        {
            Ogre::MaterialPtr material = Ogre::MaterialManager::getSingleton().create("TextureMeshMaterial", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME); 
            material->getTechnique(0)->getPass(0)->createTextureUnitState("TriMeshTexture");
            material->getTechnique(0)->getPass(0)->setDiffuse(0.85, 0.85, 0.85, 1.0);
            material->getTechnique(0)->getPass(0)->setSpecular(0.15, 0.15, 0.15, 1.0);
            material->getTechnique(0)->getPass(0)->setShininess(10);
        }
    }

    void TextureApp::UnifyTextureCoords(std::vector<double>& texCoords, double scaleValue)
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

    void TextureApp::ExportObjFile()
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
        if (triMesh == NULL)
        {
            MessageBox(NULL, "请先导入网格", "温馨提示", MB_OK);
            return;
        }
        if (triMesh->HasTriangleTexCoord() == false)
        {
            MessageBox(NULL, "请先计算纹理坐标", "温馨提示", MB_OK);
            return;
        }
        std::string fileName;
        char filterName[] = "OBJ Files(*.obj)\0*.obj\0";
        if (MagicCore::ToolKit::FileSaveDlg(fileName, filterName))
        {
            GPP::Real scaleValue = ModelManager::Get()->GetScaleValue();
            GPP::Vector3 objCenterCoord = ModelManager::Get()->GetObjCenterCoord();
            triMesh->UnifyCoords(1.0 / scaleValue, objCenterCoord * (-scaleValue));

            // Export obj by texture coordinates
            std::string objName = fileName + ".obj";
            size_t dotPos = fileName.rfind('\\');
            if (dotPos == std::string::npos)
            {
                dotPos = fileName.rfind('/');
                if (dotPos == std::string::npos)
                {
                    MessageBox(NULL, "文件名字无效", "温馨提示", MB_OK);
                    return;
                }
            }
            std::string fileNameNoPath = fileName.substr(dotPos + 1);
            std::ofstream objOut(objName.c_str());
            objOut << "mtllib " << fileNameNoPath << ".mtl" << "\n";
            objOut << "usemtl " << fileNameNoPath << "\n";
            GPP::Int vertexCount = triMesh->GetVertexCount();
            for (GPP::Int vid = 0; vid < vertexCount; vid++)
            {
                GPP::Vector3 coord = triMesh->GetVertexCoord(vid);
                objOut << "v " << coord[0] << " " << coord[1] << " " << coord[2] << "\n";
            }
            GPP::Int faceCount = triMesh->GetTriangleCount();
            for (GPP::Int fid = 0; fid < faceCount; fid++)
            {
                for (int localId = 0; localId < 3; localId++)
                {
                    GPP::Vector3 texCoord = triMesh->GetTriangleTexcoord(fid, localId);
                    objOut << "vt " << texCoord[0] << " " << texCoord[1] << "\n";
                }
            }
            GPP::Int vertexIds[3];
            for (GPP::Int fid = 0; fid < faceCount; fid++)
            {
                triMesh->GetTriangleVertexIds(fid, vertexIds);
                objOut << "f " << vertexIds[0] + 1 << "/" << fid * 3 + 1 << " " 
                    << vertexIds[1] + 1 << "/" << fid * 3 + 2 << " " << vertexIds[2] + 1 << "/" << fid * 3 + 3 << "\n"; 
            }
            objOut.close();
            
            // export mtl file
            std::string mtlName = fileName + ".mtl";
            std::ofstream mtlOut(mtlName.c_str());
            mtlOut << "newmtl " << fileNameNoPath << std::endl;
            mtlOut << "Kd " << 0.75 << " " << 0.75 << " " << 0.75 << std::endl;
            mtlOut << "map_Kd " << fileNameNoPath << ".png" << std::endl;
            mtlOut.close();
            
            // export texture image
            std::string imageName = fileName + ".png";
            cv::imwrite(imageName, mDistortionImage);
            //

            triMesh->UnifyCoords(scaleValue, objCenterCoord);
        }
    }

    void TextureApp::GenerateTextureImage(bool isByVertexColor)
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
        if (triMesh == NULL)
        {
            MessageBox(NULL, "请先导入网格", "温馨提示", MB_OK);
            return;
        }
        if (triMesh->HasTriangleTexCoord() == false)
        {
            MessageBox(NULL, "请先给网格计算纹理坐标", "温馨提示", MB_OK);
            return;
        }

        std::vector<GPP::ImageColorId> originImageColorIds = ModelManager::Get()->GetImageColorIds();
        if (isByVertexColor)
        {
            if (triMesh->HasVertexColor() == false)
            {
                MessageBox(NULL, "输入需要带颜色的网格", "温馨提示", MB_OK);
                return;
            }
        }
        else
        {
            if (originImageColorIds.size() != triMesh->GetVertexCount())
            {
                MessageBox(NULL, "顶点与图片对应文件有错", "温馨提示", MB_OK);
                return;
            }
        }

        GPP::Int faceCount = triMesh->GetTriangleCount();
        std::vector<GPP::Real> textureCoords(faceCount * 3 * 2);
        std::vector<GPP::Int> textureIds(faceCount *  3);
        for (GPP::Int fid = 0; fid < faceCount; ++fid)
        {
            for (int fvid = 0; fvid < 3; ++fvid)
            {
                GPP::Vector3 texCoord = triMesh->GetTriangleTexcoord(fid, fvid);
                GPP::Int baseIndex = fid * 3 + fvid;
                textureCoords.at(baseIndex * 2) = texCoord[0];
                textureCoords.at(baseIndex * 2 + 1) = texCoord[1];
                textureIds.at(baseIndex) = baseIndex;
            }
        }
        
        //std::vector<GPP::Vector3> imageData;
        std::vector<GPP::Color4> imageData;
        mTextureImageMasks.clear();
        GPP::ErrorCode res = GPP_NO_ERROR;
        if (isByVertexColor)
        {
            //std::vector<GPP::Vector3> vertexColors(faceCount * 3);
            std::vector<GPP::Color4> vertexColors(faceCount * 3);
            GPP::Int vertexIds[3] = {-1};
            for (GPP::Int fid = 0; fid < faceCount; ++fid)
            {
                triMesh->GetTriangleVertexIds(fid, vertexIds);
                for (int fvid = 0; fvid < 3; ++fvid)
                {
                    GPP::Vector3 vColor = triMesh->GetVertexColor(vertexIds[fvid]);
                    vertexColors.at(fid * 3 + fvid) = GPP::Color4::Vector3ToColor4(vColor);
                }
            }
            res = GPP::TextureImage::CreateTextureImageByVertexColors(textureCoords, textureIds, 
                vertexColors, mTextureImageSize, mTextureImageSize, imageData, &mTextureImageMasks);
        }
        else
        {
            std::vector<GPP::ImageColorId> imageColorIds(faceCount * 3);
            GPP::Int vertexIds[3] = {-1};
            for (GPP::Int fid = 0; fid < faceCount; ++fid)
            {
                triMesh->GetTriangleVertexIds(fid, vertexIds);
                for (int fvid = 0; fvid < 3; ++fvid)
                {
                    imageColorIds.at(fid * 3 + fvid) = originImageColorIds.at(vertexIds[fvid]);
                }
            }
            std::vector<std::string> textureImageFiles = ModelManager::Get()->GetTextureImageFiles();
            int imageCount = textureImageFiles.size();
            std::vector<std::vector<GPP::Color4> > imageListData;
            imageListData.reserve(imageCount);
            std::vector<GPP::Int> imageInfos;
            imageInfos.reserve(imageCount * 2);
            for (int iid = 0; iid < imageCount; ++iid)
            {
                cv::Mat image = cv::imread(textureImageFiles.at(iid));
                if (image.data == NULL)
                {
                    MessageBox(NULL, "图片读取失败", "温馨提示", MB_OK);
                    return;
                }
                int width = image.cols;
                int height = image.rows;

                std::vector<GPP::Color4> oneImageData(width * height);
                for (int y = 0; y < height; ++y)
                {
                    for (int x = 0; x < width; ++x)
                    {
                        const unsigned char* pixel = image.ptr(height - 1 - y, x);
                        //GPP::Vector3 color(pixel[2] / 255.0, pixel[1] / 255.0, pixel[0] / 255.0);
                        GPP::Color4 color(pixel[2], pixel[1], pixel[0]);
                        oneImageData.at(x + y * width) = color;
                    }
                }
                imageListData.push_back(oneImageData);
                imageInfos.push_back(width);
                imageInfos.push_back(height);
            }
            res = GPP::TextureImage::CreateTextureImageByRefImages(textureCoords, textureIds, imageColorIds, 
                imageListData, imageInfos, mTextureImageSize, mTextureImageSize, imageData, &mTextureImageMasks);
        }
        if (res != GPP_NO_ERROR)
        {
            MessageBox(NULL, "纹理图生成失败", "温馨提示", MB_OK);
            return;
        }
        
        cv::Mat textureImage(mTextureImageSize, mTextureImageSize, CV_8UC4);
        cv::Mat alphaImage(mTextureImageSize, mTextureImageSize, CV_8UC4);
        GPP::Int maxAlpha = *std::max_element(mTextureImageMasks.begin(), mTextureImageMasks.end());
        if (maxAlpha == 0)
        {
            maxAlpha = 1;
        }
        for (int y = 0; y < mTextureImageSize; ++y)
        {
            for (int x = 0; x < mTextureImageSize; ++x)
            {
                cv::Vec4b& col = textureImage.at<cv::Vec4b>(mTextureImageSize - 1 - y, x);
                //GPP::Vector3 vCol = imageData.at(x + y * mTextureImageSize);
                GPP::Color4 cColor = imageData.at(x + y * mTextureImageSize);
                col[0] = cColor[2];
                col[1] = cColor[1];
                col[2] = cColor[0];
                col[3] = 255;

                cv::Vec4b& alpha = alphaImage.at<cv::Vec4b>(mTextureImageSize - 1 - y, x);
                int mask = mTextureImageMasks.at(x + y * mTextureImageSize);
                if (mask < 3)
                {
                    alpha[0] = 0;
                    alpha[1] = 0;
                    alpha[2] = 0;
                    alpha[3] = 255;
                    alpha[mask] = 255;
                }
                else
                {
                    alpha[0] = mask * 255 / maxAlpha;
                    alpha[1] = mask * 255 / maxAlpha;
                    alpha[2] = mask * 255 / maxAlpha;
                    alpha[3] = 255;
                }
            }
        }
        cv::imwrite(mTextureImageName, textureImage);
        std::vector<std::string>::iterator itr = std::find(mTextureImageNames.begin(), mTextureImageNames.end(), mTextureImageName);
        if (itr == mTextureImageNames.end())
        {
            mTextureImageNames.push_back(mTextureImageName);
        }
        cv::imwrite("../../Media/TextureApp/Overlapped.png", alphaImage);

        mDistortionImage.release();
        mDistortionImage = cv::imread(mTextureImageName);
        if (mDistortionImage.data != NULL)
        {
            mTextureImageSize = mDistortionImage.rows;
        }
        else
        {
            MessageBox(NULL, "图片打开失败", "温馨提示", MB_OK);
        }
        mDisplayMode = TRIMESH_TEXTURE;
        UpdateDisplay();
    }

    void TextureApp::TuneTextureImageByVertexColor(bool isSubThread)
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        if (isSubThread)
        {
            mCommandType = TUNE_TEXTURE_IMAGE_BY_VERTEX_COLOR;
            DoCommand(true);
        }
        else
        {
            GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
            if (triMesh == NULL)
            {
                MessageBox(NULL, "请先导入网格", "温馨提示", MB_OK);
                return;
            }
            if (triMesh->HasVertexColor() == false)
            {
                MessageBox(NULL, "输入需要带颜色的网格", "温馨提示", MB_OK);
                return;
            }
            std::vector<GPP::ImageColorId> imageColorIds = ModelManager::Get()->GetImageColorIds();
            if (imageColorIds.size() != triMesh->GetVertexCount())
            {
                MessageBox(NULL, "顶点与图片对应文件有错", "温馨提示", MB_OK);
                return;
            }
            std::vector<int> vertexFlags = ModelManager::Get()->GetImageColorIdFlags();
            if (vertexFlags.empty())
            {
                vertexFlags.resize(triMesh->GetVertexCount(), 1);
            }
            std::vector<std::string> textureImageFiles = ModelManager::Get()->GetTextureImageFiles();
            int imageCount = textureImageFiles.size();
            InfoLog << "imageCount=" << imageCount << std::endl;
            int vertexCount = triMesh->GetVertexCount();
            int faceCount = triMesh->GetTriangleCount();
            std::vector<int> vertex2ImageMap(vertexCount, -1);
            std::vector<bool> faceVisitFlag(faceCount, 0);
            for (int iid = 0; iid < imageCount; iid++)
            {
                std::vector<int> vertexCoords;
                std::vector<GPP::Vector3> vertexColors;
                for (int vid = 0; vid < vertexCount; vid++)
                {
                    if (imageColorIds.at(vid).GetImageIndex() != iid)
                    {
                        continue;
                    }
                    vertex2ImageMap.at(vid) = vertexColors.size();
                    vertexCoords.push_back(imageColorIds.at(vid).GetLocalX());
                    vertexCoords.push_back(imageColorIds.at(vid).GetLocalY());
                    vertexColors.push_back(triMesh->GetVertexColor(vid));
                }
                std::vector<int> faceVertexIds;
                int vertexIds[3] = {-1};
                for (int fid = 0; fid < faceCount; fid++)
                {
                    if (faceVisitFlag.at(fid))
                    {
                        continue;
                    }
                    triMesh->GetTriangleVertexIds(fid, vertexIds);
                    bool isValid = true;
                    for (int fvid = 0; fvid < 3; fvid++)
                    {
                        if (imageColorIds.at(vertexIds[fvid]).GetImageIndex() != iid)
                        {
                            isValid = false;
                            break;
                        }
                    }
                    if (isValid)
                    {
                        faceVertexIds.push_back(vertex2ImageMap.at(vertexIds[0]));
                        faceVertexIds.push_back(vertex2ImageMap.at(vertexIds[1]));
                        faceVertexIds.push_back(vertex2ImageMap.at(vertexIds[2]));
                        faceVisitFlag.at(fid) = 1;
                    }
                }
                if (faceVertexIds.empty())
                {
                    continue;
                }
                cv::Mat image = cv::imread(textureImageFiles.at(iid));
                if (image.data == NULL)
                {
                    MessageBox(NULL, "图片读取失败", "温馨提示", MB_OK);
                    return;
                }
                int imageWidth = image.cols;
                int imageHeight = image.rows;
                std::vector<GPP::Color4> imageData(imageWidth * imageHeight);
                for (int y = 0; y < imageHeight; ++y)
                {
                    for (int x = 0; x < imageWidth; ++x)
                    {
                        const unsigned char* pixel = image.ptr(imageHeight - 1 - y, x);
                        imageData.at(x + y * imageWidth) = GPP::Color4(pixel[2], pixel[1], pixel[0]);
                    }
                }
                GPP::ErrorCode res = GPP::IntrinsicColor::TuneImageByTriangleColor(vertexCoords, vertexColors, vertexFlags,
                    faceVertexIds, imageWidth, imageHeight, imageData); 
                if (res != GPP_NO_ERROR)
                {
                    MessageBox(NULL, "纹理图优化失败", "温馨提示", MB_OK);
                    return;
                }
                for (int y = 0; y < imageHeight; ++y)
                {
                    for (int x = 0; x < imageWidth; ++x)
                    {
                        GPP::Color4 curColor = imageData.at(x + y * imageWidth);
                        unsigned char* pixel = image.ptr(imageHeight - y - 1, x);
                        pixel[0] = curColor[2];
                        pixel[1] = curColor[1];
                        pixel[2] = curColor[0];
                    }
                }
                std::string tuneImageName = textureImageFiles.at(iid) + "_tune_triangle.jpg";
                cv::imwrite(tuneImageName, image);
                textureImageFiles.at(iid) = tuneImageName;
            }
            ModelManager::Get()->SetTextureImageFiles(textureImageFiles);
        }
    }

    static void MapTriMesh2ImageSpace(const GPP::ITriMesh* triMesh, const std::vector<GPP::ImageColorId>& imageColorIds)
    {
        int vertexCount = triMesh->GetVertexCount();
        std::vector<bool> vertexVisitFlags(vertexCount, 0);
        std::vector<int> vertex2SubmeshMap(vertexCount, -1);
        int meshId = 0;
        int faceCount = triMesh->GetTriangleCount();
        for (int vid = 0; vid < vertexCount; vid++)
        {
            if (vertexVisitFlags.at(vid))
            {
                continue;
            }
            int curImageId = imageColorIds.at(vid).GetImageIndex();
            std::vector<int> vertexList;
            for (int svid = 0; svid < vertexCount; svid++)
            {
                if (vertexVisitFlags.at(svid) || imageColorIds.at(svid).GetImageIndex() != curImageId)
                {
                    continue;
                }
                vertexVisitFlags.at(svid) = 1;
                vertexList.push_back(svid);
            }
            int subVertexCount = vertexList.size();
            GPP::TriMesh subTriMesh;
            for (int svid = 0; svid < subVertexCount; svid++)
            {
                vertex2SubmeshMap.at(vertexList.at(svid)) = svid;
                GPP::ImageColorId imageColorId = imageColorIds.at(vertexList.at(svid));
                subTriMesh.InsertVertex(GPP::Vector3(imageColorId.GetLocalX(), imageColorId.GetLocalY(), 0));
            }
            int vertexIds[3] = {-1};
            for (int fid = 0; fid < faceCount; fid++)
            {
                triMesh->GetTriangleVertexIds(fid, vertexIds);
                if (imageColorIds.at(vertexIds[0]).GetImageIndex() != curImageId || 
                    imageColorIds.at(vertexIds[1]).GetImageIndex() != curImageId || 
                    imageColorIds.at(vertexIds[2]).GetImageIndex() != curImageId)
                {
                    continue;
                }
                subTriMesh.InsertTriangle(vertex2SubmeshMap.at(vertexIds[0]), vertex2SubmeshMap.at(vertexIds[1]), 
                    vertex2SubmeshMap.at(vertexIds[2]));
            }
            std::stringstream ss;
            ss << "submesh_" << meshId << ".obj";
            std::string meshName;
            ss >> meshName;
            GPP::Parser::ExportTriMesh(meshName, &subTriMesh);
            meshId++;
        }
    }

    void TextureApp::ComputeImageColorIds(bool isSubThread)
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        if (isSubThread)
        {
            mCommandType = OPTIMISE_IMAGE_COLOR_ID;
            DoCommand(true);
        }
        else
        {
            GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
            if (triMesh == NULL)
            {
                MessageBox(NULL, "请先导入网格", "温馨提示", MB_OK);
                return;
            }
            int vertexCount = triMesh->GetVertexCount();
            std::vector<GPP::ImageColorId> imageColorIds = ModelManager::Get()->GetImageColorIds();
            if (imageColorIds.empty())
            {
                MessageBox(NULL, "ImageColorId is empty", "温馨提示", MB_OK);
                return;
            }
            std::vector<int> fixFlag = ModelManager::Get()->GetImageColorIdFlags();
            if (fixFlag.empty())
            {
                fixFlag.resize(triMesh->GetVertexCount(), 1);
                return;
            }
#if MAKEDUMPFILE
            GPP::DumpOnce();
#endif
            GPP::ErrorCode res = GPP::OptimiseMapping::InterpolateImageColorIdsOnMesh(triMesh, fixFlag, imageColorIds);
            if (res == GPP_API_IS_NOT_AVAILABLE)
            {
                MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
                MagicCore::ToolKit::Get()->SetAppRunning(false);
            }
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "图像对应优化失败", "温馨提示", MB_OK);
                return;
            }
            ModelManager::Get()->SetImageColorIds(imageColorIds);
            //MapTriMesh2ImageSpace(triMesh, imageColorIds);
        }
    }

    void TextureApp::OptimiseIsolateImageColorIds(void)
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
        if (triMesh == NULL)
        {
            MessageBox(NULL, "请先导入网格", "温馨提示", MB_OK);
            return;
        }
        int vertexCount = triMesh->GetVertexCount();
        std::vector<GPP::ImageColorId> imageColorIds = ModelManager::Get()->GetImageColorIds();
        if (imageColorIds.size() != vertexCount)
        {
            MessageBox(NULL, "ImageColorId size != vertexCount", "温馨提示", MB_OK);
            return;
        }
#if MAKEDUMPFILE
        GPP::DumpOnce();
#endif
        double isolateValue = 0.001;
        GPP::ErrorCode res = GPP::OptimiseMapping::OptimiseIsolateImageColorIdsOnMesh(triMesh, imageColorIds, isolateValue);
        if (res == GPP_API_IS_NOT_AVAILABLE)
        {
            MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
            MagicCore::ToolKit::Get()->SetAppRunning(false);
        }
        if (res != GPP_NO_ERROR)
        {
            MessageBox(NULL, "点像对应孤立点优化失败", "温馨提示", MB_OK);
            return;
        }
        ModelManager::Get()->SetImageColorIds(imageColorIds);
    }

    void TextureApp::SaveImageColorInfo()
    {
        std::string fileName;
        char filterName[] = "Support format(*.gii)\0*.gii\0";
        if (MagicCore::ToolKit::FileSaveDlg(fileName, filterName))
        {
            std::ofstream fout(fileName);
            ModelManager::Get()->DumpInfo(fout);
            fout.close();
        }
    }
    
    void TextureApp::LoadImageColorInfo()
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
            ModelManager::Get()->LoadInfo(fin);
            fin.close();

            MessageBox(NULL, "请导入图像", "温馨提示", MB_OK);
            std::vector<std::string> fileNames;
            char filterName[] = "JPG Files(*.jpg)\0*.jpg\0PNG Files(*.png)\0*.png\0BMP Image(*.bmp)\0*.bmp\0";
            if (MagicCore::ToolKit::MultiFileOpenDlg(fileNames, filterName))
            {
                ModelManager::Get()->SetTextureImageFiles(fileNames);
            }
            
        }
    }

    void TextureApp::PickMeshColorFromImages()
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

        UpdateDisplay();
    }

    void TextureApp::FuseMeshColor(double sharpDiff_H, double sharpDiff_S, double sharpDiff_V, bool isSubThread)
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        if (isSubThread)
        {
            mCommandType = FUSEMESHCOLOR;
            mSharpDiff = GPP::Vector3(sharpDiff_H, sharpDiff_S, sharpDiff_V);
            DoCommand(true);
        }
        else
        {
            GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
            if (triMesh == NULL)
            {
                MessageBox(NULL, "请先导入网格", "温馨提示", MB_OK);
                return;
            }
            int vertexCount = triMesh->GetVertexCount();
            std::vector<GPP::Int> colorIds = ModelManager::Get()->GetColorIds();
            if (colorIds.empty())
            {
                MessageBox(NULL, "ColorIds is empty", "温馨提示", MB_OK);
                return;
            }
            std::vector<GPP::Vector3> vertexColors(vertexCount);
            if (triMesh->HasVertexColor() == false)
            {
                MessageBox(NULL, "网格需要颜色", "温馨提示", MB_OK);
                return;
            }
            for (int vid = 0; vid < vertexCount; vid++)
            {
                vertexColors.at(vid) = triMesh->GetVertexColor(vid);
            }
            mIsCommandInProgress = true;
#if MAKEDUMPFILE
            GPP::DumpOnce();
#endif
            GPP::ErrorCode res = GPP::IntrinsicColor::TuneMeshColorFromMultiPatch(triMesh, colorIds, vertexColors,
                GPP::Vector3(sharpDiff_H ,sharpDiff_S, sharpDiff_V));
            mIsCommandInProgress = false;
            if (res == GPP_API_IS_NOT_AVAILABLE)
            {
                MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
                MagicCore::ToolKit::Get()->SetAppRunning(false);
            }
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "网格颜色融合失败", "温馨提示", MB_OK);
                return;
            }
            for (int vid = 0; vid < vertexCount; vid++)
            {
                triMesh->SetVertexColor(vid, vertexColors.at(vid));
            }
            mUpdateDisplay = true;
        }
    }
}

#include "stdafx.h"
#include <process.h>
#include "PointShopApp.h"
#include "PointShopAppUI.h"
#include "../Common/LogSystem.h"
#include "../Common/ToolKit.h"
#include "../Common/RenderSystem.h"
#include "../Common/ViewTool.h"
#include "../Common/PickTool.h"
#include "opencv2/opencv.hpp"
#include "ToolAnn.h"
#include "AppManager.h"
#include "MeshShopApp.h"
#include "ModelManager.h"
#include "MagicPointCloud.h"
#include <algorithm>

namespace MagicApp
{
    static unsigned __stdcall RunThread(void *arg)
    {
        PointShopApp* app = (PointShopApp*)arg;
        if (app == NULL)
        {
            return 0;
        }
        app->DoCommand(false);
        return 1;
    }

    PointShopApp::PointShopApp() :
        mpUI(NULL),
        mpViewTool(NULL),
        mpPickTool(NULL),
#if DEBUGDUMPFILE
        mpDumpInfo(NULL),
#endif
        mCommandType(NONE),
        mUpdatePointCloudRendering(false),
        mIsCommandInProgress(false),
        mIsDepthImage(0),
        mReconstructionQuality(4),
        mPointSelectFlag(),
        mRightMouseType(MOVE),
        mMousePressdCoord(),
        mIgnoreBack(true),
        mNeedFillHole(false),
        mResolution(0),
        mEnterMeshShop(0),
        mSmoothCount(0),
        mNeighborCount(0),
        mColorNeighborCount(0),
        mIsolateValue(0),
        mSharpDiff()
    {
    }

    PointShopApp::~PointShopApp()
    {
        GPPFREEPOINTER(mpUI);
        GPPFREEPOINTER(mpViewTool);
        GPPFREEPOINTER(mpPickTool);
#if DEBUGDUMPFILE
        GPPFREEPOINTER(mpDumpInfo);
#endif
    }

    bool PointShopApp::Enter(void)
    {
        InfoLog << "Enter PointShopApp" << std::endl; 
        if (mpUI == NULL)
        {
            mpUI = new PointShopAppUI;
        }
        mpUI->Setup();
        SetupScene();
        UpdatePickTool();
        ResetSelection();
        UpdatePointCloudRendering();
        return true;
    }

    bool PointShopApp::Update(double timeElapsed)
    {
        if (mpUI && mpUI->IsProgressbarVisible())
        {
            int progressValue = int(GPP::GetApiProgress() * 100.0);
            mpUI->SetProgressbar(progressValue);
        }
        if (mUpdatePointCloudRendering)
        {
            UpdatePointCloudRendering();
            mUpdatePointCloudRendering = false;
        }
        if (mEnterMeshShop)
        {
            mEnterMeshShop = false;
            AppManager::Get()->EnterApp(new MeshShopApp, "MeshShopApp");
        }
        return true;
    }

    bool PointShopApp::Exit(void)
    {
        InfoLog << "Exit PointShopApp" << std::endl; 
        ShutdownScene();
        if (mpUI != NULL)
        {
            mpUI->Shutdown();
        }
        ClearData();
        return true;
    }

    void PointShopApp::SelectControlPointByRectangle(int startCoordX, int startCoordY, int endCoordX, int endCoordY)
    {
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
        GPP::Int pointCount = pointCloud->GetPointCount();
        GPP::Vector3 coord;
        GPP::Vector3 normal;
        bool ignoreBack = pointCloud->HasNormal() && mIgnoreBack;
        for (GPP::Int pid = 0; pid < pointCount; pid++)
        {
            if (mRightMouseType == SELECT_ADD && mPointSelectFlag.at(pid))
            {
                continue;
            }
            else if (mRightMouseType == SELECT_DELETE && mPointSelectFlag.at(pid) == 0)
            {
                continue;
            }
            GPP::Vector3 coord = pointCloud->GetPointCoord(pid);
            Ogre::Vector3 ogreCoord(coord[0], coord[1], coord[2]);
            ogreCoord = wvpM * ogreCoord;
            if (ogreCoord.x > minX && ogreCoord.x < maxX && ogreCoord.y > minY && ogreCoord.y < maxY)
            {
                if (ignoreBack)
                {
                    normal = pointCloud->GetPointNormal(pid);
                    Ogre::Vector4 ogreNormal(normal[0], normal[1], normal[2], 0);
                    ogreNormal = worldM * ogreNormal;
                    if (ogreNormal.z > 0)
                    {
                        if (mRightMouseType == SELECT_ADD)
                        {
                            mPointSelectFlag.at(pid) = 1;
                        }
                        else
                        {
                            mPointSelectFlag.at(pid) = 0;
                        }
                    }
                }
                else
                {
                    if (mRightMouseType == SELECT_ADD)
                    {
                        mPointSelectFlag.at(pid) = 1;
                    }
                    else
                    {
                        mPointSelectFlag.at(pid) = 0;
                    }
                }
            }
        }
    }

    void PointShopApp::UpdateRectangleRendering(int startCoordX, int startCoordY, int endCoordX, int endCoordY)
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

    void PointShopApp::ClearRectangleRendering()
    {
        MagicCore::RenderSystem::Get()->HideRenderingObject("PickRectangleObj");
    }

    void PointShopApp::SetupMagicPointCloud(MagicPointCloud& magicPointCloud)
    {
        std::vector<GPP::ImageColorId>* imageColorIds = ModelManager::Get()->GetImageColorIdsPointer();
        if (imageColorIds && imageColorIds->size() == magicPointCloud.GetPointCount())
        {
            magicPointCloud.SetImageColorIds(imageColorIds);
        }
        std::vector<int>* colorIds = ModelManager::Get()->GetColorIdsPointer();
        if (colorIds && colorIds->size() == magicPointCloud.GetPointCount())
        {
            magicPointCloud.SetColorIds(colorIds);
        }
        std::vector<int>* cloudIds = ModelManager::Get()->GetCloudIdsPointer();
        if (cloudIds && cloudIds->size() == magicPointCloud.GetPointCount())
        {
            magicPointCloud.SetCloudIds(cloudIds);
        }
    }

    void PointShopApp::SaveImageColorInfo()
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
    
    void PointShopApp::LoadImageColorInfo()
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

    void PointShopApp::PickPointCloudColorFromImages()
    {
        GPP::PointCloud* pointCloud = ModelManager::Get()->GetPointCloud();
        if (pointCloud == NULL)
        {
            MessageBox(NULL, "mpPointCloud == NULL", "温馨提示", MB_OK);
            return;
        }
        std::vector<std::string> textureImageFiles = ModelManager::Get()->GetTextureImageFiles();
        if (textureImageFiles.empty())
        {
            MessageBox(NULL, "mTextureImageFiles is empty", "温馨提示", MB_OK);
            return;
        }
        std::vector<GPP::ImageColorId> imageColorIds = ModelManager::Get()->GetImageColorIds();
        if (imageColorIds.size() != pointCloud->GetPointCount())
        {
            MessageBox(NULL, "mImageColorIds.size() != mpPointCloud->GetPointCount()", "温馨提示", MB_OK);
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
        pointCloud->SetHasColor(true);
        int pointCount = pointCloud->GetPointCount();
        for (int pid = 0; pid < pointCount; pid++)
        {
            GPP::ImageColorId colorId = imageColorIds.at(pid);
            int imageIndex = colorId.GetImageIndex();
            const unsigned char* pixel = imageList.at(imageIndex).ptr(imageHList.at(imageIndex) - colorId.GetLocalY() - 1,
                colorId.GetLocalX());
            pointCloud->SetPointColor(pid, GPP::Vector3(double(pixel[2]) / 255.0, double(pixel[1]) / 255.0, double(pixel[0]) / 255.0));
        }
        for (int fid = 0; fid < imageCount; fid++)
        {
            imageList.at(fid).release();
        }

        UpdatePointCloudRendering();
    }

    void PointShopApp::ConstructImageColorIdForMesh(GPP::TriMesh* triMesh, const GPP::IPointCloud* pointCloud)
    {
        std::vector<GPP::ImageColorId> originImageColorIds = ModelManager::Get()->GetImageColorIds();
        std::vector<int> colorIds = ModelManager::Get()->GetColorIds();
        if (originImageColorIds.size() > 0 && originImageColorIds.size() == pointCloud->GetPointCount())
        {
            std::vector<GPP::ImageColorId> meshColorIds;
            GPP::ErrorCode res = GPP::OptimiseMapping::TransferMappingToMesh(pointCloud, originImageColorIds,
                triMesh, meshColorIds, 1.0, false);
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "TransferMappingToMesh Failed", "温馨提示", MB_OK);
                return;
            }
            std::vector<int> imageColorIdFlags(triMesh->GetVertexCount(), 1);
            ModelManager::Get()->SetImageColorIds(meshColorIds);
            ModelManager::Get()->SetImageColorIdFlag(imageColorIdFlags);
        }
        if (colorIds.size() > 0 && colorIds.size() == pointCloud->GetPointCount())
        {
            GPP::PointCloudPointList pointList(pointCloud);
            GPP::Ann ann;
            GPP::ErrorCode res = ann.Init(&pointList);
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "Ann Init Failed", "温馨提示", MB_OK);
                return;
            }
            int vertexCount = triMesh->GetVertexCount();
            std::vector<GPP::Int> meshColorIds(vertexCount);
            double searchData[3] = {-1};
            int indexRes[1] = {-1};
            for (int vid = 0; vid < vertexCount; vid++)
            {
                GPP::Vector3 coord = triMesh->GetVertexCoord(vid);
                searchData[0] = coord[0];
                searchData[1] = coord[1];
                searchData[2] = coord[2];
                res = ann.FindNearestNeighbors(searchData, 1, 1, indexRes, NULL);
                if (res != GPP_NO_ERROR)
                {
                    MessageBox(NULL, "Ann FindNearestNeighbors Failed", "温馨提示", MB_OK);
                    return;
                }
                meshColorIds.at(vid) = colorIds.at(indexRes[0]);
            }
            ModelManager::Get()->SetColorIds(meshColorIds);
        }
    }

    /*void PointShopApp::ConstructImageColorIdForMesh(const GPP::ITriMesh* triMesh, const GPP::IPointCloud* pointCloud)
    {
        std::vector<int> colorIds = ModelManager::Get()->GetColorIds();
        if (colorIds.size() > 0)
        {
            GPP::PointCloudPointList pointList(pointCloud);
            GPP::Ann ann;
            GPP::ErrorCode res = ann.Init(&pointList);
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "Ann Init Failed", "温馨提示", MB_OK);
                return;
            }
            int vertexCount = triMesh->GetVertexCount();
            std::vector<GPP::Int> meshColorIds(vertexCount);
            double searchData[3] = {-1};
            int indexRes[1] = {-1};
            for (int vid = 0; vid < vertexCount; vid++)
            {
                GPP::Vector3 coord = triMesh->GetVertexCoord(vid);
                searchData[0] = coord[0];
                searchData[1] = coord[1];
                searchData[2] = coord[2];
                res = ann.FindNearestNeighbors(searchData, 1, 1, indexRes, NULL);
                if (res != GPP_NO_ERROR)
                {
                    MessageBox(NULL, "Ann FindNearestNeighbors Failed", "温馨提示", MB_OK);
                    return;
                }
                meshColorIds.at(vid) = colorIds.at(indexRes[0]);
            }
            ModelManager::Get()->SetColorIds(meshColorIds);
        }
        std::vector<GPP::ImageColorId> originImageColorIds = ModelManager::Get()->GetImageColorIds();
        if (originImageColorIds.size() > 0)
        {
            GPP::PointCloudPointList pointList(pointCloud);
            GPP::Real pointDensity;
            GPP::ErrorCode res = GPP::CalculatePointListDensity(&pointList, 5, pointDensity);
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "CalculatePointListDensity Failed", "温馨提示", MB_OK);
                return;
            }
            pointDensity *= pointDensity;
            InfoLog << "point cloud densidy: " << pointDensity << std::endl;
            GPP::TriMeshPointList triMeshList(triMesh);
            GPP::Ann ann;
            res = ann.Init(&triMeshList);
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "Ann Init Failed", "温馨提示", MB_OK);
                return;
            }
            int vertexCount = triMesh->GetVertexCount();
            std::vector<std::vector<int> > vertex2PointMap(vertexCount, std::vector<int>());
            std::vector<std::vector<double> > vertexMinDist(vertexCount, std::vector<double>());
            double searchData[3] = {-1};
            int indexRes[1] = {-1};
            double distanceRes[1] = {-1};
            int pointCount = pointCloud->GetPointCount();
            for (int pid = 0; pid < pointCount; pid++)
            {
                GPP::Vector3 coord = pointCloud->GetPointCoord(pid);
                searchData[0] = coord[0];
                searchData[1] = coord[1];
                searchData[2] = coord[2];
                res = ann.FindNearestNeighbors(searchData, 1, 1, indexRes, distanceRes);
                if (res != GPP_NO_ERROR)
                {
                    MessageBox(NULL, "Ann FindNearestNeighbors Failed", "温馨提示", MB_OK);
                    return;
                }
                vertex2PointMap.at(indexRes[0]).push_back(pid);
                vertexMinDist.at(indexRes[0]).push_back(distanceRes[0]);
            }
            int invalidMapCount = 0;
            int multiMapCount = 0;
            std::vector<int> imageColorIdFlags(vertexCount, 1);
            std::vector<GPP::ImageColorId> imageColorIds(vertexCount);
            for (int vid = 0; vid < vertexCount; vid++)
            {
                if (vertex2PointMap.at(vid).empty())
                {
                    invalidMapCount++;
                    imageColorIdFlags.at(vid) = 0;
                }
                else if (vertex2PointMap.at(vid).size() == 1)
                {
                    if (vertexMinDist.at(vid).at(0) > pointDensity)
                    {
                        invalidMapCount++;
                        imageColorIdFlags.at(vid) = 0;
                    }
                    else
                    {
                        imageColorIds.at(vid) = originImageColorIds.at(vertex2PointMap.at(vid).at(0));
                    }
                }
                else if (vertex2PointMap.at(vid).size() > 1)
                {
                    int neighborSize = vertex2PointMap.at(vid).size();
                    double minDist = GPP::REAL_LARGE;
                    bool isAccurate = false;
                    for (int nid = 0; nid < neighborSize; nid++)
                    {
                        if (vertexMinDist.at(vid).at(nid) < minDist)
                        {
                            minDist = vertexMinDist.at(vid).at(nid);
                            if (minDist < GPP::REAL_TOL)
                            {
                                imageColorIds.at(vid) = originImageColorIds.at(vertex2PointMap.at(vid).at(nid));
                                isAccurate = true;
                                break;
                            }
                        }
                    }
                    if (!isAccurate)
                    {
                        multiMapCount++;
                        std::map<int, int> imageIdCount;
                        for (int nid = 0; nid < neighborSize; nid++)
                        {
                            imageIdCount[originImageColorIds.at(vertex2PointMap.at(vid).at(nid)).GetImageIndex()] += 1;
                        }
                        int imageId = imageIdCount.begin()->first;
                        int maxImageIdCount = imageIdCount.begin()->second;
                        for (std::map<int, int>::iterator itr = imageIdCount.begin(); itr != imageIdCount.end(); ++itr)
                        {
                            if (itr->second > maxImageIdCount)
                            {
                                maxImageIdCount = itr->second;
                                imageId = itr->first;
                            }
                        }
                        double weightSum = 0.0;
                        minDist *= 1.0e-3;
                        double imageX = 0;
                        double imageY = 0;
                        for (int nid = 0; nid < neighborSize; nid++)
                        {
                            if (originImageColorIds.at(vertex2PointMap.at(vid).at(nid)).GetImageIndex() != imageId)
                            {
                                continue;
                            }
                            double curW = 1.0 / (minDist + vertexMinDist.at(vid).at(nid));
                            weightSum += curW;
                            imageX += (curW * originImageColorIds.at(vertex2PointMap.at(vid).at(nid)).GetLocalX());
                            imageY += (curW * originImageColorIds.at(vertex2PointMap.at(vid).at(nid)).GetLocalY());
                        }
                        imageX /= weightSum;
                        imageY /= weightSum;
                        imageColorIds.at(vid) = GPP::ImageColorId(imageId, int(imageX + 0.5), int(imageY + 0.5));
                    }
                }
            }
            InfoLog << " invalidMapCount=" << invalidMapCount << " vertexCount=" << vertexCount << " multiMapCount=" << multiMapCount << std::endl;
            ModelManager::Get()->SetImageColorIds(imageColorIds);
            ModelManager::Get()->SetImageColorIdFlag(imageColorIdFlags);
        }
    }*/

    bool PointShopApp::MouseMoved( const OIS::MouseEvent &arg )
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
        else if ((mRightMouseType == SELECT_ADD || mRightMouseType == SELECT_DELETE) && arg.state.buttonDown(OIS::MB_Right))
        {
            UpdateRectangleRendering(mMousePressdCoord[0], mMousePressdCoord[1], arg.state.X.abs, arg.state.Y.abs);
        }

        return true;
    }

    bool PointShopApp::MousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
    {
        if ((id == OIS::MB_Middle || id == OIS::MB_Left || (id == OIS::MB_Right && mRightMouseType == MOVE)) && mpViewTool != NULL)
        {
            mpViewTool->MousePressed(arg.state.X.abs, arg.state.Y.abs);
        }
        else if (mRightMouseType == REVERSE_NORMAL && arg.state.buttonDown(OIS::MB_Right) && mpPickTool)
        {
            mpPickTool->MousePressed(arg.state.X.abs, arg.state.Y.abs);
        }
        else if (arg.state.buttonDown(OIS::MB_Right) && ModelManager::Get()->GetPointCloud() && (mRightMouseType == SELECT_ADD || mRightMouseType == SELECT_DELETE))
        {
            mMousePressdCoord[0] = arg.state.X.abs;
            mMousePressdCoord[1] = arg.state.Y.abs;
        }
        return true;
    }

    bool PointShopApp::MouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
    {
        if ((id == OIS::MB_Middle || id == OIS::MB_Left || (id == OIS::MB_Right && mRightMouseType == MOVE)) && mpViewTool != NULL)
        {
            mpViewTool->MouseReleased(); 
        }
        else if (id == OIS::MB_Right && mRightMouseType == REVERSE_NORMAL && mpPickTool)
        {
            mpPickTool->MouseReleased(arg.state.X.abs, arg.state.Y.abs);
            GPP::Int pickedId = mpPickTool->GetPickPointId();
            mpPickTool->ClearPickedIds();
            if (pickedId != -1)
            {
                GPP::ErrorCode res = GPP::ConsolidatePointCloud::ReversePatchNormal(ModelManager::Get()->GetPointCloud(), pickedId, mNeighborCount);
                if (res == GPP_API_IS_NOT_AVAILABLE)
                {
                    MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
                    MagicCore::ToolKit::Get()->SetAppRunning(false);
                }
                if (res != GPP_NO_ERROR)
                {
                    MessageBox(NULL, "点云法线方向修复失败", "温馨提示", MB_OK);
                    return true;
                }
                UpdatePointCloudRendering();
            }
        }
        else if (id == OIS::MB_Right && ModelManager::Get()->GetPointCloud() && (mRightMouseType == SELECT_ADD || mRightMouseType == SELECT_DELETE))
        {
            SelectControlPointByRectangle(mMousePressdCoord[0], mMousePressdCoord[1], arg.state.X.abs, arg.state.Y.abs);
            ClearRectangleRendering();
            UpdatePointCloudRendering();
        }
        
        return true;
    }

    bool PointShopApp::KeyPressed( const OIS::KeyEvent &arg )
    {
        if (arg.key == OIS::KC_D)
        {
#if DEBUGDUMPFILE
            RunDumpInfo();
#endif
        }
        else if (arg.key == OIS::KC_N)
        {
            if (ModelManager::Get()->GetPointCloud())
            {
                ModelManager::Get()->GetPointCloud()->SetHasNormal(false);
                UpdatePointCloudRendering();
            }
        }
        else if (arg.key == OIS::KC_C)
        {
            GPP::PointCloud* pointCloud = ModelManager::Get()->GetPointCloud();
            if (pointCloud)
            {
                pointCloud->SetHasColor(false);
                pointCloud->SetDefaultColor(GPP::Vector3(0.09, 0.48627, 0.69));
                UpdatePointCloudRendering();
            }
        }
        else if (arg.key == OIS::KC_R) // A temporary command
        {
            GPP::PointCloud* pointCloud = ModelManager::Get()->GetPointCloud();
            if (pointCloud)
            {
                if (pointCloud->HasColor())
                {
                    int pointCount = pointCloud->GetPointCount();
                    for (int pid = 0; pid < pointCount; pid++)
                    {
                        GPP::Vector3 curColor = pointCloud->GetPointColor(pid);
                        curColor = GPP::IntrinsicColor::ConvertRGB2HSV(curColor);
                        curColor[0] /= 360;
                        pointCloud->SetPointColor(pid, GPP::Vector3(curColor[0], curColor[0], curColor[0]));
                    }
                }
                else
                {
                    pointCloud->SetHasColor(true);
                    int pointCount = pointCloud->GetPointCount();
                    for (int pid = 0; pid < pointCount; pid++)
                    {
                        pointCloud->SetPointColor(pid, GPP::Vector3(1.0, 0, 0));
                    }
                }
                UpdatePointCloudRendering();
            }
        }
        else if (arg.key == OIS::KC_G) // A temporary command
        {
            GPP::PointCloud* pointCloud = ModelManager::Get()->GetPointCloud();
            if (pointCloud)
            {
                if (pointCloud->HasColor())
                {
                    int pointCount = pointCloud->GetPointCount();
                    for (int pid = 0; pid < pointCount; pid++)
                    {
                        GPP::Vector3 curColor = pointCloud->GetPointColor(pid);
                        curColor = GPP::IntrinsicColor::ConvertRGB2HSV(curColor);
                        pointCloud->SetPointColor(pid, GPP::Vector3(curColor[1], curColor[1], curColor[1]));
                    }
                }
                else
                {
                    pointCloud->SetHasColor(true);
                    int pointCount = pointCloud->GetPointCount();
                    for (int pid = 0; pid < pointCount; pid++)
                    {
                        pointCloud->SetPointColor(pid, GPP::Vector3(0, 1.0, 0));
                    }
                }
                UpdatePointCloudRendering();
            }
        }
        else if (arg.key == OIS::KC_B) // A temporary command
        {
            GPP::PointCloud* pointCloud = ModelManager::Get()->GetPointCloud();
            if (pointCloud)
            {
                if (pointCloud->HasColor())
                {
                    int pointCount = pointCloud->GetPointCount();
                    for (int pid = 0; pid < pointCount; pid++)
                    {
                        GPP::Vector3 curColor = pointCloud->GetPointColor(pid);
                        curColor = GPP::IntrinsicColor::ConvertRGB2HSV(curColor);
                        pointCloud->SetPointColor(pid, GPP::Vector3(curColor[2], curColor[2], curColor[2]));
                    }
                }
                else
                {
                    pointCloud->SetHasColor(true);
                    int pointCount = pointCloud->GetPointCount();
                    for (int pid = 0; pid < pointCount; pid++)
                    {
                        pointCloud->SetPointColor(pid, GPP::Vector3(0, 0, 1.0));
                    }
                }
                UpdatePointCloudRendering();
            }
        }
        else if (arg.key == OIS::KC_X)
        {
            GPP::PointCloud* pointCloud = ModelManager::Get()->GetPointCloud();
            if (pointCloud)
            {
                GPP::Vector3 cameraDir(0, 0, 1);
                std::vector<int> boundaryIds;
                GPP::DetectBoundaryPointByZAngle(pointCloud, GPP::ONE_RADIAN * 80, &cameraDir, boundaryIds);
                pointCloud->SetHasColor(true);
                for (std::vector<int>::iterator itr = boundaryIds.begin(); itr != boundaryIds.end(); ++itr)
                {
                    pointCloud->SetPointColor(*itr, GPP::Vector3(1, 0, 0));
                }
                //GPP::DeletePointCloudElements(pointCloud, boundaryIds);
                UpdatePointCloudRendering();
            }
        }
        else if (arg.key == OIS::KC_Z)
        {
            GPP::PointCloud* pointCloud = ModelManager::Get()->GetPointCloud();
            std::vector<int> colorIds = ModelManager::Get()->GetColorIds();
            if (pointCloud && pointCloud->GetPointCount() == colorIds.size())
            {
                pointCloud->SetHasColor(true);
                int pointCount = pointCloud->GetPointCount();
                double deltaColor = 0.15;
                int maxId = 9;
                for (int pid = 0; pid < pointCount; pid++)
                {
                    pointCloud->SetPointColor(pid, MagicCore::ToolKit::ColorCoding((colorIds.at(pid) % maxId) * deltaColor));
                }
                UpdatePointCloudRendering();
            }
        }
        else if (arg.key == OIS::KC_I)
        {
            GPP::PointCloud* pointCloud = ModelManager::Get()->GetPointCloud();
            std::vector<GPP::ImageColorId> imageColorIds = ModelManager::Get()->GetImageColorIds();
            if (pointCloud == NULL || pointCloud->GetPointCount() != imageColorIds.size())
            {
                return true;
            }
            pointCloud->SetHasColor(true);
            int maxColorId = 10;
            double deltaColor = 0.1;
            int pointCount = pointCloud->GetPointCount();
            for (int pid = 0; pid < pointCount; pid++)
            {
                int imageIndex = imageColorIds.at(pid).GetImageIndex();
                if (imageIndex < 0)
                {
                    pointCloud->SetPointColor(pid, MagicCore::ToolKit::Get()->ColorCoding(0));
                }
                else
                {
                    pointCloud->SetPointColor(pid, MagicCore::ToolKit::Get()->ColorCoding(0.2 + imageIndex % maxColorId * deltaColor));
                }
            }
            UpdatePointCloudRendering();
        }
        return true;
    }

    void PointShopApp::WindowFocusChanged(Ogre::RenderWindow* rw)
    {
        if (mpViewTool)
        {
            mpViewTool->MouseReleased();
        }
    }

    void PointShopApp::SetupScene(void)
    {
        Ogre::SceneManager* sceneManager = MagicCore::RenderSystem::Get()->GetSceneManager();
        sceneManager->setAmbientLight(Ogre::ColourValue(0.3, 0.3, 0.3));
        Ogre::Light* light = sceneManager->createLight("PointShop_SimpleLight");
        light->setPosition(0, 0, 20);
        light->setDiffuseColour(0.8, 0.8, 0.8);
        light->setSpecularColour(0.5, 0.5, 0.5);
        InitViewTool();
        if (ModelManager::Get()->GetPointCloud())
        {
            mpUI->SetPointCloudInfo(ModelManager::Get()->GetPointCloud()->GetPointCount());
        }
    }

    void PointShopApp::ShutdownScene(void)
    {
        Ogre::SceneManager* sceneManager = MagicCore::RenderSystem::Get()->GetSceneManager();
        sceneManager->setAmbientLight(Ogre::ColourValue::Black);
        sceneManager->destroyLight("PointShop_SimpleLight");
        //MagicCore::RenderSystem::Get()->SetupCameraDefaultParameter();
        MagicCore::RenderSystem::Get()->HideRenderingObject("PointCloud_PointShop");
        MagicCore::RenderSystem::Get()->HideRenderingObject("Primitive_PointShop");
        /*if (MagicCore::RenderSystem::Get()->GetSceneManager()->hasSceneNode("ModelNode"))
        {
            MagicCore::RenderSystem::Get()->GetSceneManager()->getSceneNode("ModelNode")->resetToInitialState();
        }*/
    }

    void PointShopApp::ClearData(void)
    {
        GPPFREEPOINTER(mpUI);
        GPPFREEPOINTER(mpViewTool);
        GPPFREEPOINTER(mpPickTool);
#if DEBUGDUMPFILE
        GPPFREEPOINTER(mpDumpInfo);
#endif
        mPointSelectFlag.clear();
        mRightMouseType = MOVE;
    }

    void PointShopApp::ResetSelection()
    {
        if (ModelManager::Get()->GetPointCloud())
        {
            mPointSelectFlag = std::vector<bool>(ModelManager::Get()->GetPointCloud()->GetPointCount(), 0);
        }
        else
        {
            mPointSelectFlag.clear();
        }
        MagicCore::RenderSystem::Get()->HideRenderingObject("Primitive_PointShop");
    }

    void PointShopApp::DoCommand(bool isSubThread)
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
            case MagicApp::PointShopApp::NONE:
                break;
            case MagicApp::PointShopApp::EXPORT:
                ExportPointCloud(false);
                break;
            case MagicApp::PointShopApp::NORMALCALCULATION:
                CalculatePointCloudNormal(mIsDepthImage, mNeighborCount, false);
                break;
            case MagicApp::PointShopApp::NORMALSMOOTH:
                SmoothPointCloudNormal(mNeighborCount, false);
                break;
            case MagicApp::PointShopApp::NORMALUPDATE:
                UpdatePointCloudNormal(mNeighborCount, false);
                break;
            case MagicApp::PointShopApp::OUTLIER:
                RemovePointCloudOutlier(false);
                break;
            case MagicApp::PointShopApp::ISOLATE:
                RemoveIsolatePart(mIsolateValue, false);
                break;
            case MagicApp::PointShopApp::GEOMETRYSMOOTH:
                SmoothPointCloudGeoemtry(mSmoothCount, false);
                break;
            case MagicApp::PointShopApp::FUSECOLOR:
                FusePointCloudColor(mColorNeighborCount, mSharpDiff[0], mSharpDiff[1], mSharpDiff[2], false);
                break;
            case MagicApp::PointShopApp::FUSETEXTURE:
                FuseTextureImage(false);
                break;
            case MagicApp::PointShopApp::RECONSTRUCTION:
                ReconstructMesh(mNeedFillHole, mReconstructionQuality, false);
                return;
                break;
            default:
                break;
            }
            mpUI->StopProgressbar();
        }
    }

    bool PointShopApp::ImportPointCloud()
    {
        if (mIsCommandInProgress)
        {
            MessageBox(NULL, "请等待当前命令执行完", "温馨提示", MB_OK);
            return false;
        }
        std::string fileName;
        char filterName[] = "ASC Files(*.asc)\0*.asc\0OBJ Files(*.obj)\0*.obj\0PLY Files(*.ply)\0*.ply\0Geometry++ Point Cloud(*.gpc)\0*.gpc\0XYZ Files(*.xyz)\0*.xyz\0";
        if (MagicCore::ToolKit::FileOpenDlg(fileName, filterName))
        {
            ModelManager::Get()->ClearMesh();
            if (ModelManager::Get()->ImportPointCloud(fileName) == false)
            {
                MessageBox(NULL, "点云导入失败", "温馨提示", MB_OK);
                return false;
            }
            mpUI->SetPointCloudInfo(ModelManager::Get()->GetPointCloud()->GetPointCount());
            UpdatePickTool();
            ResetSelection();
            mRightMouseType = MOVE;
            InfoLog << " ImportPointCloud" << std::endl;
            UpdatePointCloudRendering();
            return true;
        }
        return false;
    }

    void PointShopApp::ExportPointCloud(bool isSubThread)
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        GPP::PointCloud* pointCloud = ModelManager::Get()->GetPointCloud();
        if (!pointCloud)
        {
            return;
        }
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
            GPP::Real scaleValue = ModelManager::Get()->GetScaleValue();
            GPP::Vector3 objCenterCoord = ModelManager::Get()->GetObjCenterCoord();
            pointCloud->UnifyCoords(1.0 / scaleValue, objCenterCoord * (-scaleValue));
            GPP::ErrorCode res = GPP::Parser::ExportPointCloud(fileName, pointCloud);
            pointCloud->UnifyCoords(scaleValue, objCenterCoord);
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "导出点云失败", "温馨提示", MB_OK);
            }
        }
    }

    void PointShopApp::SmoothPointCloudNormal(int neighborCount, bool isSubThread)
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        GPP::PointCloud* pointCloud = ModelManager::Get()->GetPointCloud();
        if (pointCloud->HasNormal() == false)
        {
            MessageBox(NULL, "请先给点云计算法向量", "温馨提示", MB_OK);
            return;
        }
        if (isSubThread)
        {
            mCommandType = NORMALSMOOTH;
            mNeighborCount = neighborCount;
            DoCommand(true);
        }
        else
        {
            mIsCommandInProgress = true;
#if MAKEDUMPFILE
            GPP::DumpOnce();
#endif
            GPP::ErrorCode res = GPP::ConsolidatePointCloud::SmoothNormal(pointCloud, 0.250, neighborCount);
            mIsCommandInProgress = false;
            mUpdatePointCloudRendering = true;
            if (res == GPP_API_IS_NOT_AVAILABLE)
            {
                MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
                MagicCore::ToolKit::Get()->SetAppRunning(false);
            }
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "点云法线光滑失败", "温馨提示", MB_OK);
                return;
            }
        }
    }

    void PointShopApp::UpdatePointCloudNormal(int neighborCount, bool isSubThread)
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        GPP::PointCloud* pointCloud = ModelManager::Get()->GetPointCloud();
        if (pointCloud->HasNormal() == false)
        {
            MessageBox(NULL, "请先给点云计算法向量", "温馨提示", MB_OK);
            return;
        }
        if (isSubThread)
        {
            mCommandType = NORMALUPDATE;
            mNeighborCount = neighborCount;
            DoCommand(true);
        }
        else
        {
            mIsCommandInProgress = true;
            GPP::ErrorCode res = GPP::UpdatePointCloudNormal(pointCloud, neighborCount);
            mIsCommandInProgress = false;
            mUpdatePointCloudRendering = true;
            if (res == GPP_API_IS_NOT_AVAILABLE)
            {
                MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
                MagicCore::ToolKit::Get()->SetAppRunning(false);
            }
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "点云法线更新失败", "温馨提示", MB_OK);
                return;
            }
        }
    }

    void PointShopApp::SmoothPointCloudGeoemtry(int smoothCount, bool isSubThread)
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        if (isSubThread)
        {
            mCommandType = GEOMETRYSMOOTH;
            mSmoothCount = smoothCount;
            DoCommand(true);
        }
        else
        {
            GPP::PointCloud* pointCloud = ModelManager::Get()->GetPointCloud();
            mIsCommandInProgress = true;
#if MAKEDUMPFILE
            GPP::DumpOnce();
#endif
            GPP::ErrorCode res = GPP::ConsolidatePointCloud::SmoothGeometry(pointCloud, 25, smoothCount);
            mIsCommandInProgress = false;
            if (res == GPP_API_IS_NOT_AVAILABLE)
            {
                MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
                MagicCore::ToolKit::Get()->SetAppRunning(false);
            }
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "点云光滑失败", "温馨提示", MB_OK);
                return;
            }
            mUpdatePointCloudRendering = true;
        }
    }

    void PointShopApp::FusePointCloudColor(int neighborCount, double sharpDiff_H, double sharpDiff_S, 
        double sharpDiff_V, bool isSubThread)
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        if (isSubThread)
        {
            mCommandType = FUSECOLOR;
            mColorNeighborCount = neighborCount;
            mSharpDiff = GPP::Vector3(sharpDiff_H, sharpDiff_S, sharpDiff_V);
            DoCommand(true);
        }
        else
        {
            GPP::PointCloud* pointCloud = ModelManager::Get()->GetPointCloud();
            std::vector<int> colorIds = ModelManager::Get()->GetColorIds();
            if (pointCloud->HasColor() == false)
            {
                MessageBox(NULL, "点云没有颜色信息", "温馨提示", MB_OK);
                return;
            }
            if (pointCloud->GetPointCount() != colorIds.size())
            {
                MessageBox(NULL, "点云颜色融合需要点云ID信息", "温馨提示", MB_OK);
                return;
            }
            mIsCommandInProgress = true;
            int pointCount = pointCloud->GetPointCount();
            std::vector<GPP::Vector3> pointColors(pointCount);
            for (int pid = 0; pid < pointCount; pid++)
            {
                pointColors.at(pid) = pointCloud->GetPointColor(pid);
            }
            InfoLog << "FusePointCloudColor: neighborCount=" << neighborCount << std::endl;
#if MAKEDUMPFILE
            GPP::DumpOnce();
#endif
            GPP::ErrorCode res = GPP::IntrinsicColor::TuneColorFromMultiFrame(pointCloud, neighborCount, 
                colorIds, pointColors, GPP::Vector3(sharpDiff_H, sharpDiff_S, sharpDiff_V));
            mIsCommandInProgress = false;
            if (res == GPP_API_IS_NOT_AVAILABLE)
            {
                MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
                MagicCore::ToolKit::Get()->SetAppRunning(false);
            }
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "点云颜色融合失败", "温馨提示", MB_OK);
                return;
            }
            for (int pid = 0; pid < pointCount; pid++)
            {
                pointCloud->SetPointColor(pid, pointColors.at(pid));
            }
            mUpdatePointCloudRendering = true;
        }
    }

    void PointShopApp::FuseTextureImage(bool isSubThread)
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        if (isSubThread)
        {
            mCommandType = FUSETEXTURE;
            DoCommand(true);
        }
        else
        {
            GPP::PointCloud* pointCloud = ModelManager::Get()->GetPointCloud();
            if (pointCloud->HasColor() == false)
            {
                MessageBox(NULL, "输入点云需要有颜色", "温馨提示", MB_OK);
                return;
            }
            std::vector<GPP::ImageColorId> imageColorIds = ModelManager::Get()->GetImageColorIds();
            if (imageColorIds.size() != pointCloud->GetPointCount())
            {
                MessageBox(NULL, "顶点与图片对应文件有错", "温馨提示", MB_OK);
                return;
            }
            std::vector<std::string> textureImageFiles = ModelManager::Get()->GetTextureImageFiles();
            int imageCount = textureImageFiles.size();
            InfoLog << "imageCount=" << imageCount << std::endl;
            int pointCount = pointCloud->GetPointCount();
            for (int iid = 0; iid < imageCount; iid++)
            {
                std::vector<int> pointCoords;
                std::vector<GPP::Vector3> pointColors;
                for (int pid = 0; pid < pointCount; pid++)
                {
                    if (imageColorIds.at(pid).GetImageIndex() != iid)
                    {
                        continue;
                    }
                    pointCoords.push_back(imageColorIds.at(pid).GetLocalX());
                    pointCoords.push_back(imageColorIds.at(pid).GetLocalY());
                    pointColors.push_back(pointCloud->GetPointColor(pid));
                }
                if (pointColors.empty())
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
                GPP::ErrorCode res = GPP::IntrinsicColor::TuneImageByPointColor(pointCoords, pointColors, 
                    imageWidth, imageHeight, imageData); 
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
                std::string tuneImageName = textureImageFiles.at(iid) + "_tune_point.jpg";
                cv::imwrite(tuneImageName, image);
                textureImageFiles.at(iid) = tuneImageName;
            }
            ModelManager::Get()->SetTextureImageFiles(textureImageFiles);
        }
    }

    void PointShopApp::SelectByRectangle()
    {
        mRightMouseType = SELECT_ADD;
    }

    void PointShopApp::EraseByRectangle()
    {
        mRightMouseType = SELECT_DELETE;
    }
 
    void PointShopApp::DeleteSelections()
    {
        GPP::PointCloud* pointCloud = ModelManager::Get()->GetPointCloud();
        if (pointCloud == NULL)
        {
            return;
        }
        std::vector<GPP::Int> deleteIndex;
        GPP::Int pointCount = pointCloud->GetPointCount();
        for (GPP::Int pid = 0; pid < pointCount; pid++)
        {
            if (mPointSelectFlag.at(pid))
            {
                deleteIndex.push_back(pid);
            }
        }
        MagicPointCloud magicPointCloud(pointCloud);
        SetupMagicPointCloud(magicPointCloud);
        GPP::ErrorCode res = GPP::DeletePointCloudElements(&magicPointCloud, deleteIndex);
        if (res != GPP_NO_ERROR)
        {
            MessageBox(NULL, "删点失败", "温馨提示", MB_OK);
            return;
        }
        ResetSelection();
        mUpdatePointCloudRendering = true;
        mpUI->SetPointCloudInfo(pointCloud->GetPointCount());
    }

    void PointShopApp::IgnoreBack(bool ignore)
    {
        mIgnoreBack = ignore;
    }

    void PointShopApp::MoveModel()
    {
        mRightMouseType = MOVE;
    }

    void PointShopApp::RemovePointCloudOutlier(bool isSubThread)
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        if (isSubThread)
        {
            mCommandType = OUTLIER;
            DoCommand(true);
        }
        else
        {
            GPP::PointCloud* pointCloud = ModelManager::Get()->GetPointCloud();
            GPP::Int pointCount = pointCloud->GetPointCount();
            if (pointCount < 1)
            {
                MessageBox(NULL, "点云的点个数小于1，操作失败", "温馨提示", MB_OK);
                return;
            }
            std::vector<GPP::Real> outlierValue;
            mIsCommandInProgress = true;
#if MAKEDUMPFILE
            GPP::DumpOnce();
#endif
            GPP::ErrorCode res = GPP::ConsolidatePointCloud::CalculateOutlier(pointCloud, &outlierValue);
            mIsCommandInProgress = false;
            if (res == GPP_API_IS_NOT_AVAILABLE)
            {
                MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
                MagicCore::ToolKit::Get()->SetAppRunning(false);
            }
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "点云去除飞点失败", "温馨提示", MB_OK);
                return;
            }
            /*GPP::Real cutValue = 0.8;
            for (GPP::Int pid = 0; pid < pointCount; pid++)
            {
                if (outlierValue.at(pid) < cutValue)
                {
                    pointCloud->SetPointColor(pid, GPP::Vector3(1, 0, 0));
                }
                else
                {
                    pointCloud->SetPointColor(pid, GPP::Vector3(0, 0, 1));
                }
            }
            pointCloud->SetHasColor(true);*/
            GPP::Real cutValue = 0.8;
            std::vector<GPP::Int> deleteIndex;
            for (GPP::Int pid = 0; pid < pointCount; pid++)
            {
                if (outlierValue.at(pid) > cutValue)
                {
                    deleteIndex.push_back(pid);
                }
            }
            MagicPointCloud magicPointCloud(pointCloud);
            SetupMagicPointCloud(magicPointCloud);
            res = GPP::DeletePointCloudElements(&magicPointCloud, deleteIndex);
            if (res != GPP_NO_ERROR)
            {
                return;
            }
            ResetSelection();
            mUpdatePointCloudRendering = true;
            mpUI->SetPointCloudInfo(pointCloud->GetPointCount());
        }
    }

    void PointShopApp::RemoveIsolatePart(double isolateValue, bool isSubThread)
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        GPP::PointCloud* pointCloud = ModelManager::Get()->GetPointCloud();
        if (pointCloud->HasNormal() == false)
        {
            MessageBox(NULL, "请先给点云计算法向量", "温馨提示", MB_OK);
            return;
        }
        if (isSubThread)
        {
            mCommandType = ISOLATE;
            mIsolateValue = isolateValue;
            DoCommand(true);
        }
        else
        {
            GPP::Int pointCount = pointCloud->GetPointCount();
            if (pointCount < 1)
            {
                MessageBox(NULL, "点云的点个数小于1，操作失败", "温馨提示", MB_OK);
                return;
            }
            std::vector<GPP::Real> isolation;
            mIsCommandInProgress = true;
#if MAKEDUMPFILE
            GPP::DumpOnce();
#endif
            GPP::ErrorCode res = GPP::ConsolidatePointCloud::CalculateIsolation(pointCloud, &isolation, 20, NULL);
            mIsCommandInProgress = false;
            if (res == GPP_API_IS_NOT_AVAILABLE)
            {
                MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
                MagicCore::ToolKit::Get()->SetAppRunning(false);
            }
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "点云去除孤立项失败", "温馨提示", MB_OK);
                return;
            }

            std::vector<GPP::Int> deleteIndex;
            for (GPP::Int pid = 0; pid < pointCount; pid++)
            {
                if (isolation[pid] < isolateValue)
                {
                    deleteIndex.push_back(pid);
                }
            }
            MagicPointCloud magicPointCloud(pointCloud);
            SetupMagicPointCloud(magicPointCloud);
            res = GPP::DeletePointCloudElements(&magicPointCloud, deleteIndex);
            if (res != GPP_NO_ERROR)
            {
                return;
            }
            ResetSelection();
            mUpdatePointCloudRendering = true;
            mpUI->SetPointCloudInfo(pointCloud->GetPointCount());
        }
    }

    static void SampleModelData(GPP::Int* sampleIndex, int sampleCount, int pointCount)
    {
        std::vector<GPP::ImageColorId>* imageColorIds = ModelManager::Get()->GetImageColorIdsPointer();
        if (imageColorIds && imageColorIds->size() == pointCount)
        {
            std::vector<GPP::ImageColorId> sampleImageColorIds(sampleCount);
            for (int sid = 0; sid < sampleCount; sid++)
            {
                sampleImageColorIds.at(sid) = imageColorIds->at(sampleIndex[sid]);
            }
            ModelManager::Get()->SetImageColorIds(sampleImageColorIds);
        }
        std::vector<int>* colorIds = ModelManager::Get()->GetColorIdsPointer();
        if (colorIds && colorIds->size() == pointCount)
        {
            std::vector<GPP::Int> sampleColorIds(sampleCount);
            for (int sid = 0; sid < sampleCount; sid++)
            {
                sampleColorIds.at(sid) = colorIds->at(sampleIndex[sid]);
            }
            ModelManager::Get()->SetColorIds(sampleColorIds);
        }
        std::vector<int>* cloudIds = ModelManager::Get()->GetCloudIdsPointer();
        if (cloudIds && cloudIds->size() == pointCount)
        {
            std::vector<GPP::Int> sampleCloudIds(sampleCount);
            for (int sid = 0; sid < sampleCount; sid++)
            {
                sampleCloudIds.at(sid) = cloudIds->at(sampleIndex[sid]);
            }
            ModelManager::Get()->SetCloudIds(sampleCloudIds);
        }
    }

    void PointShopApp::UniformSamplePointCloud(int targetPointCount)
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        GPP::PointCloud* pointCloud = ModelManager::Get()->GetPointCloud();
        GPP::Int originPointCount = pointCloud->GetPointCount();
        GPP::Int* sampleIndex = new GPP::Int[targetPointCount];
#if MAKEDUMPFILE
        GPP::DumpOnce();
#endif
        GPP::ErrorCode res = GPP::SamplePointCloud::UniformSample(pointCloud, targetPointCount, sampleIndex, 0);
        if (res == GPP_API_IS_NOT_AVAILABLE)
        {
            MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
            MagicCore::ToolKit::Get()->SetAppRunning(false);
        }
        if (res != GPP_NO_ERROR)
        {
            MessageBox(NULL, "点云采样失败", "温馨提示", MB_OK);
            GPPFREEARRAY(sampleIndex);
            return;
        }

        bool hasNormal = pointCloud->HasNormal();
        bool hasColor = pointCloud->HasColor();
        GPP::PointCloud* samplePointCloud = new GPP::PointCloud(hasNormal, hasColor);
        if (hasNormal)
        {
            for (GPP::Int sid = 0; sid < targetPointCount; sid++)
            {
                samplePointCloud->InsertPoint(pointCloud->GetPointCoord(sampleIndex[sid]), 
                    pointCloud->GetPointNormal(sampleIndex[sid]));
                if (hasColor)
                {
                    samplePointCloud->SetPointColor(sid, pointCloud->GetPointColor(sampleIndex[sid]));
                }
            }
        }
        else
        {
            for (GPP::Int sid = 0; sid < targetPointCount; sid++)
            {
                samplePointCloud->InsertPoint(pointCloud->GetPointCoord(sampleIndex[sid]));
                if (hasColor)
                {
                    samplePointCloud->SetPointColor(sid, pointCloud->GetPointColor(sampleIndex[sid]));
                }
            }
        }
        SampleModelData(sampleIndex, targetPointCount, originPointCount);
        ModelManager::Get()->SetPointCloud(samplePointCloud);
        ResetSelection();
        mUpdatePointCloudRendering = true;
        GPPFREEARRAY(sampleIndex);
        mpUI->SetPointCloudInfo(ModelManager::Get()->GetPointCloud()->GetPointCount());
    }

    void PointShopApp::GeometrySamplePointCloud(int targetPointCount)
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        GPP::PointCloud* pointCloud = ModelManager::Get()->GetPointCloud();
        GPP::Int originPointCount = pointCloud->GetPointCount();
        if (pointCloud->HasNormal() == false)
        {
            MessageBox(NULL, "点云需要先计算法线", "温馨提示", MB_OK);
            return;
        }
        GPP::Int* sampleIndex = new GPP::Int[targetPointCount];
#if MAKEDUMPFILE
        GPP::DumpOnce();
#endif
        GPP::ErrorCode res = GPP::SamplePointCloud::GeometrySample(pointCloud, targetPointCount, sampleIndex, 0.3, 9, 0, 
            GPP::SAMPLE_QUALITY_HIGH);
        if (res == GPP_API_IS_NOT_AVAILABLE)
        {
            MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
            MagicCore::ToolKit::Get()->SetAppRunning(false);
        }
        if (res != GPP_NO_ERROR)
        {
            MessageBox(NULL, "点云采样失败", "温馨提示", MB_OK);
            GPPFREEARRAY(sampleIndex);
            return;
        }

        //int pointCount = mpPointCloud->GetPointCount();
        //for (GPP::Int pid = 0; pid < pointCount; pid++)
        //{
        //    if (flatness.at(pid) > 0.01)
        //    {
        //        mpPointCloud->SetPointColor(pid, MagicCore::ToolKit::ColorCoding(0.2 + 0.8));
        //    }
        //    else
        //    {
        //        mpPointCloud->SetPointColor(pid, MagicCore::ToolKit::ColorCoding(0.2 + 0.1));
        //    }
        //    //mpPointCloud->SetPointColor(pid, MagicCore::ToolKit::ColorCoding(0.2 + flatness.at(pid) * 30));
        //}
        //mpPointCloud->SetHasColor(true);

        bool hasNormal = pointCloud->HasNormal();
        bool hasColor = pointCloud->HasColor();
        GPP::PointCloud* samplePointCloud = new GPP::PointCloud(hasNormal, hasColor);
        if (hasNormal)
        {
            for (GPP::Int sid = 0; sid < targetPointCount; sid++)
            {
                samplePointCloud->InsertPoint(pointCloud->GetPointCoord(sampleIndex[sid]), 
                    pointCloud->GetPointNormal(sampleIndex[sid]));
                if (hasColor)
                {
                    samplePointCloud->SetPointColor(sid, pointCloud->GetPointColor(sampleIndex[sid]));
                }
            }
        }
        else
        {
            for (GPP::Int sid = 0; sid < targetPointCount; sid++)
            {
                samplePointCloud->InsertPoint(pointCloud->GetPointCoord(sampleIndex[sid]));
                if (hasColor)
                {
                    samplePointCloud->SetPointColor(sid, pointCloud->GetPointColor(sampleIndex[sid]));
                }
            }
        }
        SampleModelData(sampleIndex, targetPointCount, originPointCount);
        ModelManager::Get()->SetPointCloud(samplePointCloud);
        ResetSelection();

        /*for (int sid = 0; sid < targetPointCount; sid++)
        {
            mpPointCloud->SetPointColor(sampleIndex[sid], GPP::Vector3(1, 0, 0));
        }
        mpPointCloud->SetHasColor(true);*/
        mUpdatePointCloudRendering = true;
        GPPFREEARRAY(sampleIndex);
        mpUI->SetPointCloudInfo(ModelManager::Get()->GetPointCloud()->GetPointCount());
    }

    void PointShopApp::SimplifyPointCloud(int resolution)
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        if (resolution > 10000 || resolution < 1)
        {
            MessageBox(NULL, "采样分辨率区间为[1, 10000]", "温馨提示", MB_OK);
            return;
        }
        GPP::PointCloud* pointCloud = ModelManager::Get()->GetPointCloud();
        GPP::PointCloud* simplifiedCloud = new GPP::PointCloud;
        if (pointCloud->HasColor())
        {
            std::vector<GPP::Real> fields;
            GPP::Int pointCount = pointCloud->GetPointCount();
            fields.reserve(pointCount * 3);
            for (GPP::Int pid = 0; pid < pointCount; pid++)
            {
                GPP::Vector3 color = pointCloud->GetPointColor(pid);
                fields.push_back(color[0]);
                fields.push_back(color[1]);
                fields.push_back(color[2]);
            }
            std::vector<GPP::Real> simplifiedFields;
#if MAKEDUMPFILE
            GPP::DumpOnce();
#endif
            GPP::ErrorCode res = GPP::SamplePointCloud::Simplify(pointCloud, resolution, simplifiedCloud, &fields, &simplifiedFields);
            if (res == GPP_API_IS_NOT_AVAILABLE)
            {
                MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
                MagicCore::ToolKit::Get()->SetAppRunning(false);
            }
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "点云简化失败", "温馨提示", MB_OK);
                GPPFREEPOINTER(simplifiedCloud);
                return;
            }
            simplifiedCloud->SetHasColor(true);
            GPP::Int simplifiedCount = simplifiedCloud->GetPointCount();
            for (GPP::Int pid = 0; pid < simplifiedCount; pid++)
            {
                simplifiedCloud->SetPointColor(pid, GPP::Vector3(simplifiedFields.at(pid * 3), 
                    simplifiedFields.at(pid * 3 + 1), simplifiedFields.at(pid * 3 + 2)));
            }
        }
        else
        {
            GPP::ErrorCode res = GPP::SamplePointCloud::Simplify(pointCloud, resolution, simplifiedCloud, NULL, NULL);
            if (res == GPP_API_IS_NOT_AVAILABLE)
            {
                MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
                MagicCore::ToolKit::Get()->SetAppRunning(false);
            }
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "点云简化失败", "温馨提示", MB_OK);
                GPPFREEPOINTER(simplifiedCloud);
                return;
            }
        }
        ModelManager::Get()->SetPointCloud(simplifiedCloud);
        ResetSelection();
        UpdatePickTool();
        UpdatePointCloudRendering();
        mpUI->SetPointCloudInfo(ModelManager::Get()->GetPointCloud()->GetPointCount());
    }

    void PointShopApp::CalculatePointCloudNormal(bool isDepthImage, int neighborCount, bool isSubThread)
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        if (isSubThread)
        {
            mCommandType = NORMALCALCULATION;
            mIsDepthImage = isDepthImage;
            mNeighborCount = neighborCount;
            DoCommand(true);
        }
        else
        {
            GPP::PointCloud* pointCloud = ModelManager::Get()->GetPointCloud();
            mIsCommandInProgress = true;
#if MAKEDUMPFILE
            GPP::DumpOnce();
#endif
            GPP::ErrorCode res = GPP::ConsolidatePointCloud::CalculatePointCloudNormal(pointCloud, isDepthImage, neighborCount);
            mIsCommandInProgress = false;
            mUpdatePointCloudRendering = true;
            if (res == GPP_API_IS_NOT_AVAILABLE)
            {
                MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
                MagicCore::ToolKit::Get()->SetAppRunning(false);
            }
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "点云法线计算失败", "温馨提示", MB_OK);
                return;
            }
        }
    }

    void PointShopApp::FlipPointCloudNormal()
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        GPP::PointCloud* pointCloud = ModelManager::Get()->GetPointCloud();
        if (pointCloud->HasNormal() == false)
        {
            MessageBox(NULL, "请先给点云计算法向量", "温馨提示", MB_OK);
            return;
        }
        GPP::Int pointCount = pointCloud->GetPointCount();
        for (GPP::Int pid = 0; pid < pointCount; pid++)
        {
            pointCloud->SetPointNormal(pid, pointCloud->GetPointNormal(pid) * -1.0);
        }
        UpdatePointCloudRendering();
    }

    void PointShopApp::ReversePatchNormal(int neighborCount)
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        if (ModelManager::Get()->GetPointCloud()->HasNormal() == false)
        {
            MessageBox(NULL, "请先给点云计算法向量", "温馨提示", MB_OK);
            return;
        }
        if (mRightMouseType == REVERSE_NORMAL)
        {
            mRightMouseType = MOVE;
        }
        else
        {
            mRightMouseType = REVERSE_NORMAL;
            mNeighborCount = neighborCount;
        }
        if (mRightMouseType == REVERSE_NORMAL)
        {
            MessageBox(NULL, "鼠标右键进入点云反转法线模式，再次点击本按钮返回平移模式", "温馨提示", MB_OK);
        }
    }

    void PointShopApp::ReconstructMesh(bool needFillHole,int quality, bool isSubThread)
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        GPP::PointCloud* pointCloud = ModelManager::Get()->GetPointCloud();
        if (pointCloud->HasNormal() == false)
        {
            MessageBox(NULL, "请先给点云计算法向量", "温馨提示", MB_OK);
            return;
        }
        else if (quality < 0 || quality > 6)
        {
            MessageBox(NULL, "三角化质量参数范围[0, 6]，数字越大，质量越好，速度越慢", "温馨提示", MB_OK);
            return;
        }
        if (isSubThread)
        {
            mCommandType = RECONSTRUCTION;
            mReconstructionQuality = quality;
            mNeedFillHole = needFillHole;
            DoCommand(true);
        }
        else
        {
            double maxHoleAreaRatio = 0.1;
            if (pointCloud->HasColor())
            {
                GPP::Int pointCount = pointCloud->GetPointCount();
                std::vector<GPP::Real> pointColorFields(pointCount * 3);
                for (GPP::Int pid = 0; pid < pointCount; pid++)
                {
                    GPP::Vector3 color = pointCloud->GetPointColor(pid);
                    GPP::Int baseId = pid * 3;
                    pointColorFields.at(baseId) = color[0];
                    pointColorFields.at(baseId + 1) = color[1];
                    pointColorFields.at(baseId + 2) = color[2];
                }
                std::vector<GPP::Real> vertexColorField;
                GPP::TriMesh* triMesh = new GPP::TriMesh;
                mIsCommandInProgress = true;
#if MAKEDUMPFILE
                GPP::DumpOnce();
#endif
                GPP::ErrorCode res = GPP::ReconstructMesh::Reconstruct(pointCloud, triMesh, quality, needFillHole, 
                    &pointColorFields, &vertexColorField, maxHoleAreaRatio);
                mIsCommandInProgress = false;
                if (res == GPP_API_IS_NOT_AVAILABLE)
                {
                    MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
                    MagicCore::ToolKit::Get()->SetAppRunning(false);
                }
                if (res != GPP_NO_ERROR)
                {
                    MessageBox(NULL, "点云三角化失败", "温馨提示", MB_OK);
                    GPPFREEPOINTER(triMesh);
                    return;
                }
                if (!triMesh)
                {
                    return;
                }
                GPP::Int vertexCount = triMesh->GetVertexCount();
                triMesh->SetHasVertexColor(true);
                for (GPP::Int vid = 0; vid < vertexCount; vid++)
                {
                    GPP::Int baseId = vid * 3;
                    triMesh->SetVertexColor(vid, GPP::Vector3(vertexColorField.at(baseId), vertexColorField.at(baseId + 1), vertexColorField.at(baseId + 2)));
                }
                ConstructImageColorIdForMesh(triMesh, pointCloud);
                //mpTriMesh = triMesh;
                ModelManager::Get()->SetMesh(triMesh);
                ModelManager::Get()->ClearPointCloud();
            }
            else
            {
                GPP::TriMesh* triMesh = new GPP::TriMesh;
                mIsCommandInProgress = true;
#if MAKEDUMPFILE
                GPP::DumpOnce();
#endif
                GPP::ErrorCode res = GPP::ReconstructMesh::Reconstruct(pointCloud, triMesh, quality, needFillHole, 
                    NULL, NULL, maxHoleAreaRatio);
                mIsCommandInProgress = false;
                if (res == GPP_API_IS_NOT_AVAILABLE)
                {
                    MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
                    MagicCore::ToolKit::Get()->SetAppRunning(false);
                }
                if (res != GPP_NO_ERROR)
                {
                    MessageBox(NULL, "点云三角化失败", "温馨提示", MB_OK);
                    GPPFREEPOINTER(triMesh);
                    return;
                }
                if (!triMesh)
                {
                    return;
                }
                ConstructImageColorIdForMesh(triMesh, pointCloud);
                ModelManager::Get()->SetMesh(triMesh);
                ModelManager::Get()->ClearPointCloud();
            }
            mEnterMeshShop = true;
        }
    }

    int PointShopApp::GetPointCount()
    {
        if (ModelManager::Get()->GetPointCloud() != NULL)
        {
            return ModelManager::Get()->GetPointCloud()->GetPointCount();
        }
        else
        {
            return 0;
        }
    }

#if DEBUGDUMPFILE
    void PointShopApp::SetDumpInfo(GPP::DumpBase* dumpInfo)
    {
    }

    void PointShopApp::RunDumpInfo()
    {
    }
#endif

    bool PointShopApp::IsCommandInProgress(void)
    {
        return mIsCommandInProgress;
    }

    void PointShopApp::UpdatePointCloudRendering()
    {
        //InfoLog << " UpdatePointCloudRendering" << std::endl;
        GPP::PointCloud* pointCloud = ModelManager::Get()->GetPointCloud();
        if (pointCloud == NULL)
        {
            MagicCore::RenderSystem::Get()->HideRenderingObject("PointCloud_PointShop");
            return;
        }
        GPP::Vector3 selectColor(1, 0, 0);
        if (pointCloud->HasNormal())
        {
            MagicCore::RenderSystem::Get()->RenderPointCloud("PointCloud_PointShop", "CookTorrancePoint", pointCloud, 
                MagicCore::RenderSystem::MODEL_NODE_CENTER, &mPointSelectFlag, &selectColor);
        }
        else
        {
            InfoLog << " no color " << std::endl;
            MagicCore::RenderSystem::Get()->RenderPointCloud("PointCloud_PointShop", "SimplePoint", pointCloud, 
                MagicCore::RenderSystem::MODEL_NODE_CENTER, &mPointSelectFlag, &selectColor);
        }
        //InfoLog << " done" << std::endl;
    }

    bool PointShopApp::IsCommandAvaliable()
    {
        if (ModelManager::Get()->GetPointCloud() == NULL)
        {
            MessageBox(NULL, "请先导入点云", "温馨提示", MB_OK);
            return false;
        }
        if (mIsCommandInProgress)
        {
            MessageBox(NULL, "请等待当前命令执行完", "温馨提示", MB_OK);
            return false;
        }
        return true;
    }

    void PointShopApp::InitViewTool()
    {
        if (mpViewTool == NULL)
        {
            mpViewTool = new MagicCore::ViewTool;
        }
    }

    void PointShopApp::UpdatePickTool()
    {
        GPPFREEPOINTER(mpPickTool);
        if (ModelManager::Get()->GetPointCloud())
        {
            mpPickTool = new MagicCore::PickTool;
            mpPickTool->SetPickParameter(MagicCore::PM_POINT, true, ModelManager::Get()->GetPointCloud(), NULL, "ModelNode");
        }
    }
}

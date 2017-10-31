#include "stdafx.h"
#include <process.h>
#include "MeasureApp.h"
#include "MeasureAppUI.h"
#include "AppManager.h"
#include "ModelManager.h"
#include "../Common/LogSystem.h"
#include "../Common/ToolKit.h"
#include "../Common/ViewTool.h"
#include "../Common/PickTool.h"
#include "../Common/RenderSystem.h"
#if DEBUGDUMPFILE
#include "DumpMeasureMesh.h"
#include "DumpSplitMesh.h"
#include "DumpOptimiseCurve.h"
#endif
#include <numeric>

namespace MagicApp
{
    static unsigned __stdcall RunThread(void *arg)
    {
        MeasureApp* app = (MeasureApp*)arg;
        if (app == NULL)
        {
            return 0;
        }
        app->DoCommand(false);
        return 1;
    }

    MeasureApp::MeasureApp() :
        mpUI(NULL),
        mpViewTool(NULL),
        mpPickTool(NULL),
        mDisplayMode(0),
        mRightMouseType(SELECT_VERTEX),
#if DEBUGDUMPFILE
        mpDumpInfo(NULL),
#endif
        mMarkIds(),
        mGeodesicsOnVertices(),
        mMarkPoints(),
        mCommandType(NONE),
        mIsCommandInProgress(false),
        mUpdateModelRendering(false),
        mUpdateMarkRendering(false),
        mGeodesicAccuracy(0.5),
        mIsFlatRenderingMode(true),
        mpRefTriMesh(NULL),
        mUpdateRefModelRendering(false),
        mMinCurvature(),
        mMaxCurvature(),
        mMinCurvatureDirs(),
        mMaxCurvatureDirs(),
        mCurvatureFlags(),
        mDisplayPrincipalCurvature(0),
        mCurvatureWeight(0),
        mIsGeodesicsClose(false)
    {
        mDetectOptions[0] = true;
        mDetectOptions[1] = true;
        mDetectOptions[2] = true;
        mDetectOptions[3] = true;
    }

    MeasureApp::~MeasureApp()
    {
        GPPFREEPOINTER(mpUI);        
        GPPFREEPOINTER(mpViewTool);
        GPPFREEPOINTER(mpPickTool);
        GPPFREEPOINTER(mpRefTriMesh);
#if DEBUGDUMPFILE
        GPPFREEPOINTER(mpDumpInfo);
#endif
    }

    bool MeasureApp::Enter()
    {
        InfoLog << "Enter MeasureApp" << std::endl; 
        if (mpUI == NULL)
        {
            mpUI = new MeasureAppUI;
        }
        mpUI->Setup();
        SetupScene();
        UpdateModelRendering();
        return true;
    }

    bool MeasureApp::Update(double timeElapsed)
    {
        if (mpUI && mpUI->IsProgressbarVisible())
        {
            int progressValue = int(GPP::GetApiProgress() * 100.0);
            mpUI->SetProgressbar(progressValue);
        }
        if (mUpdateMarkRendering)
        {
            UpdateMarkRendering();
            mUpdateMarkRendering = false;
        }
        if (mUpdateModelRendering)
        {
            UpdateModelRendering();
            mUpdateModelRendering = false;
        }
        if (mUpdateRefModelRendering)
        {
            UpdateRefModelRendering();
            mUpdateRefModelRendering = false;
        }
        return true;
    }

    bool MeasureApp::Exit()
    {
        InfoLog << "Exit MeasureApp" << std::endl; 
        ShutdownScene();
        if (mpUI != NULL)
        {
            mpUI->Shutdown();
        }
        ClearData();
        return true;
    }

    bool MeasureApp::MouseMoved( const OIS::MouseEvent &arg )
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

    bool MeasureApp::MousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
    {
        if ((arg.state.buttonDown(OIS::MB_Middle) || arg.state.buttonDown(OIS::MB_Left)) && mpViewTool != NULL)
        {
            mpViewTool->MousePressed(arg.state.X.abs, arg.state.Y.abs);
        }
        else if (arg.state.buttonDown(OIS::MB_Right) && mIsCommandInProgress == false)
        {
            if (mpPickTool)
            {
                mpPickTool->MousePressed(arg.state.X.abs, arg.state.Y.abs);
            }
        }
        return true;
    }

    bool MeasureApp::MouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
    {
        if (mpViewTool)
        {
            mpViewTool->MouseReleased();
        }
        if (mIsCommandInProgress == false && id == OIS::MB_Right)
        {
            if (mpPickTool)
            {
                mpPickTool->MouseReleased(arg.state.X.abs, arg.state.Y.abs);
                GPP::Int pickedId = -1;
                pickedId = mpPickTool->GetPickVertexId();
                if (pickedId != -1)
                {
                    mMarkIds.push_back(pickedId);
                    UpdateMarkRendering();
                }
                else
                {
                    GPP::PointOnFace pof = mpPickTool->GetPickPointOnFace();
                    if (mRightMouseType == SELECT_PRIMITIVE)
                    {
                        SelectPrimitive(pof.mFaceId);
                    }
                    else
                    {
                        if (pof.mFaceId != -1)
                        {
                            mMarkFacePoints.push_back(pof);
                            UpdateMarkRendering();
                        }
                    }
                }
                mpPickTool->ClearPickedIds();
            }
        }
        return true;
    }

    void MeasureApp::SelectPrimitive(int faceId)
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
        if (triMesh == NULL)
        {
            MessageBox(NULL, "请导入需要测量的网格", "温馨提示", MB_OK);
            return;
        }
        if (faceId < 0)
        {
            MessageBox(NULL, "invalid faceId", "温馨提示", MB_OK);
            return;
        }
        std::vector<GPP::DetectShape::PrimitiveType> typeOptions;
        if (mDetectOptions[0])
        {
            typeOptions.push_back(GPP::DetectShape::PRIMITIVE_TYPE_PLANE);
        }
        if (mDetectOptions[1])
        {
            typeOptions.push_back(GPP::DetectShape::PRIMITIVE_TYPE_CONE);
        }
        if (mDetectOptions[2])
        {
            typeOptions.push_back(GPP::DetectShape::PRIMITIVE_TYPE_SPHERE);
        }
        if (mDetectOptions[3])
        {
            typeOptions.push_back(GPP::DetectShape::PRIMITIVE_TYPE_CYLINDER);
        }
        GPP::DetectShape::PrimitiveType typeRes;
        std::vector<int> indexRes;
        //GPP::DumpOnce();
        GPP::ErrorCode res = GPP::DetectShape::_SelectPrimitive(triMesh, *(ModelManager::Get()->GetMeshInfo()), faceId, &typeOptions, &typeRes, &indexRes);
        /*GPP::ErrorCode res = GPP::DetectShape::_SelectTangentFace(triMesh, *(ModelManager::Get()->GetMeshInfo()), faceId, GPP::ONE_RADIAN * 20, &indexRes);
        typeRes = GPP::DetectShape::PRIMITIVE_TYPE_PLANE;*/
        if (res != GPP_NO_ERROR)
        {
            MessageBox(NULL, "SelectPrimitive Failed", "温馨提示", MB_OK);
            return;
        }
        triMesh->SetHasTriangleColor(true);
        int faceCount = triMesh->GetTriangleCount();
        GPP::Vector3 defaultColor(0.95, 0.95, 0.95);
        for (int fid = 0; fid < faceCount; fid++)
        {
            triMesh->SetTriangleColor(fid, 0, defaultColor);
            triMesh->SetTriangleColor(fid, 1, defaultColor);
            triMesh->SetTriangleColor(fid, 2, defaultColor);
        }
        double deltaColor = 0.2;
        GPP::Vector3 pritimiveColor = MagicCore::ToolKit::Get()->ColorCoding(0.01 + typeRes * deltaColor);
        for (std::vector<int>::const_iterator itr = indexRes.begin(); itr != indexRes.end(); ++itr)
        {
            triMesh->SetTriangleColor(*itr, 0, pritimiveColor);
            triMesh->SetTriangleColor(*itr, 1, pritimiveColor);
            triMesh->SetTriangleColor(*itr, 2, pritimiveColor);
        }
        /*GPP::Vector3 selectColor(0, 0, 0);
        triMesh->SetTriangleColor(faceId, 0, selectColor);
        triMesh->SetTriangleColor(faceId, 1, selectColor);
        triMesh->SetTriangleColor(faceId, 2, selectColor);*/

        mUpdateModelRendering = true;
    }

    void MeasureApp::DetectPrimitive(bool isSubThread)
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
        if (triMesh == NULL)
        {
            MessageBox(NULL, "请导入需要测量的网格", "温馨提示", MB_OK);
            return;
        }
        if (isSubThread)
        {
            mCommandType = DETECT_PRIMITIVE;
            DoCommand(true);
        }
        else
        {
            std::vector<GPP::DetectShape::PrimitiveType> typeOptions;
            if (mDetectOptions[0])
            {
                typeOptions.push_back(GPP::DetectShape::PRIMITIVE_TYPE_PLANE);
            }
            if (mDetectOptions[1])
            {
                typeOptions.push_back(GPP::DetectShape::PRIMITIVE_TYPE_CONE);
            }
            if (mDetectOptions[2])
            {
                typeOptions.push_back(GPP::DetectShape::PRIMITIVE_TYPE_SPHERE);
            }
            if (mDetectOptions[3])
            {
                typeOptions.push_back(GPP::DetectShape::PRIMITIVE_TYPE_CYLINDER);
            }
            std::vector<GPP::DetectShape::PrimitiveType> types;
    #if MAKEDUMPFILE
            GPP::DumpOnce();
    #endif
            GPP::ErrorCode res = GPP::DetectShape::DetectPrimitives(triMesh, &typeOptions, &types, NULL);
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "DetectPrimitives failed", "温馨提示", MB_OK);
                return;
            }
            triMesh->SetHasTriangleColor(true);
            int faceCount = triMesh->GetTriangleCount();
            double deltaColor = 0.2;
            for (int fid = 0; fid < faceCount; fid++)
            {
                GPP::Vector3 faceColor = MagicCore::ToolKit::Get()->ColorCoding(0.01 + types.at(fid) * deltaColor);
                triMesh->SetTriangleColor(fid, 0, faceColor);
                triMesh->SetTriangleColor(fid, 1, faceColor);
                triMesh->SetTriangleColor(fid, 2, faceColor);
            }
            mUpdateModelRendering = true;
        }
    }

    static bool ChamferCurve(GPP::TriMesh* triMesh, const std::vector<int>& centerCurve, const std::vector<int>& topCurve,
        const std::vector<int>& downCurve, bool isCurveClose)
    {
        int originVertexCount = triMesh->GetVertexCount();
        std::vector<std::set<int> > vertexNeighbors(originVertexCount, std::set<int>());
        int vertexIds[3] = {-1};
        int originFaceCount = triMesh->GetTriangleCount();
        for (int fid = 0; fid < originFaceCount; fid++)
        {
            triMesh->GetTriangleVertexIds(fid, vertexIds);
            vertexNeighbors.at(vertexIds[0]).insert(vertexIds[1]);
            vertexNeighbors.at(vertexIds[0]).insert(vertexIds[2]);
            vertexNeighbors.at(vertexIds[1]).insert(vertexIds[2]);
            vertexNeighbors.at(vertexIds[1]).insert(vertexIds[0]);
            vertexNeighbors.at(vertexIds[2]).insert(vertexIds[0]);
            vertexNeighbors.at(vertexIds[2]).insert(vertexIds[1]);
        }
        std::vector<int> deleteTriangles;
        if (isCurveClose)
        {
            std::vector<bool> vertexMark(originVertexCount, 0);
            for (std::vector<int>::const_iterator itr = topCurve.begin(); itr != topCurve.end(); ++itr)
            {
                vertexMark.at(*itr) = 1;
            }
            for (std::vector<int>::const_iterator itr = downCurve.begin(); itr != downCurve.end(); ++itr)
            {
                vertexMark.at(*itr) = 1;
            }
            for (std::vector<int>::const_iterator itr = centerCurve.begin(); itr != centerCurve.end(); ++itr)
            {
                vertexMark.at(*itr) = 1;
            }
            std::vector<int> vertexStack = centerCurve;
            while (vertexStack.size() > 0)
            {
                std::vector<int> vertexStackNext;
                for (std::vector<int>::iterator stackItr = vertexStack.begin(); stackItr != vertexStack.end(); ++stackItr)
                {
                    for (std::set<int>::iterator nitr = vertexNeighbors.at(*stackItr).begin(); nitr != vertexNeighbors.at(*stackItr).end(); ++nitr)
                    {
                        if (vertexMark.at(*nitr))
                        {
                            continue;
                        }
                        vertexMark.at(*nitr) = 1;
                        vertexStackNext.push_back(*nitr);
                    }
                }
                vertexStack.swap(vertexStackNext);
            }
            for (int fid = 0; fid < originFaceCount; fid++)
            {
                triMesh->GetTriangleVertexIds(fid, vertexIds);
                if (vertexMark.at(vertexIds[0]) && vertexMark.at(vertexIds[1]) && vertexMark.at(vertexIds[2]))
                {
                    deleteTriangles.push_back(fid);
                }
            }
            GPP::ErrorCode res = GPP::DeleteTriMeshTriangles(triMesh, deleteTriangles, false);
            if (res != GPP_NO_ERROR)
            {
                return false;
            }
            int stringSize = topCurve.size();
            for (int vid = 0; vid < stringSize; vid++)
            {
                triMesh->InsertTriangle(topCurve.at(vid), topCurve.at((vid - 1 + stringSize) % stringSize), downCurve.at(vid));
                triMesh->InsertTriangle(downCurve.at(vid), topCurve.at((vid - 1 + stringSize) % stringSize), downCurve.at((vid - 1 + stringSize) % stringSize));
            }
            res = GPP::DeleteIsolatedVertices(triMesh);
            if (res != GPP_NO_ERROR)
            {
                return false;
            }
            triMesh->UpdateNormal();
        }
        else
        {
            int stringSize = topCurve.size();
            std::vector<int> sectionVertexIds;
            sectionVertexIds.push_back(topCurve.at(0));
            sectionVertexIds.push_back(downCurve.at(0));
            std::vector<int> leftCurves;
            GPP::Real distance;
            GPP::ErrorCode res = GPP::MeasureMesh::ComputeApproximateGeodesics(triMesh, sectionVertexIds, false, leftCurves, distance);
            if (res != GPP_NO_ERROR)
            {
                return false;
            }
            sectionVertexIds.clear();
            sectionVertexIds.push_back(topCurve.at(stringSize - 1));
            sectionVertexIds.push_back(downCurve.at(stringSize - 1));
            std::vector<int> rightCurves;
            res = GPP::MeasureMesh::ComputeApproximateGeodesics(triMesh, sectionVertexIds, false, rightCurves, distance);
            if (res != GPP_NO_ERROR)
            {
                return false;
            }

            std::vector<bool> vertexMark(originVertexCount, 0);
            for (std::vector<int>::const_iterator itr = topCurve.begin(); itr != topCurve.end(); ++itr)
            {
                vertexMark.at(*itr) = 1;
            }
            for (std::vector<int>::const_iterator itr = downCurve.begin(); itr != downCurve.end(); ++itr)
            {
                vertexMark.at(*itr) = 1;
            }
            for (std::vector<int>::const_iterator itr = leftCurves.begin(); itr != leftCurves.end(); ++itr)
            {
                vertexMark.at(*itr) = 1;
            }
            for (std::vector<int>::const_iterator itr = rightCurves.begin(); itr != rightCurves.end(); ++itr)
            {
                vertexMark.at(*itr) = 1;
            }
            int seedVertex = centerCurve.at(centerCurve.size() / 2);
            std::vector<int> vertexStack;
            vertexStack.push_back(seedVertex);
            vertexMark.at(seedVertex) = 1;
            while (vertexStack.size() > 0)
            {
                std::vector<int> vertexStackNext;
                for (std::vector<int>::iterator stackItr = vertexStack.begin(); stackItr != vertexStack.end(); ++stackItr)
                {
                    for (std::set<int>::iterator nitr = vertexNeighbors.at(*stackItr).begin(); nitr != vertexNeighbors.at(*stackItr).end(); ++nitr)
                    {
                        if (vertexMark.at(*nitr))
                        {
                            continue;
                        }
                        vertexMark.at(*nitr) = 1;
                        vertexStackNext.push_back(*nitr);
                    }
                }
                vertexStack.swap(vertexStackNext);
            }
            for (int fid = 0; fid < originFaceCount; fid++)
            {
                triMesh->GetTriangleVertexIds(fid, vertexIds);
                if (vertexMark.at(vertexIds[0]) && vertexMark.at(vertexIds[1]) && vertexMark.at(vertexIds[2]))
                {
                    deleteTriangles.push_back(fid);
                }
            }
            res = GPP::DeleteTriMeshTriangles(triMesh, deleteTriangles, false);
            if (res != GPP_NO_ERROR)
            {
                return false;
            }
            for (int vid = 1; vid < stringSize; vid++)
            {
                triMesh->InsertTriangle(topCurve.at(vid), topCurve.at((vid - 1 + stringSize) % stringSize), downCurve.at(vid));
                triMesh->InsertTriangle(downCurve.at(vid), topCurve.at((vid - 1 + stringSize) % stringSize), downCurve.at((vid - 1 + stringSize) % stringSize));
            }
            std::vector<int> boundarySeedIds;
            boundarySeedIds.push_back(topCurve.at(0));
            boundarySeedIds.push_back(topCurve.at(stringSize - 1));
            res = GPP::FillMeshHole::FillHoles(triMesh, &boundarySeedIds);
            if (res != GPP_NO_ERROR)
            {
                return false;
            }
            res = GPP::DeleteIsolatedVertices(triMesh);
            if (res != GPP_NO_ERROR)
            {
                return false;
            }
            triMesh->UpdateNormal();
        }
        return true;
    }

    static void ComputeExtendVector(const GPP::Vector3& point, const GPP::Vector3& normal, const GPP::Vector3& opPoint,
        GPP::Vector3& extendVec)
    {
        GPP::Vector3 opVec = opPoint - point;
        double opLength = fabs(opVec * normal);
        GPP::Vector3 pointVec = normal * opLength;
        if (pointVec * opVec > 0)
        {
            pointVec *= (-1.0);
        }
        extendVec = pointVec + opVec;
    }

    static bool ComputeControlPoint(const GPP::Vector3& point, const GPP::Vector3& normal, const GPP::Vector3& opPoint,
        GPP::Vector3& controlPoint)
    {
        GPP::Vector3 opVec = opPoint - point;
        double opLength = fabs(opVec * normal);
        GPP::Vector3 pointVec = normal * opLength;
        if (pointVec * opVec > 0)
        {
            pointVec *= (-1.0);
        }
        GPP::Vector3 extendVec = pointVec + opVec;
        controlPoint = point + extendVec * 0.5;
        return true;
    }

    static void SmoothNormal(std::vector<GPP::Vector3>& vectorList, bool isClose, int maxIteration, double weight)
    {
        int vectorSize = vectorList.size();
        std::vector<GPP::Vector3> smoothVectorList(vectorSize);
        int startIndex = 0;
        int endIndex = vectorSize;
        if (!isClose)
        {
            startIndex = 1;
            endIndex = vectorSize - 1;
        }
        for (int itrIndex = 0; itrIndex < maxIteration; itrIndex++)
        {
            for (int vid = startIndex; vid < endIndex; vid++)
            {
                smoothVectorList.at(vid) = vectorList.at(vid) * (1.0 - weight) + 
                    (vectorList.at((vid + 1) % vectorSize) + vectorList.at((vid - 1 + vectorSize) % vectorSize)) * 0.5 * weight;
                smoothVectorList.at(vid).Normalise();
            }
            if (!isClose)
            {
                smoothVectorList.at(0) = vectorList.at(0);
                smoothVectorList.at(vectorSize - 1) = vectorList.at(vectorSize - 1);
            }
            vectorList.swap(smoothVectorList);
        }
    }

    static void SmoothExtendVec(std::vector<GPP::Vector3>& extendVecList, bool isClose, int maxIteration, double weight)
    {
        int vectorSize = extendVecList.size();
        int startIndex = 0;
        int endIndex = vectorSize;
        if (!isClose)
        {
            startIndex = 1;
            endIndex = vectorSize - 1;
        }
        std::vector<double> vecLengthList(vectorSize);
        if (!isClose)
        {
            vecLengthList.at(0) = extendVecList.at(0).Length();
            vecLengthList.at(vectorSize - 1) = extendVecList.at(vectorSize - 1).Length();
        }
        for (int vid = startIndex; vid < endIndex; vid++)
        {
            vecLengthList.at(vid) = extendVecList.at(vid).Normalise();
        }
        std::vector<double> smoothLengthList(vectorSize);
        for (int itrIndex = 0; itrIndex < maxIteration; itrIndex++)
        {
            for (int vid = startIndex; vid < endIndex; vid++)
            {
                smoothLengthList.at(vid) = vecLengthList.at(vid) * (1.0 - weight) + 
                    (vecLengthList.at((vid + 1) % vectorSize) + vecLengthList.at((vid - 1 + vectorSize) % vectorSize)) * 0.5 * weight;
            }
            if (!isClose)
            {
                smoothLengthList.at(0) = vecLengthList.at(0);
                smoothLengthList.at(vectorSize - 1) = vecLengthList.at(vectorSize - 1);
            }
            vecLengthList.swap(smoothLengthList);
        }
        for (int vid = startIndex; vid < endIndex; vid++)
        {
            extendVecList.at(vid) = extendVecList.at(vid) * vecLengthList.at(vid);
        }
    }

    static void SmoothControlPolylines(std::vector<std::vector<GPP::Vector3> >& subdCoordList, bool isClose,
        int maxIteration, double weight)
    {
        int innerControlSize = subdCoordList.at(0).size();
        int curveSize = subdCoordList.size();
        int startIndex = 0;
        int endIndex = curveSize;
        if (!isClose)
        {
            startIndex = 1;
            endIndex = curveSize - 1;
        }
        for (int itrIndex = 0; itrIndex < maxIteration; itrIndex++)
        {
            std::vector<GPP::Vector3> smoothCurveCoords(curveSize);
            for (int cid = 1; cid < innerControlSize - 1; cid++)
            {
                for (int vid = startIndex; vid < endIndex; vid++)
                {
                    smoothCurveCoords.at(vid) = subdCoordList.at(vid).at(cid) * (1.0 - weight) + 
                        (subdCoordList.at((vid + 1) % curveSize).at(cid) + subdCoordList.at((vid - 1 + curveSize) % curveSize).at(cid)) * 0.5 * weight;
                }
                for (int vid = startIndex; vid < endIndex; vid++)
                {
                    subdCoordList.at(vid).at(cid) = smoothCurveCoords.at(vid);
                }
            }
        }
    }

    static bool BlendCurve(GPP::TriMesh* triMesh, const std::vector<int>& centerCurve, const std::vector<int>& topCurve,
        const std::vector<int>& downCurve, bool isCurveClose)
    {
        int originVertexCount = triMesh->GetVertexCount();
        std::vector<std::set<int> > vertexNeighbors(originVertexCount, std::set<int>());
        int vertexIds[3] = {-1};
        int originFaceCount = triMesh->GetTriangleCount();
        for (int fid = 0; fid < originFaceCount; fid++)
        {
            triMesh->GetTriangleVertexIds(fid, vertexIds);
            vertexNeighbors.at(vertexIds[0]).insert(vertexIds[1]);
            vertexNeighbors.at(vertexIds[0]).insert(vertexIds[2]);
            vertexNeighbors.at(vertexIds[1]).insert(vertexIds[2]);
            vertexNeighbors.at(vertexIds[1]).insert(vertexIds[0]);
            vertexNeighbors.at(vertexIds[2]).insert(vertexIds[0]);
            vertexNeighbors.at(vertexIds[2]).insert(vertexIds[1]);
        }
        std::vector<int> deleteTriangles;
        int normalSmoothCount = 10;
        double normalSmoothWeight = 1.0;
        int extendSmoothCount = 10;
        double extendSmoothWeight = 1.0;
        int controlSmoothCount = 5;
        double controlSmoothWeight = 0.5;
        int subdCount = 3;
        if (isCurveClose)
        {
            int stringSize = topCurve.size();
            std::vector<GPP::Vector3> extrudeDirList;
            for (int vid = 0; vid < stringSize; vid++)
            {
                // Compute extrude plane
                GPP::Vector3 centerCoord = (triMesh->GetVertexCoord(topCurve.at(vid)) + triMesh->GetVertexCoord(downCurve.at(vid))) / 2.0;
                GPP::Vector3 centerNeighbors[4];
                centerNeighbors[0] = triMesh->GetVertexCoord(topCurve.at(vid));
                centerNeighbors[1] = (triMesh->GetVertexCoord(topCurve.at((vid - 1 + stringSize) % stringSize)) + 
                    triMesh->GetVertexCoord(downCurve.at((vid - 1 + stringSize) % stringSize))) / 2.0;
                centerNeighbors[2] = triMesh->GetVertexCoord(downCurve.at(vid));
                centerNeighbors[3] = (triMesh->GetVertexCoord(topCurve.at((vid + 1) % stringSize)) + 
                    triMesh->GetVertexCoord(downCurve.at((vid + 1) % stringSize))) / 2.0;
                GPP::Vector3 extrudeDir(0, 0, 0);
                for (int nid = 0; nid < 4; nid++)
                {
                    extrudeDir += (centerNeighbors[nid] - centerCoord).CrossProduct(centerNeighbors[(nid + 1) % 4] - centerCoord);
                }
                extrudeDir.Normalise();
                extrudeDirList.push_back(extrudeDir);
            }
            SmoothNormal(extrudeDirList, true, normalSmoothCount, normalSmoothWeight);

            std::vector<GPP::Vector3> topExtendVecList, downExtendVecList;
            for (int vid = 0; vid < stringSize; vid++)
            {
                // Compute extrude plane
                GPP::Vector3 centerCoord = (triMesh->GetVertexCoord(topCurve.at(vid)) + triMesh->GetVertexCoord(downCurve.at(vid))) / 2.0;
                GPP::Vector3 extrudeCoord = centerCoord + extrudeDirList.at(vid) * 
                    (triMesh->GetVertexCoord(topCurve.at(vid)) - triMesh->GetVertexCoord(downCurve.at(vid))).Length() * 0.5;
                GPP::Plane3 cuttingPlane(triMesh->GetVertexCoord(topCurve.at(vid)), 
                    triMesh->GetVertexCoord(downCurve.at(vid)), extrudeCoord);

                GPP::Vector3 topExtendVec;
                ComputeExtendVector(triMesh->GetVertexCoord(topCurve.at(vid)), cuttingPlane.ProjectVector(triMesh->GetVertexNormal(topCurve.at(vid))), 
                    triMesh->GetVertexCoord(downCurve.at(vid)), topExtendVec);
                topExtendVecList.push_back(topExtendVec);

                GPP::Vector3 downExtendVec;
                ComputeExtendVector(triMesh->GetVertexCoord(downCurve.at(vid)), cuttingPlane.ProjectVector(triMesh->GetVertexNormal(downCurve.at(vid))), 
                    triMesh->GetVertexCoord(topCurve.at(vid)), downExtendVec);
                downExtendVecList.push_back(downExtendVec);
            }
            SmoothExtendVec(topExtendVecList, true, extendSmoothCount, extendSmoothWeight);
            SmoothExtendVec(downExtendVecList, true, extendSmoothCount, extendSmoothWeight);

            std::vector<std::vector<GPP::Vector3> > subdCoordList;
            for (int vid = 0; vid < stringSize; vid++)
            {
                std::vector<GPP::Vector3> subdCoords(4);
                subdCoords.at(0) = triMesh->GetVertexCoord(topCurve.at(vid));
                subdCoords.at(1) = triMesh->GetVertexCoord(topCurve.at(vid)) + topExtendVecList.at(vid) * 0.33;
                subdCoords.at(2) = triMesh->GetVertexCoord(downCurve.at(vid)) + downExtendVecList.at(vid) * 0.33;
                subdCoords.at(3) = triMesh->GetVertexCoord(downCurve.at(vid));
                subdCoordList.push_back(subdCoords);
            }
            SmoothControlPolylines(subdCoordList, true, controlSmoothCount, controlSmoothWeight);
            GPP::ErrorCode res = GPP::OptimiseCurve::SmoothCurveOnMesh(triMesh, topCurve, GPP::ONE_RADIAN * 60, controlSmoothWeight, controlSmoothCount);
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "topCurve Smooth Failed", "温馨提示", MB_OK);
            }
            res = GPP::OptimiseCurve::SmoothCurveOnMesh(triMesh, downCurve, GPP::ONE_RADIAN * 60, controlSmoothWeight, controlSmoothCount);
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "downCurve Smooth Failed", "温馨提示", MB_OK);
            }
            for (int vid = 0; vid < stringSize; vid++)
            {
                if (GPP::OptimiseCurve::SubdividePolyline(subdCoordList.at(vid), subdCount, false) != GPP_NO_ERROR)
                {
                    return false;
                }
            }

            std::vector<bool> vertexMark(originVertexCount, 0);
            for (std::vector<int>::const_iterator itr = topCurve.begin(); itr != topCurve.end(); ++itr)
            {
                vertexMark.at(*itr) = 1;
            }
            for (std::vector<int>::const_iterator itr = downCurve.begin(); itr != downCurve.end(); ++itr)
            {
                vertexMark.at(*itr) = 1;
            }
            for (std::vector<int>::const_iterator itr = centerCurve.begin(); itr != centerCurve.end(); ++itr)
            {
                vertexMark.at(*itr) = 1;
            }
            std::vector<int> vertexStack = centerCurve;
            while (vertexStack.size() > 0)
            {
                std::vector<int> vertexStackNext;
                for (std::vector<int>::iterator stackItr = vertexStack.begin(); stackItr != vertexStack.end(); ++stackItr)
                {
                    for (std::set<int>::iterator nitr = vertexNeighbors.at(*stackItr).begin(); nitr != vertexNeighbors.at(*stackItr).end(); ++nitr)
                    {
                        if (vertexMark.at(*nitr))
                        {
                            continue;
                        }
                        vertexMark.at(*nitr) = 1;
                        vertexStackNext.push_back(*nitr);
                    }
                }
                vertexStack.swap(vertexStackNext);
            }
            for (int fid = 0; fid < originFaceCount; fid++)
            {
                triMesh->GetTriangleVertexIds(fid, vertexIds);
                if (vertexMark.at(vertexIds[0]) && vertexMark.at(vertexIds[1]) && vertexMark.at(vertexIds[2]))
                {
                    deleteTriangles.push_back(fid);
                }
            }
            res = GPP::DeleteTriMeshTriangles(triMesh, deleteTriangles, false);
            if (res != GPP_NO_ERROR)
            {
                return false;
            }

            int subdSize = subdCoordList.at(0).size();
            std::vector<std::vector<int> > subdIndexList(subdSize, std::vector<int>(stringSize)); 
            for (int vid = 0; vid < stringSize; vid++)
            {
                subdIndexList.at(0).at(vid) = topCurve.at(vid);
                subdIndexList.at(subdSize - 1).at(vid) = downCurve.at(vid);
                for (int sid = 1; sid < subdSize - 1; sid++)
                {
                    subdIndexList.at(sid).at(vid) = triMesh->InsertVertex(subdCoordList.at(vid).at(sid));
                }
            }
            for (int vid = 0; vid < stringSize; vid++)
            {
                for (int sid = 0; sid < subdSize - 1; sid++)
                {
                    triMesh->InsertTriangle(subdIndexList.at(sid).at((vid + 1) % stringSize), subdIndexList.at(sid).at(vid), 
                        subdIndexList.at(sid + 1).at((vid + 1) % stringSize));
                    triMesh->InsertTriangle(subdIndexList.at(sid).at(vid), subdIndexList.at(sid + 1).at(vid), 
                        subdIndexList.at(sid + 1).at((vid + 1) % stringSize));
                }
            }
            res = GPP::DeleteIsolatedVertices(triMesh);
            if (res != GPP_NO_ERROR)
            {
                return false;
            }
            triMesh->UpdateNormal();
        }
        else
        {
            int stringSize = topCurve.size();

            std::vector<GPP::Vector3> extrudeDirList;
            for (int vid = 0; vid < stringSize; vid++)
            {
                // Compute extrude plane
                GPP::Vector3 centerCoord = (triMesh->GetVertexCoord(topCurve.at(vid)) + triMesh->GetVertexCoord(downCurve.at(vid))) / 2.0;
                GPP::Vector3 centerNeighbors[4];
                centerNeighbors[0] = triMesh->GetVertexCoord(topCurve.at(vid));
                centerNeighbors[1] = (triMesh->GetVertexCoord(topCurve.at((vid - 1 + stringSize) % stringSize)) + 
                    triMesh->GetVertexCoord(downCurve.at((vid - 1 + stringSize) % stringSize))) / 2.0;
                centerNeighbors[2] = triMesh->GetVertexCoord(downCurve.at(vid));
                centerNeighbors[3] = (triMesh->GetVertexCoord(topCurve.at((vid + 1) % stringSize)) + 
                    triMesh->GetVertexCoord(downCurve.at((vid + 1) % stringSize))) / 2.0;
                GPP::Vector3 extrudeDir(0, 0, 0);
                for (int nid = 0; nid < 4; nid++)
                {
                    extrudeDir += (centerNeighbors[nid] - centerCoord).CrossProduct(centerNeighbors[(nid + 1) % 4] - centerCoord);
                }
                extrudeDir.Normalise();
                extrudeDirList.push_back(extrudeDir);
            }
            SmoothNormal(extrudeDirList, false, normalSmoothCount, normalSmoothWeight);

            std::vector<GPP::Vector3> topExtendVecList, downExtendVecList;
            for (int vid = 0; vid < stringSize; vid++)
            {
                // Compute extrude plane
                GPP::Vector3 centerCoord = (triMesh->GetVertexCoord(topCurve.at(vid)) + triMesh->GetVertexCoord(downCurve.at(vid))) / 2.0;
                GPP::Vector3 extrudeCoord = centerCoord + extrudeDirList.at(vid) * 
                    (triMesh->GetVertexCoord(topCurve.at(vid)) - triMesh->GetVertexCoord(downCurve.at(vid))).Length() * 0.5;
                GPP::Plane3 cuttingPlane(triMesh->GetVertexCoord(topCurve.at(vid)), 
                    triMesh->GetVertexCoord(downCurve.at(vid)), extrudeCoord);

                GPP::Vector3 topExtendVec;
                ComputeExtendVector(triMesh->GetVertexCoord(topCurve.at(vid)), cuttingPlane.ProjectVector(triMesh->GetVertexNormal(topCurve.at(vid))), 
                    triMesh->GetVertexCoord(downCurve.at(vid)), topExtendVec);
                topExtendVecList.push_back(topExtendVec);

                GPP::Vector3 downExtendVec;
                ComputeExtendVector(triMesh->GetVertexCoord(downCurve.at(vid)), cuttingPlane.ProjectVector(triMesh->GetVertexNormal(downCurve.at(vid))), 
                    triMesh->GetVertexCoord(topCurve.at(vid)), downExtendVec);
                downExtendVecList.push_back(downExtendVec);
            }
            SmoothExtendVec(topExtendVecList, false, extendSmoothCount, extendSmoothWeight);
            SmoothExtendVec(downExtendVecList, false, extendSmoothCount, extendSmoothWeight);

            std::vector<std::vector<GPP::Vector3> > subdCoordList;
            for (int vid = 0; vid < stringSize; vid++)
            {
                std::vector<GPP::Vector3> subdCoords(4);
                subdCoords.at(0) = triMesh->GetVertexCoord(topCurve.at(vid));
                subdCoords.at(1) = triMesh->GetVertexCoord(topCurve.at(vid)) + topExtendVecList.at(vid) * 0.33;
                subdCoords.at(2) = triMesh->GetVertexCoord(downCurve.at(vid)) + downExtendVecList.at(vid) * 0.33;
                subdCoords.at(3) = triMesh->GetVertexCoord(downCurve.at(vid));
                subdCoordList.push_back(subdCoords);
            }
            SmoothControlPolylines(subdCoordList, false, controlSmoothCount, controlSmoothWeight);
            GPP::ErrorCode res = GPP::OptimiseCurve::SmoothCurveOnMesh(triMesh, topCurve, GPP::ONE_RADIAN * 60, controlSmoothWeight, controlSmoothCount);
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "topCurve Smooth Failed", "温馨提示", MB_OK);
            }
            res = GPP::OptimiseCurve::SmoothCurveOnMesh(triMesh, downCurve, GPP::ONE_RADIAN * 60, controlSmoothWeight, controlSmoothCount);
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "downCurve Smooth Failed", "温馨提示", MB_OK);
            }
            for (int vid = 0; vid < stringSize; vid++)
            {
                if (GPP::OptimiseCurve::SubdividePolyline(subdCoordList.at(vid), subdCount, false) != GPP_NO_ERROR)
                {
                    return false;
                }
            }

            std::vector<int> sectionVertexIds;
            sectionVertexIds.push_back(topCurve.at(0));
            sectionVertexIds.push_back(downCurve.at(0));
            std::vector<int> leftCurves;
            GPP::Real distance;
            res = GPP::MeasureMesh::ComputeApproximateGeodesics(triMesh, sectionVertexIds, false, leftCurves, distance);
            if (res != GPP_NO_ERROR)
            {
                return false;
            }
            res = GPP::OptimiseCurve::SmoothCurveOnMesh(triMesh, leftCurves, GPP::ONE_RADIAN * 60, 0.2, 10);
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "leftCurves Smooth Failed", "温馨提示", MB_OK);
            }
            sectionVertexIds.clear();
            sectionVertexIds.push_back(topCurve.at(stringSize - 1));
            sectionVertexIds.push_back(downCurve.at(stringSize - 1));
            std::vector<int> rightCurves;
            res = GPP::MeasureMesh::ComputeApproximateGeodesics(triMesh, sectionVertexIds, false, rightCurves, distance);
            if (res != GPP_NO_ERROR)
            {
                return false;
            }
            res = GPP::OptimiseCurve::SmoothCurveOnMesh(triMesh, rightCurves, GPP::ONE_RADIAN * 60, 0.2, 10);
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "rightCurves Smooth Failed", "温馨提示", MB_OK);
            }
            std::vector<bool> vertexMark(originVertexCount, 0);
            for (std::vector<int>::const_iterator itr = topCurve.begin(); itr != topCurve.end(); ++itr)
            {
                vertexMark.at(*itr) = 1;
            }
            for (std::vector<int>::const_iterator itr = downCurve.begin(); itr != downCurve.end(); ++itr)
            {
                vertexMark.at(*itr) = 1;
            }
            for (std::vector<int>::const_iterator itr = leftCurves.begin(); itr != leftCurves.end(); ++itr)
            {
                vertexMark.at(*itr) = 1;
            }
            for (std::vector<int>::const_iterator itr = rightCurves.begin(); itr != rightCurves.end(); ++itr)
            {
                vertexMark.at(*itr) = 1;
            }
            int seedVertex = centerCurve.at(centerCurve.size() / 2);
            std::vector<int> vertexStack;
            vertexStack.push_back(seedVertex);
            vertexMark.at(seedVertex) = 1;
            while (vertexStack.size() > 0)
            {
                std::vector<int> vertexStackNext;
                for (std::vector<int>::iterator stackItr = vertexStack.begin(); stackItr != vertexStack.end(); ++stackItr)
                {
                    for (std::set<int>::iterator nitr = vertexNeighbors.at(*stackItr).begin(); nitr != vertexNeighbors.at(*stackItr).end(); ++nitr)
                    {
                        if (vertexMark.at(*nitr))
                        {
                            continue;
                        }
                        vertexMark.at(*nitr) = 1;
                        vertexStackNext.push_back(*nitr);
                    }
                }
                vertexStack.swap(vertexStackNext);
            }
            for (int fid = 0; fid < originFaceCount; fid++)
            {
                triMesh->GetTriangleVertexIds(fid, vertexIds);
                if (vertexMark.at(vertexIds[0]) && vertexMark.at(vertexIds[1]) && vertexMark.at(vertexIds[2]))
                {
                    deleteTriangles.push_back(fid);
                }
            }
            res = GPP::DeleteTriMeshTriangles(triMesh, deleteTriangles, false);
            if (res != GPP_NO_ERROR)
            {
                return false;
            }

            int subdSize = subdCoordList.at(0).size();
            std::vector<std::vector<int> > subdIndexList(subdSize, std::vector<int>(stringSize)); 
            for (int vid = 0; vid < stringSize; vid++)
            {
                subdIndexList.at(0).at(vid) = topCurve.at(vid);
                subdIndexList.at(subdSize - 1).at(vid) = downCurve.at(vid);
                for (int sid = 1; sid < subdSize - 1; sid++)
                {
                    subdIndexList.at(sid).at(vid) = triMesh->InsertVertex(subdCoordList.at(vid).at(sid));
                }
            }
            for (int vid = 0; vid < stringSize - 1; vid++)
            {
                for (int sid = 0; sid < subdSize - 1; sid++)
                {
                    triMesh->InsertTriangle(subdIndexList.at(sid).at(vid + 1), subdIndexList.at(sid).at(vid), 
                        subdIndexList.at(sid + 1).at(vid + 1));
                    triMesh->InsertTriangle(subdIndexList.at(sid).at(vid), subdIndexList.at(sid + 1).at(vid), 
                        subdIndexList.at(sid + 1).at(vid + 1));
                }
            }

            std::vector<int> boundarySeedIds;
            boundarySeedIds.push_back(topCurve.at(0));
            boundarySeedIds.push_back(topCurve.at(stringSize - 1));
            res = GPP::FillMeshHole::FillHoles(triMesh, &boundarySeedIds);
            if (res != GPP_NO_ERROR)
            {
                return false;
            }
            res = GPP::DeleteIsolatedVertices(triMesh);
            if (res != GPP_NO_ERROR)
            {
                return false;
            }
            triMesh->UpdateNormal();
        }
        return true;
    }

    static bool SharpCurve(GPP::TriMesh* triMesh, const std::vector<int>& centerCurve, const std::vector<int>& topCurve,
        const std::vector<int>& downCurve, bool isCurveClose)
    {
        int originVertexCount = triMesh->GetVertexCount();
        std::vector<std::set<int> > vertexNeighbors(originVertexCount, std::set<int>());
        int vertexIds[3] = {-1};
        int originFaceCount = triMesh->GetTriangleCount();
        for (int fid = 0; fid < originFaceCount; fid++)
        {
            triMesh->GetTriangleVertexIds(fid, vertexIds);
            vertexNeighbors.at(vertexIds[0]).insert(vertexIds[1]);
            vertexNeighbors.at(vertexIds[0]).insert(vertexIds[2]);
            vertexNeighbors.at(vertexIds[1]).insert(vertexIds[2]);
            vertexNeighbors.at(vertexIds[1]).insert(vertexIds[0]);
            vertexNeighbors.at(vertexIds[2]).insert(vertexIds[0]);
            vertexNeighbors.at(vertexIds[2]).insert(vertexIds[1]);
        }
        std::vector<int> deleteTriangles;
        int normalSmoothCount = 10;
        double normalSmoothWeight = 1.0;
        int extendSmoothCount = 10;
        double extendSmoothWeight = 1.0;
        int controlSmoothCount = 5;
        double controlSmoothWeight = 0.5;
        if (isCurveClose)
        {
            int stringSize = topCurve.size();
            std::vector<GPP::Vector3> extrudeDirList;
            for (int vid = 0; vid < stringSize; vid++)
            {
                // Compute extrude plane
                GPP::Vector3 centerCoord = (triMesh->GetVertexCoord(topCurve.at(vid)) + triMesh->GetVertexCoord(downCurve.at(vid))) / 2.0;
                GPP::Vector3 centerNeighbors[4];
                centerNeighbors[0] = triMesh->GetVertexCoord(topCurve.at(vid));
                centerNeighbors[1] = (triMesh->GetVertexCoord(topCurve.at((vid - 1 + stringSize) % stringSize)) + 
                    triMesh->GetVertexCoord(downCurve.at((vid - 1 + stringSize) % stringSize))) / 2.0;
                centerNeighbors[2] = triMesh->GetVertexCoord(downCurve.at(vid));
                centerNeighbors[3] = (triMesh->GetVertexCoord(topCurve.at((vid + 1) % stringSize)) + 
                    triMesh->GetVertexCoord(downCurve.at((vid + 1) % stringSize))) / 2.0;
                GPP::Vector3 extrudeDir(0, 0, 0);
                for (int nid = 0; nid < 4; nid++)
                {
                    extrudeDir += (centerNeighbors[nid] - centerCoord).CrossProduct(centerNeighbors[(nid + 1) % 4] - centerCoord);
                }
                extrudeDir.Normalise();
                extrudeDirList.push_back(extrudeDir);
            }
            SmoothNormal(extrudeDirList, true, normalSmoothCount, normalSmoothWeight);

            std::vector<GPP::Vector3> topExtendVecList, downExtendVecList;
            for (int vid = 0; vid < stringSize; vid++)
            {
                // Compute extrude plane
                GPP::Vector3 centerCoord = (triMesh->GetVertexCoord(topCurve.at(vid)) + triMesh->GetVertexCoord(downCurve.at(vid))) / 2.0;
                GPP::Vector3 extrudeCoord = centerCoord + extrudeDirList.at(vid) * 
                    (triMesh->GetVertexCoord(topCurve.at(vid)) - triMesh->GetVertexCoord(downCurve.at(vid))).Length() * 0.5;
                GPP::Plane3 cuttingPlane(triMesh->GetVertexCoord(topCurve.at(vid)), 
                    triMesh->GetVertexCoord(downCurve.at(vid)), extrudeCoord);

                GPP::Vector3 topExtendVec;
                ComputeExtendVector(triMesh->GetVertexCoord(topCurve.at(vid)), cuttingPlane.ProjectVector(triMesh->GetVertexNormal(topCurve.at(vid))), 
                    triMesh->GetVertexCoord(downCurve.at(vid)), topExtendVec);
                topExtendVecList.push_back(topExtendVec);

                GPP::Vector3 downExtendVec;
                ComputeExtendVector(triMesh->GetVertexCoord(downCurve.at(vid)), cuttingPlane.ProjectVector(triMesh->GetVertexNormal(downCurve.at(vid))), 
                    triMesh->GetVertexCoord(topCurve.at(vid)), downExtendVec);
                downExtendVecList.push_back(downExtendVec);
            }
            SmoothExtendVec(topExtendVecList, true, extendSmoothCount, extendSmoothWeight);
            SmoothExtendVec(downExtendVecList, true, extendSmoothCount, extendSmoothWeight);

            std::vector<std::vector<GPP::Vector3> > subdCoordList;
            for (int vid = 0; vid < stringSize; vid++)
            {
                std::vector<GPP::Vector3> subdCoords(3);
                subdCoords.at(0) = triMesh->GetVertexCoord(topCurve.at(vid));
                subdCoords.at(1) = (triMesh->GetVertexCoord(topCurve.at(vid)) + topExtendVecList.at(vid) + 
                    triMesh->GetVertexCoord(downCurve.at(vid)) + downExtendVecList.at(vid)) / 2.0;
                subdCoords.at(2) = triMesh->GetVertexCoord(downCurve.at(vid));
                subdCoordList.push_back(subdCoords);
            }
            SmoothControlPolylines(subdCoordList, true, controlSmoothCount, controlSmoothWeight);
            GPP::ErrorCode res = GPP::OptimiseCurve::SmoothCurveOnMesh(triMesh, topCurve, GPP::ONE_RADIAN * 60, controlSmoothWeight, controlSmoothCount);
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "topCurve Smooth Failed", "温馨提示", MB_OK);
            }
            res = GPP::OptimiseCurve::SmoothCurveOnMesh(triMesh, downCurve, GPP::ONE_RADIAN * 60, controlSmoothWeight, controlSmoothCount);
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "downCurve Smooth Failed", "温馨提示", MB_OK);
            }

            std::vector<bool> vertexMark(originVertexCount, 0);
            for (std::vector<int>::const_iterator itr = topCurve.begin(); itr != topCurve.end(); ++itr)
            {
                vertexMark.at(*itr) = 1;
            }
            for (std::vector<int>::const_iterator itr = downCurve.begin(); itr != downCurve.end(); ++itr)
            {
                vertexMark.at(*itr) = 1;
            }
            for (std::vector<int>::const_iterator itr = centerCurve.begin(); itr != centerCurve.end(); ++itr)
            {
                vertexMark.at(*itr) = 1;
            }
            std::vector<int> vertexStack = centerCurve;
            while (vertexStack.size() > 0)
            {
                std::vector<int> vertexStackNext;
                for (std::vector<int>::iterator stackItr = vertexStack.begin(); stackItr != vertexStack.end(); ++stackItr)
                {
                    for (std::set<int>::iterator nitr = vertexNeighbors.at(*stackItr).begin(); nitr != vertexNeighbors.at(*stackItr).end(); ++nitr)
                    {
                        if (vertexMark.at(*nitr))
                        {
                            continue;
                        }
                        vertexMark.at(*nitr) = 1;
                        vertexStackNext.push_back(*nitr);
                    }
                }
                vertexStack.swap(vertexStackNext);
            }
            for (int fid = 0; fid < originFaceCount; fid++)
            {
                triMesh->GetTriangleVertexIds(fid, vertexIds);
                if (vertexMark.at(vertexIds[0]) && vertexMark.at(vertexIds[1]) && vertexMark.at(vertexIds[2]))
                {
                    deleteTriangles.push_back(fid);
                }
            }
            res = GPP::DeleteTriMeshTriangles(triMesh, deleteTriangles, false);
            if (res != GPP_NO_ERROR)
            {
                return false;
            }

            int subdSize = subdCoordList.at(0).size();
            std::vector<std::vector<int> > subdIndexList(subdSize, std::vector<int>(stringSize)); 
            for (int vid = 0; vid < stringSize; vid++)
            {
                subdIndexList.at(0).at(vid) = topCurve.at(vid);
                subdIndexList.at(subdSize - 1).at(vid) = downCurve.at(vid);
                for (int sid = 1; sid < subdSize - 1; sid++)
                {
                    subdIndexList.at(sid).at(vid) = triMesh->InsertVertex(subdCoordList.at(vid).at(sid));
                }
            }
            for (int vid = 0; vid < stringSize; vid++)
            {
                for (int sid = 0; sid < subdSize - 1; sid++)
                {
                    triMesh->InsertTriangle(subdIndexList.at(sid).at((vid + 1) % stringSize), subdIndexList.at(sid).at(vid), 
                        subdIndexList.at(sid + 1).at((vid + 1) % stringSize));
                    triMesh->InsertTriangle(subdIndexList.at(sid).at(vid), subdIndexList.at(sid + 1).at(vid), 
                        subdIndexList.at(sid + 1).at((vid + 1) % stringSize));
                }
            }
            res = GPP::DeleteIsolatedVertices(triMesh);
            if (res != GPP_NO_ERROR)
            {
                return false;
            }
            triMesh->UpdateNormal();
        }
        else
        {
            int stringSize = topCurve.size();

            std::vector<GPP::Vector3> extrudeDirList;
            for (int vid = 0; vid < stringSize; vid++)
            {
                // Compute extrude plane
                GPP::Vector3 centerCoord = (triMesh->GetVertexCoord(topCurve.at(vid)) + triMesh->GetVertexCoord(downCurve.at(vid))) / 2.0;
                GPP::Vector3 centerNeighbors[4];
                centerNeighbors[0] = triMesh->GetVertexCoord(topCurve.at(vid));
                centerNeighbors[1] = (triMesh->GetVertexCoord(topCurve.at((vid - 1 + stringSize) % stringSize)) + 
                    triMesh->GetVertexCoord(downCurve.at((vid - 1 + stringSize) % stringSize))) / 2.0;
                centerNeighbors[2] = triMesh->GetVertexCoord(downCurve.at(vid));
                centerNeighbors[3] = (triMesh->GetVertexCoord(topCurve.at((vid + 1) % stringSize)) + 
                    triMesh->GetVertexCoord(downCurve.at((vid + 1) % stringSize))) / 2.0;
                GPP::Vector3 extrudeDir(0, 0, 0);
                for (int nid = 0; nid < 4; nid++)
                {
                    extrudeDir += (centerNeighbors[nid] - centerCoord).CrossProduct(centerNeighbors[(nid + 1) % 4] - centerCoord);
                }
                extrudeDir.Normalise();
                extrudeDirList.push_back(extrudeDir);
            }
            SmoothNormal(extrudeDirList, false, normalSmoothCount, normalSmoothWeight);

            std::vector<GPP::Vector3> topExtendVecList, downExtendVecList;
            for (int vid = 0; vid < stringSize; vid++)
            {
                // Compute extrude plane
                GPP::Vector3 centerCoord = (triMesh->GetVertexCoord(topCurve.at(vid)) + triMesh->GetVertexCoord(downCurve.at(vid))) / 2.0;
                GPP::Vector3 extrudeCoord = centerCoord + extrudeDirList.at(vid) * 
                    (triMesh->GetVertexCoord(topCurve.at(vid)) - triMesh->GetVertexCoord(downCurve.at(vid))).Length() * 0.5;
                GPP::Plane3 cuttingPlane(triMesh->GetVertexCoord(topCurve.at(vid)), 
                    triMesh->GetVertexCoord(downCurve.at(vid)), extrudeCoord);

                GPP::Vector3 topExtendVec;
                ComputeExtendVector(triMesh->GetVertexCoord(topCurve.at(vid)), cuttingPlane.ProjectVector(triMesh->GetVertexNormal(topCurve.at(vid))), 
                    triMesh->GetVertexCoord(downCurve.at(vid)), topExtendVec);
                topExtendVecList.push_back(topExtendVec);

                GPP::Vector3 downExtendVec;
                ComputeExtendVector(triMesh->GetVertexCoord(downCurve.at(vid)), cuttingPlane.ProjectVector(triMesh->GetVertexNormal(downCurve.at(vid))), 
                    triMesh->GetVertexCoord(topCurve.at(vid)), downExtendVec);
                downExtendVecList.push_back(downExtendVec);
            }
            SmoothExtendVec(topExtendVecList, false, extendSmoothCount, extendSmoothWeight);
            SmoothExtendVec(downExtendVecList, false, extendSmoothCount, extendSmoothWeight);

            std::vector<std::vector<GPP::Vector3> > subdCoordList;
            for (int vid = 0; vid < stringSize; vid++)
            {
                std::vector<GPP::Vector3> subdCoords(3);
                subdCoords.at(0) = triMesh->GetVertexCoord(topCurve.at(vid));
                subdCoords.at(1) = (triMesh->GetVertexCoord(topCurve.at(vid)) + topExtendVecList.at(vid) + 
                    triMesh->GetVertexCoord(downCurve.at(vid)) + downExtendVecList.at(vid)) / 2.0;
                subdCoords.at(2) = triMesh->GetVertexCoord(downCurve.at(vid));
                subdCoordList.push_back(subdCoords);
            }
            SmoothControlPolylines(subdCoordList, false, controlSmoothCount, controlSmoothWeight);
            GPP::ErrorCode res = GPP::OptimiseCurve::SmoothCurveOnMesh(triMesh, topCurve, GPP::ONE_RADIAN * 60, controlSmoothWeight, controlSmoothCount);
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "topCurve Smooth Failed", "温馨提示", MB_OK);
            }
            res = GPP::OptimiseCurve::SmoothCurveOnMesh(triMesh, downCurve, GPP::ONE_RADIAN * 60, controlSmoothWeight, controlSmoothCount);
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "downCurve Smooth Failed", "温馨提示", MB_OK);
            }

            std::vector<int> sectionVertexIds;
            sectionVertexIds.push_back(topCurve.at(0));
            sectionVertexIds.push_back(downCurve.at(0));
            std::vector<int> leftCurves;
            GPP::Real distance;
            res = GPP::MeasureMesh::ComputeApproximateGeodesics(triMesh, sectionVertexIds, false, leftCurves, distance);
            if (res != GPP_NO_ERROR)
            {
                return false;
            }
            res = GPP::OptimiseCurve::SmoothCurveOnMesh(triMesh, leftCurves, GPP::ONE_RADIAN * 60, 0.2, 10);
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "leftCurves Smooth Failed", "温馨提示", MB_OK);
            }
            sectionVertexIds.clear();
            sectionVertexIds.push_back(topCurve.at(stringSize - 1));
            sectionVertexIds.push_back(downCurve.at(stringSize - 1));
            std::vector<int> rightCurves;
            res = GPP::MeasureMesh::ComputeApproximateGeodesics(triMesh, sectionVertexIds, false, rightCurves, distance);
            if (res != GPP_NO_ERROR)
            {
                return false;
            }
            res = GPP::OptimiseCurve::SmoothCurveOnMesh(triMesh, rightCurves, GPP::ONE_RADIAN * 60, 0.2, 10);
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "rightCurves Smooth Failed", "温馨提示", MB_OK);
            }
            std::vector<bool> vertexMark(originVertexCount, 0);
            for (std::vector<int>::const_iterator itr = topCurve.begin(); itr != topCurve.end(); ++itr)
            {
                vertexMark.at(*itr) = 1;
            }
            for (std::vector<int>::const_iterator itr = downCurve.begin(); itr != downCurve.end(); ++itr)
            {
                vertexMark.at(*itr) = 1;
            }
            for (std::vector<int>::const_iterator itr = leftCurves.begin(); itr != leftCurves.end(); ++itr)
            {
                vertexMark.at(*itr) = 1;
            }
            for (std::vector<int>::const_iterator itr = rightCurves.begin(); itr != rightCurves.end(); ++itr)
            {
                vertexMark.at(*itr) = 1;
            }
            int seedVertex = centerCurve.at(centerCurve.size() / 2);
            std::vector<int> vertexStack;
            vertexStack.push_back(seedVertex);
            vertexMark.at(seedVertex) = 1;
            while (vertexStack.size() > 0)
            {
                std::vector<int> vertexStackNext;
                for (std::vector<int>::iterator stackItr = vertexStack.begin(); stackItr != vertexStack.end(); ++stackItr)
                {
                    for (std::set<int>::iterator nitr = vertexNeighbors.at(*stackItr).begin(); nitr != vertexNeighbors.at(*stackItr).end(); ++nitr)
                    {
                        if (vertexMark.at(*nitr))
                        {
                            continue;
                        }
                        vertexMark.at(*nitr) = 1;
                        vertexStackNext.push_back(*nitr);
                    }
                }
                vertexStack.swap(vertexStackNext);
            }
            for (int fid = 0; fid < originFaceCount; fid++)
            {
                triMesh->GetTriangleVertexIds(fid, vertexIds);
                if (vertexMark.at(vertexIds[0]) && vertexMark.at(vertexIds[1]) && vertexMark.at(vertexIds[2]))
                {
                    deleteTriangles.push_back(fid);
                }
            }
            res = GPP::DeleteTriMeshTriangles(triMesh, deleteTriangles, false);
            if (res != GPP_NO_ERROR)
            {
                return false;
            }

            int subdSize = subdCoordList.at(0).size();
            std::vector<std::vector<int> > subdIndexList(subdSize, std::vector<int>(stringSize)); 
            for (int vid = 0; vid < stringSize; vid++)
            {
                subdIndexList.at(0).at(vid) = topCurve.at(vid);
                subdIndexList.at(subdSize - 1).at(vid) = downCurve.at(vid);
                for (int sid = 1; sid < subdSize - 1; sid++)
                {
                    subdIndexList.at(sid).at(vid) = triMesh->InsertVertex(subdCoordList.at(vid).at(sid));
                }
            }
            for (int vid = 0; vid < stringSize - 1; vid++)
            {
                for (int sid = 0; sid < subdSize - 1; sid++)
                {
                    triMesh->InsertTriangle(subdIndexList.at(sid).at(vid + 1), subdIndexList.at(sid).at(vid), 
                        subdIndexList.at(sid + 1).at(vid + 1));
                    triMesh->InsertTriangle(subdIndexList.at(sid).at(vid), subdIndexList.at(sid + 1).at(vid), 
                        subdIndexList.at(sid + 1).at(vid + 1));
                }
            }

            std::vector<int> boundarySeedIds;
            boundarySeedIds.push_back(topCurve.at(0));
            boundarySeedIds.push_back(topCurve.at(stringSize - 1));
            res = GPP::FillMeshHole::FillHoles(triMesh, &boundarySeedIds);
            if (res != GPP_NO_ERROR)
            {
                return false;
            }
            res = GPP::DeleteIsolatedVertices(triMesh);
            if (res != GPP_NO_ERROR)
            {
                return false;
            }
            triMesh->UpdateNormal();
        }
        return true;
    }

    static bool AdhereVector(const GPP::Vector3& pt, GPP::Vector3& newPt)
    {
        const double tol = 0.1;
        std::vector<int> zeroCoordIdx;
        std::vector<int> halfCoordIdx;
        std::vector<int> quadCoordIdx1, quadCoordIdx2;
        for (int ii = 0; ii < 3; ++ii)
        {
            if (pt[ii] < tol)
            {
                zeroCoordIdx.push_back(ii);
            }
            else if (pt[ii] > 0.5 - tol && pt[ii] < 0.5 + tol)
            {
                halfCoordIdx.push_back(ii);
            }
            else if (pt[ii] > 0.25 - tol && pt[ii] < 0.25 + tol)
            {
                quadCoordIdx1.push_back(ii);
            }
            else if (pt[ii] > 0.75 - tol && pt[ii] < 0.75 + tol)
            {
                quadCoordIdx2.push_back(ii);
            }
        }
        if (zeroCoordIdx.size() == 2)
        {
            newPt[zeroCoordIdx[0]] = -1.0;
            newPt[zeroCoordIdx[1]] = -1.0;
            newPt[3 - zeroCoordIdx[0] - zeroCoordIdx[1]] = 1.0;
            return true;
        }
        else if (zeroCoordIdx.size() == 1 && halfCoordIdx.size() == 2)
        {
            newPt[zeroCoordIdx[0]] = -1.0;
            newPt[halfCoordIdx[0]] = 0.5;
            newPt[halfCoordIdx[1]] = 0.5;
            return true;
        }
        else if (zeroCoordIdx.size() == 1 && quadCoordIdx1.size() == 1 && quadCoordIdx2.size() == 1)
        {
            newPt[zeroCoordIdx[0]] = -1.0;
            newPt[quadCoordIdx1[0]] = 0.25;
            newPt[quadCoordIdx2[0]] = 0.75;
            return true;
        }
        return false;
    }

    static GPP::Vector3 GetCoord(const GPP::PointOnFace& pof, const GPP::ITriMesh* triMesh)
    {
        GPP::Vector3 coord;
        if (pof.mFaceId < 0 || pof.mFaceId >= triMesh->GetTriangleCount())
        {
            return coord;
        }
        else
        {
            GPP::Int vertexIds[3] = {-1, -1, -1};
            triMesh->GetTriangleVertexIds(pof.mFaceId, vertexIds);
            for (GPP::Int fvid = 0; fvid < 3; ++fvid)
            {
                GPP::Real weight = pof.mCoord[fvid];
                if (weight < 0)
                {
                    weight = 0.0;
                }
                coord += triMesh->GetVertexCoord(vertexIds[fvid]) * weight;
            }
            return coord;
        }
    }

    static GPP::Vector3 GetCoord(const GPP::PointOnEdge& edgePoint, const GPP::ITriMesh* triMesh)
    {
        if (edgePoint.mVertexIdEnd == -1)
        {
            return triMesh->GetVertexCoord(edgePoint.mVertexIdStart);
        }
        else if (edgePoint.mVertexIdStart == -1)
        {
            return GPP::Vector3();
        }
        else
        {
            return triMesh->GetVertexCoord(edgePoint.mVertexIdStart) * edgePoint.mWeight 
                + triMesh->GetVertexCoord(edgePoint.mVertexIdEnd) * (1.0 - edgePoint.mWeight);
        }
    }

    static GPP::Vector3 GetNormal(const GPP::PointOnFace& pof, const GPP::ITriMesh* triMesh)
    {
        GPP::Int vertexIds[3] = {-1, -1, -1};
        triMesh->GetTriangleVertexIds(pof.mFaceId, vertexIds);
        GPP::Real weights[3] = {0.0};
        for (int ii =0 ; ii < 3; ++ii)
        {
            weights[ii] = pof.mCoord[ii];
            if (weights[ii] < 0)
            {
                weights[ii] = 0.0;
            }
        }
        GPP::Vector3 norms[3] = {triMesh->GetVertexNormal(vertexIds[0]),
            triMesh->GetVertexNormal(vertexIds[1]), triMesh->GetVertexNormal(vertexIds[2])};
        GPP::Vector3 norm = norms[0] * weights[0] + norms[1] * weights[1] + norms[2] * weights[2];
        norm.Normalise();
        return norm;
    }

    static GPP::PointOnFace CreatePofOnVertex(GPP::Int vertexId, const GPP::ITriMesh* triMesh, GPP::TriMeshInfo* meshInfo)
    {
        GPP::ErrorCode res = meshInfo->CacheDataForVertexNbrFaceMaps(triMesh);
        if (res != GPP_NO_ERROR)
        {
            return GPP::PointOnFace();
        }

        const std::vector<GPP::Int>& vertexNbrFaces = meshInfo->GetVertexNbrFaceMaps().at(vertexId);
        if (vertexNbrFaces.empty())
        {
            return GPP::PointOnFace();
        }
        GPP::Int oneFaceId = vertexNbrFaces.front();
        GPP::Int vertexIds[3] = {-1, -1, -1};
        triMesh->GetTriangleVertexIds(oneFaceId, vertexIds);
        GPP::PointOnFace pof;
        pof.mFaceId = oneFaceId;
        for (int ii = 0; ii < 3; ++ii)
        {
            if (vertexIds[ii] == vertexId)
            {
                pof.mCoord[ii] = 1.0;
            }
            else
            {
                pof.mCoord[ii] = -1.0;
            }
        }
        return pof;
    }

    static GPP::PointOnFace CreatePofOnEdge(const GPP::PointOnEdge& edgePoint, const GPP::ITriMesh* triMesh, GPP::TriMeshInfo* meshInfo)
    {
        if (edgePoint.mVertexIdEnd == -1)
        {
            return CreatePofOnVertex(edgePoint.mVertexIdStart, triMesh, meshInfo);
        }
        GPP::ErrorCode res = meshInfo->CacheDataForEdgeInfos(triMesh, true);
        if (res != GPP_NO_ERROR)
        {
            return GPP::PointOnFace();
        }
        GPP::PointOnFace pof;
        GPP::Int smallV = edgePoint.mVertexIdStart < edgePoint.mVertexIdEnd ? edgePoint.mVertexIdStart : edgePoint.mVertexIdEnd;
        GPP::Int bigV = edgePoint.mVertexIdStart > edgePoint.mVertexIdEnd ? edgePoint.mVertexIdStart : edgePoint.mVertexIdEnd;
        const GPP::EdgeInfo& edge = meshInfo->GetEdgeInfoList().at(meshInfo->GetVertexEdgeMaps().at(smallV).at(bigV));
        GPP::Int oneFaceId = edge.mFaceIds.front();
        GPP::Int vertexIds[3] = {-1, -1, -1};
        triMesh->GetTriangleVertexIds(oneFaceId, vertexIds);
        pof.mFaceId = oneFaceId;
        pof.mCoord[0] = pof.mCoord[1] = pof.mCoord[2] = 0.0;
        for (GPP::Int fvid = 0; fvid < 3; ++fvid)
        {
            if (vertexIds[fvid] == edgePoint.mVertexIdStart)
            {
                pof.mCoord[fvid] = edgePoint.mWeight;
            }
            else if (vertexIds[fvid] == edgePoint.mVertexIdEnd)
            {
                pof.mCoord[fvid] = 1.0 - edgePoint.mWeight;
            }
            else
            {
                pof.mCoord[fvid] = -1.0;
            }
        }
        return pof;

    }

    bool MeasureApp::KeyPressed( const OIS::KeyEvent &arg )
    {
        if (arg.key == OIS::KC_L)
        {
            std::vector<GPP::Vector3> lineSegments;
            for (int mid = 0; mid < mMarkPoints.size() - 1; mid++)
            {
                lineSegments.push_back(mMarkPoints.at(mid));
                lineSegments.push_back(mMarkPoints.at(mid + 1));
            }
            GPP::Parser::ExportLineSegmentToPovray("edge.inc", lineSegments, 0.0025, GPP::Vector3(0.09, 0.48627, 0.69));
        }
        else if (arg.key == OIS::KC_O)
        {
            ComputeOffsetCurve(0.13);
        }
        else if (arg.key == OIS::KC_Z)
        {
            SmoothGeodesicsCrossVertex();
        }
        else if (arg.key == OIS::KC_G)
        {
            if (!mMarkFacePoints.empty())
            {
                // adhere some points onto the vertex/half-edge point
                for (int pid = 0; pid < mMarkFacePoints.size(); ++pid)
                {
                    GPP::PointOnFace& pof = mMarkFacePoints.at(pid);
                    GPP::Vector3 newCoord;
                    if (AdhereVector(pof.mCoord, newCoord))
                    {
                        pof.mCoord = newCoord;
                    }
                }
                mUpdateMarkRendering = true;
            }
        }
        else if (arg.key == OIS::KC_H)
        {
            AddOneCurve();
        }
        else if (arg.key == OIS::KC_J)
        {
            RemoveOneCurve();
        }
        else if (arg.key == OIS::KC_P)
        {
            GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
            GPP::TriMeshInfo* meshInfo = ModelManager::Get()->GetMeshInfo();
            if (triMesh)
            {
                bool isIntersected = false;
                if (!mGeodesicsOnPofs.empty())
                {
                    GPP::OptimiseCurve::_IsCurveSelfIntersected(triMesh, meshInfo, mGeodesicsOnPofs, isIntersected);
                }
                else if (!mCurvesOnMesh.empty())
                {
                    for (int curveId = 0; !isIntersected && curveId < mCurvesOnMesh.size(); ++curveId)
                    {
                        GPP::OptimiseCurve::_IsCurveSelfIntersected(triMesh, meshInfo, mCurvesOnMesh.at(curveId), isIntersected);
                    }
                }

                if (isIntersected)
                {
                    MessageBox(NULL, "曲线有自交", "温馨提示", MB_OK);
                }
                else
                {
                    MessageBox(NULL, "曲线无自交", "温馨提示", MB_OK);
                }
            }
        }
        else if (arg.key == OIS::KC_S)
        {
            SetSelectPrimitiveMode();
        }

        return true;
    }

    void MeasureApp::WindowFocusChanged( Ogre::RenderWindow* rw)
    {
        if (mpViewTool)
        {
            mpViewTool->MouseReleased();
        }
    }

    void MeasureApp::SetupScene()
    {
        Ogre::SceneManager* sceneManager = MagicCore::RenderSystem::Get()->GetSceneManager();
        sceneManager->setAmbientLight(Ogre::ColourValue(0.3, 0.3, 0.3));
        Ogre::Light* light = sceneManager->createLight("MeasureApp_SimpleLight");
        light->setPosition(0, 0, 20);
        light->setDiffuseColour(0.8, 0.8, 0.8);
        light->setSpecularColour(0.5, 0.5, 0.5);
        InitViewTool();
        if (ModelManager::Get()->GetMesh())
        {
            // set up pick tool
            GPPFREEPOINTER(mpPickTool);
            mpPickTool = new MagicCore::PickTool;
            mpPickTool->SetPickParameter(mRightMouseType == SELECT_VERTEX ? MagicCore::PM_POINT : MagicCore::PM_FACEPOINT, true, NULL, ModelManager::Get()->GetMesh(), "ModelNode");
            mpUI->SetModelInfo(ModelManager::Get()->GetMesh()->GetVertexCount(), ModelManager::Get()->GetMesh()->GetTriangleCount());
        }
    }

    void MeasureApp::ShutdownScene()
    {
        Ogre::SceneManager* sceneManager = MagicCore::RenderSystem::Get()->GetSceneManager();
        sceneManager->setAmbientLight(Ogre::ColourValue::Black);
        sceneManager->destroyLight("MeasureApp_SimpleLight");
        //MagicCore::RenderSystem::Get()->SetupCameraDefaultParameter();
        MagicCore::RenderSystem::Get()->HideRenderingObject("Mesh_Measure");       
        MagicCore::RenderSystem::Get()->HideRenderingObject("MeshRef_Measure");       
        MagicCore::RenderSystem::Get()->HideRenderingObject("MarkPoints_MeasureApp");       
        MagicCore::RenderSystem::Get()->HideRenderingObject("MarkPointLine_MeasureApp");
        MagicCore::RenderSystem::Get()->HideRenderingObject("MarkPointLine_MeasureApp");
        MagicCore::RenderSystem::Get()->HideRenderingObject("PrincipalLine_MeasureApp");
        //MagicCore::RenderSystem::Get()->ResertAllSceneNode();
    }

    void MeasureApp::ClearData()
    {
        GPPFREEPOINTER(mpUI);
        GPPFREEPOINTER(mpViewTool);
        GPPFREEPOINTER(mpPickTool);
        GPPFREEPOINTER(mpRefTriMesh);
        ModelManager::Get()->SetMeshInfo(NULL);
#if DEBUGDUMPFILE
        GPPFREEPOINTER(mpDumpInfo);
#endif
        ClearSelectionData();
        mMinCurvature.clear();
        mMaxCurvature.clear();
        mMinCurvatureDirs.clear();
        mMaxCurvatureDirs.clear();
        mCurvatureFlags.clear();
        mDisplayPrincipalCurvature = 0;
    }

    void MeasureApp::ClearSelectionData()
    {
        mMarkIds.clear();
        mGeodesicsOnVertices.clear();
        mMarkPoints.clear();
        mMarkFacePoints.clear();
        mGeodesicsOnPofs.clear();
        mCurvesOnMesh.clear();
    }

    void MeasureApp::DoCommand(bool isSubThread)
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
            case MagicApp::MeasureApp::NONE:
                break;
            case MagicApp::MeasureApp::GEODESICS_APPROXIMATE:
                ComputeApproximateGeodesics(false);
                break;
            case MagicApp::MeasureApp::GEODESICS_FAST_EXACT:
                FastComputeExactGeodesics(mGeodesicAccuracy, false);
                break;
            case MagicApp::MeasureApp::GEODESICS_EXACT:
                ComputeExactGeodesics(false);
                break;
            case MagicApp::MeasureApp::DISTANCE_POINTS_TO_MESH:
                ComputePointsToMeshDistance(false);
                break;
            case MagicApp::MeasureApp::GEODESICS_CURVATURE:
                ComputeCurvatureGeodesics(mCurvatureWeight, false);
                break;
            case MagicApp::MeasureApp::PRINCIPAL_CURVATURE:
                MeasurePrincipalCurvature(false);
                break;
            case MagicApp::MeasureApp::THICKNESS:
                MeasureThickness(false);
                break;
            case MagicApp::MeasureApp::SECTION_CURVE:
                ComputeSectionCurve(false);
                break;
            case MagicApp::MeasureApp::FACE_POINT_CURVE:
                ComputeFacePointCurve(false);
                break;
            case MagicApp::MeasureApp::SPLIT_MESH:
                SplitMesh(false);
                break;
            case MagicApp::MeasureApp::DETECT_PRIMITIVE:
                DetectPrimitive(false);
                break;
            default:
                break;
            }
            mpUI->StopProgressbar();
        }
    }

    bool MeasureApp::IsCommandAvaliable()
    {
        if (mIsCommandInProgress)
        {
            MessageBox(NULL, "请等待当前命令执行完", "温馨提示", MB_OK);
            return false;
        }
        return true;
    }

#if DEBUGDUMPFILE
    void MeasureApp::SetDumpInfo(GPP::DumpBase* dumpInfo)
    {        
        GPP::DumpMeshSplitInsertSplitLinesByFacePoints* splitByPofDumpInfo = dynamic_cast<GPP::DumpMeshSplitInsertSplitLinesByFacePoints*>(dumpInfo);
        if (splitByPofDumpInfo && dumpInfo->GetTriMesh(0) != NULL)
        {
            GPP::TriMesh* copiedTriMesh = GPP::CopyTriMesh(dumpInfo->GetTriMesh(0));
            //copiedTriMesh->UnifyCoords(2.0);
            copiedTriMesh->UpdateNormal();
            ModelManager::Get()->SetMesh(copiedTriMesh);
            ModelManager::Get()->ClearPointCloud();
            mCurvesOnMesh = splitByPofDumpInfo->GetFacePoints();
            mUpdateMarkRendering = true;
            mUpdateModelRendering = true;
            GPPFREEPOINTER(dumpInfo);
        }
        else if (GPP::DumpMeshMeasureSectionExact* geodDumpInfo = dynamic_cast<GPP::DumpMeshMeasureSectionExact*>(dumpInfo))
        {
            if (geodDumpInfo->GetTriMesh(0) != NULL)
            {
                GPP::TriMesh* copiedTriMesh = GPP::CopyTriMesh(dumpInfo->GetTriMesh(0));
                copiedTriMesh->UnifyCoords(2.0);
                copiedTriMesh->UpdateNormal();
                ModelManager::Get()->SetMesh(copiedTriMesh);
                ModelManager::Get()->ClearPointCloud();
                mMarkFacePoints = geodDumpInfo->GetSectionPofs();
                mUpdateMarkRendering = true;
                mUpdateModelRendering = true;
                GPPFREEPOINTER(dumpInfo);
            }
        }
        else if (GPP::DumpOptimiseCurveConnectFacePointsOnMesh* connPofDumpInfo = dynamic_cast<GPP::DumpOptimiseCurveConnectFacePointsOnMesh*>(dumpInfo))
        {
            if (connPofDumpInfo->GetTriMesh(0) != NULL)
            {
                GPP::TriMesh* copiedTriMesh = GPP::CopyTriMesh(dumpInfo->GetTriMesh(0));
                //copiedTriMesh->UnifyCoords(2.0);
                copiedTriMesh->UpdateNormal();
                ModelManager::Get()->SetMesh(copiedTriMesh);
                ModelManager::Get()->ClearPointCloud();
                mMarkFacePoints = connPofDumpInfo->GetFacePoints();
                mIsGeodesicsClose = !connPofDumpInfo->IsClosedCurve();
                mpUI->UpdateIsClosedInfo();
                mUpdateMarkRendering = true;
                mUpdateModelRendering = true;
                GPPFREEPOINTER(dumpInfo);
            }
        }
        if (ModelManager::Get()->GetMesh())
        {
            // set up pick tool
            GPPFREEPOINTER(mpPickTool);
            mpPickTool = new MagicCore::PickTool;
            mpPickTool->SetPickParameter(mRightMouseType == SELECT_VERTEX ? MagicCore::PM_POINT : MagicCore::PM_FACEPOINT, true, NULL, ModelManager::Get()->GetMesh(), "ModelNode");
            mpUI->SetModelInfo(ModelManager::Get()->GetMesh()->GetVertexCount(), ModelManager::Get()->GetMesh()->GetTriangleCount());
        }
        /*if (dumpInfo == NULL)
        {
            return;
        }
        GPPFREEPOINTER(mpDumpInfo);
        mpDumpInfo = dumpInfo;
        if (mpDumpInfo->GetTriMesh() == NULL)
        {
            return;
        }
        GPPFREEPOINTER(mpTriMeshRef);
        mpTriMeshRef = CopyTriMesh(mpDumpInfo->GetTriMesh());
        InitViewTool();
        UpdateModelRefRendering();*/
    }

    void MeasureApp::RunDumpInfo()
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
        //GPPFREEPOINTER(mpTriMeshRef);
        //mpTriMeshRef = CopyTriMesh(mpDumpInfo->GetTriMesh());
        //mpTriMeshRef->UnifyCoords(2.0);
        //mpTriMeshRef->UpdateNormal();

        //if (mpDumpInfo->GetApiName() == GPP::MESH_MEASURE_SECTION_EXACT || mpDumpInfo->GetApiName() == GPP::MESH_MEASURE_SECTION_FAST_EXACT)
        //{
        //    GPP::DumpMeshMeasureSectionExact* dumpDetails = dynamic_cast<GPP::DumpMeshMeasureSectionExact*>(mpDumpInfo);
        //    if (dumpDetails)
        //    {
        //        mRefMarkPoints = dumpDetails->GetSectionPathPoints();
        //    }
        //}

        //UpdateModelRefRendering();
        //UpdateMarkRefRendering();
        //GPPFREEPOINTER(mpDumpInfo);
    }
#endif

    bool MeasureApp::IsCommandInProgress()
    {
        return mIsCommandInProgress;
    }

    void MeasureApp::SwitchDisplayMode()
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
        mUpdateModelRendering = true;
    }

    void MeasureApp::SwitchSelectionMode()
    {
        if (mpPickTool == NULL)
        {
            return;
        }
        GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
        if (triMesh == NULL)
        {
            return;
        }

        if (mRightMouseType != SELECT_VERTEX)
        {
            mRightMouseType = SELECT_VERTEX;
            mpPickTool->SetPickParameter(MagicCore::PM_POINT, true, NULL, triMesh, "ModelNode");
            MessageBox(NULL, "目前仅能选择网格顶点", "温馨提示", MB_OK);
            
        }
        else
        {
            mRightMouseType = SELECT_FACEPOINT;
            mpPickTool->SetPickParameter(MagicCore::PM_FACEPOINT, true, NULL, triMesh, "ModelNode");
            MessageBox(NULL, "目前仅能选择网格面点", "温馨提示", MB_OK);
        }
        ClearSelectionData();
        mUpdateMarkRendering = true;
    }

    bool MeasureApp::ImportModel()
    {
        if (IsCommandAvaliable() == false)
        {
            return false;
        }
        std::string fileName;
        char filterName[] = "OBJ Files(*.obj)\0*.obj\0STL Files(*.stl)\0*.stl\0OFF Files(*.off)\0*.off\0PLY Files(*.ply)\0*.ply\0GPT Files(*.gpt)\0*.gpt\0";
        if (MagicCore::ToolKit::FileOpenDlg(fileName, filterName))
        {
            mpUI->SetGeodesicsInfo(0);
            ModelManager::Get()->ClearPointCloud();
            if (ModelManager::Get()->ImportMesh(fileName) == false)
            {
                MessageBox(NULL, "网格导入失败", "温馨提示", MB_OK);
                return false;
            }
            GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
            mpUI->SetModelInfo(triMesh->GetVertexCount(), triMesh->GetTriangleCount());
            UpdateModelRendering();
            //mMarkIds.clear();
            //mGeodesicsOnVertices.clear();
            //mMarkPoints.clear();
            ClearSelectionData();
            mMinCurvature.clear();
            mMaxCurvature.clear();
            mMinCurvatureDirs.clear();
            mMaxCurvatureDirs.clear();
            mCurvatureFlags.clear();
            mDisplayPrincipalCurvature = 0;
            UpdateMarkRendering();
            // set up pick tool
            GPPFREEPOINTER(mpPickTool);
            mpPickTool = new MagicCore::PickTool;
            mpPickTool->SetPickParameter(mRightMouseType == SELECT_VERTEX ? MagicCore::PM_POINT : MagicCore::PM_FACEPOINT, true, NULL, triMesh, "ModelNode");
            GPPFREEPOINTER(mpRefTriMesh);
            UpdateRefModelRendering();
            return true;
        }
        return false;
    }

    bool MeasureApp::ImportRefModel()
    {
        if (IsCommandAvaliable() == false)
        {
            return false;
        }
        if (ModelManager::Get()->GetMesh() == NULL)
        {
            MessageBox(NULL, "请先导入需要测量的网格", "温馨提示", MB_OK);
            return false;
        }
        std::string fileName;
        char filterName[] = "OBJ Files(*.obj)\0*.obj\0STL Files(*.stl)\0*.stl\0OFF Files(*.off)\0*.off\0PLY Files(*.ply)\0*.ply\0GPT Files(*.gpt)\0*.gpt\0";
        if (MagicCore::ToolKit::FileOpenDlg(fileName, filterName))
        {
            mpUI->SetGeodesicsInfo(0);
            ModelManager::Get()->ClearPointCloud();
            GPPFREEPOINTER(mpRefTriMesh);
            mpRefTriMesh = GPP::Parser::ImportTriMesh(fileName);
            if (mpRefTriMesh == NULL)
            {
                MessageBox(NULL, "网格导入失败", "温馨提示", MB_OK);
                return false;
            }
            mpRefTriMesh->UnifyCoords(ModelManager::Get()->GetScaleValue(), ModelManager::Get()->GetObjCenterCoord());
            mpRefTriMesh->UpdateNormal();
            mpUI->SetDistanceInfo(mpRefTriMesh->GetTriangleCount(), false, 0.0, 0.0);
            UpdateRefModelRendering();
            return true;
        }

        return false;
    }

    void MeasureApp::DeleteMeshMark()
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        if (!mMarkIds.empty())
        {
            mMarkIds.pop_back();
        }
        if (!mMarkFacePoints.empty())
        {
            mMarkFacePoints.pop_back();
        }
        mGeodesicsOnVertices.clear();
        mGeodesicsOnPofs.clear();
        mMarkPoints.clear();
        UpdateMarkRendering();
        mpUI->SetGeodesicsInfo(0);
    }

    static bool IsSameFacePointLine(const std::vector<GPP::PointOnFace>& facePointLine, const std::vector<GPP::PointOnFace>& refFacePointLine)
    {
        int pofCount = facePointLine.size();
        if (pofCount != refFacePointLine.size())
        {
            return false;
        }
        for (int pid = 0; pid < pofCount; ++pid)
        {
            const GPP::PointOnFace& pof0 = facePointLine.at(pid);
            const GPP::PointOnFace& pof1 = refFacePointLine.at(pid);
            if (pof0.mFaceId != pof1.mFaceId)
            {
                return false;
            }
            else 
            {
                for (int ii = 0; ii < 3; ++ii)
                {
                    if (abs(pof0.mCoord[ii] - pof1.mCoord[ii]) > GPP::REAL_TOL)
                    {
                        return false;
                    }
                }
            }
        }
        return true;
    }

    void MeasureApp::AddOneCurve()
    {
        if (!mGeodesicsOnPofs.empty())
        {
            if (mCurvesOnMesh.empty() || !IsSameFacePointLine(mGeodesicsOnPofs, mCurvesOnMesh.back()))
            {
                mCurvesOnMesh.push_back(mGeodesicsOnPofs);
                mGeodesicsOnPofs.clear();
                mMarkFacePoints.clear();
                mUpdateMarkRendering = true;
            }
        }

    }

    void MeasureApp::RemoveOneCurve()
    {
        if (!mCurvesOnMesh.empty())
        {
            mCurvesOnMesh.pop_back();
            mUpdateMarkRendering = true;
        }
    }

    void MeasureApp::ComputeSectionCurve(bool isSubThread)
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
        GPP::TriMeshInfo* meshInfo = ModelManager::Get()->GetMeshInfo();
        if (triMesh == NULL)
        {
            MessageBox(NULL, "请导入需要测量的网格", "温馨提示", MB_OK);
            return;
        }
        else if (mMarkIds.size() < 2 && mMarkFacePoints.size() < 2)
        {
            MessageBox(NULL, "请在测量的网格上选择标记点", "温馨提示", MB_OK);
            return;
        }

        if (isSubThread)
        {
            mCommandType = SECTION_CURVE;
            DoCommand(true);
        }
        else
        {
            mMarkPoints.clear();
            mGeodesicsOnPofs.clear();
            if (!mMarkIds.empty())
            {
                GPP::Vector3 coords[3];
                int markSize = mMarkIds.size();
                static bool useOldMethod = false;
                useOldMethod = !useOldMethod;
                for (int mid = 1; mid <= markSize; mid++)
                {
                    if (!mIsGeodesicsClose && mid == markSize)
                    {
                        break;
                    }
                    int curMarkId = mMarkIds.at(mid % markSize);
                    int preMarkId = mMarkIds.at(mid - 1);
                    std::vector<GPP::PointOnEdge> pathPointInfos;
                    coords[0] = triMesh->GetVertexCoord(preMarkId);
                    coords[1] = triMesh->GetVertexCoord(curMarkId);
                    double normalDistance = (coords[0] - coords[1]).Length();
                    coords[2] = (coords[0] + triMesh->GetVertexNormal(preMarkId) * normalDistance + 
                        coords[1] + triMesh->GetVertexNormal(curMarkId) * normalDistance) / 2.0;
                    GPP::Plane3 cuttingPlane(coords[0], coords[1], coords[2]);
                    GPP::ErrorCode res = GPP::OptimiseCurve::ConnectVertexByCuttingPlane(triMesh, preMarkId, curMarkId,
                        cuttingPlane, pathPointInfos);//, useOldMethod);
                    mIsCommandInProgress = false;
                    if (res != GPP_NO_ERROR)
                    {
                        MessageBox(NULL, "截面线计算失败", "温馨提示", MB_OK);
                        return;
                    }
                    for (int pid = 0; pid < pathPointInfos.size(); pid++)
                    {
                        GPP::PointOnEdge curPoint = pathPointInfos.at(pid);
                        mGeodesicsOnPofs.push_back(CreatePofOnEdge(curPoint, triMesh, meshInfo));
                        if (curPoint.mVertexIdEnd == -1)
                        {
                            mMarkPoints.push_back(triMesh->GetVertexCoord(curPoint.mVertexIdStart));
                        }
                        else
                        {
                            mMarkPoints.push_back(triMesh->GetVertexCoord(curPoint.mVertexIdStart) * curPoint.mWeight + 
                                triMesh->GetVertexCoord(curPoint.mVertexIdEnd) * (1.0 - curPoint.mWeight));
                        }
                    }
                }
            }
            else if (!mMarkFacePoints.empty())
            {
                GPP::Vector3 coords[3];
                int markSize = mMarkFacePoints.size();
                GPP::TriMeshInfo* meshInfo = ModelManager::Get()->GetMeshInfo();
                for (int mid = 1; mid <= markSize; mid++)
                {
                    if (!mIsGeodesicsClose && mid == markSize)
                    {
                        break;
                    }
                    const GPP::PointOnFace& curPof = mMarkFacePoints.at(mid % markSize);
                    const GPP::PointOnFace& prePof = mMarkFacePoints.at(mid - 1);
                    std::vector<GPP::PointOnEdge> pathPointInfos;
                    coords[0] = GetCoord(curPof, triMesh);
                    coords[1] = GetCoord(prePof, triMesh);
                    double normalDistance = (coords[0] - coords[1]).Length();
                    coords[2] = (coords[0] + GetNormal(prePof, triMesh) * normalDistance + 
                        coords[1] + GetCoord(curPof, triMesh) * normalDistance) / 2.0;
                    GPP::Plane3 cuttingPlane(coords[0], coords[1], coords[2]);
                    GPP::ErrorCode res = GPP::OptimiseCurve::_ConnectFacePointsByCuttingPlane(triMesh, meshInfo, prePof, curPof, cuttingPlane, pathPointInfos);
                    mIsCommandInProgress = false;
                    if (res != GPP_NO_ERROR)
                    {
                        MessageBox(NULL, "截面线计算失败", "温馨提示", MB_OK);
                        return;
                    }
                    mMarkPoints.push_back(GetCoord(prePof, triMesh));
                    mGeodesicsOnPofs.push_back(prePof);
                    for (int pid = 0; pid < pathPointInfos.size(); pid++)
                    {
                        const GPP::PointOnEdge& curPoint = pathPointInfos.at(pid);
                        mMarkPoints.push_back(GetCoord(curPoint, triMesh));
                        mGeodesicsOnPofs.push_back(CreatePofOnEdge(curPoint, triMesh, meshInfo));
                    }
                }
                if (!mIsGeodesicsClose)
                {
                    mMarkPoints.push_back(GetCoord(mMarkFacePoints.back(), triMesh));
                    mGeodesicsOnPofs.push_back(mMarkFacePoints.back());
                }
                else
                {
                    mMarkPoints.push_back(mMarkPoints.front());
                    mGeodesicsOnPofs.push_back(mMarkFacePoints.front());
                }
            }
            double distance = 0.0;
            for (int mid = 1; mid < mMarkPoints.size(); mid++)
            {
                distance += (mMarkPoints.at(mid) - mMarkPoints.at(mid - 1)).Length();
            }
            mpUI->SetGeodesicsInfo(distance / ModelManager::Get()->GetScaleValue());
            mUpdateMarkRendering = true;
        }
    }

    void MeasureApp::ComputeFacePointCurve(bool isSubThread)
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
        if (triMesh == NULL)
        {
            MessageBox(NULL, "请导入需要测量的网格", "温馨提示", MB_OK);
            return;
        }
        else if (mMarkFacePoints.size() < 2 && mMarkIds.size() < 2)
        {
            MessageBox(NULL, "请在测量的网格上选择标记点", "温馨提示", MB_OK);
            return;
        }

        if (isSubThread)
        {
            mCommandType = FACE_POINT_CURVE;
            DoCommand(true);
        }
        else
        {
            mMarkPoints.clear();
            mGeodesicsOnPofs.clear();
            if (!mMarkFacePoints.empty())
            {
                GPP::Real averageEdgeLength;
                GPP::ErrorCode res = GPP::CalculateAverageEdgeLength(triMesh, averageEdgeLength);
#if MAKEDUMPFILE
                GPP::DumpOnce();
#endif
                res = GPP::OptimiseCurve::ConnectFacePointsOnMesh(triMesh, mMarkFacePoints, mIsGeodesicsClose, averageEdgeLength, mGeodesicsOnPofs);
                if (res != GPP_NO_ERROR)
                {
                    MessageBox(NULL, "网格曲线计算失败", "温馨提示", MB_OK);
                    return;
                }
                for (int pid = 0; pid < mGeodesicsOnPofs.size(); ++pid)
                {
                    mMarkPoints.push_back(GetCoord(mGeodesicsOnPofs.at(pid), triMesh));
                }
            }
            else if (!mMarkIds.empty())
            {
                //GPP::TriMeshInfo meshInfo;
                //GPP::ErrorCode res = meshInfo.CacheData(triMesh, GPP::TriMeshInfo::TOPOLOGY_EDGEINFOS_TWOWAYS);
                //if (res != GPP_NO_ERROR)
                //{
                //    MessageBox(NULL, "网格曲线计算失败", "温馨提示", MB_OK);
                //    return;
                //}
                GPP::TriMeshInfo* meshInfo = ModelManager::Get()->GetMeshInfo();
                GPP::Real averageEdgeLength;
                GPP::ErrorCode res = GPP::CalculateAverageEdgeLength(triMesh, averageEdgeLength);
                res = meshInfo->CacheDataForVertexNbrFaceMaps(triMesh);
                std::vector<GPP::PointOnFace> onVertexPofs(mMarkIds.size());
                for (int pid = 0; pid < mMarkIds.size(); ++pid)
                {
                    onVertexPofs.at(pid) = CreatePofOnVertex(mMarkIds.at(pid), triMesh, meshInfo);
                }
#if MAKEDUMPFILE
                GPP::DumpOnce();
#endif
                res = GPP::OptimiseCurve::ConnectFacePointsOnMesh(triMesh, onVertexPofs, mIsGeodesicsClose, averageEdgeLength, mGeodesicsOnPofs);
                if (res != GPP_NO_ERROR)
                {
                    MessageBox(NULL, "网格曲线计算失败", "温馨提示", MB_OK);
                    return;
                }
                for (int pid = 0; pid < mGeodesicsOnPofs.size(); ++pid)
                {
                    mMarkPoints.push_back(GetCoord(mGeodesicsOnPofs.at(pid), triMesh));
                }
            }
        
            double distance = 0.0;
            for (int mid = 1; mid < mMarkPoints.size(); mid++)
            {
                distance += (mMarkPoints.at(mid) - mMarkPoints.at(mid - 1)).Length();
            }
            mpUI->SetGeodesicsInfo(distance / ModelManager::Get()->GetScaleValue());
            mUpdateMarkRendering = true;
        }
    }

    void MeasureApp::ComputeOffsetCurve(double offsetSize)
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
        if (triMesh == NULL)
        {
            MessageBox(NULL, "请导入需要测量的网格", "温馨提示", MB_OK);
            return;
        }
        else if (mGeodesicsOnVertices.empty())
        {
            MessageBox(NULL, "请在测量的网格上选择中心曲线", "温馨提示", MB_OK);
            return;
        }
        std::vector<std::vector<GPP::PointOnFace> > topCurve, downCurve;
#if MAKEDUMPFILE
        GPP::DumpOnce();
#endif
        GPP::ErrorCode res = GPP::OptimiseCurve::ApproximateOffsetCurveOnMesh(triMesh, mGeodesicsOnVertices, mIsGeodesicsClose, 
            offsetSize, topCurve, downCurve);
        if (res != GPP_NO_ERROR)
        {
            MessageBox(NULL, "等距曲线计算失败", "温馨提示", MB_OK);
            return;
        }
        for (std::vector<std::vector<GPP::PointOnFace> >::iterator citr = topCurve.begin(); citr != topCurve.end(); ++citr)
        {
            mMarkPoints.push_back(GetCoord(citr->at(0), triMesh));
        }
        for (std::vector<std::vector<GPP::PointOnFace> >::iterator citr = downCurve.begin(); citr != downCurve.end(); ++citr)
        {
            mMarkPoints.push_back(GetCoord(citr->at(0), triMesh));
        }
        mUpdateMarkRendering = true;
    }

    static bool AssignVertexColorBySegmentIds(GPP::TriMesh* triMesh, const std::vector<GPP::Int>& vertexSegIds)
    {
        GPP::Int maxId = *std::max_element(vertexSegIds.begin(), vertexSegIds.end());
        if (maxId == 0)
        {
            return false;
        }

        GPP::Int vertexCount = triMesh->GetVertexCount();
        triMesh->SetHasVertexColor(true);
        for (GPP::Int vid = 0; vid < vertexCount; ++vid)
        {
            GPP::Int id = vertexSegIds.at(vid);
            if (id == 0)
            {
                GPP::Vector3 color(0.86, 0.86, 0.86);
                triMesh->SetVertexColor(vid, color);
            }
            else
            {
                GPP::Vector3 color = GPP::ColorCoding(1.0 * id / maxId + 0.2);
                triMesh->SetVertexColor(vid, color);
            }
        }
        return true;
    }

    void MeasureApp::SplitMesh(bool isSubThread)
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
        if (triMesh == NULL)
        {
            MessageBox(NULL, "请导入需要编辑的网格", "温馨提示", MB_OK);
            return;
        }
        else if (mGeodesicsOnPofs.empty() && mCurvesOnMesh.empty() && mGeodesicsOnVertices.empty())
        {
            MessageBox(NULL, "请先生成需要剪切的曲线", "温馨提示", MB_OK);
            return;
        }
        if (isSubThread)
        {
            mCommandType = SPLIT_MESH;
            DoCommand(true);
        }
        else
        {
            std::vector<std::vector<GPP::Int> > splitLines;
#if MAKEDUMPFILE
            GPP::DumpOnce();
#endif
            GPP::ErrorCode res = GPP_NO_ERROR;
            // collect texture coordinates
            std::vector<GPP::Vector3> texCoords;
            if (triMesh->HasTriangleTexCoord())
            {
                int faceCount = triMesh->GetTriangleCount();
                texCoords.resize(faceCount * 3);
                for (int fid = 0; fid < faceCount; ++fid)
                {
                    for (int fvid = 0; fvid < 3; ++fvid)
                    {
                        texCoords.at(fid * 3 + fvid) = triMesh->GetTriangleTexcoord(fid, fvid);
                    }
                }
            }
            std::vector<GPP::Vector3> newTexCoords;
            if (!mCurvesOnMesh.empty())
            {
                res = GPP::SplitMesh::InsertSplitLinesByFacePoints(triMesh, mCurvesOnMesh, 
                     &splitLines, texCoords.empty() ? NULL : &texCoords, &newTexCoords);
            }
            else if (!mGeodesicsOnPofs.empty())
            {
                std::vector<std::vector<GPP::PointOnFace> > curve(1, mGeodesicsOnPofs);
                res = GPP::SplitMesh::InsertSplitLinesByFacePoints(triMesh, curve, 
                    &splitLines, texCoords.empty() ? NULL : &texCoords, &newTexCoords);
            }
            else // !mGeodesicsOnVertices.empty()
            {
                splitLines.push_back(mGeodesicsOnVertices);
            }
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "切割网格失败", "温馨提示", MB_OK);
                return;
            }

            if (!newTexCoords.empty())
            {
                int faceCount = triMesh->GetTriangleCount();
                for (int fid = 0; fid < faceCount; ++fid)
                {
                    for (int fvid = 0; fvid < 3; ++fvid)
                    {
                        triMesh->SetTriangleTexcoord(fid, fvid, newTexCoords.at(fid * 3 + fvid));
                    }
                }
            }
#if MAKEDUMPFILE
            GPP::DumpOnce();
#endif
            res = GPP::SplitMesh::SplitByLines(triMesh, splitLines);
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "生成切割后的网格失败", "温馨提示", MB_OK);
                return;
            }
            triMesh->UpdateNormal();

            // segment the triMesh by the connection infos
            std::vector<GPP::Int> vertexSegIds;
            res = GPP::SegmentTriMeshByConnection(triMesh, NULL, vertexSegIds);
            if (res != GPP_NO_ERROR)
            {
                return;
            }
            std::vector<GPP::Vector3> markPointBackup;
            AssignVertexColorBySegmentIds(triMesh, vertexSegIds);
            ModelManager::Get()->SetMeshInfo(NULL);

            ClearSelectionData();
            mUpdateMarkRendering = true;
            mUpdateModelRendering = true;
        }
    }

    void MeasureApp::ComputeApproximateGeodesics(bool isSubThread)
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
        if (triMesh == NULL)
        {
            MessageBox(NULL, "请导入需要测量的网格", "温馨提示", MB_OK);
            return;
        }
        else if (mMarkIds.size() < 2 && mMarkFacePoints.size() < 2)
        {
            MessageBox(NULL, "请在测量的网格上选择标记点", "温馨提示", MB_OK);
            return;
        }
        if (isSubThread)
        {
            mCommandType = GEODESICS_APPROXIMATE;
            DoCommand(true);
        }
        else
        {
            mGeodesicsOnVertices.clear();
            mGeodesicsOnPofs.clear();
            GPP::Real distance = 0;
#if MAKEDUMPFILE
            GPP::DumpOnce();
#endif
            mIsCommandInProgress = true;
            GPP::ErrorCode res = GPP_NO_ERROR;
            if (!mMarkIds.empty())
            {
                res = GPP::MeasureMesh::ComputeApproximateGeodesics(triMesh, mMarkIds, mIsGeodesicsClose, mGeodesicsOnVertices, distance);
                mMarkPoints.clear();
                if (res == GPP_NO_ERROR)
                {
                    for (std::vector<GPP::Int>::iterator pathItr = mGeodesicsOnVertices.begin(); pathItr != mGeodesicsOnVertices.end(); ++pathItr)
                    {
                        mMarkPoints.push_back(triMesh->GetVertexCoord(*pathItr));
                    }
                    if (mIsGeodesicsClose)
                    {
                        mMarkPoints.push_back(mMarkPoints.front());
                        mGeodesicsOnVertices.push_back(mGeodesicsOnVertices.front());
                    }
                }
            }
            else if (!mMarkFacePoints.empty())
            {
                GPP::Real totalDistance = 0.0;
                int sections = mMarkFacePoints.size();
                if (!mIsGeodesicsClose)
                {
                    sections--;
                }
                mMarkPoints.clear();
                mGeodesicsOnPofs.clear();
                GPP::TriMeshInfo* meshInfo = ModelManager::Get()->GetMeshInfo();
                for (int sid = 0; sid < sections; ++sid)
                {
                    int nextId = sid + 1;
                    if (mIsGeodesicsClose && sid == sections - 1)
                    {
                        nextId = 0;
                    }
                    const GPP::PointOnFace& startPof = mMarkFacePoints.at(sid);
                    const GPP::PointOnFace& endPof = mMarkFacePoints.at(nextId);
                    std::vector<GPP::Int> internalPathIds;
                    res = GPP::MeasureMesh::_ComputeApproximateGeodesics(triMesh, meshInfo, startPof, endPof, internalPathIds, distance);
                    if (res != GPP_NO_ERROR)
                    {
                        MessageBox(NULL, "测量失败", "温馨提示", MB_OK);
                        return;
                    }
                    mMarkPoints.push_back(GetCoord(startPof, triMesh));
                    mGeodesicsOnPofs.push_back(startPof);
                    for (int pid = 0; pid < internalPathIds.size(); ++pid)
                    {
                        GPP::Int vertexId = internalPathIds.at(pid);
                        mMarkPoints.push_back(triMesh->GetVertexCoord(vertexId));
                        mGeodesicsOnPofs.push_back(CreatePofOnVertex(vertexId, triMesh, meshInfo));
                    }
                }
                if (mIsGeodesicsClose)
                {
                    mMarkPoints.push_back(GetCoord(mMarkFacePoints.front(), triMesh));
                    mGeodesicsOnPofs.push_back(mMarkFacePoints.front());
                }
                else
                {
                    mMarkPoints.push_back(GetCoord(mMarkFacePoints.back(), triMesh));
                    mGeodesicsOnPofs.push_back(mMarkFacePoints.back());
                }
            }
            mIsCommandInProgress = false;
            if (res == GPP_API_IS_NOT_AVAILABLE)
            {
                MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
                MagicCore::ToolKit::Get()->SetAppRunning(false);
            }
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "测量失败", "温馨提示", MB_OK);
                return;
            }
            mpUI->SetGeodesicsInfo(distance / ModelManager::Get()->GetScaleValue());
            mUpdateMarkRendering = true;
        }
    }

    void MeasureApp::ComputeCurvatureGeodesics(double curvatureWeight, bool isSubThread)
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
        int vertexCount = triMesh->GetVertexCount();
        if (triMesh == NULL)
        {
            MessageBox(NULL, "请导入需要测量的网格", "温馨提示", MB_OK);
            return;
        }
        else if (mMarkIds.size() < 2)
        {
            if (mMarkFacePoints.size() > 0)
            {
                MessageBox(NULL, "该命令仅支持网格顶点，请调整选择模式", "温馨提示", MB_OK);
                return;
            }
            else
            {
                MessageBox(NULL, "请在测量的网格上选择标记点", "温馨提示", MB_OK);
            }
            return;
        }
        if (isSubThread)
        {
            mCommandType = GEODESICS_CURVATURE;
            mCurvatureWeight = curvatureWeight;
            DoCommand(true);
        }
        else
        {
#if MAKEDUMPFILE
            GPP::DumpOnce();
#endif
            mIsCommandInProgress = true;
            GPP::TriangleList triangleList(triMesh);
            int vertexCount = triMesh->GetVertexCount();
            if (mCurvatureFlags.empty())
            {
                mMaxCurvature.resize(vertexCount, 0);
                mMinCurvature.resize(vertexCount, 0);
                mMaxCurvatureDirs.resize(vertexCount);
                mMinCurvatureDirs.resize(vertexCount);
                mCurvatureFlags.resize(vertexCount, 0);
            }
            GPP::PrincipalCurvatureDistance dirDistance(&triangleList, &mMinCurvature, &mMaxCurvature, 
                &mMinCurvatureDirs, &mMaxCurvatureDirs, &mCurvatureFlags, false, curvatureWeight);
            GPP::Real maxDistance = 0;
            std::vector<GPP::Int> maxGeodesics;
            GPP::ErrorCode res = GPP::MeasureMesh::ComputeApproximateGeodesics(triMesh, mMarkIds, mIsGeodesicsClose, 
                maxGeodesics, maxDistance, &dirDistance);
            if (res == GPP_API_IS_NOT_AVAILABLE)
            {
                MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
                MagicCore::ToolKit::Get()->SetAppRunning(false);
            }
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "测量失败", "温馨提示", MB_OK);
                return;
            }

            dirDistance.SetMinDir(true);
            GPP::Real minDistance = 0;
            std::vector<GPP::Int> minGeodesics;
            res = GPP::MeasureMesh::ComputeApproximateGeodesics(triMesh, mMarkIds, mIsGeodesicsClose, minGeodesics, 
                minDistance, &dirDistance);
            mIsCommandInProgress = false;
            if (res == GPP_API_IS_NOT_AVAILABLE)
            {
                MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
                MagicCore::ToolKit::Get()->SetAppRunning(false);
            }
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "测量失败", "温馨提示", MB_OK);
                return;
            }

            InfoLog << "    MinDistance=" << minDistance << " maxDistance=" << maxDistance << std::endl;
            if (minDistance < maxDistance)
            {
                mpUI->SetGeodesicsInfo(minDistance / ModelManager::Get()->GetScaleValue());
                mGeodesicsOnVertices.swap(minGeodesics);
                InfoLog << " Min Dir" << std::endl;
            }
            else
            {
                mpUI->SetGeodesicsInfo(maxDistance / ModelManager::Get()->GetScaleValue());
                mGeodesicsOnVertices.swap(maxGeodesics);
                InfoLog << " Max Dir" << std::endl;
            }
            
            mMarkPoints.clear();
            for (std::vector<GPP::Int>::iterator pathItr = mGeodesicsOnVertices.begin(); pathItr != mGeodesicsOnVertices.end(); ++pathItr)
            {
                mMarkPoints.push_back(triMesh->GetVertexCoord(*pathItr));
            }
            mUpdateMarkRendering = true;
        }
    }

    void MeasureApp::SmoothGeodesicsOnVertex()
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        if (mGeodesicsOnVertices.empty())
        {
            return;
        }
        GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
        if (triMesh == NULL)
        {
            MessageBox(NULL, "请导入网格", "温馨提示", MB_OK);
            return;
        }
#if MAKEDUMPFILE
        GPP::DumpOnce();
#endif
        std::vector<int> optimizedCurve;
        GPP::ErrorCode res = GPP::OptimiseCurve::SmoothCurveOnMesh(triMesh, mGeodesicsOnVertices, 
            GPP::ONE_RADIAN * 60, 0.2, 10, true, &optimizedCurve);
        if (res == GPP_API_IS_NOT_AVAILABLE)
        {
            MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
            MagicCore::ToolKit::Get()->SetAppRunning(false);
        }
        if (res != GPP_NO_ERROR)
        {
            MessageBox(NULL, "顶点上的测地线优化失败", "温馨提示", MB_OK);
            return;
        }
        mGeodesicsOnVertices.swap(optimizedCurve);
        mMarkPoints.clear();
        for (std::vector<GPP::Int>::iterator pathItr = mGeodesicsOnVertices.begin(); pathItr != mGeodesicsOnVertices.end(); ++pathItr)
        {
            mMarkPoints.push_back(triMesh->GetVertexCoord(*pathItr));
        }
        if (mIsGeodesicsClose && (!mGeodesicsOnVertices.empty()))
        {
            mMarkPoints.push_back(triMesh->GetVertexCoord(mGeodesicsOnVertices.at(0)));
        }
        double distance = 0.0;
        for (int mid = 1; mid < mMarkPoints.size(); mid++)
        {
            distance += (mMarkPoints.at(mid) - mMarkPoints.at(mid - 1)).Length();
        }
        mpUI->SetGeodesicsInfo(distance / ModelManager::Get()->GetScaleValue());
        mUpdateModelRendering = true;
        mUpdateMarkRendering = true;
    }

    void MeasureApp::SmoothGeodesicsCrossVertex()
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        if (mGeodesicsOnVertices.empty())
        {
            return;
        }
        GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
        if (triMesh == NULL)
        {
            MessageBox(NULL, "请导入网格", "温馨提示", MB_OK);
            return;
        }
#if MAKEDUMPFILE
        GPP::DumpOnce();
#endif
        std::vector<int> simpleCurve;
        GPP::ErrorCode res = GPP::OptimiseCurve::ExtractSimpleCurve(mGeodesicsOnVertices, simpleCurve);
        if (res != GPP_NO_ERROR)
        {
            MessageBox(NULL, "顶点上的测地线优化失败", "温馨提示", MB_OK);
            return;
        }
        mGeodesicsOnVertices.swap(simpleCurve);
        std::vector<GPP::PointOnEdge> curve;
        for (std::vector<int>::iterator itr = mGeodesicsOnVertices.begin(); itr != mGeodesicsOnVertices.end(); ++itr)
        {
            curve.push_back(GPP::PointOnEdge(*itr, -1, 1));
        }
        std::vector<GPP::PointOnEdge> smoothCurve;
        res = GPP::OptimiseCurve::SmoothCurveCrossMesh(triMesh, curve, 0.75, 20, smoothCurve);
        if (res == GPP_API_IS_NOT_AVAILABLE)
        {
            MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
            MagicCore::ToolKit::Get()->SetAppRunning(false);
        }
        if (res != GPP_NO_ERROR)
        {
            MessageBox(NULL, "顶点上的测地线优化失败", "温馨提示", MB_OK);
            return;
        }
        mMarkPoints.clear();
        for (std::vector<GPP::PointOnEdge>::iterator itr = smoothCurve.begin(); itr != smoothCurve.end(); ++itr)
        {
            if (itr->mVertexIdEnd == -1)
            {
                mMarkPoints.push_back(triMesh->GetVertexCoord(itr->mVertexIdStart));
            }
            else
            {
                mMarkPoints.push_back(triMesh->GetVertexCoord(itr->mVertexIdStart) * itr->mWeight + 
                    triMesh->GetVertexCoord(itr->mVertexIdEnd) * (1.0 - itr->mWeight));
            }
        }
        if (mIsGeodesicsClose && !smoothCurve.empty())
        {
            GPP::PointOnEdge firstPoint = smoothCurve.at(0);
            if (firstPoint.mVertexIdEnd == -1)
            {
                mMarkPoints.push_back(triMesh->GetVertexCoord(firstPoint.mVertexIdStart));
            }
            else
            {
                mMarkPoints.push_back(triMesh->GetVertexCoord(firstPoint.mVertexIdStart) * firstPoint.mWeight + 
                    triMesh->GetVertexCoord(firstPoint.mVertexIdEnd) * (1.0 - firstPoint.mWeight));
            }
        }
        double distance = 0.0;
        for (int mid = 1; mid < mMarkPoints.size(); mid++)
        {
            distance += (mMarkPoints.at(mid) - mMarkPoints.at(mid - 1)).Length();
        }
        mpUI->SetGeodesicsInfo(distance / ModelManager::Get()->GetScaleValue());
        mUpdateModelRendering = true;
        mUpdateMarkRendering = true;
    }
 
    void MeasureApp::FastComputeExactGeodesics(double accuracy, bool isSubThread)
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
        GPP::TriMeshInfo* meshInfo = ModelManager::Get()->GetMeshInfo();
        if (triMesh == NULL)
        {
            MessageBox(NULL, "请导入需要测量的网格", "温馨提示", MB_OK);
            return;
        }
        else if (mMarkIds.size() < 2 && mMarkFacePoints.size() < 2)
        {
            MessageBox(NULL, "请在测量的网格上选择标记点", "温馨提示", MB_OK);
            return;
        }
        if (isSubThread)
        {
            mCommandType = GEODESICS_FAST_EXACT;
            mGeodesicAccuracy = accuracy;
            DoCommand(true);
        }
        else
        {
            std::vector<GPP::Vector3> pathPoints;
            std::vector<GPP::PointOnEdge> pathInfos;
            mGeodesicsOnPofs.clear();
            GPP::Real distance = 0;
            GPP::ErrorCode res = GPP_NO_ERROR;
#if MAKEDUMPFILE
            GPP::DumpOnce();
#endif
            mIsCommandInProgress = true;
            if (!mMarkIds.empty())
            {
                res = GPP::MeasureMesh::FastComputeExactGeodesics(triMesh, mMarkIds, mIsGeodesicsClose, 
                    pathPoints, distance, &pathInfos, accuracy);
                if (res == GPP_NO_ERROR)
                {
                    for (int pid = 0; pid < pathInfos.size(); ++pid)
                    {
                        mGeodesicsOnPofs.push_back(CreatePofOnEdge(pathInfos.at(pid), triMesh, meshInfo));
                    }
                }
            }
            else if (!mMarkFacePoints.empty())
            {
                //res = GPP::MeasureMesh::FastComputeExactGeodesics(triMesh, mMarkFacePoints, mIsGeodesicsClose,
                //    pathPoints, distance, NULL, accuracy);
                res = GPP::MeasureMesh::FastComputeExactGeodesics(triMesh, mMarkFacePoints, 
                    mIsGeodesicsClose, pathPoints, distance, &mGeodesicsOnPofs, accuracy);
                if (res == GPP_NO_ERROR)
                {
                    std::vector<GPP::Vector3> pathPts(mGeodesicsOnPofs.size());
                    for (int pid = 0; pid < mGeodesicsOnPofs.size(); ++pid)
                    {
                        pathPts.at(pid) = GetCoord(mGeodesicsOnPofs.at(pid), triMesh);
                    }
                    pathPoints.swap(pathPts);
                }
            }
            mIsCommandInProgress = false;
            if (res == GPP_API_IS_NOT_AVAILABLE)
            {
                MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
                MagicCore::ToolKit::Get()->SetAppRunning(false);
            }
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "测量失败", "温馨提示", MB_OK);
                return;
            }
            mpUI->SetGeodesicsInfo(distance / ModelManager::Get()->GetScaleValue());
            mMarkPoints.clear();
            mMarkPoints.swap(pathPoints);
            mUpdateMarkRendering = true;
        }
    }

    void MeasureApp::ComputeExactGeodesics(bool isSubThread)
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
        if (triMesh == NULL)
        {
            MessageBox(NULL, "请导入需要测量的网格", "温馨提示", MB_OK);
            return;
        }
        else if (mMarkIds.size() < 2)
        {            
            if (mMarkFacePoints.size() > 0)
            {
                MessageBox(NULL, "该命令仅支持网格顶点，请调整选择模式", "温馨提示", MB_OK);
                return;
            }
            else
            {
                MessageBox(NULL, "请在测量的网格上选择标记点", "温馨提示", MB_OK);
                return;
            }
        }
        else if (triMesh->GetVertexCount() > 200000 && isSubThread)
        {
            if (MessageBox(NULL, "测量网格顶点大于200k，测量时间会比较长，是否继续？", "温馨提示", MB_OKCANCEL) != IDOK)
            {
                return;
            }
        }
        if (isSubThread)
        {
            mCommandType = GEODESICS_EXACT;
            DoCommand(true);
        }
        else
        {
            std::vector<GPP::Vector3> pathPoints;
            std::vector<GPP::PointOnEdge> pathInfos;
            GPP::Real distance = 0;
#if MAKEDUMPFILE
            GPP::DumpOnce();
#endif
            mIsCommandInProgress = true;
            GPP::ErrorCode res = GPP::MeasureMesh::ComputeExactGeodesics(triMesh, mMarkIds, mIsGeodesicsClose, pathPoints, distance, &pathInfos);
            mIsCommandInProgress = false;
            if (res == GPP_API_IS_NOT_AVAILABLE)
            {
                MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
                MagicCore::ToolKit::Get()->SetAppRunning(false);
            }
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "测量失败", "温馨提示", MB_OK);
                return;
            }
            mpUI->SetGeodesicsInfo(distance / ModelManager::Get()->GetScaleValue());
            mMarkPoints.clear();
            mMarkPoints.swap(pathPoints);
            mUpdateMarkRendering = true;
        }
    }

    void MeasureApp::MeasureArea()
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
        if (triMesh == NULL)
        {
            MessageBox(NULL, "请导入需要测量的网格", "温馨提示", MB_OK);
            return;
        }
        GPP::Real area = 0;
        GPP::ErrorCode res = GPP::MeasureMesh::ComputeArea(triMesh, area);
        if (res == GPP_API_IS_NOT_AVAILABLE)
        {
            MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
            MagicCore::ToolKit::Get()->SetAppRunning(false);
        }
        if (res != GPP_NO_ERROR)
        {
            return;
        }
        mpUI->SetModelArea(area / ModelManager::Get()->GetScaleValue() / ModelManager::Get()->GetScaleValue());
    }

    void MeasureApp::MeasureVolume()
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
        if (triMesh == NULL)
        {
            MessageBox(NULL, "请导入需要测量的网格", "温馨提示", MB_OK);
            return;
        }
        GPP::Real volume = 0;
        GPP::ErrorCode res = GPP::MeasureMesh::ComputeVolume(triMesh, volume);
        if (res == GPP_API_IS_NOT_AVAILABLE)
        {
            MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
            MagicCore::ToolKit::Get()->SetAppRunning(false);
        }
        if (res != GPP_NO_ERROR)
        {
            return;
        }
        mpUI->SetModelVolume(volume / ModelManager::Get()->GetScaleValue() / ModelManager::Get()->GetScaleValue() / ModelManager::Get()->GetScaleValue());
    }

    void MeasureApp::MeasureMeanCurvature()
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
        if (triMesh == NULL)
        {
            MessageBox(NULL, "请导入需要测量的网格", "温馨提示", MB_OK);
            return;
        }
        std::vector<GPP::Real> curvature;
        GPP::ErrorCode res = GPP::MeasureMesh::ComputeMeanCurvature(triMesh, curvature);
        if (res == GPP_API_IS_NOT_AVAILABLE)
        {
            MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
            MagicCore::ToolKit::Get()->SetAppRunning(false);
        }
        if (res != GPP_NO_ERROR)
        {
            return;
        }
        GPP::Int vertexCount = triMesh->GetVertexCount();
        triMesh->SetHasVertexColor(true);
        for (GPP::Int vid = 0; vid < vertexCount; vid++)
        {
            triMesh->SetVertexColor(vid, MagicCore::ToolKit::ColorCoding(0.6 + curvature.at(vid) / 10.0));
        }
        mDisplayPrincipalCurvature = 0;
        UpdateMarkRendering();
        UpdateModelRendering();
    }

    void MeasureApp::MeasureGaussianCurvature()
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
        if (triMesh == NULL)
        {
            MessageBox(NULL, "请导入需要测量的网格", "温馨提示", MB_OK);
            return;
        }
        std::vector<GPP::Real> curvature;
        GPP::ErrorCode res = GPP::MeasureMesh::ComputeGaussCurvature(triMesh, curvature);
        if (res == GPP_API_IS_NOT_AVAILABLE)
        {
            MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
            MagicCore::ToolKit::Get()->SetAppRunning(false);
        }
        if (res != GPP_NO_ERROR)
        {
            return;
        }

        std::vector<GPP::Real> curvatureCopy = curvature;
        for (std::vector<GPP::Real>::iterator citr = curvatureCopy.begin(); citr != curvatureCopy.end(); ++citr)
        {
            if (*citr < 0)
            {
                *citr *= -1.0;
            }
        }
        int halfPointId = ceil(double(curvatureCopy.size()) / 2.0) - 1;
        std::nth_element(curvatureCopy.begin(), curvatureCopy.begin() + halfPointId, curvatureCopy.end());
        double midCurvature = curvatureCopy.at(halfPointId);
        if (midCurvature < GPP::REAL_TOL)
        {
            midCurvature = 1.0;
        }
        GPP::Int vertexCount = triMesh->GetVertexCount();
        triMesh->SetHasVertexColor(true);
        for (GPP::Int vid = 0; vid < vertexCount; vid++)
        {
            triMesh->SetVertexColor(vid, MagicCore::ToolKit::ColorCoding(curvature.at(vid) / midCurvature * 0.05 + 0.6));
        }
        mDisplayPrincipalCurvature = 0;
        UpdateMarkRendering();
        UpdateModelRendering();
    }

    void MeasureApp::MeasurePrincipalCurvature(bool isSubThread)
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
        if (triMesh == NULL)
        {
            MessageBox(NULL, "请导入需要测量的网格", "温馨提示", MB_OK);
            return;
        }
        if (isSubThread)
        {
            mCommandType = PRINCIPAL_CURVATURE;
            DoCommand(true);
        }
        else
        {
            if (mDisplayPrincipalCurvature != 0 && triMesh->GetVertexCount() == mMinCurvature.size())
            {
                mDisplayPrincipalCurvature *= -1;
            }
            else
            {
#if MAKEDUMPFILE
                GPP::DumpOnce();
#endif
                GPP::ErrorCode res = GPP::MeasureMesh::ComputePrincipalCurvature(triMesh, mMinCurvature, mMaxCurvature, 
                    mMinCurvatureDirs, mMaxCurvatureDirs);
                if (res == GPP_API_IS_NOT_AVAILABLE)
                {
                    MessageBox(NULL, "软件试用时限到了，欢迎购买激活码", "温馨提示", MB_OK);
                    MagicCore::ToolKit::Get()->SetAppRunning(false);
                }
                if (res != GPP_NO_ERROR)
                {
                    MessageBox(NULL, "主曲率计算失败", "温馨提示", MB_OK);
                    return;
                }
                mCurvatureFlags.clear();
                mCurvatureFlags.resize(triMesh->GetVertexCount(), 1);
                mDisplayPrincipalCurvature = 1;
            }
            mUpdateMarkRendering = true;
        }
    }

    void MeasureApp::MeasureThickness(bool isSubThread)
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        if (ModelManager::Get()->GetMesh() == NULL)
        {
            MessageBox(NULL, "请先导入需要测量的网格", "温馨提示", MB_OK);
            return;
        }

        if (isSubThread)
        {
            mCommandType = THICKNESS;
            DoCommand(true);
        }
        else
        {
            GPP::TriMesh* measureMesh = ModelManager::Get()->GetMesh();
            std::vector<GPP::Real> thickness;
#if MAKEDUMPFILE
            GPP::DumpOnce();
#endif
            GPP::ErrorCode res = GPP::MeasureMesh::ComputeThickness(measureMesh, thickness);
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "计算厚度失败", "温馨提示", MB_OK);
                return ;
            }

            measureMesh->SetHasVertexColor(true);
            GPP::Real maxValue = *std::max_element(thickness.begin(), thickness.end());
            GPP::Real minValue = *std::min_element(thickness.begin(), thickness.end());
            GPP::Real unChangedMaxValue = maxValue;
            if (maxValue < GPP::REAL_TOL)
            {
                maxValue = 1.0;
            }
            for (GPP::Int vid = 0; vid < measureMesh->GetVertexCount(); ++vid)
            {
                GPP::Real dist = thickness.at(vid);
                GPP::Vector3 color = MagicCore::ToolKit::ColorCoding(dist + 0.2);
                measureMesh->SetVertexColor(vid, color);
            }
            GPP::Int halfVId = thickness.size() / 2;
            std::nth_element(thickness.begin(), thickness.begin() + halfVId, thickness.end());
            GPP::Real midValue = thickness.at(halfVId);
            InfoLog << "Median thickness is: " << midValue << std::endl;
            mpUI->SetThicknessInfo(true, midValue / ModelManager::Get()->GetScaleValue());

            mUpdateModelRendering = true;
        }
    }

    void MeasureApp::ComputePointsToMeshDistance(bool isSubThread)
    {
        if (IsCommandAvaliable() == false)
        {
            return;
        }
        if (ModelManager::Get()->GetMesh() == NULL)
        {
            MessageBox(NULL, "请先导入需要测量的网格", "温馨提示", MB_OK);
            return;
        }
        if (mpRefTriMesh == NULL)
        {
            MessageBox(NULL, "请先导入参考网格", "温馨提示", MB_OK);
            return;
        }

        if (isSubThread)
        {
            mCommandType = DISTANCE_POINTS_TO_MESH;
            DoCommand(true);
        }
        else
        {
            GPP::TriMesh* measureMesh = ModelManager::Get()->GetMesh();
            std::vector<GPP::Vector3> points(measureMesh->GetVertexCount());
            for (GPP::Int pid = 0; pid < points.size(); ++pid)
            {
                points.at(pid) = measureMesh->GetVertexCoord(pid);
            }
            std::vector<GPP::Real> distances;
            GPP::MeshQueryTool queryTool;
            GPP::ErrorCode res = queryTool.Init(mpRefTriMesh);
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "网格距离工具初始化失败", "温馨提示", MB_OK);
                return;
            }
#if MAKEDUMPFILE
            GPP::DumpOnce();
#endif
            res = queryTool.QueryNearestTriangles(points, NULL, &distances);
            if (res != GPP_NO_ERROR)
            {
                MessageBox(NULL, "距离计算失败", "温馨提示", MB_OK);
                return;
            }
            measureMesh->SetHasVertexColor(true);
            GPP::Real maxValue = *std::max_element(distances.begin(), distances.end());
            GPP::Real minValue = *std::min_element(distances.begin(), distances.end());
            std::vector<GPP::Real> distancesCopy = distances;
            int halfPointId = ceil(double(distancesCopy.size()) / 2.0) - 1;
            std::nth_element(distancesCopy.begin(), distancesCopy.begin() + halfPointId, distancesCopy.end());
            double midDist = distancesCopy.at(halfPointId);
            if (midDist < GPP::REAL_TOL)
            {
                midDist = 1.0;
            }
            for (GPP::Int pid = 0; pid < points.size(); ++pid)
            {
                GPP::Real dist = distances.at(pid) / midDist * 0.2 + 0.2;
                dist = dist > 0.8 ? 0.8 : dist;
                GPP::Vector3 color = MagicCore::ToolKit::ColorCoding(dist);
                measureMesh->SetVertexColor(pid, color);
            }
            mpUI->SetDistanceInfo(measureMesh->GetVertexCount(), true, 
                minValue / ModelManager::Get()->GetScaleValue(), maxValue / ModelManager::Get()->GetScaleValue());
            mUpdateRefModelRendering = true;
            mUpdateModelRendering = true;
        }
    }

    void MeasureApp::InitViewTool()
    {
        if (mpViewTool == NULL)
        {
            mpViewTool = new MagicCore::ViewTool;
        }
    }

    void MeasureApp::UpdateModelRendering()
    {
        if (ModelManager::Get()->GetMesh() == NULL)
        {
            MagicCore::RenderSystem::Get()->HideRenderingObject("Mesh_Measure");
            return;
        }
        MagicCore::RenderSystem::Get()->RenderMesh("Mesh_Measure", "CookTorrance", ModelManager::Get()->GetMesh(), 
            MagicCore::RenderSystem::MODEL_NODE_CENTER, NULL, NULL, mIsFlatRenderingMode);
    }

    void MeasureApp::UpdateRefModelRendering()
    {
        if (mpRefTriMesh == NULL)
        {
            MagicCore::RenderSystem::Get()->HideRenderingObject("MeshRef_Measure");
            return;
        }
        MagicCore::RenderSystem::Get()->RenderMesh("MeshRef_Measure", "CookTorranceTransparent", mpRefTriMesh, 
            MagicCore::RenderSystem::MODEL_NODE_CENTER, NULL, NULL, true);
    }

    void MeasureApp::UpdateMarkRendering()
    {
        GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
        std::vector<GPP::Vector3> markCoords = mMarkPoints;
        if (!mMarkIds.empty() && triMesh != NULL)
        {
            for (std::vector<GPP::Int>::iterator itr = mMarkIds.begin(); itr != mMarkIds.end(); ++itr)
            {
                markCoords.push_back(triMesh->GetVertexCoord(*itr));
            }
            MagicCore::RenderSystem::Get()->RenderPointList("MarkPoints_MeasureApp", "SimplePoint_Large", GPP::Vector3(1, 0, 0), markCoords, MagicCore::RenderSystem::MODEL_NODE_CENTER);
            MagicCore::RenderSystem::Get()->RenderPolyline("MarkPointLine_MeasureApp", "Simple_Line", GPP::Vector3(0, 1, 0), mMarkPoints, true, MagicCore::RenderSystem::MODEL_NODE_CENTER);
        }
        else if (!mMarkFacePoints.empty() && triMesh != NULL)
        {
            for (std::vector<GPP::PointOnFace>::iterator itr = mMarkFacePoints.begin(); itr != mMarkFacePoints.end(); ++itr)
            {
                markCoords.push_back(GetCoord(*itr, triMesh));
            }
            MagicCore::RenderSystem::Get()->RenderPointList("MarkPoints_MeasureApp", "SimplePoint_Large", GPP::Vector3(1, 0, 0), markCoords, MagicCore::RenderSystem::MODEL_NODE_CENTER);
            MagicCore::RenderSystem::Get()->RenderPolyline("MarkPointLine_MeasureApp", "Simple_Line", GPP::Vector3(0, 1, 0), mMarkPoints, true, MagicCore::RenderSystem::MODEL_NODE_CENTER);
        }
        else
        {     
            MagicCore::RenderSystem::Get()->HideRenderingObject("MarkPoints_MeasureApp");       
            MagicCore::RenderSystem::Get()->HideRenderingObject("MarkPointLine_MeasureApp");
        }
        if (!mCurvesOnMesh.empty())
        {
            for (int lid = 0; lid < mCurvesOnMesh.size(); ++lid)
            {
                std::vector<GPP::Vector3> oneLineCoords(mCurvesOnMesh.at(lid).size());
                for (int pid = 0; pid < mCurvesOnMesh.at(lid).size(); ++pid)
                {
                    oneLineCoords.at(pid) = GetCoord(mCurvesOnMesh.at(lid).at(pid), triMesh);
                }
                markCoords.insert(markCoords.end(), oneLineCoords.begin(), oneLineCoords.end());
                MagicCore::RenderSystem::Get()->RenderPolyline("MarkPointLine_MeasureApp", "Simple_Line", GPP::Vector3(0, 1, 0), oneLineCoords, lid==0, MagicCore::RenderSystem::MODEL_NODE_CENTER);
            }
            MagicCore::RenderSystem::Get()->RenderPointList("MarkPoints_MeasureApp", "SimplePoint_Large", GPP::Vector3(1, 0, 0), markCoords, MagicCore::RenderSystem::MODEL_NODE_CENTER);
        }
        if (mDisplayPrincipalCurvature != 0 && triMesh != NULL)
        {
            int vertexCount = triMesh->GetVertexCount();
            if (vertexCount == mMinCurvature.size() && mDisplayPrincipalCurvature == -1)
            {
                std::vector<GPP::Vector3> startCoords(vertexCount);
                std::vector<GPP::Vector3> endCoords(vertexCount);
                double curvatureLen = 0.005;
                triMesh->SetHasVertexColor(true);
                for (int vid = 0; vid < vertexCount; vid++)
                {
                    GPP::Real minCurvature = mMinCurvature.at(vid);
                    triMesh->SetVertexColor(vid, MagicCore::ToolKit::ColorCoding(0.8 + minCurvature));
                    if (fabs(minCurvature) > GPP::REAL_TOL)
                    {
                        GPP::Vector3 vertexCoord = triMesh->GetVertexCoord(vid);
                        GPP::Vector3 minDir = mMinCurvatureDirs.at(vid);
                        startCoords.at(vid) = vertexCoord - minDir * curvatureLen;
                        endCoords.at(vid) = vertexCoord + minDir * curvatureLen;
                    }
                }
                MagicCore::RenderSystem::Get()->RenderLineSegments("PrincipalLine_MeasureApp", "Simple_Line", startCoords, endCoords);
            }
            else if (vertexCount == mMaxCurvature.size() && mDisplayPrincipalCurvature == 1)
            {
                std::vector<GPP::Vector3> startCoords(vertexCount);
                std::vector<GPP::Vector3> endCoords(vertexCount);
                double curvatureLen = 0.005;
                triMesh->SetHasVertexColor(true);
                for (int vid = 0; vid < vertexCount; vid++)
                {
                    GPP::Real maxCurvature = mMaxCurvature.at(vid);
                    triMesh->SetVertexColor(vid, MagicCore::ToolKit::ColorCoding(0.8 + maxCurvature));
                    if (fabs(maxCurvature) > GPP::REAL_TOL)
                    {
                        GPP::Vector3 vertexCoord = triMesh->GetVertexCoord(vid);
                        GPP::Vector3 maxDir = mMaxCurvatureDirs.at(vid);
                        startCoords.at(vid) = vertexCoord - maxDir * curvatureLen;
                        endCoords.at(vid) = vertexCoord + maxDir * curvatureLen;
                    }
                }
                MagicCore::RenderSystem::Get()->RenderLineSegments("PrincipalLine_MeasureApp", "Simple_Line", startCoords, endCoords);
            }
            else
            {
                MagicCore::RenderSystem::Get()->HideRenderingObject("PrincipalLine_MeasureApp");
            }
            MagicCore::RenderSystem::Get()->RenderMesh("Mesh_Measure", "CookTorrance", ModelManager::Get()->GetMesh(), 
                MagicCore::RenderSystem::MODEL_NODE_CENTER, NULL, NULL, mIsFlatRenderingMode);
        }
        else
        {
            MagicCore::RenderSystem::Get()->HideRenderingObject("PrincipalLine_MeasureApp");
        }
    }

    void MeasureApp::ShowReferenceMesh(bool isShow)
    {
        if (isShow)
        {
            UpdateRefModelRendering();
        }
        else
        {
            MagicCore::RenderSystem::Get()->HideRenderingObject("MeshRef_Measure");
        }
    }

    void MeasureApp::SwitchGeodesicClose()
    {
        mIsGeodesicsClose = !mIsGeodesicsClose;
    }

    bool MeasureApp::IsGeodesicClose()
    {
        return mIsGeodesicsClose;
    }

    void MeasureApp::SetSelectPrimitiveMode()
    {
        if (mpPickTool == NULL)
        {
            return;
        }
        GPP::TriMesh* triMesh = ModelManager::Get()->GetMesh();
        if (triMesh == NULL)
        {
            return;
        }

        if (mRightMouseType != SELECT_PRIMITIVE)
        {
            mRightMouseType = SELECT_PRIMITIVE;
            mpPickTool->SetPickParameter(MagicCore::PM_FACEPOINT, true, NULL, triMesh, "ModelNode");
            MessageBox(NULL, "Select Primitive Mode", "温馨提示", MB_OK);
        }
        else
        {
            mRightMouseType = SELECT_VERTEX;
            mpPickTool->SetPickParameter(MagicCore::PM_POINT, true, NULL, triMesh, "ModelNode");
            MessageBox(NULL, "目前仅能选择网格顶点", "温馨提示", MB_OK);
        }
        ClearSelectionData();
        mUpdateMarkRendering = true;
    }

    void MeasureApp::SetDetectOptions(bool optionPlane, bool optionCone, bool optionSphere, bool optionCylinder)
    {
        mDetectOptions[0] = optionPlane;
        mDetectOptions[1] = optionCone;
        mDetectOptions[2] = optionSphere;
        mDetectOptions[3] = optionCylinder;
    }
}

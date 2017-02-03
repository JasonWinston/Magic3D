#include "ScriptSystem.h"
#include "lua_tinker.h"
#include "LogSystem.h"
#include "../Application/ModelManager.h"
#include "../Application/AppApi.h"
#include "../Application/MeshShopApp.h"
#include "../Application/ReliefApp.h"

namespace MagicCore
{
    ScriptSystem* ScriptSystem::mpScriptSystem = NULL;

    ScriptSystem::ScriptSystem() : mpLuaState(NULL), mIsOnRunning(false)
    {

    }

    ScriptSystem::~ScriptSystem()
    {

    }

    ScriptSystem* ScriptSystem::Get()
    {
        if (mpScriptSystem == NULL)
        {
            mpScriptSystem = new ScriptSystem;
            mpScriptSystem->Init();
        }
        return mpScriptSystem;
    }

    void ScriptSystem::Close()
    {
        if (mpLuaState)
        {
            lua_close(mpLuaState);
            mpLuaState = NULL; 
        }
    }

    void ScriptSystem::Init()
    {
        Close();
        mpLuaState = luaL_newstate();
        luaL_openlibs(mpLuaState);
        lua_tinker::init(mpLuaState);
        Registrate();
    }

    void ScriptSystem::Registrate()
    {
        // constant values
        lua_tinker::table gppTable(mpLuaState, "Gpp");
        gppTable.set("REAL_TOL", GPP::REAL_TOL);
        gppTable.set("REAL_LARGE", GPP::REAL_LARGE);
        gppTable.set("INT_LARGE", GPP::INT_LARGE);
        gppTable.set("UINT_LARGE", GPP::UINT_LARGE);
        gppTable.set("GPP_PI", GPP::GPP_PI);
        gppTable.set("ONE_RADIAN", GPP::ONE_RADIAN);

        //lua_tinker::def(mpLuaState, "EnterApp", &MagicApp::AppApi::EnterApp); // not support yet

        lua_tinker::def(mpLuaState, "GetMesh", &MagicApp::AppApi::GetMesh);
        lua_tinker::class_add<GPP::TriMesh>(mpLuaState, "TriMesh");
        lua_tinker::class_def<GPP::TriMesh>(mpLuaState, "GetVertexCount", &GPP::TriMesh::GetVertexCount);
        lua_tinker::class_def<GPP::TriMesh>(mpLuaState, "GetTriangleCount", &GPP::TriMesh::GetTriangleCount);

        lua_tinker::def(mpLuaState, "GetMeshShopApp", &MagicApp::AppApi::GetMeshShopApp);
        lua_tinker::class_add<MagicApp::MeshShopApp>(mpLuaState, "MeshShopApp");
        lua_tinker::class_def<MagicApp::MeshShopApp>(mpLuaState, "ImportMesh", &MagicApp::MeshShopApp::ImportMesh);
        lua_tinker::class_def<MagicApp::MeshShopApp>(mpLuaState, "ConsolidateTopology", &MagicApp::MeshShopApp::ConsolidateTopology);
        lua_tinker::class_def<MagicApp::MeshShopApp>(mpLuaState, "ReverseDirection", &MagicApp::MeshShopApp::ReverseDirection);
        lua_tinker::class_def<MagicApp::MeshShopApp>(mpLuaState, "RemoveMeshIsolatePart", &MagicApp::MeshShopApp::RemoveMeshIsolatePart);
        lua_tinker::class_def<MagicApp::MeshShopApp>(mpLuaState, "ConsolidateGeometry", &MagicApp::MeshShopApp::ConsolidateGeometry);
        //lua_tinker::class_def<MagicApp::MeshShopApp>(mpLuaState, "OptimizeMesh", &MagicApp::MeshShopApp::OptimizeMesh);
        lua_tinker::class_def<MagicApp::MeshShopApp>(mpLuaState, "RemoveMeshNoise", &MagicApp::MeshShopApp::RemoveMeshNoise);
        lua_tinker::class_def<MagicApp::MeshShopApp>(mpLuaState, "SmoothMesh", &MagicApp::MeshShopApp::SmoothMesh);
        lua_tinker::class_def<MagicApp::MeshShopApp>(mpLuaState, "EnhanceMeshDetail", &MagicApp::MeshShopApp::EnhanceMeshDetail);
        lua_tinker::class_def<MagicApp::MeshShopApp>(mpLuaState, "LoopSubdivide", &MagicApp::MeshShopApp::LoopSubdivide);
        lua_tinker::class_def<MagicApp::MeshShopApp>(mpLuaState, "RefineMesh", &MagicApp::MeshShopApp::RefineMesh);
        lua_tinker::class_def<MagicApp::MeshShopApp>(mpLuaState, "SimplifyMesh", &MagicApp::MeshShopApp::SimplifyMesh);
        lua_tinker::class_def<MagicApp::MeshShopApp>(mpLuaState, "SimplifySelectedVertices", &MagicApp::MeshShopApp::SimplifySelectedVertices);
        lua_tinker::class_def<MagicApp::MeshShopApp>(mpLuaState, "UniformRemesh", &MagicApp::MeshShopApp::UniformRemesh);
        lua_tinker::class_def<MagicApp::MeshShopApp>(mpLuaState, "UniformSampleMesh", &MagicApp::MeshShopApp::UniformSampleMesh);
        lua_tinker::class_def<MagicApp::MeshShopApp>(mpLuaState, "FillHole", &MagicApp::MeshShopApp::FillHole);
        lua_tinker::class_def<MagicApp::MeshShopApp>(mpLuaState, "BridgeEdges", &MagicApp::MeshShopApp::BridgeEdges);
        lua_tinker::class_def<MagicApp::MeshShopApp>(mpLuaState, "UniformOffsetMesh", &MagicApp::MeshShopApp::UniformOffsetMesh);
        lua_tinker::class_def<MagicApp::MeshShopApp>(mpLuaState, "DeleteSelections", &MagicApp::MeshShopApp::DeleteSelections);

        lua_tinker::def(mpLuaState, "GetReliefApp", &MagicApp::AppApi::GetReliefApp);
        lua_tinker::class_add<MagicApp::ReliefApp>(mpLuaState, "ReliefApp");
        lua_tinker::class_def<MagicApp::ReliefApp>(mpLuaState, "ImportModel", &MagicApp::ReliefApp::ImportModel);
        lua_tinker::class_def<MagicApp::ReliefApp>(mpLuaState, "CaptureDepthPointCloud", &MagicApp::ReliefApp::CaptureDepthPointCloud);
        lua_tinker::class_def<MagicApp::ReliefApp>(mpLuaState, "SavePointCloud", &MagicApp::ReliefApp::SavePointCloud);
        lua_tinker::class_def<MagicApp::ReliefApp>(mpLuaState, "SaveDepthPointCloud", &MagicApp::ReliefApp::SaveDepthPointCloud);
        lua_tinker::class_def<MagicApp::ReliefApp>(mpLuaState, "RotateView", &MagicApp::ReliefApp::RotateView);
    }

    bool ScriptSystem::IsOnRunningScript()
    {
        return mIsOnRunning;
    }

    bool ScriptSystem::RunScript(const char* buffer)
    {
        if (mpLuaState == NULL)
        {
            return false;
        }

        mIsOnRunning = true;
        bool err = luaL_loadbuffer(mpLuaState, buffer, strlen(buffer), "scriptBuffer") || lua_pcall(mpLuaState, 0, 0, 0);
        mIsOnRunning = false;
        if (err)
        {
            InfoLog << "Run script failed: " << lua_tostring(mpLuaState, -1) << std::endl;
            lua_pop(mpLuaState, 1);
            return false;
        }
        return true;
    }

    bool ScriptSystem::RunScriptFile(const char* fileName)
    {
        if (mpLuaState == NULL)
        {
            return false;
        }

        mIsOnRunning = true;
        bool err = luaL_dofile(mpLuaState, fileName);
        mIsOnRunning = false;
        if (err)
        {
            InfoLog << "Run script file failed: " << lua_tostring(mpLuaState, -1) << std::endl;
            lua_pop(mpLuaState, 1);
            return false;
        }
        InfoLog << "Script run success. " << std::endl;
        return true;
    }
}
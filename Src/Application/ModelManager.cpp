#include "ModelManager.h"

namespace MagicApp
{
    ModelManager* ModelManager::mpModelManager = NULL;

    ModelManager::ModelManager() :
        mpPointCloud(NULL),
        mpTriMesh(NULL),
        mObjCenterCoord(),
        mScaleValue(1),
        mImageColorIds(),
        mTextureImageFiles(),
        mCloudIds(),
        mImageColorIdFlags()
    {
    }

    ModelManager* ModelManager::Get()
    {
        if (mpModelManager == NULL)
        {
            mpModelManager = new ModelManager;
        }
        return mpModelManager;
    }

    ModelManager::~ModelManager()
    {
        ClearPointCloud();
        ClearMesh();
    }

    bool ModelManager::ImportPointCloud(std::string fileName)
    {
        GPPFREEPOINTER(mpPointCloud);
        mpPointCloud = GPP::Parser::ImportPointCloud(fileName);
        if (mpPointCloud == NULL)
        {
            return false;
        }
        mpPointCloud->UnifyCoords(2.0, &mScaleValue, &mObjCenterCoord);
        return true;
    }

    void ModelManager::SetPointCloud(GPP::PointCloud* pointCloud)
    {
        GPPFREEPOINTER(mpPointCloud);
        mpPointCloud = pointCloud;
    }

    GPP::PointCloud* ModelManager::GetPointCloud()
    {
        return mpPointCloud;
    }

    void ModelManager::ClearPointCloud()
    {
        GPPFREEPOINTER(mpPointCloud);
    }

    void ModelManager::SetScaleValue(GPP::Real scaleValue)
    {
        mScaleValue = scaleValue;
    }

    GPP::Real ModelManager::GetScaleValue() const
    {
        return mScaleValue;
    }
    
    void ModelManager::SetObjCenterCoord(GPP::Vector3 objCenterCoord)
    {
        mObjCenterCoord = objCenterCoord;
    }

    GPP::Vector3 ModelManager::GetObjCenterCoord() const
    {
        return mObjCenterCoord;
    }

    void ModelManager::SetImageColorIds(const std::vector<GPP::ImageColorId>& imageColorIds)
    {
        mImageColorIds = imageColorIds;
    }

    std::vector<GPP::ImageColorId> ModelManager::GetImageColorIds() const
    {
        return mImageColorIds;
    }

    std::vector<GPP::ImageColorId>* ModelManager::GetImageColorIdsPointer()
    {
        if (mImageColorIds.empty())
        {
            return NULL;
        }
        else
        {
            return &mImageColorIds;
        }
    }

    void ModelManager::SetTextureImageFiles(const std::vector<std::string>& textureImageFiles)
    {
        mTextureImageFiles = textureImageFiles;
    }

    std::vector<std::string> ModelManager::GetTextureImageFiles(void) const
    {
        return mTextureImageFiles;
    }

    void ModelManager::SetCloudIds(const std::vector<int>& cloudIds)
    {
        mCloudIds = cloudIds;
    }

    std::vector<int> ModelManager::GetCloudIds(void) const
    {
        return mCloudIds;
    }

    std::vector<int>* ModelManager::GetCloudIdsPointer(void)
    {
        if (mCloudIds.empty())
        {
            return NULL;
        }
        else
        {
            return &mCloudIds;
        }
    }

    void ModelManager::SetColorIds(const std::vector<int>& colorIds)
    {
        mColorIds = colorIds;
    }

    std::vector<int> ModelManager::GetColorIds(void) const
    {
        return mColorIds;
    }

    std::vector<int>* ModelManager::GetColorIdsPointer(void)
    {
        if (mColorIds.empty())
        {
            return NULL;
        }
        else
        {
            return &mColorIds;
        }
    }

    void ModelManager::SetImageColorIdFlag(const std::vector<int>& flags)
    {
        mImageColorIdFlags = flags;
    }
    
    std::vector<int> ModelManager::GetImageColorIdFlags() const
    {
        return mImageColorIdFlags;
    }

    std::vector<int>* ModelManager::GetImageColorIdFlagsPointer()
    {
        if (mImageColorIdFlags.empty())
        {
            return NULL;
        }
        else
        {
            return &mImageColorIdFlags;
        }
    }

    bool ModelManager::ImportMesh(std::string fileName)
    {
        GPPFREEPOINTER(mpTriMesh);
        mpTriMesh = GPP::Parser::ImportTriMesh(fileName);
        if (mpTriMesh == NULL)
        {
            return false;
        }
        if (mpTriMesh->GetMeshType() == GPP::MeshType::MT_TRIANGLE_SOUP)
        {
            mpTriMesh->FuseVertex();
        }
        mpTriMesh->UnifyCoords(2.0, &mScaleValue, &mObjCenterCoord);
        mpTriMesh->UpdateNormal();
        return true;
    }

    void ModelManager::SetMesh(GPP::TriMesh* triMesh)
    {
        GPPFREEPOINTER(mpTriMesh);
        mpTriMesh = triMesh;
    }

    GPP::TriMesh* ModelManager::GetMesh()
    {
        return mpTriMesh;
    }

    void ModelManager::ClearMesh()
    {
        GPPFREEPOINTER(mpTriMesh);
    }

    void ModelManager::DumpInfo(std::ofstream& dumpOut) const
    {
        dumpOut << mImageColorIds.size() << std::endl;
        for (std::vector<GPP::ImageColorId>::const_iterator itr = mImageColorIds.begin(); itr != mImageColorIds.end(); ++itr)
        {
            dumpOut << itr->GetImageIndex() << " " << itr->GetLocalX() << " " << itr->GetLocalY() << " ";
        }
        dumpOut << std::endl;
           
        dumpOut << mColorIds.size() << std::endl;
        for (std::vector<int>::const_iterator itr = mColorIds.begin(); itr != mColorIds.end(); ++itr)
        {
            dumpOut << *itr << " ";
        }
        dumpOut << std::endl;

        dumpOut << mImageColorIdFlags.size() << std::endl;
        for (std::vector<int>::const_iterator itr = mImageColorIdFlags.begin(); itr != mImageColorIdFlags.end(); ++itr)
        {
            dumpOut << *itr << " ";
        }
        dumpOut << std::endl;

        dumpOut << mCloudIds.size() << std::endl;
        for (std::vector<int>::const_iterator itr = mCloudIds.begin(); itr != mCloudIds.end(); ++itr)
        {
            dumpOut << *itr << " ";
        }
        dumpOut << std::endl;
    }

    void ModelManager::LoadInfo(std::ifstream& loadIn)
    {
        mImageColorIds.clear();
        int count = 0;
        loadIn >> count;
        mImageColorIds.reserve(count);
        int imageIndex;
        double localX, localY;
        for (int iid = 0; iid < count; iid++)
        {
            loadIn >> imageIndex >> localX >> localY;
            mImageColorIds.push_back(GPP::ImageColorId(imageIndex, int(localX + 0.5), int(localY + 0.5)));
        }

        mColorIds.clear();
        loadIn >> count;
        mColorIds.reserve(count);
        int colorId;
        for (int cid = 0; cid < count; cid++)
        {
            loadIn >> colorId;
            mColorIds.push_back(colorId);
        }

        mImageColorIdFlags.clear();
        loadIn >> count;
        mImageColorIdFlags.reserve(count);
        int flag;
        for (int fid = 0; fid < count; fid++)
        {
            loadIn >> flag;
            mImageColorIdFlags.push_back(flag);
        }

        mCloudIds.clear();
        loadIn >> count;
        mCloudIds.reserve(count);
        int cloudId;
        for (int cid = 0; cid < count; cid++)
        {
            loadIn >> cloudId;
            mCloudIds.push_back(cloudId);
        }
    }
}

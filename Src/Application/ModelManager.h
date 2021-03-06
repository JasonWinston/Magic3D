#pragma once
#include "GPP.h"
#include <string>

namespace MagicApp
{
    class ModelManager
    {
    private:
        static ModelManager* mpModelManager;
        ModelManager(void);
    public:
        static ModelManager* Get(void);

        bool ImportPointCloud(std::string fileName);
        void SetPointCloud(GPP::PointCloud* pointCloud);
        GPP::PointCloud* GetPointCloud(void);
        void ClearPointCloud(void);

        void SetScaleValue(GPP::Real scaleValue);
        GPP::Real GetScaleValue(void) const;

        void SetObjCenterCoord(GPP::Vector3 objCenterCoord);
        GPP::Vector3 GetObjCenterCoord(void) const;

        void SetImageColorIds(const std::vector<GPP::ImageColorId>& imageColorIds);
        std::vector<GPP::ImageColorId> GetImageColorIds(void) const;
        std::vector<GPP::ImageColorId>* GetImageColorIdsPointer(void);

        void SetTextureImageFiles(const std::vector<std::string>& textureImageFiles);
        std::vector<std::string> GetTextureImageFiles(void) const;

        void SetCloudIds(const std::vector<int>& cloudIds);
        std::vector<int> GetCloudIds(void) const;
        std::vector<int>* GetCloudIdsPointer(void);

        void SetColorIds(const std::vector<int>& colorIds);
        std::vector<int> GetColorIds(void) const;
        std::vector<int>* GetColorIdsPointer(void);

        void SetImageColorIdFlag(const std::vector<int>& flags);
        std::vector<int> GetImageColorIdFlags(void) const;
        std::vector<int>* GetImageColorIdFlagsPointer(void);

        bool ImportMesh(std::string fileName);
        void SetMesh(GPP::TriMesh* triMesh);
        GPP::TriMesh* GetMesh(void);
        void ClearMesh(void);

        void DumpInfo(std::ofstream& dumpOut) const;
        void LoadInfo(std::ifstream& loadIn);

        ~ModelManager();

    private:
        GPP::PointCloud* mpPointCloud;
        GPP::TriMesh* mpTriMesh;
        GPP::Vector3 mObjCenterCoord;
        GPP::Real mScaleValue;
        std::vector<GPP::ImageColorId> mImageColorIds;
        std::vector<std::string> mTextureImageFiles;
        std::vector<int> mCloudIds;
        std::vector<int> mColorIds;
        std::vector<int> mImageColorIdFlags;
    };
}

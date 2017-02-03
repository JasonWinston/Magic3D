#pragma once
#include "MyGUI.h"

namespace MagicApp
{
    class TextureAppUI
    {
    public:
        TextureAppUI();
        ~TextureAppUI();

        void Setup();
        void Shutdown();

        void StartProgressbar(int range);
        void SetProgressbar(int value);
        void StopProgressbar(void);
        bool IsProgressbarVisible(void);

        void SetMeshInfo(int vertexCount, int triangleCount);

    private:
        void SwitchDisplayMode(MyGUI::Widget* pSender);
        void SwitchTextureImage(MyGUI::Widget* pSender);

        void ImportTriMesh(MyGUI::Widget* pSender);

        void ImageColorIds(MyGUI::Widget* pSender);
        void ComputeImageColorIds(MyGUI::Widget* pSender);
        void OptimiseIsolateImageColorIds(MyGUI::Widget* pSender);
        void LoadImageColorIds(MyGUI::Widget* pSender);
        void SaveImageColorIds(MyGUI::Widget* pSender);

        void MeshColor(MyGUI::Widget* pSender);
        void FuseMeshColor(MyGUI::Widget* pSender);

        void GenerateTextureImage(MyGUI::Widget* pSender);
        void GenerateTextureImageByVertexColor(MyGUI::Widget* pSender);
        void GenerateTextureImageByImage(MyGUI::Widget* pSender);
        void TuneTextureImageByVertexColor(MyGUI::Widget* pSender);

        void BackToHomepage(MyGUI::Widget* pSender);

    private:
        MyGUI::VectorWidgetPtr mRoot;
        bool mIsProgressbarVisible;
        MyGUI::TextBox* mTextInfo;
    };
}

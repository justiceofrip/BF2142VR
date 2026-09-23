#include "StereoDiagnostics.h"
#include <cstdio>
#include <cmath>
namespace bfvr::bf2142 {
void DiagnosticRequest(shared::SharedRenderRequest& request,unsigned long frame) {
    request={}; request.shouldRender=request.viewsValid=request.headPoseValid=request.headPoseTracked=1;
    request.predictedDisplayTime=static_cast<LONGLONG>(GetTickCount64())*1000000;
    request.headPose.orientationW=1;
    request.headPose.positionY=1.7f;
    // Hold neutral for the baseline, then exercise a bounded translation/yaw.
    const float t=frame<30?0.0f:0.15f;
    request.headPose.positionX=t;
    request.headPose.orientationY=std::sin(t*.5f);request.headPose.orientationW=std::cos(t*.5f);
    for(int eye=0;eye<2;++eye) {
        request.views[eye].pose=request.headPose;
        request.views[eye].pose.positionX+=(eye==0?-.032f:.032f)*std::cos(t);
        request.views[eye].pose.positionZ-=(eye==0?-.032f:.032f)*std::sin(t);
        request.views[eye].fov={-.85f,.85f,.85f,-.85f};
    }
}
void SaveDiagnosticFrame(const std::wstring& path,UINT width,UINT height,const std::vector<DWORD>& pixels) {
    if(pixels.size()!=static_cast<size_t>(width)*height) return;
    FILE* file=nullptr; if(_wfopen_s(&file,path.c_str(),L"wb") || !file)return;
    BITMAPFILEHEADER head{};head.bfType=0x4d42;head.bfOffBits=sizeof(head)+sizeof(BITMAPINFOHEADER);
    head.bfSize=head.bfOffBits+width*height*4;
    BITMAPINFOHEADER info{};info.biSize=sizeof(info);info.biWidth=width;info.biHeight=-static_cast<LONG>(height);
    info.biPlanes=1;info.biBitCount=32;info.biCompression=BI_RGB;
    fwrite(&head,sizeof(head),1,file);fwrite(&info,sizeof(info),1,file);fwrite(pixels.data(),4,pixels.size(),file);fclose(file);
}
}

#pragma once
#include "MvCameraControl.h"
#include <iostream>
#include <chrono>
#include <ctime> 
#include <sstream>
#include <numeric>
#include <map>
#include "CustomLogger.h"
typedef unsigned char uchar;
typedef unsigned int uint;

/*
海康相机类
*/

#if  USE_HALCON
using namespace HalconCpp;
#endif


class CamClass
{
#if USE_HALCON

#endif
public:
    unsigned int g_nPayloadSize = 0;
    bool connected = false;
    bool deviceIsOpen = false;
    bool deviceIsgrabbing = false;
    MV_CC_DEVICE_INFO_LIST stDeviceList;
    void* handle = NULL;
    void* expCallbackptr;
    MV_FRAME_OUT frame = { 0 };
    std::string serialNumber = ""; //该实例根据序列号连接，如果为空则连接第一个相机


    //注意多相机如果给空序列号会导致无法连接
    CamClass(std::string serialNumber ="") {
        this->serialNumber = serialNumber;
        listDevices();
        printDeviceInfo();
        MV_CC_RegisterExceptionCallBack(handle, &expCallback, &logger);
        setHeartbeat(1000);//TODO 设置心跳无用？？？
    }

    //返回设备以及序列号
    int listDevices() {
        enumDevice();
        serialRef = {};
        std::string serialNumber = "";
        for (unsigned int i = 0; i < stDeviceList.nDeviceNum; i++)
        {
            MV_CC_DEVICE_INFO* pDeviceInfo = stDeviceList.pDeviceInfo[i];
            if (NULL == pDeviceInfo) {
                serialNumber = "unknown serial Number";
            }
            if (pDeviceInfo->nTLayerType == MV_GIGE_DEVICE) {
                serialNumber = (const char*)(pDeviceInfo->SpecialInfo.stGigEInfo.chSerialNumber);
            }
            else if (pDeviceInfo->nTLayerType == MV_USB_DEVICE) {
                serialNumber = (const char*)(pDeviceInfo->SpecialInfo.stUsb3VInfo.chSerialNumber);
            }
            else {
                serialNumber = "unknown serial Number";
            }
            serialRef.insert({ serialNumber,i });
        }
        return 0;
    }

private:
    std::map<std::string, int> serialRef = {};
    std::vector<void*> ptrArray = { &logger, this };
    CustomLogger &logger = CustomLogger::get();
    std::map<bool, std::string> retRef = { {1,"成功"}, {0,"失败"} };
    std::map<int, std::string> modeRef = { {0,"关闭"}, {1,"开启"} };
    std::map<int, std::string> sourceRef = { {0,"LINE0"}, {1,"LINE1"}, {2,"LINE2"}, {3,"LINE3"}, {4,"COUNTER0"}, {7,"软触发"}, {8,"变频器"} };

    #if USE_HALCON
        std::vector<uchar> imgbuf[3]; //TODO Halcon转0
    #endif // 0

public:
    int setWidth(int width=1024) {
        MV_CC_SetWidth(handle, width);
    }

    int setHeight(int height=800) {
        MV_CC_SetHeight(handle, height);
    }

    int deviceConnected() {
        return MV_CC_IsDeviceConnected(this->handle);
    }

    int freeImageBuffer(MV_FRAME_OUT* pFrame)   {return MV_CC_FreeImageBuffer(this->handle, pFrame);}

    int executeCommand(IN const char* strKey)   {return MV_CC_SetCommandValue(this->handle, strKey);}

    int getTriggerMode() { return getValue("TriggerMode");}

    int softTrigger() {return executeCommand("TriggerSoftware");}

    void setTriggerMode(int mode,int source) {
        if (!sourceRef.count(source) or !modeRef.count(mode)) { 
            logger.warn("设置触发模式失败");
            return; 
        }
        std::string info1 = retRef[setValue("TriggerMode",mode)==0];
        std::string info2 = retRef[MV_CC_SetEnumValue(this->handle, "TriggerSource", source) == 0];
        logger.info("\n设置触发模式: {} {} \n设置触发源: {} {}", modeRef[mode], info1, sourceRef[source],info2);
    }

    void setExposureTime(int num) {
        logger.info("设置曝光时间： {} {}", num, retRef[MV_CC_SetFloatValue(this->handle, "ExposureTime", (float)num) == 0]);
    }
    
    float getExposureTime() {
        MVCC_FLOATVALUE val = {0};
        MV_CC_GetFloatValue(this->handle, "ExposureTime", &val);
        return val.fCurValue;
    }

    void printDeviceInfo() {
        MV_CC_DEVICE_INFO_LIST stDeviceList = this->stDeviceList;
        logger.info("搜寻设备中...");
        if (stDeviceList.nDeviceNum == 0) { logger.info("无设备"); }
        for (unsigned int i = 0; i < stDeviceList.nDeviceNum; i++)
        {
            logger.info("[摄像头 {}]:", i);
            MV_CC_DEVICE_INFO* pDeviceInfo = stDeviceList.pDeviceInfo[i];
            if (NULL == pDeviceInfo) {
                logger.warn("pstMVDevInfo 为空指针");
                break;
            }
            if (pDeviceInfo->nTLayerType == MV_GIGE_DEVICE) {
                int nIp1 = ((pDeviceInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0xff000000) >> 24);
                int nIp2 = ((pDeviceInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0x00ff0000) >> 16);
                int nIp3 = ((pDeviceInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0x0000ff00) >> 8);
                int nIp4 = (pDeviceInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0x000000ff);
                // print current ip and user defined name
                logger.info("IP地址:{}.{}.{}.{}\n", nIp1, nIp2, nIp3, nIp4);
                logger.info("自定义名称: {}", pDeviceInfo->SpecialInfo.stGigEInfo.chUserDefinedName);
            }
            else if (pDeviceInfo->nTLayerType == MV_USB_DEVICE) {
                logger.info("自定义名称: {}", pDeviceInfo->SpecialInfo.stUsb3VInfo.chUserDefinedName);
                logger.info("序列号: {}", pDeviceInfo->SpecialInfo.stUsb3VInfo.chSerialNumber);
                logger.info("设备数字: {}", pDeviceInfo->SpecialInfo.stUsb3VInfo.nDeviceNumber);
            }
            else{
                logger.error("设备不支持");
            }
        }
    };

    //TODO 目前只兼容单相机
    int openDevice(int camIndex = 0, bool manualMode = false) {

        logger.info("搜寻并尝试打开相机：{} ", serialNumber);
        if (serialNumber == "") {}
        else if (serialRef.count(serialNumber))
        {
            camIndex = serialRef[serialNumber];
        }
        else {
            logger.warn("未找到该序列号设备： {}", serialNumber);
            logger.info("搜寻到设备:");

            std::map<std::string, int>::iterator i;
            for (i = serialRef.begin(); i!= serialRef.end(); i++)
            {
                logger.info(i->first);
            }
            return -1;
        }
        if (deviceIsOpen) {
            logger.info("已打开设备");
            return 0;
        }
        logger.info("搜寻设备中...");
        memset(&this->stDeviceList, 0, sizeof(MV_CC_DEVICE_INFO_LIST));
        int nRet = MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE, &this->stDeviceList);

        if (MV_OK != nRet) {
            logger.info("搜寻失败: {}", nRet);
            return nRet;
        }
        int devices = this->stDeviceList.nDeviceNum;                               
        if (devices < 1) { logger.warn("无设备");   return -1; }
        
        nRet = MV_CC_CreateHandle(&handle, this->stDeviceList.pDeviceInfo[camIndex]);
        if (MV_OK != nRet){
            logger.warn("无法建立句柄: {}", nRet);
            return nRet;
        }
        if (manualMode) {return 0;}

        if (MV_CC_IsDeviceConnected(handle) == 1) {
            MV_CC_CloseDevice(handle);
        }

        nRet = MV_CC_OpenDevice(handle);//open device
        if (MV_OK != nRet){
            logger.warn("无法打开设备: {} ,{}",serialNumber, nRet);
            return nRet;
        }
        deviceIsOpen = true;
        if (this->stDeviceList.pDeviceInfo[camIndex]->nTLayerType == MV_GIGE_DEVICE)
        {
            int nPacketSize = MV_CC_GetOptimalPacketSize(handle);
            if (nPacketSize < 0) { logger.warn("无法获取数据包大小: {}", nPacketSize); }
            nRet = MV_CC_SetIntValue(handle, "GevSCPSPacketSize", nPacketSize);
            if (nRet != MV_OK) { logger.warn("无法设置数据包大小: {}",nRet); }
        }
        MVCC_INTVALUE stParam;
        memset(&stParam, 0, sizeof(MVCC_INTVALUE));
        nRet = MV_CC_GetIntValue(handle, "PayloadSize", &stParam);

        if (MV_OK != nRet){
            logger.warn("Get PayloadSize fail! nRet {}", nRet);
            return nRet;
        }
        this -> g_nPayloadSize = stParam.nCurValue;
        //send grab command to camera
        nRet = MV_CC_SetEnumValue(handle, "PixelFormat", PixelType_Gvsp_BGR8_Packed); //!!!注意这里需要根据设备写入
        if (MV_OK != nRet)  {logger.warn("无法设置像素格式 {}", nRet);}
        nRet = MV_CC_StartGrabbing(handle);
        if (MV_OK != nRet)  {logger.error("开启捕捉失败"); return nRet;}
        deviceIsgrabbing = true;
        logger.info("成功打开设备 {}",serialNumber);
        return 0;
    };


    int startGrabbing() {
        int nRet = MV_CC_StartGrabbing(this->handle);
        if (MV_OK != nRet) {
            logger.error("开启捕捉失败 {}", nRet);
            return nRet;
        }
        return 0;
    };

    int stopGrabbing() {
        int nRet = MV_CC_StopGrabbing(this -> handle);
        if (MV_OK != nRet){
            logger.warn("停止捕捉失败 {}", nRet);
            return nRet;
        }
        return 0;
    };

    int terminateCam() {
        int nRet = MV_CC_StopGrabbing(this -> handle);  // Stop grab image
        if (MV_OK != nRet) { logger.warn("无法停止捕捉: {}", nRet); }
        nRet = MV_CC_CloseDevice(this->handle);   // Close device
        if (MV_OK != nRet) { logger.warn("无法关闭相机设备: {}", nRet); }
        nRet = MV_CC_DestroyHandle(this->handle); // Destroy handle
        if (MV_OK != nRet) { logger.warn("无法相机销毁句柄: {}", nRet); }
        return nRet;
    };

    int setHeartbeat(int val) {
        int ret = MV_CC_SetHeartBeatTimeout(this->handle, val);//单位ms
        if (ret != MV_OK){  logger.warn("设置心跳失败: {}", ret);}
        return ret;
    }

    int getValue(const char* valueName) {
        MVCC_ENUMVALUE stEnumValue = { 0 };
        int nRet = MV_CC_GetEnumValue(this->handle, valueName, &stEnumValue);
        if (nRet < 0) { return nRet; }
        return stEnumValue.nCurValue;
    }

    int setValue(const char* valueName, unsigned int value) {
        return MV_CC_SetEnumValue(this->handle, valueName, value);
    }

    int grabImageRaw(char* image,int&h,int&w, int timeout = 1000) {
        int nRet;
        if (!(deviceIsOpen && deviceIsgrabbing)) {
            logger.warn("设备未打开");
            return 1;
        }
        nRet = this->getImageBuffer(&this->frame, 1000);
        if (nRet != MV_OK) {
            logger.trace("无图像数据或数据错误: {}", nRet);
            return 2;
        }
        h = frame.stFrameInfo.nHeight;
        w = frame.stFrameInfo.nWidth;
        image = new char[h*w];
        memcpy(image,frame.pBufAddr,h*w/4);
        nRet = this->freeImageBuffer(&this->frame);
        if (nRet != MV_OK) {
            logger.warn("清空缓存出错: {}", nRet);
            return nRet;
        }
        return nRet;
    };

#if USE_OPENCV
    int grabImage(cv::Mat& image, int timeout = 1000) {
        int nRet;
        if (!(deviceIsOpen && deviceIsgrabbing)) {
            logger.warn("设备未打开");
            return 1;
        }
        nRet = this->getImageBuffer(&this->frame, 1000);
        if (nRet != MV_OK) {
            logger.trace("无图像数据或数据错误: {}", nRet);
            return nRet;
        }
        nRet = Convert2Mat(&frame.stFrameInfo, frame.pBufAddr, image);
        if (nRet != MV_OK) { logger.error("相机数据转换到Mat格式错误"); }
        nRet = this->freeImageBuffer(&this->frame);
        if (nRet != MV_OK) {
            logger.warn("清空缓存出错: {}", nRet);
            return nRet;
        }
        return nRet;
    };
#endif
#if USE_HALCON
    //抓取Halcon用图像 
    int grabImage(HObject* objPtr,int timeout = 1000) {
        if (!deviceConnected()) {
            logger.warn("设备未连接!");
            return 1;
        }
        int ret;
        if (!(deviceIsOpen && deviceIsgrabbing)) {
            logger.warn("设备未打开");
            return 1;
        }
        ret = this->getImageBuffer(&this->frame, 1000);
        if (ret != MV_OK) {
            logger.error("图像数据错误: {}", ret);
            return ret;
        }
        if (frame.pBufAddr == NULL) {
            return -1;
        }
 
        int h = frame.stFrameInfo.nHeight, w = frame.stFrameInfo.nWidth;
        RGB2BGR(frame.pBufAddr, w, h);
        auto pixelType = frame.stFrameInfo.enPixelType;
        if (IsColorPixelFormat(pixelType)) { 
            ret = RGBtoHObject(objPtr, h, w, frame.pBufAddr); 
        }
        else { 
            ret = mono8toHObject(objPtr, h, w, frame.pBufAddr); 
        }
        if (ret != MV_OK) { logger.error("相机数据转换到HObject格式错误"); }
        ret = this->freeImageBuffer(&this->frame);
        if (ret != MV_OK) {logger.warn("清空缓存出错: {}", ret);}
        return ret;
    }
#endif

protected:
    //TODO 需要完善该功能
    static void expCallback(unsigned int messageType, void* expCallbackptr) {

        //CustomLogger* logger = (CustomLogger*)expCallbackptr;
        CustomLogger::get().info("相机运行中遇到错误");
        
    }

    int getImageBuffer(MV_FRAME_OUT* pFrame, int nMsec) { return MV_CC_GetImageBuffer(this->handle, pFrame, nMsec); }

    void enumDevice() {
        memset(&this->stDeviceList, 0, sizeof(MV_CC_DEVICE_INFO_LIST));
        int nRet = MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE, &this->stDeviceList);
        if (MV_OK != nRet) { logger.warn("无法读取数据 {}", nRet); }
    }

    int RGB2BGR(uchar* pRgbData, uint width, uint height) {
        if (NULL == pRgbData){return MV_E_PARAMETER;}
        uchar red;
        for (uint j = 0; j < height; j++) {
            for (uint i = 0; i < width; i++) {
                red = pRgbData[j * (width * 3) + i * 3];
                pRgbData[j * (width * 3) + i * 3] = pRgbData[j * (width * 3) + i * 3 + 2];
                pRgbData[j * (width * 3) + i * 3 + 2] = red;
            }
        }
        return MV_OK;
    }

    bool IsColorPixelFormat(MvGvspPixelType enType){
        switch (enType)
        {
        case PixelType_Gvsp_RGB8_Packed:
        case PixelType_Gvsp_BGR8_Packed:
        case PixelType_Gvsp_RGBA8_Packed:
        case PixelType_Gvsp_BGRA8_Packed:
        case PixelType_Gvsp_YUV422_Packed:
        case PixelType_Gvsp_YUV422_YUYV_Packed:

            return true;
        default:
            return false;
        }
    }

    bool IsMonoPixelFormat(MvGvspPixelType enType){
        switch (enType)
        {
        case PixelType_Gvsp_Mono8:
        case PixelType_Gvsp_Mono10:
        case PixelType_Gvsp_Mono10_Packed:
        case PixelType_Gvsp_Mono12:
        case PixelType_Gvsp_Mono12_Packed:
            return true;
        default:
            return false;
        }
    }

#if USE_OPENCV
    bool Convert2Mat(MV_FRAME_OUT_INFO_EX* pstImageInfo, unsigned char* pData, cv::Mat& image){
        if (pstImageInfo -> enPixelType == PixelType_Gvsp_Mono8){
            image = cv::Mat(pstImageInfo->nHeight, pstImageInfo->nWidth, CV_8UC1, pData);
            return 0;
        }
        else if ((pstImageInfo->enPixelType == PixelType_Gvsp_RGB8_Packed) || (pstImageInfo->enPixelType == 35127317)){
            RGB2BGR(pData, pstImageInfo->nWidth, pstImageInfo->nHeight);
            image = cv::Mat(pstImageInfo->nHeight, pstImageInfo->nWidth, CV_8UC3, pData);
            return 0;
        }
        else{
            logger.error("相机像素格式不支持");
            return 1;
        }if (image.data == NULL) { return 1; }
        image.release();
        return true;
    };
#endif
#if USE_HALCON

    int splitRGB(uchar* pRgbData, uint width, uint height) {
        if (NULL == pRgbData) { return MV_E_PARAMETER; }
        imgbuf[0].clear(); imgbuf[1].clear(); imgbuf[2].clear();
        for (uint j = 0; j < height; j++) {
            for (uint i = 0; i < width; i++) {
                imgbuf[0].push_back(pRgbData[j * (width * 3) + i * 3]);
                imgbuf[1].push_back(pRgbData[j * (width * 3) + i * 3 + 1]);
                imgbuf[2].push_back(pRgbData[j * (width * 3) + i * 3 + 2]);
            }
        }
        return MV_OK;
    }

    int RGBtoHObject(HObject* Hobj, int h, int w, uchar* pData){
        if (NULL == Hobj || NULL == pData){return MV_E_PARAMETER;}
        GenImageInterleaved(Hobj, (Hlong)pData, "rgb", w, h, -1, "byte", 0, 0, 0, 0, -1, 0);
        return MV_OK;
    }

    int mono8toHObject(HObject* Hobj, int h, int w, uchar* pData){
        if (NULL == Hobj || NULL == pData){return MV_E_PARAMETER;}
        GenImage1(Hobj, "byte", w, h, (Hlong)pData);
        return MV_OK;
    }
#endif
};

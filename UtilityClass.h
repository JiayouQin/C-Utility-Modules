#pragma once
#define WIN32_LEAN_AND_MEAN
#include <fstream>
#include <map>
#include <json.hpp>
#include "CustomLogger.h"
using json = nlohmann::json;

/*
由于向后兼容原因该模块暂时保留自动读取参数
*/

class UtilityClass {
public:
    bool matrixUpdated = false;
    std::map<std::string, std::string> camSerials = {};
    bool paramsLoaded = false;
    float angle;
private:
    double savedMatrix[6] = {0,0,0,0,0,0};
    CustomLogger* logger;
    
public:
    static UtilityClass& get(void* logger = NULL) {
        static UtilityClass instance(logger);
        return instance;
    }
    UtilityClass(UtilityClass const&) = delete;
    void operator=(UtilityClass const&) = delete;

    static std::string currentDateTime() {
        time_t     now = time(0);
        struct tm  tstruct;
        char       buf[80];
        localtime_s(&tstruct, &now);
        strftime(buf, sizeof(buf), "%Y-%m-%d %X", &tstruct);
        std::string output = buf;
        std::replace(output.begin(), output.end(), ':', '_'); // replace all 'x' to 'y'
        return output;
    }

    float average(std::vector<float> const& v) {
        if (v.empty()) {return -1;}
        auto const count = static_cast<float>(v.size());
        return std::reduce(v.begin(), v.end()) / count;
    }


#if USE_OPENCV

    int calculateCoordinates(int x, int y, double& dx, double& dy) {
        if (matrixUpdated != true) { return -1; };
        dx = -((savedMatrix[0] * x) + (savedMatrix[1] * y) + savedMatrix[2]);
        dy = -((savedMatrix[3] * x) + (savedMatrix[4] * y) + savedMatrix[5]);
        return 0;
    }

    int saveMatrix(std::vector<cv::Point2f> camPoints, std::vector<float> angles)
    {
        if (camPoints.size() < 5) { return -1; }
        cv::FileStorage store("coordinates.bin", cv::FileStorage::WRITE);
        cv::write(store, "camPoints", camPoints);
        store.release();
        cv::FileStorage store2("angles.bin", cv::FileStorage::WRITE);
        cv::write(store2, "angles", angles);
        store.release();
        return 0;
    }
    
    int loadMatrix() {
        std::vector<cv::Point2f> camPoints;
        cv::FileStorage store("coordinates.bin", cv::FileStorage::READ);
        cv::FileNode n1 = store["camPoints"];
        cv::read(n1, camPoints);
        store.release();
        std::vector<float> angles;
        cv::FileStorage store2("angles.bin", cv::FileStorage::READ);
        cv::FileNode n2 = store2["angles"];
        cv::read(n2, angles);
        store.release();
        logger->info("标定坐标:");
        for (int i = 0; i < camPoints.size(); i++) {
            logger->info("[{}, {}]",camPoints[i].x, camPoints[i].y);
        }
        if (camPoints.size() < 9) {
            logger->warn("标定点不足({})，精度下降:",camPoints.size());

            if (camPoints.size() < 5) {
                logger->error("无法加载标定数据（点数小于5）");
                return -1;
            }
        }
        angle = average(angles);
        if (angle < 0) {
            logger->error("无法加载标定角度");
            return -1;
        }
        updateMatrix(camPoints, angle);
        return 0;
    }

    void updateMatrix(std::vector<cv::Point2f> camPointsTable, float angle, float distance = 2.0) {
        std::vector<cv::Point2f> worldPointsTable;
        std::vector<cv::Point2f> worldPoints;
        std::vector<cv::Point2f> camPoints;
        for (int i = 0; i < 9; i++){
            float x = ((i % 3)-1) * distance;
            float y = (1 - i/3) * distance;
            worldPointsTable.push_back(cv::Point2f(x, y));
            logger->trace("插入相对世界坐标 {},{}",x, y);
        }
        for (int i = 0; i < 9; i++) {
            if ((camPointsTable.at(i).x < 99998) && (camPointsTable.at(i).y < 99998))
            {
                worldPoints.push_back(worldPointsTable.at(i));
                camPoints.push_back(camPointsTable.at(i));
            }
        }
        if (worldPoints.size() < 5) {
            logger->error("标定点不足");
            return;
        }
        std::vector<uchar> inliers(camPoints.size(), 0);
        cv::Mat affine1 = cv::estimateAffine2D(camPoints, worldPoints, inliers);
        double* Ppixel = (double*)affine1.data;
        for (int i = 0; i < 6; i++) {
            savedMatrix[i] = Ppixel[i];
            matrixUpdated = true;
        }
        logger->info("变换矩阵已加载");
    }

    #if !CONSOLE_MODE
        //将OpenCV图像转换为Pixmap
    QPixmap MatToPixmap(cv::Mat src) {
        QImage::Format format = QImage::Format_Grayscale8;
        int bpp = src.channels();
        if (bpp == 3)format = QImage::Format_RGB888;
        QImage img(src.cols, src.rows, format);
        uchar* sptr, * dptr;
        int linesize = src.cols * bpp;
        for (int y = 0; y < src.rows; y++) {
            sptr = src.ptr(y);
            dptr = img.scanLine(y);
            memcpy(dptr, sptr, linesize);
        }
        if (bpp == 3) return QPixmap::fromImage(img.rgbSwapped());
        return QPixmap::fromImage(img);
    }
    #endif
#endif
#if USE_HALCON == true

    #if !CONSOLE_MODE
    static void HObjectToQImage(HObject Hobj, QImage& qImage) { HImagetoQimage((HImage)Hobj, qImage);}

    static void HImagetoQimage(HImage Himage,QImage& qImage) {
        Hlong Hwidth;
        Hlong Hheight;
        Himage.GetImageSize(&Hwidth, &Hheight);
        HTuple channels = Himage.CountChannels();
        HTuple type = Himage.GetImageType();
        if (strcmp(type[0].S(), "byte")) { return;}

        QImage::Format format;
        if (channels[0].I() == 1) {format = QImage::Format_Grayscale8;}
        else { format = QImage::Format_RGB32; }

        if (qImage.width() != Hwidth ||
            qImage.height() != Hheight ||
            qImage.format() != format) {
            qImage = QImage((int)Hwidth,(int)Hheight,format);
        }
        HString Type;
        if (channels[0].I() == 1){
            uchar* pSrc = (uchar*)(Himage.GetImagePointer1(&Type, &Hwidth, &Hheight));
            memcpy(qImage.bits(), pSrc, (size_t)(Hwidth* Hheight));
        }
        else if (channels[0].I() == 3){
            uchar* R, * G, * B;
            Himage.GetImagePointer3((void**)(&R), (void**)(&G), (void**)(&B), &Type, &Hwidth, &Hheight);
            for (int row = 0; row < Hheight; row++){
                QRgb* line = (QRgb*)(qImage.scanLine(row));
                for (int col = 0; col < Hwidth; col++){
                    line[col] = qRgb(*R++, *G++, *B++);
                }
            }
        }
        return;
    }
    #endif
#endif
    
    //过时方式，转用ParamLoader模块
    int saveDefaultParams() {   //生成默认参数并储存到根目录下 params.j 中
        logger->info("生成默认参数中");
        json params = {
            {"strictMode",true},
            {"debug",true},
            {"trigger",true},
            {"showImage", true},
            {"exposureTime", 3000},
            {"initialized", 1},
            {"saveImage",false},
            {"cam1",""},
            {"cam2",""},
            {"cam3",""},
            {"cam4",""},
        };
        std::ofstream o("params.j");
        o << std::setw(4) << params << std::endl;
        return 0;
    }

    //从 params.j 中读取参数
    int loadParams(int& debug, int& trigger, int& showImage, int& exposureTime, int& saveImage) {
        json params;
        try {
            std::ifstream f("params.j");
            f >> params;
            trigger = params["trigger"];
            debug = params["debug"];
            showImage = params["showImage"];
            exposureTime = params["exposureTime"];
            saveImage = params["saveImage"];
            camSerials.insert({ "cam1",params["cam1"] });
            camSerials.insert({ "cam2",params["cam2"] });
            camSerials.insert({ "cam3",params["cam3"] });
            camSerials.insert({ "cam4",params["cam4"] });
            paramsLoaded = true;
            return 0;
        }
        catch (std::exception e) { //未找到文件会出错
            logger->warn("未找到配置文件或配置文件错误");
            saveDefaultParams();
            return loadParams(debug, trigger, showImage, exposureTime,saveImage);
        }
        return -1;
    }
private:
    UtilityClass(void* logger) {
        this->logger = (CustomLogger*)logger;
    }
};


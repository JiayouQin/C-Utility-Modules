#pragma once
#include <json.hpp>
#include <fstream>
using json = nlohmann::json;

class Params {
public:
    Params(Params const&) = delete;
    void operator=(Params const&) = delete;
    static Params& get() {
        static Params instance;
        instance.loadParams();
        return instance;
    }
     static std::map<std::string, std::string> camSerials;
     static bool paramsLoaded;
     static bool debug;
     static bool strictMode;
     static bool trigger;
     static bool showImage;
     static std::vector<int> exposureTime;
     static std::vector<int> exposureTime2;
     static int testVal;
     static int pulses;
     static int pulses2;
     static std::string motorSerial;


    int loadParams() {//从 params.j 中读取参数
        json params;
        try {
            std::ifstream f("params.j");
            f >> params;
            debug = params["debug"];
            trigger = params["trigger"];
            exposureTime.clear();
            for (auto& e : params["exposureTime"])
                exposureTime.push_back(e);
            exposureTime2.clear();
            for (auto& e : params["exposureTime2"])
                exposureTime2.push_back(e);

            showImage = params["showImage"];
            camSerials.insert({ "cam1",params["cam1"] });
            camSerials.insert({ "cam2",params["cam2"] });
            camSerials.insert({ "cam3",params["cam3"] });
            camSerials.insert({ "cam4",params["cam4"] });
            paramsLoaded = true;
            pulses = params["pulses"];
            pulses2 = params["pulses2"];
            motorSerial = params["motorSerial"];


            return 0;
        }
        catch (std::exception e) { //未找到文件
            saveParams();
            return loadParams();
        }
        return -1;
    }

    static int saveParams() {
        json params = {
            {"strictMode", strictMode},
            {"debug", debug},
            {"trigger", trigger},
            {"exposureTime", exposureTime},
            {"exposureTime2", exposureTime2},
            {"showImage", showImage},
            {"pulses", pulses},
            {"pulses2", pulses2},
            {"motorSerial",motorSerial},
            {"cam1", camSerials["cam1"]},
            {"cam2", camSerials["cam2"]},
            {"cam3", camSerials["cam3"]},
            {"cam4", camSerials["cam4"]},
        };
        std::ofstream o("params.j");
        o << std::setw(4) << params << std::endl;
        return 0;
        }
private:
    Params() {
        camSerials = {};
        paramsLoaded = false;
        loadParams();
    }
};

std::map<std::string, std::string> Params::camSerials{};
int Params::testVal{ 123 };
int Params::pulses{ 300 };
int Params::pulses2{ 1400 };
bool Params::showImage{ 0 };
bool Params::paramsLoaded{0};
bool Params::debug{0};
bool Params::strictMode{0};
bool Params::trigger{0};
std::vector<int> Params::exposureTime{ {50,3000,3000,3000} };
std::vector<int> Params::exposureTime2{ {3000,3000,3000,3000} };
std::string Params::motorSerial{ "COM3" };
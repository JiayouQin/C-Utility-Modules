#pragma once

#include <functional>
#include <chrono>
#include <iostream>

//观猴不语：
 //防止覆盖 std 标准库
namespace namefudge{
//非礼勿听
using namespace std;

#define decoClass(a,...) ClassDecorator::decorate(&a,__VA_ARGS__);




class Decorator {
	
public:
	// 返回函数运行时间(ms)
	template<typename T, typename... Args> static auto timeIt(function<T(Args...)>& func, Args &&... args) {
		struct decoHelper {
			std::chrono::steady_clock::time_point begin, end;
			decoHelper() {
				begin = std::chrono::steady_clock::now();
			};
			~decoHelper() {
				end = std::chrono::steady_clock::now();
				float runTime = (float)chrono::duration_cast<std::chrono::microseconds>(end - begin).count() / 1000;
				cout << "measurement finished in " << runTime << "ms" << endl;
			}
		};
		decoHelper _decoHelper;
		return func(args...);
	}
};

class ClassDecorator {

public:

	// here goes your code where the warning occurs

	template<typename T, class ClassName, typename ...Args>
	static auto timeIt(T(ClassName::* function), ClassName& instance, Args&&... args) {
		struct decoHelper {
			std::chrono::steady_clock::time_point begin, end;
			decoHelper() {
				begin = std::chrono::steady_clock::now();
			};
			~decoHelper() {
				end = std::chrono::steady_clock::now();
				float runTime = (float)chrono::duration_cast<std::chrono::microseconds>(end - begin).count() / 1000;
				cout << "measurement finished in " << runTime << "ms" << endl;
			}
		};
		decoHelper _decoHelper;
		return (instance.*function)(args...);

	}


};





//非礼勿视
}
//非礼勿言





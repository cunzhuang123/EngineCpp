#ifndef SCOPEDPROFILER_H
#define SCOPEDPROFILER_H

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

using namespace std;
using namespace std::chrono;

// 使用 thread_local 变量记录当前的调用缩进级别（多线程环境下不会冲突）
extern thread_local int currentIndent;

class ScopedProfiler {
public:
    // 构造函数中记录进入方法的时间，同时输出进入信息
    explicit ScopedProfiler(const string &funcName);
    // 析构函数中计算方法耗时，并输出相应信息
    ~ScopedProfiler();
    
private:
    string funcName;                          // 方法名称，用于输出
    int indent;                               // 当前方法进入时的缩进级别
    steady_clock::time_point start;           // 方法开始调用时的时间点
};

#endif // SHADERMANAGER_H
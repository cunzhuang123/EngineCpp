#include "ScopedProfiler.h"

thread_local int currentIndent = 0;

ScopedProfiler::ScopedProfiler(const string &funcName): funcName(funcName), indent(currentIndent), start(steady_clock::now()) {
    cout << string(indent * 2, ' ') << "进入 " << funcName << endl;
    // 进入方法后增加缩进级别
    ++currentIndent;
}
ScopedProfiler::~ScopedProfiler() {
    // 离开方法前先恢复缩进级别
    --currentIndent;
    auto end = steady_clock::now();
    auto duration = duration_cast<microseconds>(end - start).count();
    cout << string(indent * 2, ' ') << "退出 " << funcName << ", 耗时: " << duration/1000.0f << " 毫秒" << endl;
}

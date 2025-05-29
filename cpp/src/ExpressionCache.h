//
// Created by caoqiwen on 25-5-27.
//

#ifndef EXPRESSIONCACHE_H
#define EXPRESSIONCACHE_H

#include <memory>
#include <unordered_map>
#include <mutex>
#include <string>
#include <vector>

struct UniformValue;

class CachedExprtk;

class ExpressionCache {
public:
    static ExpressionCache& getInstance();

    double evaluate(const std::string& expr, const std::unordered_map<std::string, UniformValue>& variables);

private:
    ExpressionCache();
    ~ExpressionCache();

    class Impl;
    std::unique_ptr<Impl> pImpl;

    std::unordered_map<std::string, CachedExprtk> cacheMap;
    std::mutex mutex_;  // 用于线程同步

    void initializeVariables(CachedExprtk& cache, const std::unordered_map<std::string, UniformValue>& variables);

    // void compileExpression(const std::string& expr, CachedExprtk& cache);

    double getScalarValue(const UniformValue& value);
    void updateVectorValue(std::vector<double>& target, const UniformValue& value);
    void updateVariables(CachedExprtk& cache,
                        const std::unordered_map<std::string, UniformValue>& variables);
};



#endif //EXPRESSIONCACHE_H

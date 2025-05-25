//
// Created by caoqiwen on 25-5-25.
//

#ifndef EXPRESSIONCACHE_H
#define EXPRESSIONCACHE_H

#include <iostream>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include <stdexcept>
#include <regex>
#include <memory>
#include <unordered_set>
#include <mutex>

// 包含 ExprTk 头文件，请确保路径正确
#include "../exprtk/exprtk.hpp"

// 包含 nlohmann::json 头文件 (https://github.com/nlohmann/json)
#include "../nlohmann/json.hpp"

// GLM 头文件
#include <glm/glm.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>


#include "Materials.h"

using json = nlohmann::json;

struct CachedExpression {
    exprtk::expression<double> expression;
    std::unordered_map<std::string, double> scalarVars;
    std::unordered_map<std::string, std::vector<double>> vectorVars;
    exprtk::symbol_table<double> symbolTable;  // 用于注册变量
};

class ExpressionCache {
public:
    static ExpressionCache& getInstance() {
        static ExpressionCache instance;
        return instance;
    }

    double evaluate(const std::string& expr, const std::unordered_map<std::string, UniformValue>& variables) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = cacheMap.find(expr);
        if (it == cacheMap.end()) {
            // 第一次处理该表达式
            CachedExpression cache;
            initializeVariables(cache, variables);
            compileExpression(expr, cache);
            it = cacheMap.emplace(expr, std::move(cache)).first;
        }

        updateVariables(it->second, variables);
        return it->second.expression.value();
    }


private:
    std::unordered_map<std::string, CachedExpression> cacheMap;
    std::mutex mutex_;  // 用于线程同步

    void initializeVariables(CachedExpression& cache, const std::unordered_map<std::string, UniformValue>& variables) {
        for (const auto& [name, value] : variables) {
            switch (value.type) {
                case UniformType::Int:
                case UniformType::Float:
                    cache.scalarVars[name] = (value.type == UniformType::Int)
                        ? static_cast<double>(std::get<int>(value.value))
                        : static_cast<double>(std::get<float>(value.value));
                    cache.symbolTable.add_variable(name, cache.scalarVars[name]);
                    break;

                case UniformType::Vec2f: {
                    glm::vec2 vec = std::get<glm::vec2>(value.value);
                    std::vector<double> data(3);  // dummy at index 0
                    data[1] = static_cast<double>(vec.x);
                    data[2] = static_cast<double>(vec.y);
                    cache.vectorVars[name] = std::move(data);
                    cache.symbolTable.add_vector(name, cache.vectorVars[name]);
                    break;
                }

                case UniformType::Vec3f: {
                    glm::vec3 vec = std::get<glm::vec3>(value.value);
                    std::vector<double> data(4);  // dummy at index 0
                    data[1] = static_cast<double>(vec.x);
                    data[2] = static_cast<double>(vec.y);
                    data[3] = static_cast<double>(vec.z);
                    cache.vectorVars[name] = std::move(data);
                    cache.symbolTable.add_vector(name, cache.vectorVars[name]);
                    break;
                }

                case UniformType::Vec4f: {
                    glm::vec4 vec = std::get<glm::vec4>(value.value);
                    std::vector<double> data(5);  // dummy at index 0
                    data[1] = static_cast<double>(vec.x);
                    data[2] = static_cast<double>(vec.y);
                    data[3] = static_cast<double>(vec.z);
                    data[4] = static_cast<double>(vec.w);
                    cache.vectorVars[name] = std::move(data);
                    cache.symbolTable.add_vector(name, cache.vectorVars[name]);
                    break;
                }

                default:
                    std::cerr << "Unsupported uniform type: " << static_cast<int>(value.type) << std::endl;
            }
        }
    }

    void compileExpression(const std::string& expr, CachedExpression& cache) {
        cache.expression.register_symbol_table(cache.symbolTable);
        exprtk::parser<double> parser;
        if (!parser.compile(expr, cache.expression)) {
            throw std::runtime_error("Failed to compile expression: " + expr);
        }
    }

    double getScalarValue(const UniformValue& value) {
        switch (value.type) {
            case UniformType::Int:
                return static_cast<double>(std::get<int>(value.value));
            case UniformType::Float:
                return static_cast<double>(std::get<float>(value.value));
            default:
                std::cerr << "Error: Attempted to get scalar value from non-scalar type "
                        << static_cast<int>(value.type) << std::endl;
                return 0.0;
        }
    }
    void updateVectorValue(std::vector<double>& target, const UniformValue& value) {
        switch (value.type) {
            case UniformType::Vec4f: {
                glm::vec4 vec4 = std::get<glm::vec4>(value.value);
                if (target.size() < 5) {
                    std::cerr << "Error: Vector size mismatch for Vec4f" << std::endl;
                    return;
                }
                target[1] = static_cast<double>(vec4.x);
                target[2] = static_cast<double>(vec4.y);
                target[3] = static_cast<double>(vec4.z);
                target[4] = static_cast<double>(vec4.w);
                break;
            }
            case UniformType::Vec3f: {
                glm::vec3 vec3 = std::get<glm::vec3>(value.value);
                if (target.size() < 4) {
                    std::cerr << "Error: Vector size mismatch for Vec3f" << std::endl;
                    return;
                }
                target[1] = static_cast<double>(vec3.x);
                target[2] = static_cast<double>(vec3.y);
                target[3] = static_cast<double>(vec3.z);
                break;
            }
            case UniformType::Vec2f: {
                glm::vec2 vec2 = std::get<glm::vec2>(value.value);
                if (target.size() < 3) {
                    std::cerr << "Error: Vector size mismatch for Vec2f" << std::endl;
                    return;
                }
                target[1] = static_cast<double>(vec2.x);
                target[2] = static_cast<double>(vec2.y);
                break;
            }
            default:
                std::cerr << "Error: Attempted to update vector with non-vector type "
                        << static_cast<int>(value.type) << std::endl;
        }
    }
    void updateVariables(CachedExpression& cache,
                        const std::unordered_map<std::string, UniformValue>& variables) {
        // 仅更新已存在的变量值（避免重复创建）
        for (auto& varPair : variables) {
            const auto& name = varPair.first;
            const auto& value = varPair.second;

            // 处理标量更新
            auto scalarIt = cache.scalarVars.find(name);
            if (scalarIt != cache.scalarVars.end()) {
                // 更新现有变量值
                scalarIt->second = getScalarValue(value);
            }

            // 处理向量更新
            auto vectorIt = cache.vectorVars.find(name);
            if (vectorIt != cache.vectorVars.end()) {
                updateVectorValue(vectorIt->second, value);
            }
        }
    }
};


#endif //EXPRESSIONCACHE_H

// CachedExprtk.h
#ifndef CACHEDEXPRTK_H
#define CACHEDEXPRTK_H

#include <iostream>
#include <regex>

#include "../tinyexpr/tinyexpr.h"
#include <unordered_map>
#include <vector>
#include <string>
#include <stdexcept>

class CachedExprtk {
private:
    te_expr* expr_;
    std::vector<te_variable> vars_;
    std::string last_expression_;
    std::vector<std::string> var_name_storage_;

public:
    std::unordered_map<std::string, double> scalarVars;
    std::unordered_map<std::string, std::vector<double>> vectorVars;

    // 完全避开初始化列表
    CachedExprtk() {
        expr_ = nullptr;
    }

    ~CachedExprtk() {
        if (expr_ != nullptr) {
            te_free(expr_);
            expr_ = nullptr;
        }
    }

    // 拷贝构造函数
    CachedExprtk(const CachedExprtk& other) {
        expr_ = nullptr;
        scalarVars = other.scalarVars;
        vectorVars = other.vectorVars;
        last_expression_ = other.last_expression_;
        var_name_storage_ = other.var_name_storage_;
        vars_ = other.vars_;

        if (!last_expression_.empty()) {
            compileExpression(last_expression_);
        }
    }

    // 赋值操作符
    CachedExprtk& operator=(const CachedExprtk& other) {
        if (this != &other) {
            if (expr_ != nullptr) {
                te_free(expr_);
                expr_ = nullptr;
            }

            scalarVars = other.scalarVars;
            vectorVars = other.vectorVars;
            last_expression_ = other.last_expression_;
            var_name_storage_ = other.var_name_storage_;
            vars_ = other.vars_;

            if (!last_expression_.empty()) {
                compileExpression(last_expression_);
            }
        }
        return *this;
    }


    bool compileExpression(const std::string& expression_str) {
        last_expression_ = expression_str;

        if (expr_ != nullptr) {
            te_free(expr_);
            expr_ = nullptr;
        }

        // 清空之前的存储
        vars_.clear();
        var_name_storage_.clear();

        // 先收集所有需要的变量名，确保内存布局稳定
        std::vector<std::string> all_var_names;

        // 收集标量变量名
        for (const auto& pair : scalarVars) {
            all_var_names.push_back(pair.first);
        }

        // 收集向量变量名
        for (const auto& pair : vectorVars) {
            const std::string& vectorName = pair.first;
            const std::vector<double>& vec = pair.second;

            for (size_t i = 0; i < vec.size(); ++i) {
                std::string varName = vectorName + "_" + std::to_string(i);
                all_var_names.push_back(varName);
            }
        }

        // 一次性分配所有字符串存储，避免重新分配
        var_name_storage_ = std::move(all_var_names);

        // 现在构建 te_variable 数组
        size_t var_index = 0;

        // 添加标量变量
        for (auto& pair : scalarVars) {
            te_variable var;
            var.name = var_name_storage_[var_index].c_str();
            var.address = &pair.second;
            var.type = 0;  // TE_VARIABLE
            var.context = nullptr;
            vars_.push_back(var);
            var_index++;
        }

        // 添加向量变量
        for (auto& pair : vectorVars) {
            std::vector<double>& vec = pair.second;

            for (size_t i = 0; i < vec.size(); ++i) {
                te_variable var;
                var.name = var_name_storage_[var_index].c_str();
                var.address = &vec[i];
                var.type = 0;  // TE_VARIABLE
                var.context = nullptr;
                vars_.push_back(var);
                var_index++;
            }
        }

        // 预处理表达式，将 array[1] 转换为 array_1
        std::string processed_expr = preprocessExpression(expression_str);

        // 调试信息
        // std::cout << "Original expression: " << expression_str << std::endl;
        // std::cout << "Processed expression: " << processed_expr << std::endl;
        // std::cout << "Variables registered:" << std::endl;
        // for (const auto& var : vars_) {
        //     std::cout << "  " << var.name << " = " << *(const double*)var.address << std::endl;
        // }

        int error_pos = 0;
        expr_ = te_compile(processed_expr.c_str(),
                          vars_.data(), static_cast<int>(vars_.size()), &error_pos);

        if (expr_ == nullptr) {
            std::cerr << "Failed to compile expression: " << processed_expr << std::endl;
            std::cerr << "Error at position: " << error_pos << std::endl;
            if (error_pos < processed_expr.length()) {
                std::cerr << "Near: '" << processed_expr.substr(error_pos) << "'" << std::endl;
            }
            return false;
        }

        // std::cout << "Expression compiled successfully!" << std::endl;
        return true;
    }
    // 添加表达式预处理函数
    std::string preprocessExpression(const std::string& expr) {
        std::string result = expr;

        // 将 array[1] 转换为 array_1
        std::regex arrayPattern(R"((\w+)\[(\d+)\])");
        result = std::regex_replace(result, arrayPattern, "$1_$2");

        return result;
    }

    double getValue() {
        if (expr_ == nullptr) {
            return 0.0;
        }
        return te_eval(expr_);
    }

    void addVariable(const std::string& name, double value) {
        scalarVars[name] = value;
    }

    void addVector(const std::string& name, const std::vector<double>& vec) {
        vectorVars[name] = vec;
    }

    void updateVariable(const std::string& name, double value) {
        auto it = scalarVars.find(name);
        if (it != scalarVars.end()) {
            it->second = value;
        }
    }

    void updateVector(const std::string& name, const std::vector<double>& vec) {
        auto it = vectorVars.find(name);
        if (it != vectorVars.end()) {
            it->second = vec;
            if (!last_expression_.empty()) {
                compileExpression(last_expression_);
            }
        }
    }

    void updateVectorElement(const std::string& name, size_t index, double value) {
        auto it = vectorVars.find(name);
        if (it != vectorVars.end() && index < it->second.size()) {
            it->second[index] = value;
        }
    }

    static void compileExpression(const std::string& expr, CachedExprtk& cache) {
        if (!cache.compileExpression(expr)) {
            throw std::runtime_error("Failed to compile expression: " + expr);
        }
    }
};

#endif //CACHEDEXPRTK_H
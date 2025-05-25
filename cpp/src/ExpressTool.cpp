#include "ExpressTool.h"
#include "../CoreUtils.h"
#include "ScopedProfiler.h"
#include "ExpressionCache.h"

// #include "../nlohmann/json.hpp"
// using json = nlohmann::json;

#include "Materials.h"

std::unordered_map<std::string, std::string> ExpressTool::transformExpressionCache;

/// 利用正则表达式提取表达式中所有标识符（符合 [A-Za-z_][A-Za-z0-9_]* ）
/// 仅返回那些在 candidateSet 中的标识符
std::vector<std::string> ExpressTool::extractIdentifiers(const std::string &expr,
                                              const std::unordered_set<std::string>& candidateSet)
{
    std::vector<std::string> ids;
    std::regex re("([A-Za-z_][A-Za-z0-9_]*)");
    auto begin = std::sregex_iterator(expr.begin(), expr.end(), re);
    auto end = std::sregex_iterator();
    for (auto it = begin; it != end; ++it)
    {
        std::string token = it->str();
        if (candidateSet.find(token) != candidateSet.end())
            ids.push_back(token);
    }
    return ids;
}

/// 尝试去掉表达式外层一对多余的括号（要求成对且包围整个字符串）
std::string ExpressTool::removeOuterParentheses(const std::string &expr)
{
    std::string s = CoreUtils::trim(expr);
    if (s.size() >= 2 && s.front() == '(' && s.back() == ')')
    {
        int count = 0;
        bool valid = true;
        for (size_t i = 0; i < s.size(); ++i)
        {
            if (s[i] == '(')
                count++;
            else if (s[i] == ')')
                count--;
            // 在末尾之前 count 应该不为 0
            if (count == 0 && i < s.size() - 1)
            {
                valid = false;
                break;
            }
        }
        if (valid && count == 0)
            return s.substr(1, s.size() - 2);
    }
    return s;
}

// --- 依赖闭包计算与递归内联 ---------------------------------------------

// 递归帮助函数：从表达式 expr 出发，收集所有在 defMap（候选集合 validVars）中出现的标识符
void ExpressTool::computeClosureHelper(const std::string &expr,
                          const std::unordered_set<std::string> &validVars,
                          const std::unordered_map<std::string, std::string> &defMap,
                          std::unordered_set<std::string> &closure,
                          std::unordered_set<std::string> &visited)
{
    // 本函数专门查找出现在 expr 中且在 validVars 中的标识符
    std::vector<std::string> ids = extractIdentifiers(expr, validVars);
    for (const auto &id : ids)
    {
        if (closure.find(id) == closure.end())
        {
            closure.insert(id);
            // 防止循环递归
            if (visited.find(id) == visited.end())
            {
                visited.insert(id);
                auto it = defMap.find(id);
                if (it != defMap.end())
                {
                    computeClosureHelper(it->second, validVars, defMap, closure, visited);
                }
            }
        }
    }
}

/// 计算最终表达式中所依赖的中间变量集合
std::unordered_set<std::string> ExpressTool::computeClosure(const std::string &expr,
                                                 const std::unordered_set<std::string> &validVars,
                                                 const std::unordered_map<std::string, std::string> &defMap)
{
    std::unordered_set<std::string> closure;
    std::unordered_set<std::string> visited;
    computeClosureHelper(expr, validVars, defMap, closure, visited);
    return closure;
}

/// 递归内联变量。只对在 closure 中的变量做内联替换。内联结果总用 () 包裹。
std::string ExpressTool::resolveVar(const std::string &var,
                       const std::unordered_map<std::string, std::string> &defMap,
                       const std::unordered_set<std::string> &closure,
                       std::unordered_map<std::string, std::string> &memo)
{
    if(memo.find(var) != memo.end())
        return memo[var];
    
    auto it = defMap.find(var);
    if(it == defMap.end())
    {
        // 没有定义，则直接返回变量名
        return var;
    }
    
    std::string raw = it->second; // 如 "sourceWidth + control_size * 2"
    // 构造 candidate 集合：这里候选就是 closure 内的变量
    std::unordered_set<std::string> cand(closure.begin(), closure.end());
    std::vector<std::string> tokens = extractIdentifiers(raw, cand);
    
    for (const auto &subVar : tokens)
    {
        std::regex re("\\b" + subVar + "\\b");
        std::string resolvedSub = resolveVar(subVar, defMap, closure, memo);
        raw = std::regex_replace(raw, re, resolvedSub);
    }
    std::string result = "(" + raw + ")";
    memo[var] = result;
    return result;
}

/// 在最终表达式中，把 closure 内的变量依次替换为它们的内联表达式（从 memo 中取）
std::string ExpressTool::inlineFinalExpression(const std::string &expr,
                                    const std::unordered_set<std::string> &closure,
                                    const std::unordered_map<std::string, std::string> &memo)
{
    std::string result = expr;
    // 对 closure 中的每个变量进行替换
    for (const auto &var : closure)
    {
        std::regex re("\\b" + var + "\\b");
        result = std::regex_replace(result, re, memo.at(var));
    }
    return result;
}

// --- 主转换函数 ---------------------------------------------------------

/*
 * 转换表达式：
 * 本方法只沿最终表达式的依赖链查找中间变量，并将其内联替换。对于本例，最终返回：
 *   sourceWidth + control_size * 2
 */
std::string ExpressTool::transformExpressionSelective(const std::string &input)
{
    // ScopedProfiler findPassProfiler("ExpressTool::transformExpressionSelective " + input);

        // 检查缓存
    auto cacheIt = transformExpressionCache.find(input);
    if (cacheIt != transformExpressionCache.end())
        return cacheIt->second;


    // 1. 拆分语句
    std::vector<std::string> statements = CoreUtils::split(input, ";");
    std::unordered_map<std::string, std::string> defMap; // 保存所有 var 声明
    std::string finalExpr;
    
    for (const auto &stmt : statements)
    {
        if (stmt.find("var ") == 0)
        {
            // 解析形如 "var foo = expression"
            std::string remainder = stmt.substr(4); // 去掉 "var "
            size_t eqPos = remainder.find('=');
            if(eqPos == std::string::npos)
                continue; // 格式出错则跳过
            std::string varName = CoreUtils::trim(remainder.substr(0, eqPos));
            std::string exprPart = CoreUtils::trim(remainder.substr(eqPos + 1));
            defMap[varName] = exprPart;
        }
        else
        {
            // 非 var 声明的语句当作最终表达式（取最后一次出现的）
            finalExpr = stmt;
        }
    }
    
    // 2. 计算依赖闭包：只考虑 defMap 中定义的变量
    std::unordered_set<std::string> validVars;
    for (const auto &p : defMap)
        validVars.insert(p.first);
    
    std::unordered_set<std::string> closure = computeClosure(finalExpr, validVars, defMap);
    
    // 3. 对 closure 中的变量做递归内联替换，缓存结果到 memo 中
    std::unordered_map<std::string, std::string> memo;
    for (const auto &var : closure)
    {
        resolveVar(var, defMap, closure, memo);
    }
    
    // 4. 最终表达式中，替换所有属于 closure 的变量为其内联表达式
    std::string inlinedFinal = inlineFinalExpression(finalExpr, closure, memo);
    
    // 5. 如果内层外层只有一对多余括号则去掉
    std::string result = removeOuterParentheses(inlinedFinal);

    // 存入缓存
    transformExpressionCache[input] = result;

    return result;
}

RenderTargetInfo ExpressTool::findInputeRenderTargetInfo(const std::shared_ptr<Material> pass)
{
    for (auto& uniformPair : pass->uniforms) {
        std::string uniformName = uniformPair.first;
        UniformValue& uniform = uniformPair.second;
        if (uniform.type == UniformType::RenderTarget) {
            return std::get<RenderTargetInfo>(uniform.value);
        }
        else if (uniform.type == UniformType::MaterialPtr) {
            return findInputeRenderTargetInfo(std::get<std::shared_ptr<Material>>(uniform.value));
        }
    }
    return RenderTargetInfo();
}

std::unordered_map<std::string, UniformValue> ExpressTool::collectMaterialExpressValue(std::shared_ptr<RendererResource> rendererResource, std::string rendererName, std::shared_ptr<Material> rendererMaterial, const json& plugin, int pluginIndex, RenderTargetInfo defaultSequenceRenderTarget)
{
    // ScopedProfiler profiler("ExpressTool::collectMaterialExpressValue " + rendererName);

    std::unordered_map<std::string, UniformValue> result;

    if (pluginIndex <= 0)
    {
        if(rendererResource)
        {
            UniformValue uv;
            uv.type = UniformType::Int;
            uv.value = static_cast<int>(rendererResource->getSourceWidth());
            result["sourceWidth"] = uv;
    
            uv.type = UniformType::Int;
            uv.value = static_cast<int>(rendererResource->getSourceHeight());
            result["sourceHeight"] = uv;    
        }
        else 
        {
            std::string passEndName = rendererName + "_plugin_" + std::to_string(pluginIndex);
            std::vector<std::shared_ptr<Material>> targetPasses;
            std::shared_ptr<Material> rootPass = std::get<std::shared_ptr<Material>>(rendererMaterial->uniforms["u_texture"].value);
            if (!rootPass) {
                // 若为空则无法计算，直接返回
                return result;
            }
            findPass(rootPass, passEndName, targetPasses);
            auto targetPass = targetPasses[0];

            auto renderTargetInfo = findInputeRenderTargetInfo(targetPass);
            if(renderTargetInfo.width > 0 && renderTargetInfo.height > 0)
            {
                UniformValue uv;
                uv.type = UniformType::Int;
                uv.value = renderTargetInfo.width;
                result["sourceWidth"] = uv;
        
                uv.type = UniformType::Int;
                uv.value = renderTargetInfo.height;
                result["sourceHeight"] = uv;    
            }
            else 
            {
                UniformValue uv;
                uv.type = UniformType::Int;
                uv.value = defaultSequenceRenderTarget.width;
                result["sourceWidth"] = uv;
        
                uv.type = UniformType::Int;
                uv.value = defaultSequenceRenderTarget.height;
                result["sourceHeight"] = uv;    
            }
        }
    }
    else {
        std::string passEndName = rendererName + "_plugin_" + std::to_string(pluginIndex - 1);
        std::vector<std::shared_ptr<Material>> targetPasses;
        std::shared_ptr<Material> rootPass = std::get<std::shared_ptr<Material>>(rendererMaterial->uniforms["u_texture"].value);
        if (!rootPass) {
            // 若为空则无法计算，直接返回
            return result;
        }
        findPass(rootPass, passEndName, targetPasses);
        auto targetPass = targetPasses[0];

        UniformValue uv;
        uv.type = UniformType::Int;
        uv.value = targetPass->renderTargetInfo.width;
        result["sourceWidth"] = uv;

        uv.type = UniformType::Int;
        uv.value = targetPass->renderTargetInfo.height;
        result["sourceHeight"] = uv;
    }
        
    const json& controlData = plugin["control"];

    for (auto it = controlData.begin(); it != controlData.end(); ++it)
    {
        std::string key = "control_" + it.key();
        UniformValue uv = convertJsonValueToUniformValue(it.value());
        result[key] = uv;
    }

    return result;
}

/// 使用 ExprTk 根据给定的表达式字符串和变量表求值。
///
/// 注意：ExprTk 只能直接处理 double 类型的标量，因此对于 UniformValue 中的数值，
/// 1. 若类型为 Int 或 Float，则直接注册变量。
/// 2. 若类型为 Vec4f 或 Vec2f，则将其拆分为一个 std::vector<double>
///    并在 vector 的第 0 个位置设置 dummy 值，从而保证数组下标从 1 开始访问，
///    使表达式中可以像 mathjs 那样使用 v[1]、v[2] 的方式访问。

double ExpressTool::evaluateExpression(const std::string &expressionText,
                          const std::unordered_map<std::string, UniformValue>& variableMap)
{
    // ScopedProfiler profiler("ExpressTool::evaluateExpression " + expressionText);
    static ExpressionCache& exprCache = ExpressionCache::getInstance();
    return exprCache.evaluate(expressionText, variableMap);
}


/// 辅助函数：将 JSON 数据转换为 UniformValue，仅支持 Int、Float 以及数组（转换为 Vec2f 与 Vec4f）。
 UniformValue ExpressTool::convertJsonValueToUniformValue(const json& j)
{
    UniformValue uv;
    if (j.is_number_integer())
    {
        uv.type = UniformType::Int;
        uv.value = j.get<int>();
    }
    else if (j.is_number_float())
    {
        uv.type = UniformType::Float;
        uv.value = j.get<float>();
    }
    else if (j.is_array())
    {
        // 假定数组元素全为数字，转换为 std::vector<float>
        std::vector<float> values;
        try {
            values = j.get<std::vector<float>>();
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error parsing array: " << e.what() << std::endl;
            uv.type = UniformType::Int; // 默认处理
            uv.value = std::monostate{};
            return uv;
        }
        if (values.size() == 2)
        {
            uv.type = UniformType::Vec2f;
            uv.value = glm::vec2(values[0], values[1]);
        }
        else if (values.size() == 3)
        {
            uv.type = UniformType::Vec3f;
            uv.value = glm::vec3(values[0], values[1], values[2]);
        }
        else if (values.size() == 4)
        {
            uv.type = UniformType::Vec4f;
            uv.value = glm::vec4(values[0], values[1], values[2], values[3]);
        }
        else
        {
            std::cerr << "Unsupported array size: " << values.size() << std::endl;
            uv.type = UniformType::Int;
            uv.value = std::monostate{};
        }
    }
    else
    {
        if (j.is_string())
        {
            std::string str = j.get<std::string>();
            auto color = CoreUtils::convertHexToColorArray(str);
            if (color)
            {
                uv.type = UniformType::Vec4f;
                uv.value = glm::vec4(color->at(0), color->at(1), color->at(2), color->at(3));    
            }
            else 
            {
                std::cerr << "Unsupported json string type: " << j << std::endl;
                uv.type = UniformType::Int;
                uv.value = std::monostate{};        
            }
        }
        else{
            std::cerr << "Unsupported json type: " << j << std::endl;
            uv.type = UniformType::Int;
            uv.value = std::monostate{};    
        }
    }
    return uv;
}

void ExpressTool::findPass(const std::shared_ptr<Material> pass, const std::string &passEndName, std::vector<std::shared_ptr<Material>>& result)
{

    if (CoreUtils::endsWith(pass->passName, passEndName)) {
        // 注意此处将 const_cast 出来，假定后续修改是安全的
        result.push_back(pass);
    }
    for (auto &kv : pass->uniforms) {
        UniformValue uniform = kv.second;
        // 如果该 uniform 中关联了子 Pass，则递归查找
        if (uniform.type == UniformType::MaterialPtr) {
            std::shared_ptr<Material> dependentPass = std::get<std::shared_ptr<Material>>(uniform.value);
            findPass(dependentPass, passEndName, result);
        }
    }
}


void ExpressTool::caculateMaterialExpress(std::string rendererName, std::shared_ptr<Material> rendererMaterial, const std::unordered_map<std::string, UniformValue>& expressValue, int pluginIndex)
{
    // ScopedProfiler profiler("ExpressTool::caculateMaterialExpress " + rendererName);

    // 拼接目标 Pass 名称后缀：例如 "rendererName_plugin_1"
    std::string passEndName = rendererName + "_plugin_" + std::to_string(pluginIndex);
    std::vector<std::shared_ptr<Material>> targetPasses;
    std::shared_ptr<Material> rootPass = std::get<std::shared_ptr<Material>>(rendererMaterial->uniforms["u_texture"].value);
    if (!rootPass) {
        // 若为空则无法计算，直接返回
        return;
    }

    {
        // ScopedProfiler findPassProfiler("ExpressTool::findPass " + passEndName);
        findPass(rootPass, passEndName, targetPasses);
    }

    for (std::shared_ptr<Material> pass : targetPasses) {
        if (!pass->renderTargetInfo.widthExpress.empty())
        {
            auto ep = transformExpressionSelective(pass->renderTargetInfo.widthExpress);
            double newWidth = evaluateExpression(ep, expressValue);
            pass->renderTargetInfo.width = static_cast<int>(newWidth);
        }
        if (!pass->renderTargetInfo.heightExpress.empty())
        {
            auto ep = transformExpressionSelective(pass->renderTargetInfo.heightExpress);
            double newHeight = evaluateExpression(ep, expressValue);
            pass->renderTargetInfo.height = static_cast<int>(newHeight);
        }
        // 遍历 Pass 中的所有 uniforms，若存在表达式，则计算并更新 uniform 的值
        for (auto &kv : pass->uniforms) {
            UniformValue& uniform = kv.second;
            if (!uniform.express.empty())
            {
                auto ep = transformExpressionSelective(uniform.express);
                uniform.value = evaluateParseExpression(uniform.type, ep, expressValue);
            }
        }
    }
}


UniformVariant ExpressTool::evaluateParseExpression(const UniformType& type, const std::string& expr, const std::unordered_map<std::string, UniformValue>& expressValue) {
    // ScopedProfiler profiler("ExpressTool::evaluateParseExpression " + expr);
    // 这只是一个简化示例，你需要开发一个更健壮的解析器
    // 解析 "[control_color[1], control_color[2], control_color[3], 1.0]" 形式的表达式
    // 简单解析示例（非常简化）
    if (type == UniformType::Vec4f) {
        std::string content = expr.substr(1, expr.length() - 2);
        std::vector<std::string> parts = CoreUtils::split(content, ",");
        glm::vec4 vec4Value;
        for (int i = 0; i < parts.size(); i++)
        {
            double value = evaluateExpression(parts[i], expressValue);
            vec4Value[i] = static_cast<float>(value);
        }
        return UniformVariant(vec4Value);
    }
    else if (type == UniformType::Vec3f) {
        std::string content = expr.substr(1, expr.length() - 2);
        std::vector<std::string> parts = CoreUtils::split(content, ",");
        glm::vec3 vec3Value;
        for (int i = 0; i < parts.size(); i++)
        {
            double value = evaluateExpression(parts[i], expressValue);
            vec3Value[i] = static_cast<float>(value);
        }
        return UniformVariant(vec3Value);
    }
    else if (type == UniformType::Vec2f) {
        std::string content = expr.substr(1, expr.length() - 2);
        std::vector<std::string> parts = CoreUtils::split(content, ",");
        glm::vec2 vec2Value;
        for (int i = 0; i < parts.size(); i++)
        {
            double value = evaluateExpression(parts[i], expressValue);
            vec2Value[i] = static_cast<float>(value);
        }
        return UniformVariant(vec2Value);
    }

    else if (type == UniformType::Vec3i) {
        std::string content = expr.substr(1, expr.length() - 2);
        std::vector<std::string> parts = CoreUtils::split(content, ",");
        glm::ivec3 vec3Value;
        for (int i = 0; i < parts.size(); i++)
        {
            double value = evaluateExpression(parts[i], expressValue);
            vec3Value[i] = static_cast<int>(value);
        }
        return UniformVariant(vec3Value);
    }
    else if (type == UniformType::Vec2i) {
        std::string content = expr.substr(1, expr.length() - 2);
        std::vector<std::string> parts = CoreUtils::split(content, ",");
        glm::ivec2 vec2Value;
        for (int i = 0; i < parts.size(); i++)
        {
            double value = evaluateExpression(parts[i], expressValue);
            vec2Value[i] = static_cast<int>(value);
        }
        return UniformVariant(vec2Value);
    }

    else if (type == UniformType::Float) {
        double value = evaluateExpression(expr, expressValue);
        return UniformVariant(static_cast<float>(value));
    }
    else if (type == UniformType::Int) {
        double value = evaluateExpression(expr, expressValue);
        int intValue = static_cast<int>(value);
        return UniformVariant(intValue);
    }
    else {
        std::cerr << "不支持的表达式类型：" << type << std::endl;
        return UniformVariant();
    }
}

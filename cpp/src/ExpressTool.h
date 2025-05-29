
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <unordered_set>

// 包含 nlohmann::json 头文件 (https://github.com/nlohmann/json)
#include "../nlohmann/json.hpp"

using json = nlohmann::json;


class RendererResource;
struct Material;
struct RenderTargetInfo;
struct UniformValue;
enum class UniformType;

class ExpressTool {
public:

    /// 利用正则表达式提取表达式中所有标识符（符合 [A-Za-z_][A-Za-z0-9_]* ）
    /// 仅返回那些在 candidateSet 中的标识符
    static std::vector<std::string> extractIdentifiers(const std::string &expr,
                                                const std::unordered_set<std::string>& candidateSet);

    /// 尝试去掉表达式外层一对多余的括号（要求成对且包围整个字符串）
    static std::string removeOuterParentheses(const std::string &expr);

    // --- 依赖闭包计算与递归内联 ---------------------------------------------

    // 递归帮助函数：从表达式 expr 出发，收集所有在 defMap（候选集合 validVars）中出现的标识符
    static void computeClosureHelper(const std::string &expr,
                            const std::unordered_set<std::string> &validVars,
                            const std::unordered_map<std::string, std::string> &defMap,
                            std::unordered_set<std::string> &closure,
                            std::unordered_set<std::string> &visited);

    /// 计算最终表达式中所依赖的中间变量集合
    static std::unordered_set<std::string> computeClosure(const std::string &expr,
                                                    const std::unordered_set<std::string> &validVars,
                                                    const std::unordered_map<std::string, std::string> &defMap);
    /// 递归内联变量。只对在 closure 中的变量做内联替换。内联结果总用 () 包裹。
    static std::string resolveVar(const std::string &var,
                        const std::unordered_map<std::string, std::string> &defMap,
                        const std::unordered_set<std::string> &closure,
                        std::unordered_map<std::string, std::string> &memo);

    /// 在最终表达式中，把 closure 内的变量依次替换为它们的内联表达式（从 memo 中取）
    static std::string inlineFinalExpression(const std::string &expr,
                                        const std::unordered_set<std::string> &closure,
                                        const std::unordered_map<std::string, std::string> &memo);

    // --- 主转换函数 ---------------------------------------------------------

    /*
    * 转换表达式：
    *
    * 本方法只沿最终表达式的依赖链查找中间变量，并将其内联替换。对于本例，最终返回：
    *   sourceWidth + control_size * 2
    */
    static std::string transformExpressionSelective(const std::string &input);


    
    /// 收集 renderer 与 plugin 控制数据，并转成 UniformValue 类型。
    ///
    /// 1. 对 renderer，采集 sourceWidth 与 sourceHeight（若存在 renderTarget 则使用其宽高）。
    /// 2. 对 plugin.control 内的数据进行转换，支持 int、float、数组（数组长度为 2 得到 Vec2f，长度为3/4 得到 Vec4f，
    ///    注意：三维数组会自动补齐 alpha 为 1.0）。
    static std::unordered_map<std::string, UniformValue> collectMaterialExpressValue(std::shared_ptr<RendererResource> rendererResource, std::string rendererName, std::shared_ptr<Material> rendererMaterial, const json& plugin, int pluginIndex, RenderTargetInfo defaultSequenceRenderTarget);
    
    /// 使用 ExprTk 根据给定的表达式字符串和变量表求值。
    ///
    /// 注意：ExprTk 只能直接处理 double 类型的标量，因此对于 UniformValue 中的数值，
    /// 1. 若类型为 Int 或 Float，则直接注册变量。
    /// 2. 若类型为 Vec4f 或 Vec2f，则将其拆分为一个 std::vector<double>
    ///    并在 vector 的第 0 个位置设置 dummy 值，从而保证数组下标从 1 开始访问，
    ///    使表达式中可以像 mathjs 那样使用 v[1]、v[2] 的方式访问。
    static double evaluateExpression(const std::string &expressionText, const std::unordered_map<std::string, UniformValue>& variableMap);
    
    /// 辅助函数：将 JSON 数据转换为 UniformValue，仅支持 Int、Float 以及数组（转换为 Vec2f 与 Vec4f）。
    static UniformValue convertJsonValueToUniformValue(const json& j);
    
    static void findPass(const std::shared_ptr<Material> pass, const std::string &passEndName, std::vector<std::shared_ptr<Material>>& result);

    static RenderTargetInfo findInputeRenderTargetInfo(const std::shared_ptr<Material> pass);

    static void caculateMaterialExpress(std::string rendererName, std::shared_ptr<Material> rendererMaterial, const std::unordered_map<std::string, UniformValue>& expressValue, int pluginIndex);

private:
    static std::unordered_map<std::string, std::string> transformExpressionCache;

};
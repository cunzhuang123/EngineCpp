
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <unordered_set>
// 

#include <variant>

#include <GL/glew.h>
#include <glm/glm.hpp>

struct UniformValue;
struct RenderTargetInfo;
class Material;
class RendererResource;
using UniformVariant = std::variant<std::monostate, int, float, glm::mat4, GLuint, std::shared_ptr<Material>, RenderTargetInfo, glm::vec4, glm::vec2, glm::ivec2, glm::ivec3, glm::vec3>;
enum class UniformType;


#include "../nlohmann/json.hpp"
using json = nlohmann::json;

// namespace nlohmann {
//     template<
//         template<typename U, typename V, typename... Args> class ObjectType,
//         template<typename U, typename... Args> class ArrayType,
//         class StringType, class BooleanType, class NumberIntegerType,
//         class NumberUnsignedType, class NumberFloatType,
//         template<typename U> class AllocatorType,
//         template<typename T, typename SFINAE> class JSONSerializer,
//         class BinaryType
//     >
//     class basic_json;
//     // using json = basic_json<>;
// };
// using json = nlohmann::basic_json<>;

class ExpressionCache;
struct CachedExpression;
// struct Material;

class ExpressTool {
public:
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
    static std::string transformExpressionSelective(const std::string &input);
    static std::unordered_map<std::string, UniformValue> collectMaterialExpressValue(std::shared_ptr<RendererResource> rendererResource, std::string rendererName, std::shared_ptr<Material> rendererMaterial, const json& plugin, int pluginIndex, RenderTargetInfo defaultSequenceRenderTarget);
    static double evaluateExpression(const std::string &expressionText, const std::unordered_map<std::string, UniformValue>& variableMap);
    
    /// 辅助函数：将 JSON 数据转换为 UniformValue，仅支持 Int、Float 以及数组（转换为 Vec2f 与 Vec4f）。
    static UniformValue convertJsonValueToUniformValue(const json& j);
    
    static void findPass(const std::shared_ptr<Material> pass, const std::string &passEndName, std::vector<std::shared_ptr<Material>>& result);

    static RenderTargetInfo findInputeRenderTargetInfo(const std::shared_ptr<Material> pass);

    static void caculateMaterialExpress(std::string rendererName, std::shared_ptr<Material> rendererMaterial, const std::unordered_map<std::string, UniformValue>& expressValue, int pluginIndex);

    static UniformVariant evaluateParseExpression(const UniformType& type, const std::string& expr, const std::unordered_map<std::string, UniformValue>& expressValue);

private:
    static std::unordered_map<std::string, std::string> transformExpressionCache;

};
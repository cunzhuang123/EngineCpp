#ifndef MATERIALS_H
#define MATERIALS_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <string>
#include <map>
#include <variant>
#include <memory>
#include <iostream>
#include <glm/glm.hpp>
#include "../nlohmann/json.hpp"
#include "RendererResource.h"



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
// using json = nlohmann::json;


struct Material; // 前向声明
struct RenderTargetInfo {
    std::string name;
    int width;
    int height;
    std::string widthExpress;
    std::string heightExpress;
};

using UniformVariant = std::variant<std::monostate, int, float, glm::mat4, GLuint, std::shared_ptr<Material>, RenderTargetInfo, glm::vec4, glm::vec2, glm::ivec2, glm::ivec3, glm::vec3>;

// 定义 uniform 可能的类型
enum class UniformType {
     Int, Float, Mat4, Texture2D, MaterialPtr, RenderTarget, Vec4f, Vec2f, Vec2i, Vec3i, Vec3f 
};


struct UniformValue {
    UniformType type;
    UniformVariant value;
    std::string express;
};


struct Material {
    static void updateTextrue(std::shared_ptr<Material> material, GLuint oldTexture, GLuint newTexture);
    static std::shared_ptr<Material> deserialize(const nlohmann::json& materialData, std::map<std::string, std::shared_ptr<RendererResource>> rendererResourceMap, GLuint screenBuffer, GLuint ndcBuffer, std::string seqId);
    static std::shared_ptr<Material> deserializePass(const nlohmann::json& passJson, std::map<std::string, std::shared_ptr<RendererResource>> rendererResourceMap, GLuint screenBuffer, GLuint ndcBuffer);

    static glm::mat4 parseMat4(const nlohmann::json& matJson);
    // 解析 glm::vec4 从 JSON
    static glm::vec4 parseVec4(const nlohmann::json& vecJson);
    static glm::vec3 parseVec3(const nlohmann::json& vecJson);
    static glm::vec2 parseVec2(const nlohmann::json& vecJson);

    static glm::ivec3 parseVec3i(const nlohmann::json& vecJson);
    static glm::ivec2 parseVec2i(const nlohmann::json& vecJson);
    static std::vector<std::string> splitString(const std::string& str, char delimiter);

    std::string passName;
    RenderTargetInfo renderTargetInfo;
    std::string vertexShader;
    std::string fragmentShader;
    GLuint attributeBuffer;
    std::map<std::string, UniformValue> uniforms;
    unsigned int clear = (std::numeric_limits<unsigned int>::max)();
    std::shared_ptr<float[]> clearColor = nullptr;
    GLint a_position = -1;
    GLint a_texCoord = -1;
};

extern Material Blit;
extern Material Stand;
extern Material Outline;

std::ostream& operator<<(std::ostream& os, const UniformType& type);

#endif // MATERIALS_H
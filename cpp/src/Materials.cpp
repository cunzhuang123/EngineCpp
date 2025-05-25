#include "Materials.h"
#include <sstream>  // 包含字符串流的头文件

// #include "../nlohmann/json.hpp"

// 实现输出流操作符重载
std::ostream& operator<<(std::ostream& os, const UniformType& type) {
    switch(type) {
        case UniformType::Int:
            os << "Int";
            break;
        case UniformType::Float:
            os << "Float";
            break;
        case UniformType::Mat4:
            os << "Mat4";
            break;
        case UniformType::Texture2D:
            os << "Texture2D";
            break;
        case UniformType::MaterialPtr:
            os << "MaterialPtr";
            break;
        default:
            os << "Unknown";
            break;
    }
    return os;
}



// 解析 glm::mat4 从 JSON
glm::mat4 Material::parseMat4(const nlohmann::json& matJson) {
    glm::mat4 mat(1.0f);
    for(int i = 0; i < 16; ++i) {
        mat[i / 4][i % 4] = matJson[std::to_string(i)].get<float>();
    }
    return mat;
}

// 解析 glm::vec4 从 JSON
glm::vec4 Material::parseVec4(const nlohmann::json& vecJson) {
    return glm::vec4(
        vecJson[0].get<float>(),
        vecJson[1].get<float>(),
        vecJson[2].get<float>(),
        vecJson[3].get<float>()
    );
}

glm::vec3 Material::parseVec3(const nlohmann::json& vecJson) {
    return glm::vec3(
        vecJson[0].get<float>(),
        vecJson[1].get<float>(),
        vecJson[2].get<float>()
    );
}

glm::vec2 Material::parseVec2(const nlohmann::json& vecJson) {
    return glm::vec2(
        vecJson[0].get<float>(),
        vecJson[1].get<float>()
    );
}

glm::ivec3 Material::parseVec3i(const nlohmann::json& vecJson)
{
    return glm::ivec3(
        vecJson[0].get<int>(),
        vecJson[1].get<int>(),
        vecJson[2].get<int>()
    );
}

glm::ivec2 Material::parseVec2i(const nlohmann::json& vecJson)
{
    return glm::ivec2(
        vecJson[0].get<int>(),
        vecJson[1].get<int>()
    );
}


std::vector<std::string> Material::splitString(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while(std::getline(ss, token, delimiter)) {
        tokens.emplace_back(token);
    }
    return tokens;
}

// 反序列化单个 pass
std::shared_ptr<Material> Material::deserializePass(const nlohmann::json& passJson, std::map<std::string, std::shared_ptr<RendererResource>> rendererResourceMap, GLuint screenBuffer, GLuint ndcBuffer) {
    try {
        // 获取唯一标识符，假设 passName 唯一
        std::string passName = passJson["passName"].get<std::string>();
        
        // 创建新的 Material 对象并缓存
        std::shared_ptr<Material> material = std::make_shared<Material>();

        // 解析基本属性
        material->passName = passName;

        // 处理 renderTarget，假设为 null 时设为 0
        auto renderTargetInfo = passJson["renderTarget"];
        material->renderTargetInfo.name = renderTargetInfo["name"].get<std::string>(); // 占位
        material->renderTargetInfo.width = renderTargetInfo["width"].get<int>();
        material->renderTargetInfo.height = renderTargetInfo["height"].get<int>();
        if (renderTargetInfo.contains("widthExpress") && renderTargetInfo.contains("heightExpress"))
        {
            material->renderTargetInfo.widthExpress = renderTargetInfo["widthExpress"].get<std::string>();
            material->renderTargetInfo.heightExpress = renderTargetInfo["heightExpress"].get<std::string>();    
        }

        // 解析 vertexShader 和 fragmentShader
        material->vertexShader = passJson["vertexShader"].get<std::string>();
        material->fragmentShader = passJson["fragmentShader"].get<std::string>();

        // 处理 attributeBuffer，用户负责后续处理，这里设为 0
        std::string attributeBufferStr = passJson["attributeBuffer"].get<std::string>();
        if ("bufferResourceId:sreenBuffer" == attributeBufferStr)
        {
            material->attributeBuffer = screenBuffer;
        }
        else if ("bufferResourceId:ndcBuffer" == attributeBufferStr)
        {
            material->attributeBuffer = ndcBuffer;
        }
        else if (attributeBufferStr.find("bufferResourceId:") == 0)
        {
            GLuint buffer = 0;
            glGenBuffers(1, &buffer);
            glBindBuffer(GL_ARRAY_BUFFER, buffer);
            const std::vector<float>& vertices = rendererResourceMap[attributeBufferStr]->getVertices();  // 使用常量引用，避免拷贝
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            material->attributeBuffer = buffer;
        }
        else 
        {
            std::cerr << "不支持的attributeBuffer类型：" << attributeBufferStr << std::endl;
            return nullptr; // 其他类型处理或跳过
        }


        // 解析 uniforms
        auto uniformsJson = passJson["uniforms"];
        for(auto it = uniformsJson.begin(); it != uniformsJson.end(); ++it) {
            std::string uniformName = it.key();
            auto uniformObj = it.value();

            std::string uniformTypeStr = uniformObj["type"].get<std::string>();
            UniformType uniformType;
            UniformVariant uniformValue;
            std::string express;
            if (uniformObj.contains("express"))
            {
                express = uniformObj["express"].get<std::string>();
            }

            if(uniformTypeStr == "int") {
                uniformType = UniformType::Int;
                int value = uniformObj["value"].get<int>();
                uniformValue = value;
            }
            else if(uniformTypeStr == "float") {
                uniformType = UniformType::Float;
                float value = uniformObj["value"].get<float>();
                uniformValue = value;
            }
            else if(uniformTypeStr == "mat4") {
                uniformType = UniformType::Mat4;
                glm::mat4 mat = parseMat4(uniformObj["value"]);
                uniformValue = mat;
            }
            else if(uniformTypeStr == "vec4") {
                uniformType = UniformType::Vec4f;
                glm::vec4 vec = parseVec4(uniformObj["value"]);
                uniformValue = vec;
            }
            else if(uniformTypeStr == "vec2") {
                uniformType = UniformType::Vec2f;
                glm::vec2 vec = parseVec2(uniformObj["value"]);
                uniformValue = vec;
            }
            

            else if(uniformTypeStr == "ivec2") {
                uniformType = UniformType::Vec2i;
                glm::ivec2 vec = parseVec2i(uniformObj["value"]);
                uniformValue = vec;
            }        
            else if(uniformTypeStr == "ivec3") {
                uniformType = UniformType::Vec3i;
                glm::ivec3 vec = parseVec3i(uniformObj["value"]);
                uniformValue = vec;
            }        
            else if(uniformTypeStr == "vec3") {
                uniformType = UniformType::Vec3f;
                glm::vec3 vec = parseVec3(uniformObj["value"]);
                uniformValue = vec;
            }        
            else if(uniformTypeStr == "bool") {
                uniformType = UniformType::Int;
                bool vl = uniformObj["value"].get<bool>();
                uniformValue = vl ? 1 : 0;
            }        


            else if(uniformTypeStr == "sampler2D") {
                // 判断 value 是字符串还是对象
                auto& value = uniformObj["value"];
                if(value.is_string()) {
                    // 处理 Texture2D ID，这里设为 0，后续用户自行处理
                    uniformType = UniformType::Texture2D;

                    std::string textureStr = value.get<std::string>();
                    if (textureStr.find("textureResourceId:") == 0)
                    {
                        uniformValue = rendererResourceMap[textureStr]->getTexture();
                    }
                    else 
                    {
                        std::cerr << "纹理解析错误：" << textureStr << std::endl;
                        return nullptr; // 其他类型处理或跳过
                    }
                }
                else if(value.is_object() && value["passName"].is_string()) {
                    // 处理嵌套的 MaterialPtr
                    auto nestedPass = value;
                    auto nestedMaterial = deserializePass(nestedPass, rendererResourceMap, screenBuffer, ndcBuffer);
                    uniformType = UniformType::MaterialPtr;
                    uniformValue = nestedMaterial;
                }
                else if(value.is_object() && value["name"].is_string() && value["width"].is_number_integer()) {
                    uniformType = UniformType::RenderTarget;
                    RenderTargetInfo info = {value["name"].get<std::string>(), value["width"].get<int>(), value["height"].get<int>()};
                    uniformValue = info;
                }
                else {
                    std::cerr << "不支持的纹理：" << value << std::endl;
                    return nullptr; // 其他类型处理或跳过
                }
            }
            else {
                std::cerr << "不支持的材质类型：" << uniformTypeStr << std::endl;
                return nullptr; // 其他类型处理或跳过
            }

            // 将 UniformValue 添加到 Material 的 uniforms 中
            material->uniforms[uniformName] = UniformValue{uniformType, uniformValue, express};
        }
        return material;
    } catch (const std::exception& e) {
        std::cerr << "解析材质失败：" << e.what() << std::endl;
        return nullptr;
    }
}

// 主反序列化函数
std::shared_ptr<Material> Material::deserialize(const nlohmann::json& materialData, std::map<std::string, std::shared_ptr<RendererResource>> rendererResourceMap, GLuint screenBuffer, GLuint ndcBuffer, std::string seqId) {
    // 获取 materialPasses
    auto materialPasses = materialData["materialPasses"];
    // 假设选择第一个 pass 作为根 Material
    if(materialPasses.empty()) {
        return nullptr; // 没有 pass，返回空指针
    }
    auto pass = materialPasses[seqId];
    return deserializePass(pass, rendererResourceMap, screenBuffer, ndcBuffer);
};

void Material::updateTextrue(std::shared_ptr<Material> material, GLuint oldTexture, GLuint newTexture)
{
    for (auto& [uniformName, uniformValue] : material->uniforms) {
        if (uniformValue.type == UniformType::Texture2D && std::get<GLuint>(uniformValue.value) == oldTexture) {
            uniformValue.value = newTexture;
        }
        else if (uniformValue.type == UniformType::MaterialPtr) {
            auto nestedMaterial = std::get<std::shared_ptr<Material>>(uniformValue.value);
            updateTextrue(nestedMaterial, oldTexture, newTexture);
        }
    }
}

RenderTargetInfo renderTargetInfo;


Material Stand = {
    "Stand",
    renderTargetInfo,
    "standVertex.glsl",
    "standFragment.glsl",
    0,
    {
        {"u_texture", UniformValue{UniformType::Texture2D, GLuint(0)}},
        {"u_modelMatrix", UniformValue{UniformType::Mat4, glm::mat4(1.0f)}},
        {"u_viewMatrix", UniformValue{UniformType::Mat4, glm::mat4(1.0f)}},
        {"u_projectionMatrix", UniformValue{UniformType::Mat4, glm::mat4(1.0f)}},
        {"u_color", UniformValue{UniformType::Vec4f, glm::vec4(1.0f)}}
    }
};



Material Blit = {
    "Blit",
    renderTargetInfo,
    "vertex.glsl",
    "fragment.glsl",
    0,
    {
        {"u_texture", UniformValue{UniformType::Texture2D, GLuint(0)}},
    }
};




Material Outline = {
    "Outline",
    renderTargetInfo,
    "outlineVertex.glsl",
    "outlineFragment.glsl",
    0,
    {
        {"u_texture", UniformValue{UniformType::Texture2D, GLuint(0)}},
        {"u_uvTransform", UniformValue{UniformType::Vec4f, glm::vec4(-0.021231422505307858,
            -0.0872093023255814,
            1.0424628450106157,
            1.1744186046511629)}},
        // {"u_uvTransform", UniformValue{UniformType::Vec4f, glm::vec4(-0.042231422505307858,
        //     -0.1772093023255814,
        //     2.0824628450106157,
        //     2.3444186046511629)}},
    
        {"u_texResolution", UniformValue{UniformType::Vec2f, glm::vec2(1473.0,
            404.0)}},
        {"u_steps", UniformValue{UniformType::Int, 30}},
        {"u_outlineColor", UniformValue{UniformType::Vec4f, glm::vec4(1,
            0,
            0,
            1)}},
    }
};
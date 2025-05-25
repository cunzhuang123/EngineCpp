// ShaderManager.cpp

#include "ShaderManager.h"
#include <fstream>
#include <sstream>
#include <iostream>

// 声明嵌入数据
extern std::map<std::string, struct EmbeddedFile> embedded_files;

struct EmbeddedFile {
    const unsigned char* data;
    std::size_t size;
};

ShaderManager::ShaderManager() {
}

ShaderManager::~ShaderManager() {
    for (auto& pair : programCache) {
        glDeleteProgram(pair.second);
    }
}

std::string ShaderManager::loadShaderFile(const std::string& path) {
    if (path.find(".glsl") == std::string::npos) {
        return std::string("#version 330 core\n") + extendShaders[path].get<std::string>();
    }

    auto it = embedded_files.find(path);
    if (it != embedded_files.end()) {
        const EmbeddedFile& file = it->second;
        return std::string("#version 330 core\n") + std::string(reinterpret_cast<const char*>(file.data), file.size);
    }
    else{
        std::cerr << "没找到对应着色器：" << path << std::endl;
    }
    return "";
}

GLuint ShaderManager::compileShader(GLenum type, const std::string& source) {
    GLuint shader = glCreateShader(type);
    const GLchar* src = source.c_str();
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);

    GLint compileStatus;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
    if (!compileStatus) {
        GLchar infoLog[512];
        glGetShaderInfoLog(shader, sizeof(infoLog), NULL, infoLog);
        std::cerr << "着色器编译失败：" << infoLog << std::endl;
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

GLuint ShaderManager::createProgram(GLuint vertexShader, GLuint fragmentShader) {
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);

    glLinkProgram(program);

    GLint linkStatus;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    if (!linkStatus) {
        GLchar infoLog[512];
        glGetProgramInfoLog(program, sizeof(infoLog), NULL, infoLog);
        std::cerr << "程序链接失败：" << infoLog << std::endl;
        glDeleteProgram(program);
        return 0;
    }

    return program;
}

GLuint ShaderManager::getProgram(const std::string& vertexShaderPath, const std::string& fragmentShaderPath) {
    std::string key = vertexShaderPath + "|" + fragmentShaderPath;
    if (programCache.find(key) != programCache.end()) {
        return programCache[key];
    }

    std::string vertexSource = loadShaderFile(vertexShaderPath);
    std::string fragmentSource = loadShaderFile(fragmentShaderPath);

    if (vertexSource.empty() || fragmentSource.empty()) {
        return 0;
    }

    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);

    if (vertexShader == 0 || fragmentShader == 0) {
        return 0;
    }

    GLuint program = createProgram(vertexShader, fragmentShader);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    if (program != 0) {
        programCache[key] = program;
    }

    return program;
}

void ShaderManager::setExtendShader(const nlohmann::json& shaders)
{
    extendShaders = shaders;
}
// ShaderManager.h

#ifndef SHADERMANAGER_H
#define SHADERMANAGER_H

#include <GL/glew.h>
#include <string>
#include <map>
#include "../nlohmann/json.hpp"

class ShaderManager {
public:
    ShaderManager();
    ~ShaderManager();

    GLuint getProgram(const std::string& vertexShaderPath, const std::string& fragmentShaderPath);
    void setExtendShader(const nlohmann::json& shaders);
private:
    std::string loadShaderFile(const std::string& path);
    GLuint compileShader(GLenum type, const std::string& source);
    GLuint createProgram(GLuint vertexShader, GLuint fragmentShader);

    std::map<std::string, GLuint> programCache;
    nlohmann::json extendShaders;
};

#endif // SHADERMANAGER_H
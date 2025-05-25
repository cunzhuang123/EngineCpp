// RenderPass.cpp

#include "RenderPass.h"
#include <iostream>
#include "ScopedProfiler.h"

RenderPass::RenderPass(std::shared_ptr<ShaderManager> shaderManager, GLuint width, GLuint height, RenderTargetInfo defaultRenderTargetInfo, GLuint defaultFramebuffer)
    : shaderManager(shaderManager), width(width), height(height), renderTargetPool(RenderTargetPool::instance()) {
    renderTargetPool.initialize(width, height, defaultRenderTargetInfo, defaultFramebuffer);
    // renderTargetPool = RenderTargetPool::getInstance();
    // renderTargetPool = std::make_shared<RenderTargetPool>(width, height, defaultRenderTargetInfo, defaultFramebuffer);
}

RenderPass::~RenderPass() {
}

void RenderPass::render(std::vector<std::shared_ptr<Material>>& videoRendererMaterials, bool isRelease) {
    renderedSet.clear();
    for (auto& material : videoRendererMaterials) {
        renderPass(material, renderedSet);
    }

    if (isRelease)
    {
        renderTargetPool.releaseUnused();
    }
}

void RenderPass::renderPass(std::shared_ptr<Material> pass, std::set<std::string>& renderedSet) {
    if (renderedSet.find(pass->passName) != renderedSet.end()) {
        return;
    }

    // 检查 Pass 的依赖关系
    for (auto& uniformPair : pass->uniforms) {
        std::string uniformName = uniformPair.first;
        UniformValue& uniform = uniformPair.second;
        if (uniform.type == UniformType::MaterialPtr) {
            std::shared_ptr<Material> dependentPass = std::get<std::shared_ptr<Material>>(uniform.value);
            if (dependentPass) {
                renderPass(dependentPass, renderedSet);
            }
        }
    }

    // 渲染当前 Pass
    renderSinglePass(pass);

    // 标记为已渲染
    renderedSet.insert(pass->passName);
}

void RenderPass::renderSinglePass(std::shared_ptr<Material> pass) {
    // ScopedProfiler profilerVideoResource("RenderPass::renderSinglePass" + pass->passName);

    GLuint program = shaderManager->getProgram(pass->vertexShader, pass->fragmentShader);
    if (!program) {
        std::cerr << "无法渲染通道: " << pass->passName << "。Shader program 初始化失败。" << std::endl;
        return;
    }

    glUseProgram(program);

    {
        // ScopedProfiler profilerVideoResource("acquire RenderTarget" + pass->passName);

        // 渲染到纹理
        std::shared_ptr<RenderTarget> renderTarget = renderTargetPool.acquire(pass->renderTargetInfo);
        if (!renderTarget) {
            std::cerr << "无法获取渲染目标用于 Pass: " << pass->passName << std::endl;
            return;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, renderTarget->framebuffer);

        glViewport(0, 0, pass->renderTargetInfo.width, pass->renderTargetInfo.height);
    }
    {
        // ScopedProfiler profilerVideoResource("glBindBuffer and  Vertex " + pass->passName);


        // 绑定属性缓冲区
        glBindBuffer(GL_ARRAY_BUFFER, pass->attributeBuffer);

        {
            // ScopedProfiler profilerVideoResource("Vertex " + pass->passName);

            // 设置顶点属性
            GLint a_position;
            if (pass->a_position != -1)
            {
                a_position = pass->a_position;
            }
            else
            {
                a_position = glGetAttribLocation(program, "a_position");
                pass->a_position = a_position;
            }
            glEnableVertexAttribArray(a_position);
            glVertexAttribPointer(a_position, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);

            GLint a_texCoord;
            if (pass->a_texCoord != -1)
            {
                a_texCoord = pass->a_texCoord;
            }
            else
            {
                a_texCoord = glGetAttribLocation(program, "a_texCoord");
                pass->a_texCoord = a_texCoord;
            }
            glEnableVertexAttribArray(a_texCoord);
            glVertexAttribPointer(a_texCoord, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        }
    }

    {
        // ScopedProfiler profilerVideoResource("update uniforms" + pass->passName);

        // 设置统一变量（uniforms）
        GLuint currentTextureUnit = 0;
        for (auto& uniformPair : pass->uniforms) {
            std::string uniformName = uniformPair.first;
            UniformValue& uniform = uniformPair.second;
            GLint location = glGetUniformLocation(program, uniformName.c_str());
            if (location == -1) {
                std::cerr << "Uniform \"" << uniformName << "\" 不存在于着色器，pass \"" << pass->passName << "\"" << std::endl;
                continue;
            }

            if (uniform.type == UniformType::Texture2D) {
                GLuint texture = std::get<GLuint>(uniform.value);

                glActiveTexture(GL_TEXTURE0 + currentTextureUnit);
                glBindTexture(GL_TEXTURE_2D, texture);
                glUniform1i(location, currentTextureUnit);
                currentTextureUnit++;
            } else if (uniform.type == UniformType::MaterialPtr) {
                std::shared_ptr<Material> dependentPass = std::get<std::shared_ptr<Material>>(uniform.value);
                if (dependentPass) {
                    auto dependentRenderTarget = renderTargetPool.acquire(dependentPass->renderTargetInfo);
                    if (!dependentRenderTarget) {
                        std::cerr << "Pass \"" << dependentPass->passName << "\" 没有渲染目标纹理。" << std::endl;
                        continue;
                    }
                    GLuint texture = dependentRenderTarget->texture;

                    glActiveTexture(GL_TEXTURE0 + currentTextureUnit);
                    glBindTexture(GL_TEXTURE_2D, texture);
                    glUniform1i(location, currentTextureUnit);
                    currentTextureUnit++;
                }
            } else if (uniform.type == UniformType::RenderTarget) {
                RenderTargetInfo renderTargetInfo = std::get<RenderTargetInfo>(uniform.value);
                auto dependentRenderTarget = renderTargetPool.acquire(renderTargetInfo);
                if (!dependentRenderTarget) {
                    std::cerr << "renderTargetInfo.name \"" << renderTargetInfo.name << "\" 没有渲染目标纹理。" << std::endl;
                    continue;
                }
                GLuint texture = dependentRenderTarget->texture;

                glActiveTexture(GL_TEXTURE0 + currentTextureUnit);
                glBindTexture(GL_TEXTURE_2D, texture);
                glUniform1i(location, currentTextureUnit);
                currentTextureUnit++;
            } else if (uniform.type == UniformType::Mat4) {
                if (std::holds_alternative<glm::mat4>(uniform.value)) {
                    glm::mat4 mat = std::get<glm::mat4>(uniform.value);
                    glUniformMatrix4fv(location, 1, GL_FALSE, &mat[0][0]);
                } else {
                    std::cerr << "mat4 类型的 uniform \"" << uniformName << "\" 的值类型不正确。" << std::endl;
                }
            } else if (uniform.type == UniformType::Float) {
                if (std::holds_alternative<float>(uniform.value)) {
                    float val = std::get<float>(uniform.value);
                    glUniform1f(location, val);
                } else {
                    std::cerr << "float 类型的 uniform \"" << uniformName << "\" 的值类型不正确。" << std::endl;
                }
            } else if (uniform.type == UniformType::Vec4f) {
                if (std::holds_alternative<glm::vec4>(uniform.value)) {
                    glm::vec4 vec4f = std::get<glm::vec4>(uniform.value);
                    glUniform4fv(location, 1, &vec4f[0]);
                } else {
                    std::cerr << "Vec4f 类型的 uniform \"" << uniformName << "\" 的值类型不正确。" << std::endl;
                }
            } else if (uniform.type == UniformType::Vec2f) {
                if (std::holds_alternative<glm::vec2>(uniform.value)) {
                    glm::vec2 vec2f = std::get<glm::vec2>(uniform.value);
                    glUniform2fv(location, 1, &vec2f[0]);
                } else {
                    std::cerr << "Vec2f 类型的 uniform \"" << uniformName << "\" 的值类型不正确。" << std::endl;
                }
            } else if (uniform.type == UniformType::Int) {
                if (std::holds_alternative<int>(uniform.value)) {
                    int iv = std::get<int>(uniform.value);
                    glUniform1i(location, iv);
                } else {
                    std::cerr << "Int 类型的 uniform \"" << uniformName << "\" 的值类型不正确。" << std::endl;
                }
            }
            
            else if (uniform.type == UniformType::Vec2i) {
                if (std::holds_alternative<glm::ivec2>(uniform.value)) {
                    glm::ivec2 vec2 = std::get<glm::ivec2>(uniform.value);
                    glUniform2iv(location, 1, &vec2[0]);
                } else {
                    std::cerr << "Vec2i 类型的 uniform \"" << uniformName << "\" 的值类型不正确。" << std::endl;
                }
            }
            else if (uniform.type == UniformType::Vec3i) {
                if (std::holds_alternative<glm::ivec3>(uniform.value)) {
                    glm::ivec3 vec3 = std::get<glm::ivec3>(uniform.value);
                    glUniform3iv(location, 1, &vec3[0]);
                } else {
                    std::cerr << "Vec3i 类型的 uniform \"" << uniformName << "\" 的值类型不正确。" << std::endl;
                }
            }
            else if (uniform.type == UniformType::Vec3f) {
                if (std::holds_alternative<glm::vec3>(uniform.value)) {
                    glm::vec3 vec3f = std::get<glm::vec3>(uniform.value);
                    glUniform3fv(location, 1, &vec3f[0]);
                } else {
                    std::cerr << "Vec3f 类型的 uniform \"" << uniformName << "\" 的值类型不正确。" << std::endl;
                }
            }
            
            else {
                std::cerr << "不支持的 uniform 类型 \"" << uniform.type << "\"，uniform \"" << uniformName << "\" 在 pass \"" << pass->passName << "\"" << std::endl;
            }
        }
    }

    {
            // ScopedProfiler profilerVideoResource("glDrawArrays" + pass->passName);

            if (pass->clearColor != nullptr) {
                glClearColor(pass->clearColor[0], pass->clearColor[1], pass->clearColor[2], pass->clearColor[3]);
            }
            if (pass->clear != std::numeric_limits<unsigned int>::max())
            {
                glClear(pass->clear);
            }
            // 绘制
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    {
        // ScopedProfiler profilerVideoResource("unbind " + pass->passName);

        // 解绑
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0);
    }
}
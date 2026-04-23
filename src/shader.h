#pragma once
#include <string>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

class Shader {
public:
    unsigned int ID = 0;

    Shader() = default;
    Shader(const char* vertexPath, const char* fragmentPath);

    void use() const { glUseProgram(ID); }

    void setBool (const std::string& n, bool v)              const { glUniform1i (loc(n), (int)v); }
    void setInt  (const std::string& n, int v)               const { glUniform1i (loc(n), v); }
    void setFloat(const std::string& n, float v)             const { glUniform1f (loc(n), v); }
    void setVec2 (const std::string& n, const glm::vec2& v)  const { glUniform2fv(loc(n), 1, glm::value_ptr(v)); }
    void setVec3 (const std::string& n, const glm::vec3& v)  const { glUniform3fv(loc(n), 1, glm::value_ptr(v)); }
    void setVec4 (const std::string& n, const glm::vec4& v)  const { glUniform4fv(loc(n), 1, glm::value_ptr(v)); }
    void setMat4 (const std::string& n, const glm::mat4& v)  const { glUniformMatrix4fv(loc(n), 1, GL_FALSE, glm::value_ptr(v)); }

private:
    GLint loc(const std::string& name) const { return glGetUniformLocation(ID, name.c_str()); }
    void  checkErrors(unsigned int obj, const std::string& type);
};

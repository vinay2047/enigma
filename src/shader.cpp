#include "shader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <glad/glad.h>

Shader::Shader(const char* vertexPath, const char* fragmentPath)
{
    std::ifstream vf(vertexPath), ff(fragmentPath);
    if (!vf.is_open() || !ff.is_open()) {
        std::cerr << "[Shader] Cannot open: " << vertexPath
                  << " or " << fragmentPath << "\n";
        return;
    }
    std::stringstream vs, fs;
    vs << vf.rdbuf(); fs << ff.rdbuf();
    std::string vsrc = vs.str(), fsrc = fs.str();
    const char* vc = vsrc.c_str(), *fc = fsrc.c_str();

    unsigned int v = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(v, 1, &vc, nullptr);
    glCompileShader(v);
    checkErrors(v, "VERTEX");

    unsigned int f = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(f, 1, &fc, nullptr);
    glCompileShader(f);
    checkErrors(f, "FRAGMENT");

    ID = glCreateProgram();
    glAttachShader(ID, v);
    glAttachShader(ID, f);
    glLinkProgram(ID);
    checkErrors(ID, "PROGRAM");

    glDeleteShader(v);
    glDeleteShader(f);
}

void Shader::checkErrors(unsigned int obj, const std::string& type)
{
    int  ok; char log[1024];
    if (type != "PROGRAM") {
        glGetShaderiv(obj, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            glGetShaderInfoLog(obj, 1024, nullptr, log);
            std::cerr << "[Shader:" << type << "] " << log << "\n";
        }
    } else {
        glGetProgramiv(obj, GL_LINK_STATUS, &ok);
        if (!ok) {
            glGetProgramInfoLog(obj, 1024, nullptr, log);
            std::cerr << "[Shader:LINK] " << log << "\n";
        }
    }
}

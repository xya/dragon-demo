// Copyright (c) 2009-2015, Pierre-Andre Saulais <pasaulais@free.fr>
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer. 
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <cstdio>
#include "Platform.h"
#include "RenderStateGL2.h"
#include "MeshGL2.h"

RenderStateGL2::RenderStateGL2() : RenderState()
{
    m_matrixMode = ModelView;
    m_matrix[(int)ModelView].setIdentity();
    m_matrix[(int)Projection].setIdentity();
    m_matrix[(int)Texture].setIdentity();
    m_ambient0 = vec4(1.0, 1.0, 1.0, 1.0);
    m_diffuse0 = vec4(1.0, 1.0, 1.0, 1.0);
    m_specular0 = vec4(1.0, 1.0, 1.0, 1.0);
    m_light0_pos = vec4(0.0, 1.0, 1.0, 0.0);
    m_vertexShader = 0;
    m_pixelShader = 0;
    m_program = 0;
    m_modelViewMatrixLoc = -1;
    m_projMatrixLoc = -1;
    m_positionAttr = -1;
    m_normalAttr = -1;
    m_texCoordsAttr = -1;
}

RenderStateGL2::~RenderStateGL2()
{
    if(m_vertexShader != 0)
        glDeleteShader(m_vertexShader);
    if(m_pixelShader != 0)
        glDeleteShader(m_pixelShader);
    if(m_program != 0)
        glDeleteProgram(m_program);
}

Mesh * RenderStateGL2::createMesh() const
{
    return new MeshGL2(this);
}

void RenderStateGL2::drawMesh(Mesh *m)
{
    if(!m)
        return;
    glUniformMatrix4fv(m_modelViewMatrixLoc, 1, GL_FALSE,
                       (const GLfloat *)m_matrix[(int)ModelView].d);
    glUniformMatrix4fv(m_projMatrixLoc, 1, GL_FALSE,
                       (const GLfloat *)m_matrix[(int)Projection].d);
    m->draw(m_output, this, m_meshOutput);
    if(m_drawNormals)
        m->drawNormals(this);
}

void RenderStateGL2::freeTextures()
{
    map<string, uint32_t>::iterator it;
    for(it = m_textures.begin(); it != m_textures.end(); it++)
        glDeleteTextures(1, &it->second);
    m_textures.clear();
}

void RenderStateGL2::setMatrixMode(RenderStateGL2::MatrixMode newMode)
{
    m_matrixMode = newMode;
}

void RenderStateGL2::loadIdentity()
{
    int i = (int)m_matrixMode;
    m_matrix[i].setIdentity();
}

void RenderStateGL2::multiplyMatrix(const matrix4 &m)
{
    int i = (int)m_matrixMode;
    m_matrix[i] = m_matrix[i] * m;
}

void RenderStateGL2::pushMatrix()
{
    int i = (int)m_matrixMode;
    m_matrixStack[i].push_back(m_matrix[i]);
}

void RenderStateGL2::popMatrix()
{
    int i = (int)m_matrixMode;
    m_matrix[i] = m_matrixStack[i].back();
    m_matrixStack[i].pop_back();
}

void RenderStateGL2::translate(float dx, float dy, float dz)
{
    multiplyMatrix(matrix4::translate(dx, dy, dz));
}

void RenderStateGL2::rotate(float angle, float rx, float ry, float rz)
{
    multiplyMatrix(matrix4::rotate(angle, rx, ry, rz));
}

void RenderStateGL2::scale(float sx, float sy, float sz)
{
    multiplyMatrix(matrix4::scale(sx, sy, sz));
}

matrix4 RenderStateGL2::currentMatrix() const
{
    return m_matrix[(int)m_matrixMode];
}

void RenderStateGL2::pushMaterial(const Material &m)
{
    m_materialStack.push_back(m);
    beginApplyMaterial(m);
}

void RenderStateGL2::popMaterial()
{
    Material m = m_materialStack.back();
    m_materialStack.pop_back();
    endApplyMaterial(m);
    if(m_materialStack.size() > 0)
        beginApplyMaterial(m_materialStack.back());
}

void RenderStateGL2::beginApplyMaterial(const Material &m)
{
    setUniformValue("u_material_ambient", m.ambient());
    setUniformValue("u_material_diffuse", m.diffuse());
    setUniformValue("u_material_specular", m.specular());
    setUniformValue("u_material_shine", m.shine());
    if(m.texture() != 0)
    {
        glActiveTexture(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, m.texture());
        setUniformValue("u_material_texture", 0);
        setUniformValue("u_has_texture", 1);
    }
    else
    {
        setUniformValue("u_has_texture", 0);
    }
}

void RenderStateGL2::endApplyMaterial(const Material &m)
{
    if(m.texture() != 0)
        glBindTexture(GL_TEXTURE_2D, 0);
}

void RenderStateGL2::beginFrame(int w, int h)
{
    glPushAttrib(GL_ENABLE_BIT);
    glUseProgram(m_program);
    initShaders();
    glEnable(GL_DEPTH_TEST);
    setUniformValue("u_light_ambient", m_ambient0);
    setUniformValue("u_light_diffuse", m_diffuse0);
    setUniformValue("u_light_specular", m_specular0);
    setUniformValue("u_light_pos", m_light0_pos);
    setupViewport(w, h);
    setMatrixMode(ModelView);
    pushMatrix();
    loadIdentity();
    glClearColor(m_bgColor.x, m_bgColor.y, m_bgColor.z, m_bgColor.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void RenderStateGL2::endFrame()
{
    glFlush();
    setMatrixMode(ModelView);
    popMatrix();
    glUseProgram(0);
    glPopAttrib();
}

void RenderStateGL2::setupViewport(int w, int h)
{
    glViewport(0, 0, w, h);
    setMatrixMode(Projection);
    loadIdentity();
    float r = (float)w / (float)h;
    if(m_projection)
        multiplyMatrix(matrix4::perspective(45.0f, r, 0.1f, 100.0f));
    else if (w <= h)
        multiplyMatrix(matrix4::ortho(-1.0, 1.0, -1.0 / r, 1.0 / r, -10.0, 10.0));
    else
        multiplyMatrix(matrix4::ortho(-1.0 * r, 1.0 * r, -1.0, 1.0, -10.0, 10.0));
    setMatrixMode(ModelView);
}

void RenderStateGL2::init()
{
    loadShaders();
}

int RenderStateGL2::positionAttr() const
{
    return m_positionAttr;
}

int RenderStateGL2::normalAttr() const
{
    return m_normalAttr;
}

int RenderStateGL2::texCoordsAttr() const
{
    return m_texCoordsAttr;
}

uint32_t RenderStateGL2::loadShader(string path, uint32_t type) const
{
    char *code = loadFileData(path);
    if(!code)
        return 0;
    uint32_t shader = glCreateShader(type);
    glShaderSource(shader, 1, (const GLchar **)&code, 0);
    freeFileData(code);
    glCompileShader(shader);
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if(!status)
    {
        GLint log_size;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_size);
        if(log_size)
        {
            GLchar *log = new GLchar[log_size];
            glGetShaderInfoLog(shader, log_size, 0, log);
            fprintf(stderr, "Error compiling shader: %s\n", log);
            delete [] log;
        }
        else
        {
            fprintf(stderr, "Error compiling shader.\n");
        }
        return 0;
    }
    return shader;
}

bool RenderStateGL2::loadShaders()
{
    //TODO fallback to GL1 when in a pinch
    uint32_t vertexShader = loadShader("vertex.glsl", GL_VERTEX_SHADER);
    if(vertexShader == 0)
        return false;
    uint32_t pixelShader = loadShader("fragment.glsl", GL_FRAGMENT_SHADER);
    if(pixelShader == 0)
    {
        glDeleteShader(vertexShader);
        return false;
    }
    uint32_t program = glCreateProgram();
    if(program == 0)
    {
        glDeleteShader(vertexShader);
        glDeleteShader(pixelShader);
        return false;
    }
    glAttachShader(program, vertexShader);
    glAttachShader(program, pixelShader);
    glLinkProgram(program);
    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if(!status)
    {
        GLint log_size;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_size);
        if(log_size)
        {
            GLchar *log = new GLchar[log_size];
            glGetProgramInfoLog(program, log_size, 0, log);
            fprintf(stderr, "Error linking program: %s\n", log);
            delete [] log;
        }
        else
        {
            fprintf(stderr, "Error linking program.\n");
        }
        glDeleteProgram(program);
        glDeleteShader(vertexShader);
        glDeleteShader(pixelShader);
        return 0;
    }
    m_program = program;
    m_vertexShader = vertexShader;
    m_pixelShader = pixelShader;
    m_modelViewMatrixLoc = glGetUniformLocation(program, "u_modelViewMatrix");
    m_projMatrixLoc = glGetUniformLocation(program, "u_projectionMatrix");
    m_positionAttr = glGetAttribLocation(program, "a_position");
    m_normalAttr = glGetAttribLocation(program, "a_normal");
    m_texCoordsAttr = glGetAttribLocation(program, "a_texCoords");
    return true;
}

void RenderStateGL2::initShaders()
{
}

void RenderStateGL2::setUniformValue(string name, const vec4 &v)
{
    int location = glGetUniformLocation(m_program, name.c_str());
    glUniform4fv(location, 1, (GLfloat *)&v);
}

void RenderStateGL2::setUniformValue(string name, float f)
{
    int location = glGetUniformLocation(m_program, name.c_str());
    glUniform1f(location, f);
}

void RenderStateGL2::setUniformValue(string name, int i)
{
    int location = glGetUniformLocation(m_program, name.c_str());
    glUniform1i(location, i);
}

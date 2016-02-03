/* 
 * File:   ObjRenderer.cpp
 * Author: swl
 * 
 * Created on November 29, 2015, 12:02 PM
 */

#include <limits>
#include <GL/glew.h>
#include "ShaderUtils.h"
#include "ObjRenderer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <bits/unordered_map.h>
#include "ImageUtils.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

GLuint ObjRenderer::colorTexID = 0;
GLuint ObjRenderer::depthBufferID = 0;
GLuint ObjRenderer::frameBufferID = 0;
GLuint ObjRenderer::shaderProgID = 0;
GLuint ObjRenderer::vertexBufferID = 0;
GLuint ObjRenderer::renderSize = 512;
GLuint ObjRenderer::nTriangles = 0;
GLuint ObjRenderer::texCount = 0;
unsigned ObjRenderer::shaderSeed = 0;
unsigned ObjRenderer::shaderOutputID = 0;
bool ObjRenderer::flipNormals = false;
bool ObjRenderer::faceNormals = false;
std::vector<unsigned> ObjRenderer::matGroupSizeList;
RenderMode ObjRenderer::renderMode = RENDER_MODE_PLAIN;
std::vector<glm::vec3> ObjRenderer::vertices;

inline glm::vec3 getVec(const float* data)
{
    glm::vec3 v;
    v.x = data[0];
    v.y = data[1];
    v.z = data[2];
    return v;
}

void ObjRenderer::init()
{   
    int ac = 0;
    char** av;
    glutInit(&ac, av);
    glutInitWindowSize(renderSize, renderSize);
    glutCreateWindow("SketchRenderer");
    glewInit();
    
    shaderSeed = time(0);
    
    glutDisplayFunc(render);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_SINGLE | GLUT_RGBA);
    glClearColor(0, 0, 0, 0);
    
    shaderProgID = loadShaders("Shader/phong.vert", "Shader/phong.frag");
    if(!shaderProgID)
        shaderProgID = loadShaders("../Shader/phong.vert", "../Shader/phong.frag");
    
    glClampColor(GL_CLAMP_READ_COLOR, GL_FALSE);
    glClampColor(GL_CLAMP_VERTEX_COLOR, GL_FALSE);
    glClampColor(GL_CLAMP_FRAGMENT_COLOR, GL_FALSE);
    
    glGenTextures(1, &colorTexID);
    glBindTexture(GL_TEXTURE_2D, colorTexID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    //NULL means reserve texture memory, but texels are undefined
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, renderSize, renderSize, 
            0, GL_RGBA, GL_FLOAT, NULL);
    //-------------------------
    glGenFramebuffers(1, &frameBufferID);
    glBindFramebuffer(GL_FRAMEBUFFER, frameBufferID);
    //Attach 2D texture to this FBO
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexID, 0);
    //-------------------------
    glGenRenderbuffers(1, &depthBufferID);
    glBindRenderbuffer(GL_RENDERBUFFER, depthBufferID);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, 
            renderSize, renderSize);
    //-------------------------
    //Attach depth buffer to FBO
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBufferID);
    glutHideWindow();
    
    glGenBuffers(1, &vertexBufferID);
}

void ObjRenderer::makeTex(const std::string& shaderVarname, const cv::Mat& tex)
{
    GLuint textureID;

    glGenTextures(1, &textureID);
    
    glActiveTexture(GL_TEXTURE0+texCount);

    glBindTexture(GL_TEXTURE_2D, textureID);

    GLenum format = 0;
    GLenum type = 0;
    
    GLint swizzleMask[] = {GL_RED, GL_RED, GL_RED, GL_RED};
    
    switch(tex.type())
    {
    case CV_8UC3:
        type = GL_UNSIGNED_BYTE;
        format = GL_RGB;
        break;
    case CV_32FC1:
        type = GL_FLOAT;
        format = GL_RED;
        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
        break;
    case CV_32FC3:
        type = GL_FLOAT;
        format = GL_RGB;
        break;
    }
    glTexImage2D(GL_TEXTURE_2D, 0, format, tex.cols, tex.rows, 1, 
            format, type, tex.data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    
    glGenerateMipmap(GL_TEXTURE_2D);
    
    glUniform1i(glGetUniformLocation(shaderProgID, shaderVarname.c_str()), texCount);
    texCount++;
}

void ObjRenderer::nextSeed()
{
    shaderSeed = shaderSeed * 86028121 + 236887691;
}


void ObjRenderer::loadEnvMap(const std::string& path, bool gray)
{
    cv::Mat map;
    fipImage fip_map;
    
    if(path.substr(path.length()-3, 3) == "hdr")
    {
        fip_map.load(path.c_str());
        fi2mat(fip_map, map);
    }
    else
    {
        map = cv::imread(path);
    }
    
    if(gray)
    {
        cv::cvtColor(map, map, CV_RGB2GRAY);
    }
    
    cv::pow(map, 1/2.2, map);
    
    cv::Mat map_diff = map.clone();
    cv::Mat map_spec = map.clone();
    
    cv::GaussianBlur(map_diff, map_diff, 
            cv::Size(int(map_diff.cols*0.5)*2+1, int(map_diff.rows*0.5)*2+1), 0, 0);
    
    cv::GaussianBlur(map_spec, map_spec, 
            cv::Size(int(map_spec.cols*0.1)*2+1, int(map_spec.rows*0.1)*2+1), 0, 0);
    
    
    if(map.type() == CV_32FC3)
    {
        uniform_horizontal_edges<cv::Vec3f>(map_diff);
        uniform_horizontal_edges<cv::Vec3f>(map_spec);
    }
    else
    {
        uniform_horizontal_edges<float>(map_diff);
        uniform_horizontal_edges<float>(map_spec);
    }
    
    makeTex("envmap_diff", map_diff);
    
    makeTex("envmap_spec", map_spec);
}

void ObjRenderer::loadModel(const std::string& path, RenderMode mode)
{
    renderMode = mode;
    std::string::size_type pos = path.rfind('/');
    std::string mtl_base_path = "";
    if(pos != std::string::npos)
    {
        mtl_base_path = path.substr(0, pos+1);
    }
    // load obj -- begin
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;
    bool ret = tinyobj::LoadObj(shapes, materials, err, path.c_str(), mtl_base_path.c_str());
    if (!err.empty()) 
    { // `err` may contain warning message.
        std::cerr << err << std::endl;
    }
    if (!ret) 
    {
        exit(1);
    }
    // load obj -- end
    
    // find bound -- begin
    glm::vec3 lb(std::numeric_limits<float>::max());
    glm::vec3 ub(std::numeric_limits<float>::min());
    
    for(size_t shape_index=0; shape_index<shapes.size(); shape_index++)
    {
        const tinyobj::mesh_t & mesh = shapes[shape_index].mesh;
        for(size_t index=0; index<mesh.positions.size(); index++)
        {
            const float pos = mesh.positions[index];
            lb[index % 3] = lb[index % 3] > pos ? pos : lb[index % 3];
            ub[index % 3] = ub[index % 3] < pos ? pos : ub[index % 3];
        }
    }
    
    glm::vec3 center = (ub + lb) / 2.0f;
    float diagLen = glm::length(ub-lb);
    // find bound -- end
    
    // make attribute buffers -- begin
    
    std::vector<Attribute> attributeData;
    std::unordered_map<unsigned, std::vector<Attribute> > mat_id2attrList;
    
    unsigned mat_id = 0;
    
    for(size_t shape_index=0; shape_index<shapes.size(); shape_index++)
    {
        const tinyobj::mesh_t& mesh = shapes[shape_index].mesh;
        Attribute attr;
        
        for(size_t index=0; index < mesh.indices.size(); index++)
        {
            const unsigned vert_index = mesh.indices[index];
            
            if(index % 3 == 0)
            {
                unsigned i1 = mesh.indices[index];
                unsigned i2 = mesh.indices[index+1];
                unsigned i3 = mesh.indices[index+2];
                glm::vec3 v1 = getVec(&mesh.positions[i1*3]);
                glm::vec3 v2 = getVec(&mesh.positions[i2*3]);
                glm::vec3 v3 = getVec(&mesh.positions[i3*3]);
                attr.normal = glm::normalize(glm::cross(v2-v1, v3-v2));
                if(flipNormals) 
                    attr.normal = -attr.normal;
                
                if(materials.size())
                {
                    mat_id = mesh.material_ids[index / 3];
                    attr.kd = glm::vec3(0.5);
                    attr.ka = attr.ks = glm::vec3(0);
                    
                    if(mat_id < materials.size())
                    {
                        const tinyobj::material_t mat = materials[mat_id];
                        attr.kd.x = mat.diffuse[2];
                        attr.kd.y = mat.diffuse[1];
                        attr.kd.z = mat.diffuse[0];

                        attr.ka.x = mat.ambient[2];
                        attr.ka.y = mat.ambient[1];
                        attr.ka.z = mat.ambient[0];

                        attr.ks.x = mat.specular[2];
                        attr.ks.y = mat.specular[1];
                        attr.ks.z = mat.specular[0];

                        attr.shiness = mat.shininess;
                    }
                }
            }
            
            if(!faceNormals && vert_index*3+2 < mesh.normals.size())
            {
                attr.normal = getVec(&mesh.normals[vert_index*3]);
                attr.normal.z = attr.normal.x;
                attr.normal.x = mesh.normals[vert_index*3+2];
                if(flipNormals) 
                    attr.normal = -attr.normal;
            }
            
            if(vert_index*2+1 < mesh.texcoords.size())
            {
                attr.texCoord.x = mesh.texcoords[vert_index*2];
                attr.texCoord.y = mesh.texcoords[vert_index*2+1];
            }
            
            glm::vec3 unit_pos;
            unit_pos = getVec(&mesh.positions[vert_index*3]);
            unit_pos = (unit_pos-center) * 2.0f / diagLen;
            attr.vertex = unit_pos;
            attributeData.push_back(attr);
            if(renderMode == RENDER_MODE_TEXTURE)
                mat_id2attrList[mat_id].push_back(attr);
        }
    }
    
    if(renderMode == RENDER_MODE_TEXTURE)
    {
        unsigned current_size = 0;

        for(auto it = mat_id2attrList.begin(); it != mat_id2attrList.end(); it++)
        {
            for(unsigned i=0; i<it->second.size(); i++)
                attributeData[current_size+i] = it->second[i];
            current_size += it->second.size();
            matGroupSizeList.push_back(it->second.size());
        }
    }
    
    nTriangles = attributeData.size() / 3;
    
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
    
    glBufferData(GL_ARRAY_BUFFER, sizeof(Attribute)*attributeData.size(), 
            attributeData.data(), GL_STATIC_DRAW);
    
    GLuint loc;
    
    loc = glGetAttribLocation(shaderProgID, "vertex");
    glEnableVertexAttribArray(loc);
    glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 
          sizeof(Attribute), BUFFER_OFFSET(offsetof(Attribute, vertex)));
    
    loc = glGetAttribLocation(shaderProgID, "normal");
    glEnableVertexAttribArray(loc);
    glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 
          sizeof(Attribute), BUFFER_OFFSET(offsetof(Attribute, normal)));
    
    loc = glGetAttribLocation(shaderProgID, "texCoord");
    glEnableVertexAttribArray(loc);
    glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, 
          sizeof(Attribute), BUFFER_OFFSET(offsetof(Attribute, texCoord)));
    
    loc = glGetAttribLocation(shaderProgID, "ambient");
    glEnableVertexAttribArray(loc);
    glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 
          sizeof(Attribute), BUFFER_OFFSET(offsetof(Attribute, ka)));
    
    loc = glGetAttribLocation(shaderProgID, "diffuse");
    glEnableVertexAttribArray(loc);
    glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 
          sizeof(Attribute), BUFFER_OFFSET(offsetof(Attribute, kd)));
    
    loc = glGetAttribLocation(shaderProgID, "specular");
    glEnableVertexAttribArray(loc);
    glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 
          sizeof(Attribute), BUFFER_OFFSET(offsetof(Attribute, ks)));
    
    loc = glGetAttribLocation(shaderProgID, "shiness");
    glEnableVertexAttribArray(loc);
    glVertexAttribPointer(loc, 1, GL_FLOAT, GL_FALSE, 
          sizeof(Attribute), BUFFER_OFFSET(offsetof(Attribute, shiness)));
    
    // make attribute buffers -- end
}

void ObjRenderer::renderView(const glm::vec3& front, const glm::vec3& up)
{
    glUniform1ui(glGetUniformLocation(shaderProgID, "seed"), shaderSeed);
    glUniform1ui(glGetUniformLocation(shaderProgID, "outputID"), shaderOutputID);
    
    glm::vec3 right = glm::normalize(glm::cross(front, up));
    glm::vec3 rectUp = glm::cross(right, front); 
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
   
    glm::mat4 projMat = glm::perspective<float>(30, 1, 0.01, 10);
    
    glUniformMatrix4fv(glGetUniformLocation(shaderProgID, "projMat"), 
            1, false, (float*)&projMat);
    
    glm::vec3 eye = -front;
    glm::vec3 center = glm::vec3(0, 0, 0);
    
    glBindFramebuffer(GL_FRAMEBUFFER, frameBufferID);
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    
    glEnable(GL_DEPTH_TEST);

    glm::mat4 viewMat = glm::lookAt(eye, center, rectUp);
    
    glUniformMatrix4fv(glGetUniformLocation(shaderProgID, "viewMat"), 
            1, false, (float*)&viewMat);
    
    glUniform3fv(glGetUniformLocation(shaderProgID, "eyePos"), 
            1, (float*)&eye);
    
    glUseProgram(shaderProgID);
    
    glEnable(GL_BLEND);

    glDrawArrays(GL_TRIANGLES, 0, 3*nTriangles);
    
    glPopAttrib();
    glFlush();
}

cv::Mat4f ObjRenderer::genShading(const glm::vec3& front, const glm::vec3& up)
{
    cv::Mat4f image(renderSize, renderSize);
    image.setTo(0.0);
    renderView(front, up);
    glReadPixels(0, 0, renderSize, renderSize, GL_RGBA, GL_FLOAT, image.data);
    cv::flip(image, image, 0);
    return image;
}

void ObjRenderer::render()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, 1, 0, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, colorTexID);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0);
    glVertex2f(0, 0);
    glTexCoord2f(0, 1);
    glVertex2f(0, 1);
    glTexCoord2f(1, 1);
    glVertex2f(1, 1);
    glTexCoord2f(1, 0);
    glVertex2f(1, 0);
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glFlush();
}
/* 
 * File:   ObjRenderer.cpp
 * Author: swl
 * 
 * Created on November 29, 2015, 12:02 PM
 */
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#include <limits>
#include <GL/glew.h>
#include "ShaderUtils.h"
#include "ShaderData.h"
#include "ObjRenderer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <bits/unordered_map.h>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "ImageUtils.h"

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

GLuint ObjRenderer::colorTexID = 0;
GLuint ObjRenderer::depthBufferID = 0;
GLuint ObjRenderer::frameBufferID = 0;
GLuint ObjRenderer::shaderProgID = 0;
GLuint ObjRenderer::vertexBufferID = 0;
GLuint ObjRenderer::renderSize = 512;
GLuint ObjRenderer::nTriangles = 0;
GLuint ObjRenderer::mapDiffID = 0;
GLuint ObjRenderer::mapSpecID = 0;
GLuint ObjRenderer::blankTexID = 0;
unsigned ObjRenderer::shaderSeed = 0;
unsigned ObjRenderer::shaderOutputID = 0;
bool ObjRenderer::flipNormals = false;
bool ObjRenderer::faceNormals = false;
std::vector<ObjRenderer::MatGroupInfo> ObjRenderer::matGroupInfoList;
std::vector<glm::vec3> ObjRenderer::vertices;
std::unordered_map<std::string, GLuint> ObjRenderer::texPath2texID;
std::unordered_map<std::string, GLuint> ObjRenderer::shaderTexName2texUnit;
glm::vec3 ObjRenderer::eyeFocus(0, 0, 0);
glm::vec3 ObjRenderer::eyeUp(0, 1, 0);
glm::vec3 ObjRenderer::eyePos(2, 2, 2);

GLuint ObjRenderer::getTexID(const std::string& path)
{
    if(texPath2texID.find(path) == texPath2texID.end())
    {
        cv::Mat image = loadImage(path);
        texPath2texID[path] = makeTex(image);
    }
    return texPath2texID[path];
}

void ObjRenderer::clearTextures()
{
    for(auto it = texPath2texID.begin(); it != texPath2texID.end(); it++)
    {
        glDeleteTextures(1, &it->second);
    }
    texPath2texID.clear();
}

void ObjRenderer::init(unsigned size)
{   
    renderSize = size;
    int ac = 0;
    char** av;
    glutInit(&ac, av);
    glutInitWindowSize(renderSize, renderSize);
    glutCreateWindow("SketchRenderer");
    glewInit();
    
    shaderSeed = time(0);
    
    glutDisplayFunc(render);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
    glClearColor(0, 0, 0, 0);
    
    std::vector<std::string> fragList;
    fragList.push_back("Shader/main.frag");
    fragList.push_back("Shader/coord.frag");
    fragList.push_back("Shader/phong.frag");
    fragList.push_back("Shader/brdf.frag");
    
    shaderProgID = loadShaders("Shader/geo.vert", fragList);
    
    glClampColor(GL_CLAMP_READ_COLOR, GL_FALSE);
    glClampColor(GL_CLAMP_VERTEX_COLOR, GL_FALSE);
    glClampColor(GL_CLAMP_FRAGMENT_COLOR, GL_FALSE);
    
    glGenTextures(1, &colorTexID);
    glBindTexture(GL_TEXTURE_2D, colorTexID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
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
    
    glGenBuffers(1, &vertexBufferID);
    
    cv::Mat blank_image(8, 8, CV_8UC1, cv::Scalar(255));
    blankTexID = makeTex(blank_image);
}

GLuint ObjRenderer::makeTex(const cv::Mat& tex)
{
    
    GLuint textureID;

    glGenTextures(1, &textureID);

    glBindTexture(GL_TEXTURE_2D, textureID);

    GLenum int_format = 0;
    GLenum format = 0;
    GLenum type = 0;
    
    GLint swizzleMask[] = {GL_RED, GL_RED, GL_RED, GL_RED};
    
    switch(tex.type())
    {
    case CV_8UC4:
        type = GL_UNSIGNED_BYTE;
        format = GL_RGBA;
        int_format = GL_BGRA;
        break;
    case CV_8UC3:
        type = GL_UNSIGNED_BYTE;
        format = GL_RGB;
        int_format = GL_BGR;
        break;
    case CV_32FC1:
        type = GL_FLOAT;
        format = GL_RED;
        int_format = GL_RED;
        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
        break;
    case CV_8UC1:
        type = GL_UNSIGNED_BYTE;
        format = GL_RED;
        int_format = GL_RED;
        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
        break;
    case CV_32FC3:
        type = GL_FLOAT;
        format = GL_RGB;
        int_format = GL_BGR;
        break;
    }
    
    glTexImage2D(GL_TEXTURE_2D, 0, format, tex.cols, tex.rows, 1, 
            int_format, type, tex.data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
    
    glGenerateMipmap(GL_TEXTURE_2D);
    
    return textureID;
}

void ObjRenderer::nextSeed()
{
    shaderSeed = shaderSeed * 86028121 + 236887691;
}

void ObjRenderer::loadEnvMap(const std::string& path, bool gray)
{
    cv::Mat colored_map = loadImage(path);
    cv::Mat map;
    
    if(gray)
    {
        cv::cvtColor(colored_map, map, CV_BGR2GRAY);
    }
    else
    {
        map = colored_map;
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
    
    glUseProgram(shaderProgID);
    mapDiffID = makeTex(map_diff);
    useTexture("envmap_diff", mapDiffID);
    mapSpecID = makeTex(map_spec);
    useTexture("envmap_spec", mapSpecID);
}

void ObjRenderer::useTexture(const std::string& shaderVarName, GLuint texID)
{
    GLuint unit = 0;
    if(shaderTexName2texUnit.find(shaderVarName) == shaderTexName2texUnit.end())
    {
        unit = shaderTexName2texUnit.size();
        shaderTexName2texUnit[shaderVarName] = unit;
    }
    else
    {
        unit = shaderTexName2texUnit[shaderVarName];
    }
    
    glUseProgram(shaderProgID);
    
    glUniform1i(glGetUniformLocation(shaderProgID, shaderVarName.c_str()), unit+1);
    
    glActiveTexture(GL_TEXTURE1+unit);
    glBindTexture(GL_TEXTURE_2D, texID);
    if(texID == 0)
        glBindTexture(GL_TEXTURE_2D, blankTexID);
    glActiveTexture(GL_TEXTURE1+shaderTexName2texUnit.size());
}

void ObjRenderer::loadModel(const std::string& path, bool unitize)
{
    matGroupInfoList.clear();
    clearTextures();
    
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
                glm::vec3 v1 = getVec<glm::vec3>(&mesh.positions[i1*3]);
                glm::vec3 v2 = getVec<glm::vec3>(&mesh.positions[i2*3]);
                glm::vec3 v3 = getVec<glm::vec3>(&mesh.positions[i3*3]);
                attr.normal = glm::normalize(glm::cross(v2-v1, v3-v2));
                if(flipNormals) 
                    attr.normal = -attr.normal;
                
                if(materials.size())
                    mat_id = mesh.material_ids[index / 3];
            }
            
            if(!faceNormals && vert_index*3+2 < mesh.normals.size())
            {
                attr.normal = getVec<glm::vec3>(&mesh.normals[vert_index*3]);
                if(flipNormals) 
                    attr.normal = -attr.normal;
            }
            
            if(vert_index*2+1 < mesh.texcoords.size())
            {
                attr.texCoord.x = mesh.texcoords[vert_index*2];
                attr.texCoord.y = -mesh.texcoords[vert_index*2+1];
            }
            
            attr.vertex = getVec<glm::vec3>(&mesh.positions[vert_index*3]);
            
            if(unitize)
            {
                glm::vec3 unit_pos;
                unit_pos = getVec<glm::vec3>(&mesh.positions[vert_index*3]);
                unit_pos = (unit_pos-center) * 2.0f / diagLen;
                attr.vertex = unit_pos;
            }
            attributeData.push_back(attr);
            mat_id2attrList[mat_id].push_back(attr);
        }
    }
    
    unsigned current_size = 0;
    // iterate through material groups

    for(auto it = mat_id2attrList.begin(); it != mat_id2attrList.end(); it++)
    {
        for(unsigned i=0; i<it->second.size(); i++)
            attributeData[current_size+i] = it->second[i];
        current_size += it->second.size();

        MatGroupInfo info;
        info.size = it->second.size();

        if(it->first < materials.size())
        {
            const tinyobj::material_t& mat = materials[it->first];
            info.shaderData = makeMaterial(mat, mtl_base_path);
        }

        matGroupInfoList.push_back(info);
    }
    
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
    
    // make attribute buffers -- end
}

void ObjRenderer::renderView()
{
    
    glBindFramebuffer(GL_FRAMEBUFFER, frameBufferID);
    glUseProgram(shaderProgID);
    
    glUniform1ui(glGetUniformLocation(shaderProgID, "seed"), shaderSeed);
    glUniform1ui(glGetUniformLocation(shaderProgID, "outputID"), shaderOutputID);
    
    glm::vec3 front = eyeFocus - eyePos;
    
    glm::vec3 right = glm::normalize(glm::cross(front, eyeUp));
    glm::vec3 rectUp = glm::cross(right, front); 
   
    glm::mat4 projMat = glm::perspective<float>(30, 1, 0.01, 100);
    
    glUniformMatrix4fv(glGetUniformLocation(shaderProgID, "projMat"), 
            1, false, (float*)&projMat);
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    
    glEnable(GL_DEPTH_TEST);

    glm::mat4 viewMat = glm::lookAt(eyePos, eyeFocus, rectUp);
    
    glUniformMatrix4fv(glGetUniformLocation(shaderProgID, "viewMat"), 
            1, false, (float*)&viewMat);
    
    glUniform3fv(glGetUniformLocation(shaderProgID, "eyePos"), 
            1, (float*)&eyePos);
    
    unsigned base_offset = 0;
    
    useTexture("envmap_diff", mapDiffID);
    
    useTexture("envmap_spec", mapSpecID);
    
    for(unsigned i=0; i<matGroupInfoList.size(); i++)
    {
        const MatGroupInfo &info = matGroupInfoList[i];
        info.shaderData->send2shader(shaderProgID);
        if(shaderOutputID > 0)
            glUniform1ui(glGetUniformLocation(shaderProgID, "outputID"), shaderOutputID);
        glDrawArrays(GL_TRIANGLES, base_offset, info.size);
        base_offset += info.size;
    }
    
    glPopAttrib();
    glFlush();
}

cv::Mat4f ObjRenderer::genShading()
{
    cv::Mat4f image(renderSize, renderSize);
    image.setTo(0.0);
    renderView();
    glReadPixels(0, 0, renderSize, renderSize, GL_RGBA, GL_FLOAT, image.data);
    cv::flip(image, image, 0);
    cv::cvtColor(image, image, CV_RGBA2BGRA);
    return image;
}

void ObjRenderer::render()
{
    renderView();
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(0);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, colorTexID);
    glEnable(GL_TEXTURE_2D);
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, 1, 0, 1);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glColor4f(1, 1, 1, 1);
    
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
    glutSwapBuffers();
}
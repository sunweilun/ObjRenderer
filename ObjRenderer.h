/* 
 * File:   ObjRenderer.h
 * Author: swl
 *
 * Created on November 29, 2015, 12:02 PM
 */

#ifndef OBJRENDERER_H
#define	OBJRENDERER_H

#include "ShaderData.h"

#include <opencv2/opencv.hpp>
#include <memory>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <stdio.h>
#include <vector>
#include <unordered_map>
#include "tiny_obj_loader.h"

template<typename T> inline T getVec(const float* data)
{
    T v;
    memcpy(&v, data, sizeof(T));
    return v;
}

class ObjRenderer
{
    friend class ShaderDataPhong;
public:
    
    static void init(unsigned size = 512);
    static void nextSeed();
    static void loadEnvMap(const std::string& path, bool gray = false);
    static void loadModel(const std::string& path, bool unitize = true);
    static void setEyeUp(const glm::vec3& up) { eyeUp = up; }
    static void setEyeFocus(const glm::vec3& focus) { eyeFocus = focus; }
    static void setEyePos(const glm::vec3& pos) { eyePos = pos; }
    static cv::Mat4f genShading(bool forceOutputID = false);
    static unsigned setShaderOutputID(unsigned id) { shaderOutputID = id; }
    
    
protected:
    struct MatGroupInfo
    {
        unsigned size;
        std::shared_ptr<ShaderData> shaderData;
    };
    static std::vector<MatGroupInfo> matGroupInfoList;
    struct Attribute
    {
        glm::vec3 vertex;
        glm::vec3 normal;
        glm::vec2 texCoord;
    };
    static void render();
    static glm::vec3 eyePos, eyeFocus, eyeUp;
    static void renderView(bool forceOutputID = false);
    static void clearTextures();
    static GLuint makeTex(const cv::Mat& tex);
    static GLuint getTexID(const std::string& path);
    static std::shared_ptr<ShaderData> makeMaterial(const tinyobj::material_t& mat, 
        const std::string& mtl_base_path);
    static void useTexture(const std::string& shaderVarName, GLuint texID);
    static std::vector<glm::vec3> vertices;
    static GLuint colorTexID;
    static GLuint depthBufferID;
    static GLuint frameBufferID;
    static GLuint shaderProgID;
    static GLuint renderSize;
    static GLuint vertexBufferID;
    static GLuint nTriangles;
    static GLuint mapDiffID;
    static GLuint mapSpecID;
    static GLuint blankTexID;
    static unsigned shaderSeed;
    static unsigned shaderOutputID;
    static bool flipNormals;
    static bool faceNormals;
    static std::unordered_map<std::string, GLuint> shaderTexName2texUnit;
    static std::unordered_map<std::string, GLuint> texPath2texID;
};

#endif	/* SKETCHRENDERER_H */


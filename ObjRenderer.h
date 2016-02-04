/* 
 * File:   ObjRenderer.h
 * Author: swl
 *
 * Created on November 29, 2015, 12:02 PM
 */

#ifndef OBJRENDERER_H
#define	OBJRENDERER_H

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/freeglut.h>
#include <glm/glm.hpp>
#include <opencv2/opencv.hpp>

#include <stdio.h>
#include <vector>
#include <unordered_map>

typedef enum {RENDER_MODE_PLAIN, RENDER_MODE_TEXTURE} RenderMode;

class ObjRenderer
{
public:
    static GLuint makeTex(const cv::Mat& tex);
    static void init(unsigned size = 512);
    static void nextSeed();
    static void loadEnvMap(const std::string& path, bool gray = false);
    static void loadModel(const std::string& path, RenderMode mode = RENDER_MODE_TEXTURE);
    static cv::Mat4f genShading(const glm::vec3& front, const glm::vec3& up);
    static unsigned setShaderOutputID(unsigned id) { shaderOutputID = id; }
    static void clearTextures();
protected:
    struct MatGroupInfo
    {
        unsigned size;
        GLuint diffTexID;
    };
    static std::vector<MatGroupInfo> matGroupInfoList;
    struct Attribute
    {
        glm::vec3 vertex;
        glm::vec3 normal;
        glm::vec2 texCoord;
        glm::vec3 ka;
        glm::vec3 kd;
        glm::vec3 ks;
        float shiness;
    };
    
    static RenderMode renderMode;
    static void renderView(const glm::vec3& front, const glm::vec3& up);
    static void render();
    static GLuint getTexID(const std::string& path);
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


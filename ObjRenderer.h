/* 
 * File:   ObjRenderer.h
 * Author: swl
 *
 * Created on November 29, 2015, 12:02 PM
 */

#ifndef OBJRENDERER_H
#define	OBJRENDERER_H

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/freeglut.h>
#include <glm/glm.hpp>
#include <opencv2/opencv.hpp>

#include <stdio.h>
#include <vector>
#include <unordered_map>

class ObjRenderer
{
public:
    static void makeTex(const std::string& shaderVarname, const cv::Mat& tex);
    static void init();
    static void loadEnvMap(const std::string& path);
    static void loadModel(const std::string& path);
    static cv::Mat3f genShading(const glm::vec3& front, const glm::vec3& up);
protected:
    static GLuint texCount;
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
    static void renderView(const glm::vec3& front, const glm::vec3& up);
    static void render();
    static std::vector<glm::vec3> vertices;
    static GLuint colorTexID;
    static GLuint depthBufferID;
    static GLuint frameBufferID;
    static GLuint shaderProgID;
    static GLuint renderSize;
    static GLuint vertexBufferID;
    static GLuint nTriangles;
    static bool flipNormals;
    static bool faceNormals;
};

#endif	/* SKETCHRENDERER_H */


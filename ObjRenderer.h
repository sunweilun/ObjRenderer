/* 
 * File:   ObjRenderer.h
 * Author: swl
 *
 * Created on November 29, 2015, 12:02 PM
 */

#ifndef OBJRENDERER_H
#define	OBJRENDERER_H

#include "tiny_obj_loader.h"
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/freeglut.h>
#include <glm/glm.hpp>
#include <opencv2/opencv.hpp>

#include <stdio.h>
#include <vector>

class ObjRenderer
{
public:
    static void init();
    static void load(const std::string& path);
    static cv::Mat3f genShading(const glm::vec3& front, const glm::vec3& up);
protected:
    static void renderView(const glm::vec3& front, const glm::vec3& up);
    static void render();
    static std::vector<glm::vec3> vertices;
    static GLuint listID;
    static GLuint colorTexID;
    static GLuint depthBufferID;
    static GLuint frameBufferID;
    static GLuint renderSize;
    static bool flipNormals;
    static bool faceNormals;
};

#endif	/* SKETCHRENDERER_H */


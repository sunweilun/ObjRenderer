/* 
 * File:   main.cpp
 * Author: swl
 *
 * Created on January 16, 2016, 2:05 PM
 */

#include <cstdlib>
#include "tiny_obj_loader.h"
#include "ObjRenderer.h"

using namespace std;

/*
 * 
 */
int main(int argc, char** argv) 
{   
    ObjRenderer::init();
    ObjRenderer::loadEnvMap("envmaps/envmap.jpg");
    ObjRenderer::loadModel("models/car1.obj");
    
    
    cv::Mat3f image = ObjRenderer::genShading(glm::vec3(-1, -0.7, -1)*0.8f, glm::vec3(0, 1, 0));
    cv::namedWindow("image");
    cv::imshow("image", image);
    cv::waitKey();
    cv::imwrite("out.png", image*255.0);
    
    return 0;
}


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
    
    ObjRenderer::load("models/car2.obj");
    
    cv::Mat3f image = ObjRenderer::genShading(glm::vec3(-0.8, -0.5, 0.8), glm::vec3(0, 1, 0));
    cv::namedWindow("image");
    cv::imshow("image", image);
    cv::waitKey();
    cv::imwrite("out.png", image*255.0);
    
    return 0;
}


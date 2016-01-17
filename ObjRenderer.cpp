/* 
 * File:   ObjRenderer.cpp
 * Author: swl
 * 
 * Created on November 29, 2015, 12:02 PM
 */

#include <limits>
#include <GL/glew.h>

#include "ObjRenderer.h"
#include <iostream>

GLuint ObjRenderer::colorTexID = 0;
GLuint ObjRenderer::depthBufferID = 0;
GLuint ObjRenderer::frameBufferID = 0;
GLuint ObjRenderer::listID = 0;
GLuint ObjRenderer::renderSize = 800;
bool ObjRenderer::flipNormals = true;
bool ObjRenderer::faceNormals = false;
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
    listID = glGenLists(1);
    glutDisplayFunc(render);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_SINGLE | GLUT_RGB);
    glClearColor(0, 0, 0, 0);
    
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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, renderSize, renderSize, 
            0, GL_RGB, GL_FLOAT, NULL);
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
}

void ObjRenderer::load(const std::string& path)
{
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
    
    // compile render list -- begin
    
    glNewList(listID, GL_COMPILE);
    glBegin(GL_TRIANGLES);
    for(size_t shape_index=0; shape_index<shapes.size(); shape_index++)
    {
        const tinyobj::mesh_t& mesh = shapes[shape_index].mesh;
        glm::vec3 normal;
        for(size_t index=0; index < mesh.indices.size(); index++)
        {
            const unsigned vert_index = mesh.indices[index];
            
            if(index % 3 == 0)
            {
                unsigned mat_id = mesh.material_ids[index / 3];
                const tinyobj::material_t mat = materials[mat_id];
                glm::vec3 color;
                color.x = mat.diffuse[2];
                color.y = mat.diffuse[1];
                color.z = mat.diffuse[0];
                glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, (float*)&color);
                
                color.x = mat.ambient[2];
                color.y = mat.ambient[1];
                color.z = mat.ambient[0];
                glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, (float*)&color);
                
                color.x = mat.specular[2];
                color.y = mat.specular[1];
                color.z = mat.specular[0];
                glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, (float*)&color);
                
                glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, mat.shininess);
                
                if(faceNormals)
                {
                    unsigned i1 = mesh.indices[index];
                    unsigned i2 = mesh.indices[index+1];
                    unsigned i3 = mesh.indices[index+2];
                    glm::vec3 v1 = getVec(&mesh.positions[i1*3]);
                    glm::vec3 v2 = getVec(&mesh.positions[i2*3]);
                    glm::vec3 v3 = getVec(&mesh.positions[i3*3]);
                    normal = glm::normalize(glm::cross(v2-v1, v3-v2));
                    if(flipNormals) 
                        normal = -normal;
                }
            }
            
            if(!faceNormals)
            {
                normal = getVec(&mesh.normals[vert_index*3]);
                normal.z = normal.x;
                normal.x = mesh.normals[vert_index*3+2];
                if(flipNormals) 
                    normal = -normal;
            }
            
            glNormal3fv((float*)&normal);
            glTexCoord2fv(&mesh.texcoords[vert_index*2]);
            glm::vec3 unit_pos;
            unit_pos = getVec(&mesh.positions[vert_index*3]);
            unit_pos = (unit_pos-center) * 2.0f / diagLen;
            glVertex3fv((float*)&unit_pos);
        }
    }
    glEnd();
    
    glEndList();
    // compile render list -- end
}

void ObjRenderer::renderView(const glm::vec3& front, const glm::vec3& up)
{
    glm::vec3 right = glm::normalize(glm::cross(front, up));
    glm::vec3 rectUp = glm::cross(right, front); 
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
   
    gluPerspective(90, 1, 0.01, 10);
    
    glm::vec3 eye = -front;
    glm::vec3 center = glm::vec3(0, 0, 0);
    
    
    
    glBindFramebuffer(GL_FRAMEBUFFER, frameBufferID);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_COLOR_MATERIAL_FACE);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    float lp0[] = {0, 0, 0, 1};
    glLightfv(GL_LIGHT0, GL_POSITION, lp0);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    
    gluLookAt(eye.x, eye.y, eye.z, 
            center.x, center.y, center.z,
            rectUp.x, rectUp.y, rectUp.z);
    
    glCallList(listID);
    glPopAttrib();
    glFlush();
}

cv::Mat3f ObjRenderer::genShading(const glm::vec3& front, const glm::vec3& up)
{
    cv::Mat3f image(renderSize, renderSize);
    image.setTo(0.0);
    renderView(front, up);
    std::vector<glm::vec3> color(renderSize*renderSize);
    glReadPixels(0, 0, renderSize, renderSize, GL_RGB, GL_FLOAT, color.data());
    
    for(unsigned y=0; y<renderSize; y++)
    {
        for(unsigned x=0; x<renderSize; x++)
        {
            const glm::vec3& c = color[(renderSize-1-y)*renderSize+x];
            image.at<cv::Vec3f>(y, x) = cv::Vec3f(c.x, c.y, c.z);
        }
    }
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
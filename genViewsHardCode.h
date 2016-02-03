/* 
 * File:   genViewsHardCode.h
 * Author: swl
 *
 * Created on January 18, 2016, 8:39 PM
 */

#ifndef GENVIEWSHARDCODE_H
#define	GENVIEWSHARDCODE_H

#include "ObjRenderer.h"
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>

struct Args
{
    int theta_inc;
    int phi_inc;
    int phi_max;
    float brightness;
    bool output_vertex;
};

inline int is_big_endian()
{
    union {
        uint32_t i;
        char c[4];
    } bint = {0x01020304};

    return bint.c[0] == 1; 
}

void processModel(const std::string& rootPath, 
        const std::string& filePath,
        const Args& args)
{
    ObjRenderer::loadModel(rootPath+filePath);
    ObjRenderer::nextSeed();
    
    char cmd[1024];
    
    std::string viewFolderPath = rootPath+
            filePath.substr(0, filePath.length()-4)+"_views";
    
    sprintf(cmd, "rm -f -r %s", viewFolderPath.c_str());
    
    system(cmd);
    
    mkdir(viewFolderPath.c_str(), 0777);
    
    char fn[1024];
    
    for(int theta = 0; theta < 360; theta += args.theta_inc)
    {
        for(int phi = 0; phi <= args.phi_max; phi += args.phi_inc)
        {
            float t = theta * M_PI / 180;
            float p = phi * M_PI / 180;
            
            glm::vec3 front(cos(p)*cos(t), sin(p), cos(p)*sin(t));
            
            cv::Mat4f image;
            cv::Mat4f aa_image;   
            
                    
            // output image
            ObjRenderer::setShaderOutputID(0);
            image = ObjRenderer::genShading(-front*4.f, glm::vec3(0, 1, 0));
            
            cv::GaussianBlur(image, image, cv::Size(3, 3), 0, 0);
            cv::resize(image, aa_image, cv::Size(256, 256));
            
            for(int i=0; i<aa_image.rows; i++)
            {
                for(int j=0; j<aa_image.cols; j++)
                {
                    cv::Vec4f c = aa_image.at<cv::Vec4f>(i, j);
                    aa_image.at<cv::Vec4f>(i, j) = cv::Vec4f(c[0]*args.brightness,
                            c[1]*args.brightness, c[2]*args.brightness, c[3]);
                }
            }
            
            sprintf(fn, "%s/%d_%d.png", viewFolderPath.c_str(), theta, phi);
            cv::imwrite(fn, aa_image*255.0);
            
            if(!args.output_vertex)
                continue;
            
            // output vertex
            ObjRenderer::setShaderOutputID(1);
            image = ObjRenderer::genShading(-front*4.f, glm::vec3(0, 1, 0));
            
            cv::GaussianBlur(image, image, cv::Size(3, 3), 0, 0);
            cv::resize(image, aa_image, cv::Size(256, 256));
            
            sprintf(fn, "%s/%d_%d.pfm", viewFolderPath.c_str(), theta, phi);

            std::vector<glm::vec3> pfm_colors(aa_image.rows*aa_image.cols);
            
            for(int i=0; i<aa_image.rows; i++)
            {
                for(int j=0; j<aa_image.cols; j++)
                {
                    cv::Vec4f c = aa_image.at<cv::Vec4f>(i, j);
                    glm::vec3 &pc = pfm_colors[i*aa_image.cols+j];
                    pc[0] = c[0];
                    pc[1] = c[1];
                    pc[2] = c[2];
                }
            }
            
            FILE* file = fopen(fn, "wb");
            fprintf(file, "PF\n%d %d\n", aa_image.cols, aa_image.rows);
            fprintf(file, "%f\n", is_big_endian()?1.0:-1.0);
            fwrite(pfm_colors.data(), sizeof(glm::vec3), aa_image.rows*aa_image.cols, file);
            fclose(file);
        }
    }
}

void
findModelsInFolder(const std::string& root, std::vector<std::pair<std::string, std::string> >& pathList)
{
    DIR *dp;
    struct dirent *dirp;
    if((dp  = opendir(root.c_str())) == NULL) {
        std::cout << "Error(" << errno << ") opening " << root
                << root << std::endl;

    }

    while ((dirp = readdir(dp)) != NULL) {
        std::string path = std::string(dirp->d_name);
        if(path == "." || path == "..")
            continue;
        if(int(dirp->d_type) == 4)
        {
            findModelsInFolder(root+path+"/", pathList);
            continue;
        }
        
        if(path.substr(path.length()-4, 4) == ".obj")
        {
            pathList.push_back(std::make_pair(root, path));
        } 
    }
    closedir(dp);
}

void genViews(const std::string envPath, const std::string& folderPath, const Args& args)
{
    ObjRenderer::init();
    ObjRenderer::loadEnvMap(envPath, true);
    
    std::vector<std::pair<std::string, std::string> > pathList;
    findModelsInFolder(folderPath, pathList);
    for(unsigned i=0; i<pathList.size(); i++)
    {
        std::string fullPath = pathList[i].first + pathList[i].second;
        printf("Processing %s (%d / %d)\n", fullPath.c_str(), 
                i+1, pathList.size());
        processModel(pathList[i].first, pathList[i].second, args);
    }
}

#endif	/* GENVIEWSHARDCODE_H */


/* 
 * File:   main.cpp
 * Author: swl
 *
 * Created on January 16, 2016, 2:05 PM
 */

#include "genViewsHardCode.h"

int main(int argc, char** argv) 
{   
    Args args;
    char folderPath[1024];
    char envPath[1024];
    args.theta_inc = 30;
    args.phi_inc = 10;
    args.phi_max = 30;
    args.brightness = 1;
    args.output_vertex = 1;
    
    FILE *file = fopen("config.txt", "r");
    fscanf(file, "folder_path = %s\n", &folderPath);
    fscanf(file, "envmap_path = %s\n", &envPath);
    fscanf(file, "theta_inc = %d\n", &args.theta_inc);
    fscanf(file, "phi_inc = %d\n", &args.phi_inc);
    fscanf(file, "phi_max = %d\n", &args.phi_max);
    fscanf(file, "output_vertex = %d\n", &args.output_vertex);
    fscanf(file, "brightness = %f\n", &args.brightness);
    fclose(file);
    
    genViews(envPath, folderPath, args);
    return 0;
}


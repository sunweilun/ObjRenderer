/* 
 * File:   ImageUtils.h
 * Author: swl
 *
 * Created on January 18, 2016, 2:16 PM
 */

#ifndef IMAGEUTILS_H
#define	IMAGEUTILS_H

#include <opencv/cv.h>
#include <FreeImagePlus.h>

void fi2mat(FIBITMAP* src, cv::Mat& dst)
{
    //FIT_BITMAP    //standard image : 1 - , 4 - , 8 - , 16 - , 24 - , 32 - bit
    //FIT_UINT16    //array of unsigned short : unsigned 16 - bit
    //FIT_INT16     //array of short : signed 16 - bit
    //FIT_UINT32    //array of unsigned long : unsigned 32 - bit
    //FIT_INT32     //array of long : signed 32 - bit
    //FIT_FLOAT     //array of float : 32 - bit IEEE floating point
    //FIT_DOUBLE    //array of double : 64 - bit IEEE floating point
    //FIT_COMPLEX   //array of FICOMPLEX : 2 x 64 - bit IEEE floating point
    //FIT_RGB16     //48 - bit RGB image : 3 x 16 - bit
    //FIT_RGBA16    //64 - bit RGBA image : 4 x 16 - bit
    //FIT_RGBF      //96 - bit RGB float image : 3 x 32 - bit IEEE floating point
    //FIT_RGBAF     //128 - bit RGBA float image : 4 x 32 - bit IEEE floating point

    int bpp = FreeImage_GetBPP(src);
    FREE_IMAGE_TYPE fit = FreeImage_GetImageType(src);

    int cv_type = -1;
    int cv_cvt = -1;
    
    switch (fit)
    {
    case FIT_UINT16: cv_type = cv::DataType<ushort>::type; break;
    case FIT_INT16: cv_type = cv::DataType<short>::type; break;
    case FIT_UINT32: cv_type = cv::DataType<unsigned>::type; break;
    case FIT_INT32: cv_type = cv::DataType<int>::type; break;
    case FIT_FLOAT: cv_type = cv::DataType<float>::type; break;
    case FIT_DOUBLE: cv_type = cv::DataType<double>::type; break;
    case FIT_COMPLEX: cv_type = cv::DataType<cv::Complex<double>>::type; break;
    case FIT_RGB16: cv_type = cv::DataType<cv::Vec<ushort, 3>>::type; cv_cvt = CV_RGB2BGR; break;
    case FIT_RGBA16: cv_type = cv::DataType<cv::Vec<ushort, 4>>::type; cv_cvt = CV_RGBA2BGRA; break;
    case FIT_RGBF: cv_type = cv::DataType<cv::Vec<float, 3>>::type; cv_cvt = CV_RGB2BGR; break;
    case FIT_RGBAF: cv_type = cv::DataType<cv::Vec<float, 4>>::type; cv_cvt = CV_RGBA2BGRA; break;
    case FIT_BITMAP:
        switch (bpp) {
        case 8: cv_type = cv::DataType<cv::Vec<uchar, 1>>::type; break;
        case 16: cv_type = cv::DataType<cv::Vec<uchar, 2>>::type; break;
        case 24: cv_type = cv::DataType<cv::Vec<uchar, 3>>::type; break;
        case 32: cv_type = cv::DataType<cv::Vec<uchar, 4>>::type; break;
        default:
            // 1, 4 // Unsupported natively
            cv_type = -1;
        }
        break;
    default:
        // FIT_UNKNOWN // unknown type
        dst = cv::Mat(); // return empty Mat
        return;
    }

    int width = FreeImage_GetWidth(src);
    int height = FreeImage_GetHeight(src);
    int step = FreeImage_GetPitch(src);
    if (cv_type >= 0) {
        dst = cv::Mat(height, width, cv_type, FreeImage_GetBits(src), step);
        if (cv_cvt > 0)
        {
            cv::cvtColor(dst, dst, cv_cvt);
        }
    }
    else {

        cv::vector<uchar> lut;
        int n = pow(2, bpp);
        for (int i = 0; i < n; ++i)
        {
            lut.push_back(static_cast<uchar>((255 / (n - 1))*i));
        }

        FIBITMAP* palletized = FreeImage_ConvertTo8Bits(src);
        BYTE* data = FreeImage_GetBits(src);
        for (int r = 0; r < height; ++r) {
            for (int c = 0; c < width; ++c) {
                dst.at<uchar>(r, c) = cv::saturate_cast<uchar>(lut[data[r*step + c]]);
            }
        }
    }

    cv::flip(dst, dst, 0);
}

template<typename T> void uniform_horizontal_edges(cv::Mat& mat)
{
    T top;
    T bottom;
    for(int i=0; i<mat.cols; i++)
    {
        top += mat.at<T>(0, i);
        bottom += mat.at<T>(mat.rows-1, i);
    }
    top /= mat.cols;
    bottom /= mat.cols;
    
    for(int i=0; i<mat.cols; i++)
    {
        T diff_top = top - mat.at<T>(0, i);
        T diff_bottom = bottom - mat.at<T>(mat.rows-1, i);
        for(int j=0; j<mat.rows; j++)
        {
            float ratio = j / float(mat.rows - 1);
            mat.at<T>(j, i) += (1-ratio)*diff_top + ratio*diff_bottom;
        }
    }
}

#endif	/* IMAGEUTILS_H */


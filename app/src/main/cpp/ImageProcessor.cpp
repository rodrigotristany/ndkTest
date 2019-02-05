//
// Created by troop on 09.08.2015.
//

#include "ImageProcessor.h"

void ImageProcessor::YuvToRgb(unsigned char* yuv420sp, jint width, jint height) {
    _width = width;
    _height = height;
    int frameSize = width * height;
    _data = new int[frameSize];

    int             sz;
    int             i;
    int             j;
    int             Y;
    int             Cr = 0;
    int             Cb = 0;
    int       pixPtr = 0;
    int             jDiv2 = 0;
    int             R = 0;
    int             G = 0;
    int             B = 0;
    int             cOff;
    int w = width;
    int h = height;
    sz = w * h;
    for(j = 0; j < h ; j++) {
        pixPtr = j * w;
        jDiv2 = j >> 1;
        for(i = 0; i < w; i++) {
            Y = yuv420sp[pixPtr];
            if(Y < 0) Y += 255;
            if((i & 0x1) != 1) {
                cOff = sz + jDiv2 * w + (i >> 1) * 2;
                Cb = yuv420sp[cOff];
                if(Cb < 0) Cb += 127; else Cb -= 128;
                Cr = yuv420sp[cOff + 1];
                if(Cr < 0) Cr += 127; else Cr -= 128;
            }

            //ITU-R BT.601 conversion
            //
            //R = 1.164*(Y-16) + 2.018*(Cr-128);
            //G = 1.164*(Y-16) - 0.813*(Cb-128) - 0.391*(Cr-128);
            //B = 1.164*(Y-16) + 1.596*(Cb-128);
            //
            Y = Y + (Y >> 3) + (Y >> 5) + (Y >> 7);
            R = Y + (Cr << 1) + (Cr >> 6);
            if(R < 0) R = 0; else if(R > 255) R = 255;
            G = Y - Cb + (Cb >> 3) + (Cb >> 4) - (Cr >> 1) + (Cr >> 3);
            if(G < 0) G = 0; else if(G > 255) G = 255;
            B = Y + Cb + (Cb >> 1) + (Cb >> 4) + (Cb >> 5);
            if(B < 0) B = 0; else if(B > 255) B = 255;
            _data[pixPtr++] = GetPixelFromRGB(R,G,B);
        }
    }
}


jobject ImageProcessor::getBitmap(JNIEnv * env) {
    void *bitmapPixels;
    int ret;
    jclass bitmapCls = env->FindClass("android/graphics/Bitmap");
    jmethodID createBitmapFunction = env->GetStaticMethodID(bitmapCls, "createBitmap",
                                                            "(IILandroid/graphics/Bitmap$Config;)Landroid/graphics/Bitmap;");
    jstring configName = env->NewStringUTF("ARGB_8888");
    jclass bitmapConfigClass = env->FindClass("android/graphics/Bitmap$Config");
    jmethodID valueOfBitmapConfigFunction = env->GetStaticMethodID(bitmapConfigClass, "valueOf",
                                                                   "(Ljava/lang/String;)Landroid/graphics/Bitmap$Config;");
    jobject bitmapConfig = env->CallStaticObjectMethod(bitmapConfigClass,
                                                       valueOfBitmapConfigFunction, configName);
    jobject newBitmap = env->CallStaticObjectMethod(bitmapCls, createBitmapFunction, _width,
                                                    _height, bitmapConfig);

    if ((ret = AndroidBitmap_lockPixels(env, newBitmap, &bitmapPixels)) < 0) {
        LOGD("AndroidBitmap_lockPixels() failed ! error=%d", ret);

        return NULL;
    }
    LOGD("pixel locked");
    uint32_t* newBitmapPixels = (uint32_t*) bitmapPixels;
    memcpy(newBitmapPixels,(uint32_t*) _data, (_width * _height * sizeof(uint32_t)));
    LOGD("memcopy start");
    LOGD("memcopy end");

    AndroidBitmap_unlockPixels(env, newBitmap);
    //free(_data);

    return newBitmap;
}

void ImageProcessor::Release()
{
    if (_data != NULL) {
        LOGD("Release");
        delete [] _data;
    }
}

void ImageProcessor::DrawToSurface(JNIEnv * env, jobject surface)
{
    ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
    ANativeWindow_Buffer buffer;
    if (ANativeWindow_lock(window, &buffer, NULL) == 0) {
        memcpy(buffer.bits,_data, _width*_height* sizeof(int));
    }
    ANativeWindow_unlockAndPost(window);

    ANativeWindow_release(window);
    //delete [] native;
}

void ImageProcessor::DrawToBitmap(JNIEnv * env, jobject bitmap)
{
    void* pixels;
    int ret;
    if ((ret = AndroidBitmap_lockPixels(env, bitmap, &pixels)) < 0) {
        LOGD("AndroidBitmap_lockPixels() failed ! error=%d", ret);
    }
    memcpy(pixels,_data, _width*_height* sizeof(int));
    AndroidBitmap_unlockPixels(env, bitmap);
}

jobject ImageProcessor::GetData(JNIEnv * env)
{
    jintArray result;
    jint size = _width*_height;
    result = env->NewIntArray(size);
    env->SetIntArrayRegion(result, 0, size, _data);

    return result;
}

jobjectArray ImageProcessor::GetHistogramm(JNIEnv * env)
{
    jint* red = new int[256];
    jint* green = new int[256];
    jint* blue =new  int[256];
    for ( int i = 0 ; i < _width ; i ++) {
        for ( int j = 0 ; j < _height ; j ++) {
            int index = j * _width + i ;
            int b = (int) ((_data[index] >> 16) & 0xFF);
            int g = (int) ((_data[index] >> 8) & 0xFF);
            int r = (int) (_data[index] & 0xFF);
            red[r]++;
            green[g]++;
            blue[b]++;
        }
    }

    jclass intArray1DClass = env->FindClass("[I");
    jclass intArray2DClass = env->FindClass("[[I");

    jintArray  redar = env->NewIntArray(256);
    env->SetIntArrayRegion(redar, (jsize) 0, (jsize) 256, (jint*)red);

    jintArray  bluear = env->NewIntArray(256);
    env->SetIntArrayRegion(bluear, (jsize) 0, (jsize) 256, (jint*)blue);

    jintArray  greenar = env->NewIntArray(256);
    env->SetIntArrayRegion(greenar, (jsize) 0, (jsize) 256, (jint*)green);

    /*jobject rgbar = env->NewObjectArray(3, intArray2DClass, NULL);
    env->SetObjectArrayElement(rgbar, 0,(jintArray)redar);
    //env->SetObjectArrayElement(rgbar, 0, redar);
    env->SetObjectArrayElement(rgbar, 1, (jint*) greenar);
    env->SetObjectArrayElement(rgbar, 2, (jint*) bluear );*/

    jobjectArray array2D = env->NewObjectArray(
            3, intArray1DClass, NULL);
    env->SetObjectArrayElement(array2D, 0, redar);
    env->SetObjectArrayElement(array2D, 1, greenar);
    env->SetObjectArrayElement(array2D, 2, bluear);
    return array2D;
}



void ImageProcessor::applyLanczos() {
    int* newarray = new int[_width * _height];
    for (int y = 1; y < _height - 1; y++) {
        for (int x = 1; x < _width - 1; x++) {
            int c00 = GetPixel(x - 1, y - 1);
            int c01 = GetPixel(x - 1, y);
            int c02 = GetPixel(x - 1, y + 1);
            int c10 = GetPixel(x, y - 1);
            int c11 = GetPixel(x, y);
            int c12 = GetPixel(x, y + 1);
            int c20 = GetPixel(x + 1, y - 1);
            int c21 = GetPixel(x + 1, y);
            int c22 = GetPixel(x + 1, y + 1);
            int r = -GetPixelRedFromInt(c00) - GetPixelRedFromInt(c01) - GetPixelRedFromInt(c02) +
                    -GetPixelRedFromInt(c10) + 8 * GetPixelRedFromInt(c11) - GetPixelRedFromInt(c12) +
                    -GetPixelRedFromInt(c20) - GetPixelRedFromInt(c21) - GetPixelRedFromInt(c22);
            int g = -GetPixelGreenFromInt(c00) - GetPixelGreenFromInt(c01) - GetPixelGreenFromInt(c02) +
                    -GetPixelGreenFromInt(c10) + 8 * GetPixelGreenFromInt(c11) - GetPixelGreenFromInt(c12) +
                    -GetPixelGreenFromInt(c20) - GetPixelGreenFromInt(c21) - GetPixelGreenFromInt(c22);
            int b = -GetPixelBlueFromInt(c00) - GetPixelBlueFromInt(c01) - GetPixelBlueFromInt(c02) +
                    -GetPixelBlueFromInt(c10) + 8 * GetPixelBlueFromInt(c11) - GetPixelBlueFromInt(c12) +
                    -GetPixelBlueFromInt(c20) - GetPixelBlueFromInt(c21) - GetPixelBlueFromInt(c22);
            if (r < 0) r = 0; else if (r > 255) r = 255;
            if (g < 0) g = 0; else if (g > 255) g = 255;
            if (b < 0) b = 0; else if (b > 255) b = 255;
            WritePixel(x, y, GetPixelFromRGB(r, g, b), newarray);
        }
    }
    memcpy(_data,newarray, (_width * _height * sizeof(int)));
    delete [] newarray;
}

void ImageProcessor::applyFocusPeak()
{
    int factorForTrans = 50;
    int* newarray = new int[_width * _height * sizeof(int)];
    for (int y = 1; y < _height - 1; y++) {
        for (int x = 1; x < _width - 1; x++) {
            int r = -GetPixelRed(x - 1, y - 1) - GetPixelRed(    x - 1, y) - GetPixelRed(x - 1, y + 1) +
                    -GetPixelRed(x    , y - 1) + 8 * GetPixelRed(x    , y) - GetPixelRed(x    , y + 1) +
                    -GetPixelRed(x + 1, y - 1) - GetPixelRed(    x + 1, y) - GetPixelRed(x + 1, y + 1);
            if (r < 0) r = 0; else if (r > 255) r = 255;
            if(r < factorForTrans ) {
                WritePixel(x, y, GetPixelFromARGB(0, 0, 0, 0), newarray);
            }
            else {
                WritePixel(x, y, GetPixelFromRGB(0, 0 , 255), newarray);
                //LOGD("Wrote non black Pixel");
            }
        }
    }
    memcpy(_data,newarray, (_width * _height * sizeof(int)));
    delete [] newarray;
}

void ImageProcessor::Apply3x3Filter(int filter[3][3])
{
    LOGD("Apply 3x3 Filter");
    int* newarray = new int[_width * _height];
    double factor = 1.0;
    double bias = 0.0;
    int filterWidth = 3;
    int filterHeight = 3;
    //apply the filter
    for (int y = 1; y < _height - 1; y++) {
        for (int x = 1; x < _width - 1; x++) {
            int c00 = GetPixel(x - 1, y - 1);
            int c01 = GetPixel(x - 1, y);
            int c02 = GetPixel(x - 1, y + 1);
            int c10 = GetPixel(x, y - 1);
            int c11 = GetPixel(x, y);
            int c12 = GetPixel(x, y + 1);
            int c20 = GetPixel(x + 1, y - 1);
            int c21 = GetPixel(x + 1, y);
            int c22 = GetPixel(x + 1, y + 1);
            int r, g, b = 0;

            r =     -GetPixelRedFromInt(c00)*filter[0][0] - GetPixelRedFromInt(c01)*filter[0][1] + GetPixelRedFromInt(c02)*filter[0][2]+
                    -GetPixelRedFromInt(c10)*filter[1][0] + GetPixelRedFromInt(c11)*filter[1][1] - GetPixelRedFromInt(c12)*filter[1][2] +
                    -GetPixelRedFromInt(c20)*filter[2][0] - GetPixelRedFromInt(c21)*filter[2][1] - GetPixelRedFromInt(c22)*filter[2][2];

            g =     -GetPixelGreenFromInt(c00)*filter[0][0] - GetPixelGreenFromInt(c01)*filter[0][1]   - GetPixelGreenFromInt(c02)*filter[0][2] +
                    -GetPixelGreenFromInt(c10)*filter[1][0] + GetPixelGreenFromInt(c11)*filter[1][1]   - GetPixelGreenFromInt(c12)*filter[1][2] +
                    -GetPixelGreenFromInt(c20)*filter[2][0] - GetPixelGreenFromInt(c21)*filter[2][1]   - GetPixelGreenFromInt(c22)*filter[2][2];

            b =     -GetPixelBlueFromInt(c00)*filter[0][0]  - GetPixelBlueFromInt(c01)*filter[0][1] - GetPixelBlueFromInt(c02)*filter[0][2] +
                    -GetPixelBlueFromInt(c10)*filter[1][0]  + GetPixelBlueFromInt(c11)*filter[1][1] - GetPixelBlueFromInt(c12)*filter[1][2] +
                    -GetPixelBlueFromInt(c20)*filter[2][0]  - GetPixelBlueFromInt(c21)*filter[2][1] - GetPixelBlueFromInt(c22)*filter[2][2];
            //truncate values smaller than zero and larger than 255
            if (r < 0) r = 0; else if (r > 255) r = 255;
            if (g < 0) g = 0; else if (g > 255) g = 255;
            if (b < 0) b = 0; else if (b > 255) b = 255;
            //LOGD("R:%i G:%i B:%i",r,g,b);
            WritePixel(x, y, GetPixelFromRGB(r, g, b), newarray);
        }
    }
    _data = newarray;
    LOGD("Done 3x3 Filter");
}




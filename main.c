#include <stdio.h>
#include <stdlib.h>
#include <turbojpeg.h>
#include <math.h>

int readJpegFile();

int compressJpegFile();

void printScalingFactorList();

int scaleJpegFile();

int main(int argc, char **argv) {
//    readJpegFile();
//    compressJpegFile();
//    printScalingFactorList();
    scaleJpegFile();

    return 0;
}

/**
 * read jpeg info and print it
 * @return 0 if success, -1 if error
 */
int readJpegFile() {
    FILE *inFile = fopen("./test.jpg", "r");
    if (inFile == NULL) {
        perror("Error open jpeg file");
        return -1;
    }

    unsigned char fileHeader[2];
    if (fread(fileHeader, sizeof(fileHeader[0]), 2, inFile) != 2) {
        perror("Error read file header");
        goto endFile;
    }
    if (fileHeader[0] != 0xFF || fileHeader[1] != 0xD8) {
        perror("Error file header is not jpeg");
        goto endFile;
    }

    long jpegFileSize;
    if (fseek(inFile, 0, SEEK_END) != 0
        || (jpegFileSize = ftell(inFile)) < 1
        || fseek(inFile, 0, SEEK_SET) != 0) {
        perror("Error get file size");
        goto endFile;
    }

    printf("file size is %ld\n", jpegFileSize);

    unsigned char *buffer = malloc(jpegFileSize);
    if (buffer == NULL) {
        perror("Error in allocating buffer");
        goto endFile;
    }
    if (fread(buffer, sizeof(unsigned char), jpegFileSize, inFile) != jpegFileSize) {
        perror("Error in reading file");
        goto endBuffer;
    }

    int width;
    int height;
    int subsam;
    int colorspace = 0;
    tjhandle handle = tjInitDecompress();
    if (handle == NULL) {
        fprintf(stderr, "Error init decompress: %s\n", tjGetErrorStr2(handle));
        goto endBuffer;
    }
    if (tjDecompressHeader3(handle, buffer, jpegFileSize, &width, &height, &subsam, &colorspace) == -1) {
        fprintf(stderr, "Error decompress header: %s\n", tjGetErrorStr2(handle));
        goto endHandle;
    }
    printf("jpeg image width=%d, height=%d, subsmp=%d, colorspace=%d\n", width, height, subsam, colorspace);
    goto bailout;

    endHandle:
    if (handle != NULL) {
        tjDestroy(handle);
        handle = NULL;
        return -1;
    }

    endBuffer:
    if (buffer != NULL) {
        free(buffer);
        buffer = NULL;
        return -1;
    }

    endFile:
    if (inFile != NULL) {
        fclose(inFile);
        inFile = NULL;
        return -1;
    }

    bailout:
    tjDestroy(handle);
    handle = NULL;
    free(buffer);
    buffer = NULL;
    fclose(inFile);
    inFile = NULL;
    return 0;
}

int compressJpegFile() {
    FILE *inFile = fopen("test.jpg", "rb");
    if (inFile == NULL) {
        perror("Error open jpeg file");
        return -1;
    }

    unsigned char fileHeader[2];
    if (fread(fileHeader, sizeof(unsigned char), 2, inFile) != 2) {
        perror("Error read jpeg file");
        fclose(inFile);
        return -1;
    }
    if (fileHeader[0] != 0xFF || fileHeader[1] != 0xD8) {
        perror("Error file header is not jpeg");
        fclose(inFile);
        return -1;
    }
    long jpegFileSize;
    if (fseek(inFile, 0, SEEK_END) != 0
        || (jpegFileSize = ftell(inFile)) == -1
        || fseek(inFile, 0, SEEK_SET) != 0) {
        perror("Error get file size");
        fclose(inFile);
        return -1;
    }

    unsigned char *buffer = tjAlloc((int) jpegFileSize);
    if (buffer == NULL) {
        perror("Error allocate buffer");
        fclose(inFile);
        return -1;
    }
    if (fread(buffer, sizeof(unsigned char), jpegFileSize, inFile) != jpegFileSize) {
        perror("Error read file");
        fclose(inFile);
        tjFree(buffer);
        return -1;
    }
    fclose(inFile);
    inFile = NULL;

    tjhandle handle = tjInitDecompress();
    if (handle == NULL) {
        perror("Error init decompress");
        tjFree(buffer);
        return -1;
    }
    int width;
    int height;
    int subsam;
    int colorspace;
    if (tjDecompressHeader3(handle, buffer, jpegFileSize, &width, &height, &subsam, &colorspace) != 0) {
        fprintf(stderr, "Error decompress header: %s\n", tjGetErrorStr2(handle));
        tjDestroy(handle);
        tjFree(buffer);
        return -1;
    }
    printf("jpeg file: width=%d, height=%d, subsam=%d, colorspace=%d\n", width, height, subsam, colorspace);

    int pixelFormat = TJPF_BGRX;
    unsigned char *rgbBuffer = tjAlloc(width * height * tjPixelSize[pixelFormat]);
    if (rgbBuffer == NULL) {
        perror("Error allocate memory");
        tjDestroy(handle);
        tjFree(buffer);
        return -1;
    }
    if (tjDecompress2(handle, buffer, jpegFileSize, rgbBuffer, width, 0, height, pixelFormat, TJFLAG_FASTDCT) != 0) {
        fprintf(stderr, "Error decompress: %s\n", tjGetErrorStr2(handle));
        tjFree(rgbBuffer);
        tjDestroy(handle);
        tjFree(buffer);
        return -1;
    }
    tjDestroy(handle);
    handle = NULL;
    tjFree(buffer);
    buffer = NULL;

    printf("rgb buffer size=%d, component size is %d\n", width * height * tjPixelSize[pixelFormat],
           tjPixelSize[pixelFormat]);

    handle = tjInitCompress();
    if (handle == NULL) {
        perror("Error init compress");
        tjFree(rgbBuffer);
        return -1;
    }
    unsigned char *outBuffer = NULL;
    unsigned long outBufferSize = 0;
    if (tjCompress2(handle, rgbBuffer, width, 0, height, pixelFormat, &outBuffer, &outBufferSize, subsam, 40,
                    TJFLAG_ACCURATEDCT | TJFLAG_FASTDCT) != 0) {
        fprintf(stderr, "Error compress: %s\n", tjGetErrorStr2(handle));
        tjFree(rgbBuffer);
        tjDestroy(handle);
        return -1;
    }
    tjFree(rgbBuffer);
    rgbBuffer = NULL;
    tjDestroy(handle);
    handle = NULL;

    printf("out jpeg file: size=%ld\n", outBufferSize);
    FILE *outFile = fopen("out.jpg", "w");
    if (outFile == NULL) {
        perror("Error open out file");
        tjFree(outBuffer);
        return -1;
    }
    if (fwrite(outBuffer, sizeof(unsigned char), outBufferSize, outFile) != outBufferSize) {
        perror("Error write out file");
        fclose(outFile);
        tjFree(outBuffer);
        return -1;
    }
    puts("jpeg file compress success");
    fclose(outFile);
    tjFree(outBuffer);

    return 0;
}

void printScalingFactorList() {
    int count = 0;
    tjscalingfactor *p = tjGetScalingFactors(&count);
    printf("scaling factor list size=%d\n", count);
    for (int i = 0; i < count; i++) {
        printf("scaling factor %d: num=%d, denom=%d\n", i, (*p).num, (*p).denom);
        p++;
    }
}

/**
 * find recent scaling factor
 */
int recentScalingFactor(tjscalingfactor *scalingFactor, float bestFactor) {
    if (scalingFactor == NULL) {
        return -1;
    }
    int count = 0;
    tjscalingfactor *p = tjGetScalingFactors(&count);
    if (p == NULL) return -1;

    scalingFactor->num = p->num;
    scalingFactor->denom = p->denom;
    for (int i = 1; i < count; i++) {
        p++;

        float oldFactor = (float) scalingFactor->num / scalingFactor->denom;
        float nowFactor = (float) p->num / p->denom;
        if (fabsf(oldFactor - bestFactor) > fabsf(nowFactor - bestFactor)) {
            scalingFactor->num = p->num;
            scalingFactor->denom = p->denom;
        } else if (fabsf(oldFactor - bestFactor) == fabsf(nowFactor - bestFactor)
                   && oldFactor > nowFactor) {
            scalingFactor->num = p->num;
            scalingFactor->denom = p->denom;
        }
    }
    return 0;
}

int scaleJpegFile() {
    FILE *inFile = fopen("./test.jpg", "r");
    if (!inFile) {
        perror("Error open file");
        return -1;
    }

    unsigned char fileHeader[2];
    if (fread(fileHeader, sizeof(fileHeader[0]), 2, inFile) != 2) {
        perror("Error read file");
        fclose(inFile);
        return -1;
    }
    if (fileHeader[0] != 0xFF || fileHeader[1] != 0xD8) {
        perror("This file is not a jpeg file");
        fclose(inFile);
        return -1;
    }

    long jpegFileSize;
    if (fseek(inFile, 0, SEEK_END) != 0
        || (jpegFileSize = ftell(inFile)) == -1
        || fseek(inFile, 0, SEEK_SET) != 0) {
        perror("Error read file size");
        fclose(inFile);
        return -1;
    }

    unsigned char *jpegBuffer = tjAlloc((int) jpegFileSize);
    if (!jpegBuffer) {
        perror("Error allocate memory for jpeg buffer");
        fclose(inFile);
        return -1;
    }
    if (fread(jpegBuffer, sizeof(unsigned char), jpegFileSize, inFile) != jpegFileSize) {
        perror("Error read file");
        tjFree(jpegBuffer);
        fclose(inFile);
        return -1;
    }
    fclose(inFile);
    inFile = NULL;

    tjhandle handle = tjInitDecompress();
    if (!handle) {
        perror("Error init decompress");
        tjFree(jpegBuffer);
        return -1;
    }

    int width;
    int height;
    int subsamp;
    int colorspace;
    if (tjDecompressHeader3(handle, jpegBuffer, jpegFileSize, &width, &height, &subsamp, &colorspace)) {
        fprintf(stderr, "Error decode jpeg file header: %s\n", tjGetErrorStr2(handle));
        tjDestroy(handle);
        tjFree(jpegBuffer);
        return -1;
    }

    printf("jpeg info: width=%d, height=%d, subsamp=%d, colorspace=%d\n", width, height, subsamp, colorspace);

    tjscalingfactor scalingFactor;
    if (recentScalingFactor(&scalingFactor, 0.4F) == -1) {
        tjDestroy(handle);
        tjFree(jpegBuffer);
        perror("Error get scaling factor");
        return -1;
    }
    int outWidth = TJSCALED(width, scalingFactor);
    int outHeight = TJSCALED(height, scalingFactor);
    int outSubsamp = subsamp;

    int outPixelFormat = TJPF_BGRX;
    int rgbBufferSize = outWidth * outHeight * tjPixelSize[outPixelFormat];

    unsigned char *rgbBuffer = tjAlloc(rgbBufferSize);
    if (rgbBuffer == NULL) {
        perror("Error allocate memory for rgb buffer");
        tjDestroy(handle);
        tjFree(jpegBuffer);
        return -1;
    }

    // decompress with specified width and height
    if (tjDecompress2(handle, jpegBuffer, jpegFileSize, rgbBuffer, outWidth, 0, outHeight, outPixelFormat, 0) == -1) {
        fprintf(stderr, "Error decode jpeg file: %s\n", tjGetErrorStr2(handle));
        tjFree(rgbBuffer);
        tjDestroy(handle);
        tjFree(jpegBuffer);
        return -1;
    }
    tjDestroy(handle);
    handle = NULL;
    tjFree(jpegBuffer);
    jpegBuffer = NULL;

    handle = tjInitCompress();
    if (!handle) {
        fprintf(stderr, "Error init compress: %s\n", tjGetErrorStr2(handle));
        tjFree(rgbBuffer);
        return -1;
    }

    unsigned char *outJpegBuffer = NULL;
    unsigned long outJpegFileSize = 0;
    if (tjCompress2(handle, rgbBuffer, outWidth, 0, outHeight, outPixelFormat, &outJpegBuffer, &outJpegFileSize,
                     outSubsamp, 80, 0)) {
        fprintf(stderr, "Error compress: %s\n", tjGetErrorStr2(handle));
        tjDestroy(handle);
        tjFree(rgbBuffer);
        return -1;
    }

    printf("out file info: width=%d, height=%d, subsamp=%d, size=%ld, scale factor=%d/%d\n", outWidth, outHeight,
           outSubsamp, outJpegFileSize, scalingFactor.num, scalingFactor.denom);

    tjDestroy(handle);
    handle = NULL;
    tjFree(rgbBuffer);
    rgbBuffer = NULL;

    // write file
    FILE *outFile = fopen("./out.jpg", "w");
    if (!outFile) {
        perror("Error open out file");
        tjFree(outJpegBuffer);
        return -1;
    }

    if (fwrite(outJpegBuffer, 1, outJpegFileSize, outFile) != outJpegFileSize) {
        perror("Error write out file");
        tjFree(outJpegBuffer);
        fclose(outFile);
        return -1;
    }

    tjFree(outJpegBuffer);
    fclose(outFile);
    puts("scale file success!");

    return 0;
}


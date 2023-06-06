#include <stdio.h>
#include <stdlib.h>
#include <turbojpeg.h>

int readJpegFile();

int compressJpegFile();

int main(int argc, char **argv) {
//    readJpegFile();
    compressJpegFile();

    return 0;
}

/**
 * read jpeg info and print it
 * @return 0 if success, -1 if error
 */
int readJpegFile() {
    FILE *inFile = fopen("./test.jpg", "r");
    if (inFile == NULL) {
        puts("Error open jpeg file");
        return -1;
    }

    unsigned char fileHeader[2];
    if (fread(fileHeader, sizeof(fileHeader[0]), 2, inFile) != 2) {
        puts("Error read file header");
        goto endFile;
    }
    if (fileHeader[0] != 0xFF || fileHeader[1] != 0xD8) {
        puts("Error file header is not jpeg");
        goto endFile;
    }

    long jpegFileSize;
    if (fseek(inFile, 0, SEEK_END) != 0
        || (jpegFileSize = ftell(inFile)) < 1
        || fseek(inFile, 0, SEEK_SET) != 0) {
        puts("Error get file size");
        goto endFile;
    }

    printf("file size is %ld\n", jpegFileSize);

    unsigned char *buffer = malloc(jpegFileSize);
    if (buffer == NULL) {
        puts("Error in allocating buffer");
        goto endFile;
    }
    if (fread(buffer, sizeof(unsigned char), jpegFileSize, inFile) != jpegFileSize) {
        puts("Error in reading file");
        goto endBuffer;
    }

    int width;
    int height;
    int subsam;
    int colorspace = 0;
    tjhandle handle = tjInitDecompress();
    if (handle == NULL) {
        printf("Error init decompress: %s\n", tjGetErrorStr2(handle));
        goto endHandle;
    }
    if (tjDecompressHeader3(handle, buffer, jpegFileSize, &width, &height, &subsam, &colorspace) == -1) {
        printf("Error decompress header: %s\n", tjGetErrorStr2(handle));
        goto endHandle;
    }
    printf("jpeg image width=%d, height=%d, subsmp=%d, colorspace=%d\n", width, height, subsam, colorspace);

    tjDestroy(handle);
    handle = NULL;
    free(buffer);
    buffer = NULL;
    fclose(inFile);
    inFile = NULL;
    return 0;

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
        tjFree(buffer);
        return -1;
    }
    printf("jpeg file: width=%d, height=%d, subsam=%d, colorspace=%d\n", width, height, subsam, colorspace);

    int pixelFormat = TJPF_BGRX;
    unsigned char *rgbBuffer = tjAlloc(width * height * tjPixelSize[pixelFormat]);
    if (rgbBuffer == NULL) {
        tjFree(buffer);
        return -1;
    }
    if (tjDecompress2(handle, buffer, jpegFileSize, rgbBuffer, width, 0, height, pixelFormat, TJFLAG_FASTDCT) != 0) {
        fprintf(stderr, "Error decompress: %s\n", tjGetErrorStr2(handle));
        tjFree(buffer);
        tjFree(rgbBuffer);
        return -1;
    }
    tjDestroy(handle);

    printf("rgb buffer size=%d, component size is %d\n", width * height * tjPixelSize[pixelFormat],
           tjPixelSize[pixelFormat]);

    handle = tjInitCompress();
    if (handle == NULL) {
        perror("Error init compress");
        tjFree(buffer);
        return -1;
    }
    unsigned char *outBuffer = NULL;
    unsigned long outBufferSize = 0;
    if (tjCompress2(handle, rgbBuffer, width, 0, height, pixelFormat, &outBuffer, &outBufferSize, subsam, 40,
                    TJFLAG_ACCURATEDCT | TJFLAG_FASTDCT) != 0) {
        fprintf(stderr, "Error compress: %s\n", tjGetErrorStr2(handle));
        tjFree(buffer);
        tjDestroy(handle);
        return -1;
    }
    tjDestroy(handle);

    printf("out jpeg file: size=%ld\n", outBufferSize);
    FILE *outFile = fopen("out.jpg", "w");
    if (outFile == NULL) {
        perror("Error open out file");
        tjFree(outBuffer);
        free(buffer);
        return -1;
    }
    if (fwrite(outBuffer, sizeof(unsigned char), outBufferSize, outFile) != outBufferSize) {
        perror("Error write out file");
        tjFree(outBuffer);
        tjFree(buffer);
        fclose(outFile);
        return -1;
    }
    puts("jpeg file compress success");
    fclose(outFile);

    return 0;
}
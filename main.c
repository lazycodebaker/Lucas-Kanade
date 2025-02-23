#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

typedef struct
{
    float u; // x-direction velocity
    float v; // y-direction velocity
} FlowVector;

// Function to read JPG and convert to grayscale
float *readJPG(const char *filename, int *width, int *height)
{
    int channels;
    unsigned char *data = stbi_load(filename, width, height, &channels, 0);
    if (!data)
    {
        printf("Error: Could not load %s - %s\n", filename, stbi_failure_reason());
        return NULL;
    }

    float *grayData = (float *)malloc(*width * *height * sizeof(float));

    // Convert to grayscale (assuming 1 or 3 channels)
    for (int i = 0; i < *width * *height; i++)
    {
        if (channels == 1)
        {
            grayData[i] = (float)data[i];
        }
        else if (channels == 3)
        {
            int idx = i * 3;
            grayData[i] = 0.299 * data[idx] + 0.587 * data[idx + 1] + 0.114 * data[idx + 2];
        }
        else
        {
            printf("Unsupported channel count: %d\n", channels);
            free(data);
            free(grayData);
            return NULL;
        }
    }

    stbi_image_free(data);
    return grayData;
}

// Load original image for visualization (RGB)
unsigned char *loadRGB(const char *filename, int *width, int *height)
{
    int channels;
    unsigned char *data = stbi_load(filename, width, height, &channels, 3); // Force 3 channels (RGB)
    if (!data)
    {
        printf("Error: Could not load %s for visualization\n", filename);
        return NULL;
    }
    return data;
}

// Compute spatial gradients
void computeGradients(float *image, float *gradX, float *gradY, int width, int height)
{
    for (int y = 1; y < height - 1; y++)
    {
        for (int x = 1; x < width - 1; x++)
        {
            int idx = y * width + x;
            gradX[idx] = (image[idx + 1] - image[idx - 1]) / 2.0;
            gradY[idx] = (image[idx + width] - image[idx - width]) / 2.0;
        }
    }
}

// Lucas-Kanade for a single window
FlowVector lucasKanadeWindow(float *I1, float *I2, float *gradX, float *gradY,
                             int x, int y, int windowSize, int width, int height)
{
    FlowVector flow = {0.0, 0.0};

    int halfWindow = windowSize / 2;
    if (x - halfWindow < 0 || x + halfWindow >= width ||
        y - halfWindow < 0 || y + halfWindow >= height)
    {
        return flow;
    }

    float Axx = 0.0, Axy = 0.0, Ayy = 0.0;
    float bx = 0.0, by = 0.0;

    for (int dy = -halfWindow; dy <= halfWindow; dy++)
    {
        for (int dx = -halfWindow; dx <= halfWindow; dx++)
        {
            int idx = (y + dy) * width + (x + dx);

            float Ix = gradX[idx];
            float Iy = gradY[idx];
            float It = I2[idx] - I1[idx];

            Axx += Ix * Ix;
            Axy += Ix * Iy;
            Ayy += Iy * Iy;
            bx += Ix * It;
            by += Iy * It;
        }
    }

    float det = Axx * Ayy - Axy * Axy;
    if (fabs(det) < 1e-6)
    {
        return flow;
    }

    flow.u = (Ayy * (-bx) - Axy * (-by)) / det;
    flow.v = (Axx * (-by) - Axy * (-bx)) / det;

    return flow;
}

// Main Lucas-Kanade function
void lucasKanade(float *image1, float *image2, FlowVector *flowField,
                 int width, int height, int windowSize)
{
    float *gradX = (float *)malloc(width * height * sizeof(float));
    float *gradY = (float *)malloc(width * height * sizeof(float));

    computeGradients(image1, gradX, gradY, width, height);

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            flowField[y * width + x] = lucasKanadeWindow(
                image1, image2, gradX, gradY, x, y, windowSize, width, height);
        }
    }

    free(gradX);
    free(gradY);
}

// Simple line drawing function (Bresenham's algorithm)
void drawLine(unsigned char *image, int width, int height, int x0, int y0, int x1, int y1)
{
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    while (1)
    {
        if (x0 >= 0 && x0 < width && y0 >= 0 && y0 < height)
        {
            int idx = (y0 * width + x0) * 3;
            image[idx] = 0;       // R
            image[idx + 1] = 255; // G (green arrows)
            image[idx + 2] = 0;   // B
        }
        if (x0 == x1 && y0 == y1)
            break;
        e2 = 2 * err;
        if (e2 >= dy)
        {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}

// Visualize flow field and save
void saveFlowImage(const char *filename, unsigned char *baseImage, FlowVector *flowField, int width, int height)
{
    unsigned char *visImage = (unsigned char *)malloc(width * height * 3);
    memcpy(visImage, baseImage, width * height * 3); // Copy base image

    // Draw flow arrows every 10 pixels
    for (int y = 0; y < height; y += 10)
    {
        for (int x = 0; x < width; x += 10)
        {
            int idx = y * width + x;
            float u = flowField[idx].u;
            float v = flowField[idx].v;
            int x1 = (int)(x + u);
            int y1 = (int)(y + v);
            drawLine(visImage, width, height, x, y, x1, y1);
        }
    }

    stbi_write_jpg(filename, width, height, 3, visImage, 90); // 90 is quality
    free(visImage);
}

int main()
{
    const int MAX_FRAMES = 100;
    int width, height, prevWidth = 0, prevHeight = 0;
    float *prevImage = NULL, *currImage = NULL;
    FlowVector *flowField = NULL;
    unsigned char *baseImage = NULL;

    int frameCount = 0;
    char inputFilename[32], outputFilename[32];

    // Process frames in a loop
    for (int i = 1; i <= MAX_FRAMES; i++)
    {
        snprintf(inputFilename, sizeof(inputFilename), "frames_input/frame_%04d.jpg", i);

        // Load current frame
        currImage = readJPG(inputFilename, &width, &height);
        if (!currImage)
        {
            printf("No more frames or error at frame %d\n", i);
            break;
        }

        frameCount++;

        // Check size consistency
        if (prevWidth != 0 && (prevWidth != width || prevHeight != height))
        {
            printf("Error: Frame size mismatch at frame %d\n", i);
            free(currImage);
            break;
        }

        prevWidth = width;
        prevHeight = height;

        // Load RGB version for visualization
        baseImage = loadRGB(inputFilename, &width, &height);
        if (!baseImage)
        {
            free(currImage);
            break;
        }

        if (prevImage)
        {
            // Allocate flow field
            flowField = (FlowVector *)malloc(width * height * sizeof(FlowVector));

            // Run Lucas-Kanade between previous and current frame
            lucasKanade(prevImage, currImage, flowField, width, height, 5);

            // Save flow visualization
            snprintf(outputFilename, sizeof(outputFilename), "frames_output/flow_%04d.jpg", i - 1);
            saveFlowImage(outputFilename, baseImage, flowField, width, height);

            free(flowField);
        }

        // Clean up previous frame and set current as previous
        if (prevImage)
            free(prevImage);
        prevImage = currImage;
        stbi_image_free(baseImage);
    }

    // Clean up last frame
    if (prevImage)
        free(prevImage);

    printf("Processed %d frames\n", frameCount);

    return 0;
}

// ffmpeg -i input.mp4 {input_folder_name}frame_%04d.jpg
// EXAMPLE: ffmpeg -i input.mp4 frames_input/frame_%04d.jpg

// ffmpeg -framerate 30 -i frames_output/flow_%04d.jpg -c:v libx264 -pix_fmt yuv420p output.mp4

// ffprobe -v error -count_frames -select_streams v:0 -show_entries stream=nb_read_frames,r_frame_rate,duration -of default=nokey=1:noprint_wrappers=1 track.mp4
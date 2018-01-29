#include <cstdio>
#include <cstring>
#include <cstdlib>
#define cimg_display 0
#include "CImg.h"
#include <ctime>
#include "utils.h"
#include "bilateral_filter_gpu.h"
#include "bilateral_filter_cpu.h"
#include "cstdio"
#include <sys/time.h>
#include <iostream>

int main(int argc, char **argv) {

    if (argc < 4) {
        printf("Usage: ./bilateral <image file> <spatial standard deviation> <color standard deviation>\n");
        return 1;
    }

    //Load the image
    cimg_library::CImg<unsigned char> image(argv[1]);
    int N = image.width() * image.height();
    float pixel_depth = 255.0;

    int sdims[2]{image.height(), image.width()};

    //get a flat float array
    auto flat_cpu = get_flat_float_from_image(image, pixel_depth);
    //get a copy
    auto flat_gpu = new float[N*3]{0};
    for(int i = 0; i < N * 3; i++)
        flat_gpu[i] = flat_cpu[i];

    // compute the bilateral kernel

    float theta_alpha = atof(argv[2]);
    float theta_beta = atof(argv[3]);

    float invSpatialStdev = 1.0f /theta_alpha;
    float invColorStdev = 1.0f /theta_beta;


    // Construct the position vectors out of x, y, r, g, and b.
    auto positions = compute_kernel(image, invSpatialStdev, invColorStdev);

    auto positions_cpu = compute_bilateral_kernel(flat_cpu, N, 3, 2, sdims, theta_alpha, theta_beta);

    double diff=0;
    for(int i =0; i< 5*N; i++){
        diff += positions_cpu[i] - positions[i];
    }
    std::cout.precision(17);
    std::cout << "diff: " << diff +0.1 << std::endl;

    //GPU
    {
        printf("Calling filter GPU...\n");
        std::clock_t begin = std::clock();
        bilateral_filter_gpu(flat_gpu, positions_cpu, 5, 3, N);
        std::clock_t end = std::clock();
        double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
        printf("Measured from function call: %f seconds\n", elapsed_secs);
    }


    //CPU
    {
        printf("Calling filter...\n");
        std::clock_t begin = std::clock();
        bilateral_filter_cpu(flat_cpu, positions_cpu, 5, 3, N);
        std::clock_t end = std::clock();
        double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
        printf("Measured from function call: %f seconds\n", elapsed_secs);
    }
    printf("new here\n");
    int tol{0};
    int wrong_pixels{0};
    for(int i=0; i<N*3; i++){
        auto cpu_value = (int) (flat_cpu[i] * 255);
        auto gpu_value = (int) (flat_gpu[i] * 255);
        if(cpu_value != gpu_value)
            wrong_pixels += 1;
    }
    if(wrong_pixels==0)
        printf("The algorithm produced the correct result\n");
    else
        printf("The result is not correct, it is %f percent different (%d values)\n", (100.0*wrong_pixels/(3.0*N)), wrong_pixels);

    delete[] flat_cpu;
    delete[] flat_gpu;
    delete[] positions;
    delete[] positions_cpu;

    return 0;
}

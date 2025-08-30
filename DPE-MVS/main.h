#ifndef _MAIN_H_
#define _MAIN_H_

// Includes Opencv
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>
// Includes CUDA
#include <cuda_runtime.h>
#include <cuda.h>
#include <cuda_runtime_api.h>
#include <cuda_texture_types.h>
#include <curand_kernel.h>
#include <vector_types.h>
// Includes STD libs
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <map>
#include <memory>
#include <chrono>
#include <iomanip>
#include <unordered_set>
#include <cstdarg>
#include <random>
#include <unordered_map>
#include <filesystem>

// Define some const var
#define OUT_NAME "DPE"
#define MAX_IMAGES 32
#define NEIGHBOUR_NUM 9
#define SURF_COEFF_NUM 10
#define MAX_SEARCH_RADIUS 4096
#define DEBUG_POINT_X 753
#define DEBUG_POINT_Y 259
//#define DEBUG_COST_LINE
//#define DEBUG_NEIGHBOUR

namespace fs = std::filesystem;

struct Camera {
	float K[9];
	float R[9];
	float t[3];
	float c[3];
	int height;
	int width;
	float depth_min;
	float depth_max;
};

struct PointList {
	float3 coord;
	float3 color;
};

enum RunState {
	FIRST_INIT,
	REFINE_INIT,
	REFINE_ITER
};

enum PixelState {
	WEAK,
	STRONG,
	UNKNOWN
};

struct PatchMatchParams {
	int max_iterations = 3;
	int num_images = 5;
	float sigma_spatial = 5.0f;
	float sigma_color = 3.0f;
	int top_k = 4;	// ETH3D: 4 TAT: 8
	float depth_min = 0.0f;
	float depth_max = 1.0f;
	bool geom_consistency = false;
	int strong_radius = 5;
	int strong_increment = 2;
	int weak_radius = 5;
	int weak_increment = 5;
	bool use_APD = true;
	//=====================================================
	bool use_edge = true;		// 强像素的感受野扩展（采样）
	bool use_limit = true;  	// 弱像素三角构成的边缘限制
	bool use_label = true;		// 弱像素的感受野扩展（采样）
	bool use_radius = true;		// 自适应调整弱像素匹配代价计算策略
	bool high_res_img = true;  // ETH：true TAT：false（在ETH3D这种大分辨率上效果好，说明尽量不要用小分辨率）
	int max_scale_size = 1;
	int scale_size = 1;
	//=====================================================
	int weak_peak_radius = 2;
	int rotate_time = 4;
	float ransac_threshold = 0.005;
	float geom_factor = 0.2f;
	RunState state;
};

struct Problem {
	int index;
	int ref_image_id;
	std::vector<int> src_image_ids;
	fs::path dense_folder;
	fs::path result_folder;
	int scale_size = 1;
	PatchMatchParams params;
	bool show_medium_result = true;
	int iteration;
};

#endif // !_MAIN_H_

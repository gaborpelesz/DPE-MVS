#include "main.h"
#include "DPE.h"
#include <filesystem>
#include <fstream>
#include <sstream>


void GenerateSampleList(const fs::path &dense_folder, std::vector<Problem> &problems)
{
	fs::path cluster_list_path = dense_folder / fs::path("pair.txt");
	problems.clear();
	std::ifstream file(cluster_list_path);
	std::stringstream iss;
	std::string line;

	int num_images;
	iss.clear();
	std::getline(file, line);
	iss.str(line);
	iss >> num_images;

	for (int i = 0; i < num_images; ++i) {
		Problem problem;
		problem.index = i;
		problem.src_image_ids.clear();
		iss.clear();
		std::getline(file, line);
		iss.str(line);
		iss >> problem.ref_image_id;

		problem.dense_folder = dense_folder;
		problem.result_folder = dense_folder / fs::path(OUT_NAME) / fs::path(ToFormatIndex(problem.ref_image_id));
		create_directory(problem.result_folder);

		int num_src_images;
		iss.clear();
		std::getline(file, line);
		iss.str(line);
		iss >> num_src_images;
		for (int j = 0; j < num_src_images; ++j) {
			int id;
			float score;
			iss >> id >> score;
			if (score <= 0.0f) {
				continue;
			}
			problem.src_image_ids.push_back(id);
		}
		problems.push_back(problem);
	}
}

bool CheckImages(const std::vector<Problem> &problems) {
	if (problems.size() == 0) {
		return false;
	}
	fs::path image_path = problems[0].dense_folder / fs::path("images") / fs::path(ToFormatIndex(problems[0].ref_image_id) + ".jpg");
	cv::Mat image = cv::imread(image_path.string());
	if (image.empty()) {
		return false;
	}
	const int width = image.cols;
	const int height = image.rows;
	for (size_t i = 1; i < problems.size(); ++i) {
		image_path = problems[i].dense_folder / fs::path("images") / fs::path(ToFormatIndex(problems[i].ref_image_id) + ".jpg");
		image = cv::imread(image_path.string());
		if (image.cols != width || image.rows != height) {
			return false;
		}
	}
	return true;
}

void GetProblemEdges(const Problem &problem) {
	std::cout << "Getting image edges: " << std::setw(8) << std::setfill('0') << problem.ref_image_id << "..." << std::endl;
	int scale = 0;
	while((1 << scale) < problem.scale_size) scale++;

	fs::path image_folder = problem.dense_folder / fs::path("images");
	fs::path image_path = image_folder / fs::path(ToFormatIndex(problem.ref_image_id) + ".jpg");
	cv::Mat image_uint = cv::imread(image_path.string(), cv::IMREAD_GRAYSCALE);
	cv::Mat src_img;
	image_uint.convertTo(src_img, CV_32FC1);
	const float factor = 1.0f / (float)(problem.scale_size);
	const int new_cols = std::round(src_img.cols * factor);
	const int new_rows = std::round(src_img.rows * factor);
	cv::Mat scaled_image_float;
	cv::resize(src_img, scaled_image_float, cv::Size(new_cols, new_rows), 0, 0, cv::INTER_LINEAR);
	scaled_image_float.convertTo(src_img, CV_8UC1);
	std::cout << "size: " << new_cols << "x" << new_rows << "\n";

	if (problem.params.use_edge) {
		// path edge_path = problem.result_folder / path("edges.dmb");
		fs::path edge_path = problem.result_folder / fs::path("edges_" + std::to_string(scale) + ".dmb");
		std::ifstream edge_file(edge_path.string());
		bool edge_exists = edge_file.good();
		edge_file.close();
		if (!edge_exists) {
			std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
			cv::Mat edge = EdgeSegment(scale, src_img, 0, true, problem.params.high_res_img);
			std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
			std::cout << "Fine edge cost time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms" << std::endl;
			WriteBinMat(edge_path, edge);
			if (problem.show_medium_result) {
				fs::path ref_image_edge_path = problem.result_folder / fs::path("rawedge_" + std::to_string(scale) + ".jpg");
				cv::imwrite(ref_image_edge_path.string(), edge);
			}
		}
	}

	if (problem.params.use_label) {
		// path label_path = problem.result_folder / path("labels.dmb");
		fs::path label_path = problem.result_folder / fs::path("labels_" + std::to_string(scale) + ".dmb");
		std::ifstream label_file(label_path.string());
		bool label_exists = label_file.good();
		label_file.close();
		if (!label_exists) {
			std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
			cv::Mat label = EdgeSegment(scale, image_uint, 1, false, problem.params.high_res_img);
			std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
			std::cout << "Coarse edge cost time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms" << std::endl;
			WriteBinMat(label_path, label);
			if (problem.show_medium_result) {
				fs::path ref_image_con_path = problem.result_folder / fs::path("connect_" + std::to_string(scale) + ".jpg");
				cv::imwrite(ref_image_con_path.string(), EdgeSegment(scale, image_uint, -1, false, problem.params.high_res_img));
			}
		}
	}

	std::cout << "Getting image edges: " << std::setw(8) << std::setfill('0') << problem.ref_image_id << " done!" << std::endl;
}

int ComputeRoundNum(const std::vector<Problem> &problems) {
	if (problems.size() == 0) {
		return 0;
	}
	fs::path image_path = problems[0].dense_folder / fs::path("images") / fs::path(ToFormatIndex(problems[0].ref_image_id) + ".jpg");
	cv::Mat image = cv::imread(image_path.string());
	if (image.empty()) {
		return 0;
	}
	int max_size = MAX(image.cols, image.rows);
	int round_num = 1;
	while (max_size > 800) {
		max_size /= 2;
		round_num++;
	}
	return round_num;
}


void ProcessProblem(const Problem &problem) {
	std::cout << "Processing image: " << std::setw(8) << std::setfill('0') << problem.ref_image_id << "..." << std::endl;
    std::cout << "iteration: " << problem.iteration << std::endl;
	std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

	DPE DPE(problem);
	DPE.InuputInitialization();
	DPE.SupportInitialization();
	DPE.CudaSpaceInitialization();
	DPE.SetDataPassHelperInCuda();
	DPE.RunPatchMatch();

	int width = DPE.GetWidth(), height = DPE.GetHeight();
	cv::Mat depth = cv::Mat(height, width, CV_32FC1);
	cv::Mat normal = cv::Mat(height, width, CV_32FC3);
	cv::Mat pixel_states = DPE.GetPixelStates();
	for (int r = 0; r < height; ++r) {
		for (int c = 0; c < width; ++c) {
			float4 plane_hypothesis = DPE.GetPlaneHypothesis(r, c);
			depth.at<float>(r, c) = plane_hypothesis.w;
			if (depth.at<float>(r, c) < DPE.GetDepthMin() || depth.at<float>(r, c) > DPE.GetDepthMax()) {
				depth.at<float>(r, c) = 0;
				pixel_states.at<uchar>(r, c) = UNKNOWN;
			}
			normal.at<cv::Vec3f>(r, c) = cv::Vec3f(plane_hypothesis.x, plane_hypothesis.y, plane_hypothesis.z);
		}
	}
	
	fs::path depth_path = problem.result_folder / fs::path("depths.dmb");
	WriteBinMat(depth_path, depth);
	fs::path normal_path = problem.result_folder / fs::path("normals.dmb");
	WriteBinMat(normal_path, normal);
	fs::path weak_path = problem.result_folder / fs::path("weak.bin");
	WriteBinMat(weak_path, pixel_states);
	fs::path selected_view_path = problem.result_folder / fs::path("selected_views.bin");
	WriteBinMat(selected_view_path, DPE.GetSelectedViews());

	if (problem.show_medium_result) {
		fs::path depth_img_path = problem.result_folder / fs::path("depth_" + std::to_string(problem.iteration) + ".jpg");
		fs::path normal_img_path = problem.result_folder / fs::path("normal_" + std::to_string(problem.iteration) + ".jpg");
		fs::path weak_img_path = problem.result_folder / fs::path("weak_" + std::to_string(problem.iteration) + ".jpg");
		ShowDepthMap(depth_img_path, depth, DPE.GetDepthMin(), DPE.GetDepthMax());
		ShowNormalMap(normal_img_path, normal);
		ShowWeakImage(weak_img_path, pixel_states);

		if ((problem.iteration + 1) % 4 == 0) {
			fs::path image_folder = problem.dense_folder / fs::path("images");
			fs::path cam_folder = problem.dense_folder / fs::path("cams");
			fs::path image_path = image_folder / fs::path(ToFormatIndex(problem.ref_image_id) + ".jpg");
			fs::path cam_path = cam_folder / fs::path(ToFormatIndex(problem.ref_image_id) + "_cam.txt");
			fs::path point_cloud_path = problem.result_folder / fs::path("point_" + std::to_string(problem.iteration) + ".ply");
			// path point_cloud_path = problem.result_folder / path("point_test_" + std::to_string(problem.iteration) + ".ply");

			// for (int r = 0; r < height; ++r) for (int c = 0; c < width; ++c) if (pixel_states.at<uchar>(r, c) != STRONG) depth.at<float>(r, c) = 0;
			// ExportDepthImagePointCloud(point_cloud_path, image_path, cam_path, depth, DPE.GetDepthMin(), DPE.GetDepthMax());
			// remove(point_cloud_path);
		}
	}
	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
	std::cout << "Processing image: " << std::setw(8) << std::setfill('0') << problem.ref_image_id << " done!" << std::endl;
	std::cout << "Cost time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms" << std::endl;
}

int main(int argc, char **argv) {
	if (argc < 2) {
		std::cerr << "USAGE: DPE dense_folder\n";
		return EXIT_FAILURE;
	}
	fs::path dense_folder(argv[1]);
	fs::path output_folder = dense_folder / fs::path(OUT_NAME);
	create_directory(output_folder);
	// set cuda device for multi-gpu machine
	int gpu_index = 0;
	if (argc == 3) {
		gpu_index = std::atoi(argv[2]);
	}
	cudaSetDevice(gpu_index);
	// generate problems
	std::vector<Problem> problems;
	GenerateSampleList(dense_folder, problems);
	if (!CheckImages(problems)) {
		std::cerr << "Images may error, check it!\n";
		return EXIT_FAILURE;
	}
	int num_images = problems.size();
	std::cout << "There are " << num_images << " problems needed to be processed!" << std::endl;

	int round_num = ComputeRoundNum(problems);
	for (auto &problem : problems) {
		problem.params.max_scale_size = 1;
		for (int i = 0; i < round_num; ++i) {
			problem.scale_size = static_cast<int>(std::pow(2, round_num - 1 - i)); // scale 
			GetProblemEdges(problem); // 注意要先得到 scale_size
			problem.params.max_scale_size = MAX(problem.scale_size, problem.params.max_scale_size);
		}
	}

	std::cout << "Round nums: " << round_num << std::endl;
	int iteration_index = 0;
	for (int i = 0; i < round_num; ++i) {
		for (auto &problem : problems) {
			problem.iteration = iteration_index;
			problem.scale_size = static_cast<int>(std::pow(2, round_num - 1 - i)); // scale 
			problem.params.scale_size = problem.scale_size;
			{
				auto &params = problem.params;
				if (i == 0) {
					params.state = FIRST_INIT;
					params.use_APD = false;
					params.use_edge = false;
				} else {
					params.state = REFINE_INIT;
					params.use_APD = true;
					params.use_edge = true;
					params.ransac_threshold = 0.01 - i * 0.00125;
					params.rotate_time = MIN(static_cast<int>(std::pow(2, i)), 4);
				}
				params.geom_consistency = false;
				params.max_iterations = 3;
				params.weak_peak_radius = 6;
			}
			ProcessProblem(problem);
		}
		iteration_index++;
		for (int j = 0; j < 3; ++j) {
			for (auto &problem : problems) {
				problem.iteration = iteration_index;
				problem.scale_size = static_cast<int>(std::pow(2, round_num - 1 - i)); // scale 
				problem.params.scale_size = problem.scale_size;
				{
					auto &params = problem.params;
					params.state = REFINE_ITER;
					if (i == 0) {
						params.use_APD = false;
						params.use_edge = false;
					} else {
						params.use_APD = true;
						params.use_edge = true;
					}
					params.ransac_threshold = 0.01 - i * 0.00125;
					params.rotate_time = MIN(static_cast<int>(std::pow(2, i)), 4);
					params.geom_consistency = true;
					params.max_iterations = 3;
					params.weak_peak_radius = MAX(4 - 2 * j, 2);
				}
				ProcessProblem(problem);
			}
			iteration_index++;
		}
		std::cout << "Round: " << i << " done\n";
	}

	RunFusion(dense_folder, problems);
	{// delete files
		for (size_t i = 0; i < problems.size(); ++i) {
			const auto &problem = problems[i];
			remove(problem.result_folder / fs::path("weak.bin"));
			remove(problem.result_folder / fs::path("depths.dmb"));
			remove(problem.result_folder / fs::path("normals.dmb"));
			remove(problem.result_folder / fs::path("selected_views.bin"));
			remove(problem.result_folder / fs::path("neighbour.bin")); 
			remove(problem.result_folder / fs::path("neighbour_map.bin"));
			for (int j = 0; j < round_num; j++) {
				remove(problem.result_folder / fs::path("edges_" + std::to_string(j) + ".dmb"));
				remove(problem.result_folder / fs::path("labels_" + std::to_string(j) + ".dmb"));
			}
		}
	}
	std::cout << "All done\n";
	return EXIT_SUCCESS;
}

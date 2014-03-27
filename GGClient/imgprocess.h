#pragma once
#include <vector>
#include <opencv2\core\core.hpp>

extern int width;
extern int height;

int img_encode(const cv::Mat &, unsigned char* &);
int get_src_test(unsigned char* &, int color = 0);
void convert_points(const std::vector<cv::Point2f> &, std::vector<int> &);
bool get_src_points(std::vector<int> &);
bool find_glyphs(const cv::Mat &, std::vector<cv::Point> &);
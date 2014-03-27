#pragma once
#include <Windows.h>
#include <opencv2\core\core.hpp>
#include <string>

extern std::string win_name;

bool CALLBACK lpEnumFunc(HWND hWND, LPARAM lParam);
int set_window(int &width, int &height);
bool capture(cv::Mat &img, int color = 0);
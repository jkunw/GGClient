#include "windowcapture.h"
#include <opencv2\imgproc\imgproc.hpp>

using namespace std;

HWND win = NULL; // Handle to the window to be captured.
string win_name; // Part of window's name.
int resolution_w, resolution_h; // Resolution of the monitor.

/* An application-defined callback function used with the EnumWindows function.
Stop enumeration if it returns FALSE:
1. enumerate all windows;
2. find the window to be captured (with window name `win_name`);
If the window is found, then `win` will be set to its window handle.
*/
bool CALLBACK lpEnumFunc(HWND hWND, LPARAM lParam) {
	if(hWND == NULL)
		return FALSE;

	char *cname = new char[20]();
	if(IsWindowVisible(hWND)) {
		GetWindowText(hWND, cname, 20);
		string name(cname);
		delete cname;
		if(name.find(*(string*)lParam) != string::npos) {
			win = hWND;
			return FALSE;
		}
	}
	return TRUE;
}

/* Set the window to be captured and get the image size (width X height).
Return 0 if `win_name` title is not found in `title.ini`.
Return -1 if title `test` is found; in this case, no window is captured and a test src image is used.
Return 1 if successful.
*/
int set_window(int &width, int &height) {
	if(win_name == string("test"))
		return -1;

	// Enumerate all windows to get the window with title `win_name`.
	EnumWindows((WNDENUMPROC)lpEnumFunc, (LPARAM)&win_name);
	if(win == NULL) {
		MessageBox(0, "Window not found", "Error", MB_OK);
		return 0;
	}

	// Get monitor resolution.
	HWND desktop = GetDesktopWindow();
	RECT rect;
	GetWindowRect(desktop, &rect);
	resolution_h = rect.bottom;
	resolution_w = rect.right;

	// Get image size.
	cv::Mat img;
	capture(img);
	width = img.cols;
	height = img.rows;

	return 1;
}

/* Capture the window and store the captured img in `cv::Mat img`.
If color is not set (default), captured image will be grayscale.
Return false if not successful.
*/
bool capture(cv::Mat &img, int color) {
	if(win == NULL)
		return false;

	// If the window is minimized, restore it.
	if(IsIconic(win))
		ShowWindow(win, SW_RESTORE);

	// Get the size of the window.
	RECT wrect;
	if(!GetWindowRect(win, &wrect))
		return false;

	// If window is out of the borders of monitor,
	// move it to the upper-left corner and recapture the window.
	if(wrect.bottom > resolution_h || wrect.top  < 0 ||
	   wrect.right  > resolution_w || wrect.left < 0) {
		if(!SetWindowPos(win, HWND_NOTOPMOST, 100, 100, 0, 0, SWP_NOSIZE))
			return false;
		return capture(img);
	}

	// Get the coordinate of upper-left corner of the window.
	POINT ul;
	ul.x = wrect.left;
	ul.y = wrect.top;

	// Get the size of client area (area without margins to show image).
	RECT crect;
	if(!GetClientRect(win, &crect))
		return false;

	// Get the size of image.
	int width = crect.right;
	int height = crect.bottom;

	// Create DC
	HDC hDC = GetWindowDC(win);
	HDC memDC = CreateCompatibleDC(hDC);
	HBITMAP memBM = CreateCompatibleBitmap(hDC, width, height);
	SelectObject(memDC, memBM);

	// Get the upper and left margins of the window.
	if(!ScreenToClient(win, &ul))
		return false;
	ul.x = abs(ul.x);
	ul.y = abs(ul.y);

	// Copy the client area to memBM.
	if(!BitBlt(memDC, 0, 0, width, height, hDC, ul.x, ul.y, SRCCOPY))
		return false;

	// Convert BITMAP to CV::Mat
	BITMAPINFOHEADER  bi;
	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = width;
	bi.biHeight = -height;
	bi.biPlanes = 1;
	bi.biBitCount = 24;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;
	img = cv::Mat(height, width, CV_8UC3);
	GetDIBits(memDC, memBM, 0, height, img.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

	// Release resources.
	DeleteObject(memBM);
	DeleteDC(memDC);
	ReleaseDC(win, hDC);

	// If color is not set, convert image to grayscale.
	if(!color)
		cv::cvtColor(img, img, cv::COLOR_BGR2GRAY);

	return true;
}
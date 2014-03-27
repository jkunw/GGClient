#include <opencv2\highgui\highgui.hpp>
#include <opencv2\imgproc\imgproc.hpp>
#include "imgprocess.h"
#include <Windows.h>

using namespace cv;
using namespace std;

/* Patterns of the barcode in four glyph markers.
`1` stands for white cell and `0` for black cell.
For example, one glyph pattern will be represented as 
        b b b b b        
		b b w b b      0 1 0  
		b w w w b  =>  1 1 1  =>  0 1 0 1 1 1 1 0 1.
		b w b w b      1 0 1
		b b b b b
The pattern can be seen from four directions, so pattern list has four rows.
*/
int pattern1[] = {0, 1, 0, 1, 1, 1, 1, 0, 1,
				  1, 1, 0, 0, 1, 1, 1, 1, 0,
				  1, 0, 1, 1, 1, 1, 0, 1, 0,
				  0, 1, 1, 1, 1, 0, 0, 1, 1};
int pattern2[] = {0, 0, 1, 1, 0, 1, 0, 1, 0,
				  0, 1, 0, 1, 0, 0, 0, 1, 1,
				  0, 1, 0, 1, 0, 1, 1, 0, 0,
				  1, 1, 0, 0, 0, 1, 0, 1, 0};
int pattern3[] = {0, 0, 1, 0, 1, 0, 1, 0, 1,
				  1, 0, 0, 0, 1, 0, 1, 0, 1,
				  1, 0, 1, 0, 1, 0, 1, 0, 0,
				  1, 0, 1, 0, 1, 0, 0, 0, 1};
int pattern4[] = {0, 1, 0, 0, 1, 1, 0, 0, 1,
				  0, 0, 0, 0, 1, 1, 1, 1, 0,
				  1, 0, 0, 1, 1, 0, 0, 1, 0,
				  0, 1, 1, 1, 1, 0, 0, 0, 0};
const int *pattern[] = {pattern1, pattern2, pattern3, pattern4};

const int APPROX_POLY_EPSILON  = 3;  // Epsilon for `approxPolyDP`.
const int MIN_CONTOUR_AREA     = 50; // Minimal contour area to filter contours.
const int CELL_NUM_ROW         = 3;  // Cell number per row.
const int GLYPH_SIZE           = 30; // Resized glyph size.
const int CELL_NUM             = CELL_NUM_ROW * CELL_NUM_ROW; // Total cell number.
const int CELL_SIZE            = GLYPH_SIZE / CELL_NUM_ROW;   // Resized cell size.

// Dst points to normalize glyph area to square.
// Clockwise 
vector<Point2f> cw_dst_points  = {Point2f(-CELL_SIZE            , -CELL_SIZE),
							   	  Point2f(GLYPH_SIZE + CELL_SIZE, -CELL_SIZE),
							   	  Point2f(GLYPH_SIZE + CELL_SIZE, GLYPH_SIZE + CELL_SIZE),
							   	  Point2f(-CELL_SIZE            , GLYPH_SIZE + CELL_SIZE)};
// Counterclockwise
vector<Point2f> ccw_dst_points = {Point2f(-CELL_SIZE            , -CELL_SIZE),
								  Point2f(-CELL_SIZE            , GLYPH_SIZE + CELL_SIZE),
								  Point2f(GLYPH_SIZE + CELL_SIZE, GLYPH_SIZE + CELL_SIZE),
								  Point2f(GLYPH_SIZE + CELL_SIZE, -CELL_SIZE)};

int jpeg_quality = 5;   // Compress JPEG quality.
int width        = 640; // Width of image.
int height       = 480; // Height of image.

vector<unsigned char> buff; // Buff is used to store converted image data.

/* Find glyph centers in image `src`.
Return true if all four glyphs are detected.
*/
bool find_glyphs(const Mat &src, vector<Point2f> &glyph_centers) {
	Mat gs_src;
	if(src.type() != CV_8UC1)
		cvtColor(src, gs_src, COLOR_BGR2GRAY);
	else
		gs_src = src;

	Mat bw_src;
	Mat edges;
	double high_threshold = threshold(gs_src, bw_src, 0, 255, THRESH_BINARY | THRESH_OTSU);
	Canny(gs_src, edges, 0.5*high_threshold, high_threshold);

	vector<vector<Point>> contours;
	findContours(edges, contours, RETR_LIST, CHAIN_APPROX_SIMPLE);

	vector<vector<Point>> vertices;
	for(const auto &cnt : contours) {
		vector<Point> v;
		approxPolyDP(cnt, v, APPROX_POLY_EPSILON, true);
		if(v.size() == 4 &&                        
		   contourArea(v) > MIN_CONTOUR_AREA &&   
		   isContourConvex(v))                   
		   vertices.push_back(v);
	}
	if(vertices.size() < 4)
		return false;

	Mat m;                               
	Size dsize(GLYPH_SIZE, GLYPH_SIZE); 
	Mat glyph(dsize, CV_8UC1);         
	int p[CELL_NUM + 1];              
	int found[4] = {0, 0, 0, 0};     
	vector<Point2f> fv(4);
	for(const auto &v : vertices) {
		for(int i = 0; i < 4; i++) {
			Point2f f = Point(v[i]);
			fv[i] = f;
		}

		if(contourArea(v, true) < 0)
			m = getPerspectiveTransform(fv, cw_dst_points);
		else
			m = getPerspectiveTransform(fv, ccw_dst_points);
		warpPerspective(gs_src, glyph, m, dsize);
		threshold(glyph, glyph, 0, 255, THRESH_BINARY | THRESH_OTSU);

		for(int i = 0; i < CELL_NUM; i++) {
			int sum = 0;
			int row = (i / 3) * CELL_SIZE;
			int col = (i % 3) * CELL_SIZE;
			for(int r = row; r < row + CELL_SIZE; r++)
				for(int c = col; c<col + CELL_SIZE; c++)
					sum += (int)glyph.at<uchar>(c, r);
			if(sum / 255 > CELL_SIZE*CELL_SIZE / 2)
				p[i] = 1;
			else
				p[i] = 0;
		}

		for(int j = 4, i = 0; i < 4 && j == 4; i++) {
			for(j = 0; j < 4; j++) {
				int k;
				for(k = 0; k < CELL_NUM; k++) {
					if(pattern[i][j*CELL_NUM + k] != p[k])
						break;
				}
				if(k == CELL_NUM) {
					Point2f center = (v[0] + v[1] + v[2] + v[3]) * 0.25;
					if(!found[i]) {
						glyph_centers[i] = center;
						found[i]++;
						break;
					}
				}
			}
		}
	}

	if(found[0] && found[1] && found[2] && found[3])
		return true;
	else
		return false;
}

/* Get src points from template image.
Four src points are stored in `glyph_center_int` as [x0, y0, x1, y1, x2, y2, x3, y3].
Return true if successful.
*/
bool get_src_points(vector<int> &glyph_center_int) {
	// Load template file (either in JPEG or BMP type).
	Mat templ = imread("template.jpg", CV_LOAD_IMAGE_GRAYSCALE);
	if(templ.data == NULL) {
		templ = imread("template.bmp", CV_LOAD_IMAGE_GRAYSCALE);
		if(templ.data == NULL) {
			MessageBox(0, "Template not found", "Error", MB_OK);
			return false;
		}
	}

	// Find four glyph markers in template.
	vector<Point2f> glyph_centers(4);
	if(!find_glyphs(templ, glyph_centers)) {
		MessageBox(0, "Four markers not found in template", "Error", MB_OK);
		return false;
	}

	// Convert to `glyph_center_int`.
	for(const auto &p : glyph_centers) {
		glyph_center_int.push_back(int(p.x));
		glyph_center_int.push_back(int(p.y));
	}
	return true;
}

/* Compress cv::Mat to JEPG file.
Return compressed JPEG length in byte.
*/
int img_encode(const Mat &img, unsigned char* &img_data) {
	// Set compress quality.
	vector<int> p;
	p.push_back(CV_IMWRITE_JPEG_QUALITY);
	p.push_back(jpeg_quality);

	imencode(".jpg", img, buff, p);
	img_data = reinterpret_cast<unsigned char*>(buff.data());
	return buff.size();
}

/* Get compressed src image data FOR TEST ONLY!
The image is 24bit color image if `color` is set to 1; grayscale image if 0 (default). 
Return compressed JPEG length in byte.
*/
int get_src_test(unsigned char* &img_data, int color) {
    Mat src;
    if(color) {
        src = Mat(Size(width, height), CV_8UC3, Scalar(0));
        circle(src, Point(width / 2, height / 2), 50, Scalar(0, 255, 0), -1);
    } else {
        src = Mat(Size(width, height), CV_8UC1, Scalar(0));
        circle(src, Point(width / 2, height / 2), 50, Scalar(255), -1);
    }
	return img_encode(src, img_data);
}

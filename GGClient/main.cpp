#include "imgprocess.h"
#include "ggclient.h"
#include "windowcapture.h"
#include "readini.h"
#include "keyevent.h"

#include <iostream>
#include <string>
#include <vector>
#include <Windows.h>

using namespace std;

/* Get src image through window-capture.
*/
int get_src_capture(unsigned char* &img_data, int color) {
    cv::Mat src;
    if(!capture(src, color))
        return 0;
    return img_encode(src, img_data);
}

int main() {
    // Get `win_name` and `ip` and `process` from `config.ini`.
    cout << "Read config.ini..." << endl;
    // If `process` is set to 0, color image is captured and 
    // sent to server; server will show the color image without processing it.
    int process;
    int zoom, x, y;
    if(!parse_ini(win_name, ip, process, jpeg_quality, zoom, x, y))
        return 1;

    // Set the function to get src image.
    int(*get_src)(unsigned char* &, int); // Function pointer
    int flag = set_window(width, height);
    if(flag == -1) {
        // Get src image for test when title in `config.ini` is set to `test`.
        get_src = get_src_test;
        cout << "Get src image for test..." << endl;
    } else if(flag == 1) {
        // Get src image through window-capture.
        get_src = get_src_capture;
        cout << "Get src image from window-capture..." << endl;
    } else
        return 1;

    // Get src points in template.
    vector<int> src_points;
    if(!get_src_points(src_points))
        return 1;

    // Initialize socket module and client.
    cout << "Initialize socket..." << endl;
    if(!init_socket())
        return 1;

    // Input IP and connect to server.
    cout << "Connected to server..." << endl;
    if(!connect_server())
        return 1;

    // Send image size to server.
    cout << "Send image size..." << endl;
    if(!send_int(width) || !send_int(height))
        return 1;

    // Send process flag.
    if(!send_int(process))
        return 1;
    if(process == 0) {
        cout << "Server will show the captured image without processing..." << endl;
    } else {
        // Send src_points found in template to server.
        cout << "Send src points in template..." << endl;
        if(!send_src_points(src_points))
            return 1;
    }

    // Send zoom in/out and translation value to sever.
    if(!send_int2(zoom) || !send_int2(x) || !send_int2(y))
        return 1;

    cout << "Start listening key input..." << endl;
    CreateThread(0, 0, LPTHREAD_START_ROUTINE(wait_key), 0, 0, 0);

    cout << "Start transferring image data..." << endl;
    // Keep waiting ready signal from server to send image data.
    while(true) {
        // Get ready signal from server.
        if(server_ready()) {
            // Get image data char array.
            unsigned char *img_data;
            int length = get_src(img_data, !process);
            if(length == 0)
                break;

            // Send image data.
            // If error occurs, end the program.
            if(!send_img((char*)img_data, length))
                break;
        } else
            break;
    }
    cout << "End connection..." << endl;

    // Close socket and clean up.
    close_socket();

    return 0;
}
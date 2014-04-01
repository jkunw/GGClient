#include <Windows.h>
#include <string>
#include "readini.h"

using namespace std;

const char *ini_name          = ".\\config.ini";
const int TRANSLATION_EPSILON = 5;
const int ZOOM_EPSILON        = 1;

/* Parse initialization file to get 
1. window name;
2. IP address;
3. JPEG compress quality;
4. Process or not;
Create a new one and return false if ini file does not exist.
*/
bool parse_ini(string &win_name, string &ip, int &process, int &jpeg_quality, int &zoom, int &x, int &y) {
	char *buff = new char[20]();

    // Get window title.
	GetPrivateProfileString("WindowCapture", "Title", NULL, buff, 20, ini_name);
	if(GetLastError() == 0x2) { // File does not exist.
		WritePrivateProfileString("WindowCapture", "Title", "test", ini_name);
		WritePrivateProfileString("Server", "IP", "192.168.1.100", ini_name);
		WritePrivateProfileString("Image", "Process", "1", ini_name);
		MessageBox(0, "Please window title and IP address in config.ini", "Error", MB_OK);
		return false;
	}
	win_name = string(buff);

    // Get server IP address.
	GetPrivateProfileString("Server", "IP", NULL, buff, 20, ini_name);
	ip = string(buff);

    // Get process flag.
    process = GetPrivateProfileInt("Image", "Process", 1, ini_name);

    // Get JPEG compress quality.
    jpeg_quality = GetPrivateProfileInt("Image", "ImageQuality", 5, ini_name);

    // Get zoom and translation value.
    zoom = GetPrivateProfileInt("Image", "Zoom", 10, ini_name);
    x = GetPrivateProfileInt("Image", "X", 500, ini_name);
    y = GetPrivateProfileInt("Image", "Y", 500, ini_name);

	return true;
}

/* Save current zoom in/out and translation value to `config.ini`.
*/
void set_TM(char key) {
    char buff[20];
    int x, y, zoom;
    switch(key) {
    case 'L':
        x = GetPrivateProfileInt("Image", "X", 500, ini_name);
        x -= TRANSLATION_EPSILON;
        itoa(x, buff, 10);
        WritePrivateProfileString("Image", "X", buff, ini_name);
        break;
    case 'R':
        x = GetPrivateProfileInt("Image", "X", 500, ini_name);
        x += TRANSLATION_EPSILON;
        itoa(x, buff, 10);
        WritePrivateProfileString("Image", "X", buff, ini_name);
        break;
    case 'U':
        y = GetPrivateProfileInt("Image", "Y", 500, ini_name);
        y -= TRANSLATION_EPSILON;
        itoa(y, buff, 10);
        WritePrivateProfileString("Image", "Y", buff, ini_name);
        break;
    case 'D':
        y = GetPrivateProfileInt("Image", "Y", 500, ini_name);
        y += TRANSLATION_EPSILON;
        itoa(y, buff, 10);
        WritePrivateProfileString("Image", "Y", buff, ini_name);
        break;
    case 'I':
        zoom = GetPrivateProfileInt("Image", "Zoom", 10, ini_name);
        zoom += ZOOM_EPSILON;
        itoa(zoom, buff, 10);
        WritePrivateProfileString("Image", "Zoom", buff, ini_name);
        break;
    case 'O':
        zoom = GetPrivateProfileInt("Image", "Zoom", 10, ini_name);
        zoom -= ZOOM_EPSILON;
        itoa(zoom, buff, 10);
        WritePrivateProfileString("Image", "Zoom", buff, ini_name);
        break;
    }
}
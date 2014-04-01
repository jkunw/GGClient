#pragma once
#include <vector>
#include <string>

extern std::string ip;
bool send_src_points(std::vector<int> &s);
bool send_img(const char* img_data, int length);
bool server_ready();
bool close_socket();
bool send_int(const int &num);
bool send_int2(const int &num);
bool send_key(const unsigned char &key);
bool connect_server();
bool init_socket();
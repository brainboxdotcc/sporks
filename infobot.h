#pragma once

#include <string>
#include <mutex>
#include "queue.h"
#include "config.h"

std::string ReplaceString(std::string subject, const std::string& search, const std::string& replace);
void infobot_socket(std::mutex* input_mutex, std::mutex* output_mutex, Queue* inputs, Queue* outputs, rapidjson::Document* config);


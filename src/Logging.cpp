#pragma once
#include "Logging.hpp"
#include <iostream>

Logger::Logger(std::string filename, bool wtf, bool wts, bool omit_metrics)
    : write_to_file(wtf)
    , write_to_stdout(wts)
    , no_metrics(omit_metrics)
{
    this->output_filename = filename;
    this->output_stream   = std::ofstream(filename);
}

Logger::~Logger()
{
    this->output_stream.close();
    if (!no_metrics)
        this->exportMetrics();
}

void Logger::setMetric(std::string key, uintmax_t new_value)
{
    if (no_metrics)
        return;
    this->metrics[key] = new_value;
    this->report("METRICS", key + "=" + std::to_string(new_value));
}

void Logger::changeMetric(std::string key, uintmax_t change_in_value)
{
    if (no_metrics)
        return;
    if (!this->metrics.contains(key))
        this->metrics[key] = 0;
    this->metrics[key] += change_in_value;
    this->report("METRICS", key + "=" + std::to_string(this->metrics[key]));
}

uintmax_t Logger::getMetric(std::string key)
{
    if (!this->metrics.contains(key) || no_metrics)
        return 0;
    return this->metrics[key];
}

void Logger::exportMetrics()
{
    auto datfile = std::ofstream("metrics.dat");
    auto logs    = std::ifstream(output_filename);
    std::string line;
    int start_timestamp = 0;
    std::unordered_map<std::string, std::string> datas;
    while (getline(logs, line)) {
        if (line.ends_with("Starting Server")) {
            auto timestamp_del    = line.find("@ ");
            auto metadata_end_del = line.find("]");
            if (timestamp_del == std::string::npos)
                continue;
            auto time_str   = line.substr(timestamp_del + 2,
                                          metadata_end_del - timestamp_del - 2);
            start_timestamp = std::stoi(time_str);
        } else if (line.starts_with("[METRICS")) {
            auto timestamp_del = line.find("@ ");
            auto keyval_del    = line.find("]: ");
            if (keyval_del == std::string::npos
                || timestamp_del == std::string::npos)
                continue;
            auto timestamp = line.substr(timestamp_del + 2,
                                         keyval_del - timestamp_del - 2);
            auto keyval    = line.substr(keyval_del + 3);
            int time_int   = 0;
            try {
                time_int = std::stoi(timestamp);
            } catch (std::invalid_argument &e) {
                std::cout << "Error: " << timestamp << ", " << e.what()
                          << std::endl;
                return;
            }
            auto key_del = keyval.find("=");
            if (key_del == std::string::npos)
                return;
            auto key = keyval.substr(0, key_del);
            auto val = keyval.substr(key_del + 1);
            if (!datas.contains(key))
                datas[key] = std::to_string(time_int - start_timestamp) + " "
                    + val + "\n";
            else
                datas[key] += std::to_string(time_int - start_timestamp) + " "
                    + val + "\n";
        }
    }
    size_t i = 0;
    for (auto &[key, valpair] : datas) {
        datfile << "\"" << key << "\"" << std::endl;
        if (i++ == datas.size() - 1)
            datfile << valpair;
        else
            datfile << valpair << std::endl << std::endl;
    }
    datfile.close();
}

void Logger::report(std::string type, std::string msg)
{
    this->log_mutex.lock();
    std::time_t time        = std::time(nullptr);
    std::string time_str    = std::to_string(time);
    std::string color_start = "", color_end = "";
    if (type == "ERROR") {
        color_start = "\033[1;31m";
        color_end   = "\033[1;0m";
    }

    if (write_to_file)
        this->output_stream << "[" << type << " @ " << time_str << "]: " << msg
                            << std::endl;
    if (write_to_stdout)
        std::cout << color_start << "[" << type << " @ " << time_str
                  << "]: " << msg << color_end << std::endl;
    this->log_mutex.unlock();
}

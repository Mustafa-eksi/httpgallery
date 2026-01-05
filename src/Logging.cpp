#include "Logging.hpp"

Logger::Logger(std::string filename, bool wtf, bool wts)
    : write_to_file(wtf), write_to_stdout(wts) {
    this->output_filename = filename;
    this->output_stream = std::ofstream(filename);
}

Logger::~Logger() {
    this->output_stream.close();
}

void Logger::setMetric(std::string key, uintmax_t new_value) {
    this->metrics[key] = new_value;
    this->report("METRICS", key + "=" + std::to_string(new_value));
}

void Logger::changeMetric(std::string key, uintmax_t change_in_value) {
    if (!this->metrics.contains(key))
        this->metrics[key] = 0;
    this->metrics[key] += change_in_value;
    this->report("METRICS", key + "+=" + std::to_string(change_in_value));
}

uintmax_t Logger::getMetric(std::string key) {
    if (!this->metrics.contains(key))
        return 0;
    return this->metrics[key];
}

void Logger::report(std::string type, std::string msg) {
    this->log_mutex.lock();
    std::time_t time = std::time(nullptr);
    auto time_str = std::put_time(std::localtime(&time), "%T %F");
    if (write_to_file)
        this->output_stream << "[ERROR @ " << time_str << "]: " << msg << std::endl;
    if (write_to_stdout)
        std::cout << "["<< type <<" @ " << time_str << "]: " << msg << std::endl;
    this->log_mutex.unlock();
}


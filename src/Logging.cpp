#include "Logging.hpp"

Logger::Logger(std::string filename, bool wtf, bool wts)
    : write_to_file(wtf), write_to_stdout(wts) {
    this->output_filename = filename;
    this->output_stream = std::ofstream(filename);
}

Logger::~Logger() {
    this->output_stream.close();
}

void Logger::error(std::string msg) {
    this->log_mutex.lock();
    std::time_t time = std::time(nullptr);
    auto time_str = std::put_time(std::localtime(&time), "%T %F");
    if (write_to_file)
        this->output_stream << "[ERROR @ " << time_str << "]: " << msg << std::endl;
    if (write_to_stdout)
        std::cout << "[ERROR @ " << time_str << "]: " << msg << std::endl;
    this->log_mutex.unlock();
}

void Logger::warning(std::string msg) {
    this->log_mutex.lock();
    std::time_t time = std::time(nullptr);
    auto time_str = std::put_time(std::localtime(&time), "%T %F");
    if (write_to_file)
        this->output_stream << "[WARNING @ " << time_str << "]: " << msg << std::endl;
    if (write_to_stdout)
        std::cout << "[WARNING @ " << time_str << "]: " << msg << std::endl;
    this->log_mutex.unlock();
}

void Logger::info(std::string msg) {
    this->log_mutex.lock();
    auto id = std::this_thread::get_id();
    std::time_t time = std::time(nullptr);
    auto time_str = std::put_time(std::localtime(&time), "%T %F");
    if (write_to_file)
        this->output_stream << "[INFO @ " << time_str << "]: " << msg << std::endl;
    if (write_to_stdout)
        std::cout << "[INFO @ " << time_str << " by id "<< id <<" ]: " << msg << std::endl;
    this->log_mutex.unlock();
}

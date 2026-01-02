#include <ctime>
#include <iomanip>
#include <thread>

class Logger {
    std::mutex log_mutex;
    std::string output_filename;
    std::ofstream output_stream;
    bool write_to_file, write_to_stdout;
public:
    Logger(std::string filename, bool wtf = true, bool wts = false);
    ~Logger();
    void error(std::string msg);
    void warning(std::string msg);
    void info(std::string msg);
};

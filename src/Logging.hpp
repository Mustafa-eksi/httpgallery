#include <ctime>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <thread>
#include <unordered_map>

class Logger {
    std::mutex log_mutex;
    std::string output_filename;
    std::ofstream output_stream;
    bool write_to_file, write_to_stdout, no_metrics;
    std::unordered_map<std::string, uintmax_t> metrics;

public:
    Logger(std::string filename, bool wtf = true, bool wts = false,
           bool omit_metrics = false);
    ~Logger();
    void setMetric(std::string key, uintmax_t new_value);
    void changeMetric(std::string key, uintmax_t change_in_value);
    uintmax_t getMetric(std::string key);
    void exportMetrics();

    void report(std::string type, std::string msg);
};

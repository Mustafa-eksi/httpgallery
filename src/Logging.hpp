#ifndef HTTPGALLERY_LOGGING
#define HTTPGALLERY_LOGGING
#include <ctime>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <thread>
#include <unordered_map>

/**
 * @brief Custom logging class that also supports metrics.
 */
class Logger {
    std::mutex log_mutex;
    std::string output_filename;
    std::ofstream output_stream;
    bool write_to_file, write_to_stdout, no_metrics;
    std::unordered_map<std::string, uintmax_t> metrics;

    void reportFormat(std::string type);

public:
    /**
     * @brief Initializes Logger class
     *
     * Logger can write both to stdout and to a file. It also has support for
     * metrics.
     *
     * @param filename output filename to write the logs to.
     * @param wtf Logger writes to file when set to true.
     * @param wts Logger writes to stdout when set to true.
     * @param omit_metrics Logger ignores metrics when set to true.
     */
    Logger(std::string filename, bool wtf = true, bool wts = false,
           bool omit_metrics = false);
    ~Logger();
    /**
     * @brief Sets a metric value.
     *
     * First call initiates the metric and then sets the value.
     *
     * @param key name of the metric.
     * @param new_value New value of the metric.
     */
    void setMetric(std::string key, uintmax_t new_value);
    /**
     * @brief Changes a metric value.
     *
     * First call initiates the metric and then sets the value. The initial
     * value is 0 by convention.
     *
     * @param key name of the metric.
     * @param change_in_value Value change of the metric.
     */
    void changeMetric(std::string key, uintmax_t change_in_value);
    /**
     * @breif Retrieves the value of a metric specified by _key_.
     *
     * @param key Name of the metric.
     * @return Returns 0 if the metric doesn't exist.
     */
    uintmax_t getMetric(std::string key);
    /**
     * @brief writes the metrics to metrics.dat file in the working directory.
     */
    void exportMetrics();

    /**
     * @brief Log formatted messages.
     *
     * Arguments must be types that can be converted to strings by output
     * operator (<<).
     *
     * @param type "ERROR" type annotates the log message written to stdout with
     * red color. "METRICS" is reserved for other metrics functions. Other types
     * have no effect.
     */
    template <typename T, typename... Rest>
    void reportFormat(std::string type, T one_arg, Rest... rest);

    /**
     * @brief Logs unformatted messages
     *
     * The name of this function is report instead of log because I changed it
     * to make a refactor easier.
     */
    void report(std::string type, std::string msg);
};
#endif // HTTPGALLERY_LOGGING

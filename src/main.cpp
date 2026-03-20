#include <csignal>
#include <iostream>
#include <utility>

#include "Configuration.hpp"
#include "FileServer.hpp"
#include "Logging.hpp"

const char HELP_MESSAGE[]
    = "Usage: httpgallery <path-to-folder> [options]\n"
      "Options:\n"
      "   -h, --help\n"
      "       Shows this message\n"
#ifndef HTTPGALLERY_NO_OPENSSL
      "   -s, --secure <path-to-certificate> <path-to-PrivateKey>\n"
      "       Enable HTTPS support (using openssl).\n"
#endif
      "   -l, --log-file <path-to-logs-file>\n"
      "       Specify log file path to write to. Default is current working\n"
      "       directory.\n"
      "   -p, --port <number>\n"
      "       Use <number> instead of 8000 as the port\n"
      "   --cache-files <bool>\n"
      "       Store visited files in a LRU cache. Default is true\n"
      "   --cache-size <number>\n"
      "       Maximum number of files stored in cache at once. Default is 100\n"
      "   --silent\n"
      "       Do not print the logs to stdout\n"
      "   --backlog <number>\n"
      "       Specifies backlog (default is 10). Only effective in http mode.\n"
      "   -c, --config <path-to-config>\n"
      "       Use config file specified. If this flag is present, program\n"
      "       ignores other flags.\n"
      "   --enable-metrics\n"
      "       Include metrics in the logs (also generates .dat file)\n";

std::atomic<bool> *shouldClose = NULL;

void exit_handler(int s)
{
    (void)s;
    if (shouldClose)
        *shouldClose = true;
}

int main(int argc, char **argv)
{
    signal(SIGINT, &exit_handler);
    Configuration config = Configuration::Default();

    if (argc > 1 && !std::string(argv[1]).starts_with("-"))
        config.map["config"]["WorkingDirectory"] = argv[1];

    std::string config_file;

    for (int i = 1; i < argc; i++) {
        std::string current_arg = argv[i];
        if (current_arg == "-h" || current_arg == "--help") {
            std::cout << HELP_MESSAGE;
            return 0;
        } else if (current_arg == "-s" || current_arg == "--secure") {
#ifndef HTTPGALLERY_NO_OPENSSL
            if (i + 2 >= argc) {
                std::cout << "\033[1;31mError: Wrong flag usage\033[1;0m"
                          << std::endl;
                std::cout << HELP_MESSAGE;
                return -1;
            }
            config.map["config"]["CertificationPath"] = argv[i + 1];
            config.map["config"]["PrivateKeyPath"]    = argv[i + 2];
            config.map["config"]["UseHttps"]          = true;
#else
            std::cout
                << "\033[1;31mError: HTTPS feature is not available.\033[1;0m"
                << std::endl;
            return -1;
#endif
        } else if (current_arg == "-l" || current_arg == "--log-file") {
            if (i + 1 >= argc) {
                std::cout << "\033[1;31mError: Wrong flag usage\033[1;0m"
                          << std::endl;
                std::cout << HELP_MESSAGE;
                return -1;
            }
            config.map["config"]["LogsFilePath"] = argv[i + 1];
        } else if (current_arg == "-c" || current_arg == "--config") {
            if (i + 1 >= argc) {
                std::cout << "\033[1;31mError: Wrong flag usage\033[1;0m"
                          << std::endl;
                std::cout << HELP_MESSAGE;
                return -1;
            }
            config_file = argv[i + 1];
        } else if (current_arg == "-p" || current_arg == "--port") {
            if (i + 1 >= argc) {
                std::cout << "\033[1;31mError: Wrong flag usage\033[1;0m"
                          << std::endl;
                std::cout << HELP_MESSAGE;
                return -1;
            }
            try {
                config.map["config"]["Port"] = std::stoi(argv[i + 1]);
            } catch (std::out_of_range &e) {
                std::cout << "\033[1;31mError: String to Integer conversion "
                             "error:\033[1;0m"
                          << argv[i + 1] << std::endl;
                return -1;
            } catch (std::invalid_argument &e) {
                std::cout << "\033[1;31mError: String to Integer conversion "
                             "error:\033[1;0m"
                          << argv[i + 1] << std::endl;
                return -1;
            }
        } else if (current_arg == "--backlog") {
            if (i + 1 >= argc) {
                std::cout << "\033[1;31mError: Wrong flag usage\033[1;0m"
                          << std::endl;
                std::cout << HELP_MESSAGE;
                return -1;
            }
            try {
                config.map["config"]["Backlog"] = std::stoi(argv[i + 1]);
            } catch (std::out_of_range &e) {
                std::cout << "\033[1;31mError: String to Integer conversion "
                             "error:\033[1;0m"
                          << argv[i + 1] << std::endl;
                return -1;
            } catch (std::invalid_argument &e) {
                std::cout << "\033[1;31mError: String to Integer conversion "
                             "error:\033[1;0m"
                          << argv[i + 1] << std::endl;
                return -1;
            }
        } else if (current_arg == "--cache-size") {
            if (i + 1 >= argc) {
                std::cout << "\033[1;31mError: Wrong flag usage\033[1;0m"
                          << std::endl;
                std::cout << HELP_MESSAGE;
                return -1;
            }
            try {
                config.map["config"]["CacheSize"] = std::stoi(argv[i + 1]);
            } catch (std::out_of_range &e) {
                std::cout << "\033[1;31mError: String to Integer conversion "
                             "error:\033[1;0m"
                          << argv[i + 1] << std::endl;
                return -1;
            } catch (std::invalid_argument &e) {
                std::cout << "\033[1;31mError: String to Integer conversion "
                             "error:\033[1;0m"
                          << argv[i + 1] << std::endl;
                return -1;
            }
        } else if (current_arg == "--cache-files") {
            if (i + 1 >= argc) {
                std::cout << "\033[1;31mError: Wrong flag usage\033[1;0m"
                          << std::endl;
                std::cout << HELP_MESSAGE;
                return -1;
            }
            if (std::string(argv[i + 1]) == "true") {
                config.map["config"]["CacheFiles"] = true;
            } else if (std::string(argv[i + 1]) == "false") {
                config.map["config"]["CacheFiles"] = false;
            } else {
                std::cout << "\033[1;31mError: Wrong flag usage\033[1;0m"
                          << std::endl;
                std::cout << HELP_MESSAGE;
                return -1;
            }
        } else if (current_arg == "--silent") {
            config.map["config"]["Silent"] = true;
        } else if (current_arg == "--enable-metrics") {
            config.map["config"]["NoMetrics"] = false;
        }
    }
    if (!std::filesystem::exists(config.configString("WorkingDirectory"))) {
        std::cout << "\033[1;31mError: specified path does not exist\033[1;0m"
                  << std::endl;
        std::cout << HELP_MESSAGE;
        return -2;
    }

    if (!config_file.empty() && std::filesystem::exists(config_file)) {
        // TODO: Initialize configuration object
        config = Configuration(config_file);
    }
    Logger logger = Logger(
        config.configString("LogsFilePath") + "httpgallery_logs.txt", true,
        !config.configBool("Silent"), config.configBool("NoMetrics"));
    logger.report("INFO", "Starting Server");
    int ret = system("ffmpegthumbnailer -v > /dev/null 2>&1");
    config.map["config"]["HasVideoThumbnailer"] = (ret == 0);
    bool https                                  = config.configBool("UseHttps");
    FileServer file_server = FileServer(logger, std::move(config));
    shouldClose            = &file_server.shouldClose;
    if (https) {
#ifndef HTTPGALLERY_NO_OPENSSL
        file_server.startHttps();
#endif
    } else {
        file_server.start();
    }
    return 0;
}

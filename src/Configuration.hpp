#include <fstream>
#include <string>
#include <unordered_map>
#include <variant>

#include "FileSystemInterface.cpp"

typedef std::variant<std::string, int, bool> ConfigVar;
typedef std::unordered_map<std::string, ConfigVar> ConfigMap;
typedef std::unordered_map<std::string, ConfigMap> StructuredMap;

class Configuration {
    StructuredMap map;

public:
    /**
     * @breif True if the config file includes errors.
     */
    bool faultyConfig;

    /**
     * @breif Line number which the error occurred. It is set to 0 if no error
     * occurred.
     */
    int faultLine;

    /**
     * @breif Initializes an empty Configuration object.
     */
    Configuration();

    /**
     * @breif Parses the config file specified with config_path parameter.
     *
     * @param config_path Path to the config file.
     */
    Configuration(std::string config_path);

    /**
     * @breif Converts s to appropriate ConfigVar.
     *
     * @param s String value to be converted.
     * @return Converted ConfigVar.
     */
    ConfigVar interpretString(std::string s);

    /**
     * @brief Operator overloading for accessing configuration elements.
     *
     * @param key Key to the config element;
     * @return Returns the ConfigVar of the element.
     */
    ConfigMap operator[](std::string key);

    /**
     * @brief Prints the configuration.
     */
    void print();
};

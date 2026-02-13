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
     * @breif Checks if key has value in [config].
     *
     * @return Returns true if key is accessible with config["config"][key].
     */
    bool containsConfig(std::string key);

    ///@{
    /**
     * @breif These functions gets the type from variant and returns them.
     */
    std::string configString(std::string key);
    int configInt(std::string key);
    bool configBool(std::string key);
    ///@}

    /**
     * @brief Prints the configuration.
     */
    void print();
};

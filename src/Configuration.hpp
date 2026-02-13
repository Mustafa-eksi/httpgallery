#include <fstream>
#include <string>
#include <unordered_map>
#include <variant>

#include "FileSystemInterface.cpp"

typedef std::variant<std::string, int, bool> ConfigVar;
typedef std::unordered_map<std::string, ConfigVar> ConfigMap;
typedef std::unordered_map<std::string, ConfigMap> StructuredMap;

/**
 * @brief Configuration class manages the configuration file and related
 * operations.
 */
class Configuration {
    StructuredMap map;

public:
    /**
     * @brief True if the config file includes errors.
     */
    bool faultyConfig;

    /**
     * @brief Line number which the error occurred. It is set to 0 if no error
     * occurred.
     */
    int faultLine;

    /**
     * @brief Initializes an empty Configuration object.
     */
    Configuration();

    /**
     * @brief Parses the config file specified with config_path parameter.
     *
     * @param config_path Path to the config file.
     */
    Configuration(std::string config_path);

    /**
     * @brief Converts s to appropriate ConfigVar.
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
     * @brief Checks if key has value in [config].
     *
     * @return Returns true if key is accessible with config["config"][key].
     */
    bool containsConfig(std::string key);

    ///@{
    /**
     * @brief These functions gets the type from variant and returns them.
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

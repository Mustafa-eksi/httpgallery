#pragma once
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "FileSystemInterface.hpp"

enum PermissionType {
    P_VIEW,
    P_READ,
    P_WRITE,
    P_UPDATE,
    P_DELETE,
};

const std::unordered_map<std::string, enum PermissionType> PERMISSION_STR = {
    { "R", P_READ },
    { "W", P_WRITE },
    { "U", P_UPDATE },
    { "D", P_DELETE },
};

typedef std::variant<std::string, int, bool, enum PermissionType> ConfigVar;
typedef std::unordered_map<std::string, ConfigVar> ConfigMap;
typedef std::unordered_map<std::string, ConfigMap> StructuredMap;

/**
 * @brief PermissionNode is a tree structure for managing permission system.
 */
class PermissionNode {
public:
    std::string name;
    std::unordered_map<std::string, PermissionNode *> children;
    PermissionNode *parent;
    std::set<enum PermissionType> permissions;

    PermissionNode();
    PermissionNode(std::string n);

    /**
     * @brief Creates a child in this node with the name n.
     */
    void addChild(std::string child_name);
    void addChildWithPerm(std::string child_name,
                          std::vector<enum PermissionType> ps);
};

/**
 * @brief Configuration class manages the configuration file and related
 * operations.
 */
class Configuration {
    StructuredMap map;
    PermissionNode permission_root;

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
     * @brief Creates permission tree with [permissions].
     */
    void createPermissionTree();

    /**
     * @brief Returns wether the user has permission to view the path or not.
     *
     * @param path Path for which the permission query is made.
     * @param pt Permission type that is querying.
     */
    bool askPermission(std::string path, enum PermissionType pt);

    /**
     * @brief Prints the configuration.
     */
    void print();
};

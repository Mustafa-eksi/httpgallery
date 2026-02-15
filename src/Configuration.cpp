#include "Configuration.hpp"

PermissionNode::PermissionNode()
{
    permissions = std::set<enum PermissionType>();
}

PermissionNode::PermissionNode(std::string n)
    : name(n)
{
    permissions = std::set<enum PermissionType>();
}

void PermissionNode::addChild(std::string child_name)
{
    if (children.contains(child_name))
        return;
    children[child_name]         = new PermissionNode(child_name);
    children[child_name]->parent = this;
}

void PermissionNode::addChildWithPerm(std::string child_name,
                                      std::vector<enum PermissionType> ps)
{
    this->addChild(child_name);
    this->children[child_name]->permissions.insert_range(ps);
}

Configuration::Configuration() { permission_root = PermissionNode(); }

Configuration::Configuration(std::string config_path)
{
    std::string config_file = read_binary_to_string(config_path);
    size_t prev             = 0;
    faultLine               = 0;
    int line_index          = 0;
    std::string current_name;
    for (auto line_end = config_file.find("\n"); line_end != std::string::npos;
         line_end      = config_file.find("\n", prev)) {

        std::string line = config_file.substr(prev, line_end - prev);
        if (line.starts_with("[")) {
            auto name_end = line.find("]");
            if (name_end == std::string::npos || name_end != line.size() - 1
                || line.size() < 2)
                goto fault;
            current_name      = line.substr(1, name_end - 1);
            map[current_name] = ConfigMap();
        } else if (!line.starts_with("#") && !line.empty()) {
            if (current_name.empty())
                goto fault;
            auto eq = line.find("=");
            if (eq == std::string::npos || eq == line.size() - 1 || eq == 0)
                goto fault;
            auto key               = line.substr(0, eq);
            auto val               = line.substr(eq + 1);
            map[current_name][key] = interpretString(val);
        }
        prev = line_end + 1;
        line_index++;
    }
    this->createPermissionTree();
    return;
fault:
    faultLine    = line_index + 1;
    faultyConfig = true;
}

ConfigVar Configuration::interpretString(std::string s)
{
    auto lower_s  = s;
    bool isnumber = true;
    for (auto &c : lower_s) {
        if (c >= 'A' && c <= 'Z')
            c += 32;
        if (c < '0' || c > '9')
            isnumber = false;
    }
    if (lower_s == "true")
        return ConfigVar(true);
    if (lower_s == "false")
        return ConfigVar(false);
    // FIXME: This can return out of bounds exception but I'm not sure if i
    // want to handle this.
    if (isnumber)
        return ConfigVar(std::stoi(s));
    return ConfigVar(s);
}

ConfigMap Configuration::operator[](std::string key) { return map[key]; }

bool Configuration::containsConfig(std::string key)
{
    if (map.find("config") == map.end())
        return false;
    return map["config"].find(key) != map["config"].end();
}

void Configuration::print()
{
    for (auto &[name, keyval] : map) {
        std::cout << "[" << name << "]" << std::endl;
        for (auto &[key, val] : keyval) {
            std::cout << key << "=";
            switch (val.index()) {
            case 0:
                std::cout << std::get<std::string>(val) << " (string)";
                break;
            case 1:
                std::cout << std::get<int>(val) << " (int)";
                break;
            case 2:
                std::cout << (std::get<bool>(val) ? "True" : "False")
                          << " (bool)";
                break;
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }
}

std::string Configuration::configString(std::string key)
{
    return std::get<0>(map["config"][key]);
}

int Configuration::configInt(std::string key)
{
    return std::get<1>(map["config"][key]);
}

bool Configuration::configBool(std::string key)
{
    return std::get<2>(map["config"][key]);
}

void Configuration::createPermissionTree()
{
    if (map.find("permissions") == map.end())
        return;
    permission_root.name = "/";
    for (auto [file_key, perm] : map["permissions"]) {
        PermissionNode *cursor = &permission_root;
        auto file              = std::filesystem::canonical(file_key).string();
        // canonical path is also an absolute path so it must start with '/'
        file = file.substr(1);
        std::string current_name;
        while (!file.empty()) {
            auto slash   = file.find("/");
            current_name = file.substr(0, slash);
            cursor->addChild(current_name);
            cursor = cursor->children[current_name];
            if (slash == std::string::npos || slash == file.size() - 1)
                break;
            file = file.substr(slash + 1);
        }
        std::vector<enum PermissionType> perms;
        std::string fperm = std::get<0>(perm);
        while (!fperm.empty()) {
            auto delim   = fperm.find(",");
            auto permstr = fperm.substr(0, delim);
            if (PERMISSION_STR.contains(permstr))
                perms.push_back(PERMISSION_STR.at(permstr));
            if (delim + 1 >= fperm.size() || delim == std::string::npos)
                break;
            fperm = fperm.substr(delim + 1);
        }
        cursor->permissions.insert_range(perms);
    }
}

bool Configuration::askPermission(std::string path, enum PermissionType pt)
{
    if (permission_root.name.empty())
        return true;
    PermissionNode *cursor = &permission_root;
    auto file              = std::filesystem::canonical(path).string();
    file                   = file.substr(1);
    while (cursor) {
        if (file.empty())
            return cursor->permissions.count(pt) > 0;
        auto slash        = file.find("/");
        auto current_name = file.substr(0, slash);
        if (cursor->name == current_name)
            return cursor->permissions.count(pt) > 0;
        if (!cursor->children.contains(current_name)) {
            return cursor->permissions.count(pt) > 0;
        }
        file   = file.substr(slash + 1);
        cursor = cursor->children[current_name];
    }
    return true;
}

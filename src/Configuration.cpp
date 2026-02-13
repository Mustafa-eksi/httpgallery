#include "Configuration.hpp"

Configuration::Configuration() { }

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

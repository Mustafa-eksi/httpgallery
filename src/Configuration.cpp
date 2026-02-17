#include "Configuration.hpp"

PermissionNode::PermissionNode() { permissions = UserPermissionMap(); }

PermissionNode::PermissionNode(std::string n)
    : name(n)
{
    permissions = UserPermissionMap();
}

void PermissionNode::addChild(std::string child_name)
{
    if (children.contains(child_name))
        return;
    children[child_name]
        = std::make_unique<PermissionNode>(PermissionNode(child_name));
    children[child_name]->parent = this;
}

void PermissionNode::addChildWithPerm(std::string child_name, std::string user,
                                      std::vector<enum PermissionType> ps)
{
    this->addChild(child_name);
    if (!this->children[child_name]->permissions.contains(user))
        this->children[child_name]->permissions[user]
            = std::set<enum PermissionType>();
    this->children[child_name]->permissions[user].insert_range(ps);
}

void PermissionNode::print(int level = 1)
{
    std::cout << "- " << name << std::endl;
    for (auto &[chk, ch] : children) {
        for (int i = 0; i < level; i++)
            std::cout << "  ";
        ch->print(level + 1);
    }
}

Configuration::Configuration()
{
    permission_root = std::make_unique<PermissionNode>(PermissionNode(""));
}

Configuration::Configuration(std::string config_path)
{
    permission_root = std::make_unique<PermissionNode>(PermissionNode(""));
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

PermissionSet Configuration::parsePermissions(std::string perm_list)
{
    PermissionSet perms;
    while (!perm_list.empty()) {
        auto delim   = perm_list.find(",");
        auto permstr = perm_list.substr(0, delim);
        if (PERMISSION_STR.contains(permstr))
            perms.insert(PERMISSION_STR.at(permstr));
        if (delim + 1 >= perm_list.size() || delim == std::string::npos)
            break;
        perm_list = perm_list.substr(delim + 1);
    }
    return perms;
}

void Configuration::createPermissionTree()
{
    if (!map.contains("permissions"))
        return;
    permission_root->name = "/";
    for (auto &[file_key, perm] : map["permissions"]) {
        PermissionNode *cursor = permission_root.get();
        std::string file;
        if (file_key.find("*") != std::string::npos)
            file = file_key;
        else
            file = std::filesystem::canonical(file_key).string();
        // canonical path is also an absolute path so it must start with '/'
        file = file.substr(1);
        std::string current_name;
        while (!file.empty()) {
            auto slash   = file.find("/");
            current_name = file.substr(0, slash);
            cursor->addChild(current_name);
            cursor = cursor->children[current_name].get();
            if (slash == std::string::npos || slash == file.size() - 1)
                break;
            file = file.substr(slash + 1);
        }
        UserPermissionMap perms;
        std::string fperm = std::get<0>(perm);
        while (!fperm.empty()) {
            auto username_end = fperm.find("{");
            auto perm_end     = fperm.find("}");
            if (username_end == std::string::npos
                || perm_end == std::string::npos)
                break;
            auto username = fperm.substr(0, username_end);
            auto permstr
                = fperm.substr(username_end + 1, perm_end - username_end - 1);
            perms[username] = parsePermissions(permstr);
            if (perm_end + 2 >= fperm.size())
                break;
            fperm = fperm.substr(perm_end + 2);
        }
        cursor->permissions = perms;
    }
}

bool Configuration::askPermission(std::string path, std::string username,
                                  enum PermissionType pt)
{
    if (permission_root->name.empty())
        return true;
    PermissionNode *cursor = permission_root.get();
    auto file              = std::filesystem::canonical(path).string();
    file                   = file.substr(1);
    while (cursor) {
        if (file.empty())
            return cursor->permissions[username].count(pt) > 0;
        auto slash        = file.find("/");
        auto current_name = file.substr(0, slash);
        if (cursor->name == current_name)
            return cursor->permissions[username].count(pt) > 0;
        if (!cursor->children.contains(current_name)) {
            for (auto &[n, p] : cursor->children) {
                if (n.find("*") != std::string::npos) {
                    auto art = n.find("*");
                    std::string pre, post;
                    pre = n.substr(0, art);
                    if (art + 1 < cursor->name.size())
                        post = n.substr(art + 1);
                    if (file.starts_with(pre) && file.ends_with(post))
                        return p->permissions[username].count(pt) > 0;
                    else
                        return cursor->permissions[username].count(pt) > 0;
                }
            }
        }
        file   = file.substr(slash + 1);
        cursor = cursor->children[current_name].get();
    }
    return true;
}

std::pair<bool, std::string> Configuration::authenticate(std::string userpass)
{
    auto decoded = base64_decode(userpass);
    auto colon   = decoded.find(":");
    if (colon == std::string::npos)
        return std::make_pair(false, "");
    auto username = decoded.substr(0, colon);
    auto password = decoded.substr(colon + 1);
    if (!map["users"].contains(username))
        return std::make_pair(false, "");
    if (std::get<0>(map["users"][username]) != password)
        return std::make_pair(false, "");
    return std::make_pair(true, username);
}

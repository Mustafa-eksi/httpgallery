#include "Logging.cpp"
#include <list>
#include <mutex>
#include <optional>
#include <unordered_map>

template <typename T, typename U> class FileCache {
    using File = T;
    using Key  = U;
    size_t cap;
    std::unordered_map<Key, std::pair<File, typename std::list<Key>::iterator>>
        cache;
    std::list<Key> ordered_keys;
    std::mutex m;

public:
    FileCache(size_t capacity)
        : cap(capacity) { };

    std::optional<T> get(U key)
    {
        std::lock_guard<std::mutex> lg(m);
        if (!this->cache.contains(key))
            return std::nullopt;
        this->ordered_keys.splice(this->ordered_keys.end(), this->ordered_keys,
                                  this->cache[key].second);
        return this->cache[key].first;
    }

    void put(U key, T value)
    {
        std::lock_guard<std::mutex> lg(m);
        if (this->cache.contains(key)) {
            this->cache[key].first = value;
            this->ordered_keys.splice(this->ordered_keys.end(),
                                      this->ordered_keys,
                                      this->cache[key].second);
        } else {
            if (this->cache.size() >= this->cap) {
                this->cache[this->ordered_keys.front()].first.clear();
                // FIXME: std::string only:
                this->cache[this->ordered_keys.front()].first.shrink_to_fit();
                this->cache.erase(this->ordered_keys.front());
                this->ordered_keys.pop_front();
            }
            this->ordered_keys.push_back(key);
            cache[key]
                = std::make_pair(value, std::prev(this->ordered_keys.end()));
        }
    }

    ~FileCache()
    {
        for (auto &[k, p] : cache) {
            p.first.clear();
            p.first.shrink_to_fit();
        }
    }
};

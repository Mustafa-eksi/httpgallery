#include <cstdint>
#include <list>
#include <optional>
#include <unordered_map>

/**
 * @brief Simple file entry struct for LRU caching.
 */
template <typename T, typename U> struct FileEntry {
    T data;
    std::list<U>::iterator pointer;
    std::pair<uintmax_t, uintmax_t> range;
};

/**
 * @brief Simple LRU Cache
 */
template <typename T, typename U> class FileCache {
    using File      = T;
    using Key       = U;
    using Range     = std::pair<uintmax_t, uintmax_t>;
    using FileEntry = FileEntry<File, Key>;
    size_t cap;
    std::unordered_map<Key, FileEntry> cache;
    std::list<Key> ordered_keys;

public:
    FileCache(size_t capacity)
        : cap(capacity) { };

    std::optional<FileEntry> get(U key)
    {
        if (!this->cache.contains(key))
            return std::nullopt;
        this->ordered_keys.splice(this->ordered_keys.end(), this->ordered_keys,
                                  this->cache[key].pointer);
        return this->cache[key];
    }

    void put(U key, T value, Range range)
    {
        if (this->cache.contains(key)) {
            this->cache[key]
                = (FileEntry) { .data    = value,
                                .pointer = this->cache[key].pointer,
                                .range   = range };
            this->ordered_keys.splice(this->ordered_keys.end(),
                                      this->ordered_keys,
                                      this->cache[key].pointer);
        } else {
            if (this->cache.size() >= this->cap) {
                this->cache[this->ordered_keys.front()].data.clear();
                // FIXME: std::string only:
                this->cache[this->ordered_keys.front()].data.shrink_to_fit();
                this->cache.erase(this->ordered_keys.front());
                this->ordered_keys.pop_front();
            }
            this->ordered_keys.push_back(key);
            cache[key] = (FileEntry) {
                .data    = value,
                .pointer = std::prev(this->ordered_keys.end()),
                .range   = range,
            };
        }
    }
    ~FileCache()
    {
        for (auto &[k, p] : cache) {
            p.data.clear();
            // FIXME: std::string only:
            p.data.shrink_to_fit();
        }
    }
};

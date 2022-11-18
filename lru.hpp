#include <mutex>
#include <string>

#include <tbb/concurrent_hash_map.h>

class LruCache {
    struct Node {
        Node() = default;
        explicit Node(const std::string& key) : key(key) {
        }
        bool inList() const noexcept {
            return !!prev;
        }

        void unlink() {
            prev->next = next;
            next->prev = prev;
            prev = nullptr;
            next = nullptr;
        }
        // to find in map on lru erase
        // TODO: can be string_view if map supports comparable types
        //       and if map do not copy values
        std::string key;
        Node* next{};
        Node* prev{};
    };

    struct Value {
        std::string val;
        mutable Node node{}; // modified on read
    };

    using HashMap = tbb::concurrent_hash_map<std::string, Value>;
    using ConstAccessor = typename HashMap::const_accessor;
    using Accessor = typename HashMap::accessor;

    explicit LruCache(size_t maxSize) : maxSize_(maxSize) {
        head_.next = &tail_;
        tail_.prev = &head_;
    }

public:
    static LruCache& get(size_t maxSize) {
        static LruCache* cache = new LruCache(maxSize);
        return *cache;
    }

    std::string resolve(const std::string& name) {
        ConstAccessor accessor;
        if(!map_.find(accessor, name))
            return std::string();

        updateLru(accessor->second.node);

        return accessor->second.val;
    }

    void update(const std::string& name, const std::string& ip) {
        Accessor accessor;
        if(!map_.insert(accessor, name)) {
            accessor->second.val = ip;
            return;
        }

        // value inserted - check max before insert to lru
        // or erase can try to delete new inserted value
        size_t size = size_.load();
        bool exceeded = size >= maxSize_;
        if(exceeded)
            eraseLru();

        // insert, then increment
        accessor->second.val = ip;
        accessor->second.node.key = name;
        {
            std::unique_lock lock(listMx_);
            pushFront(accessor->second.node);
        }

        // if not exceeded - size not changed
        if(!exceeded)
            size = size_++;

        // when N thread try to insert - size_ can exceed max by N
        // try to fix it on every insert
        if(size > maxSize_) {
            if(size_.compare_exchange_strong(size, size - 1))
                eraseLru();
        }
    }

private:
    void updateLru(Node& node) {
        std::unique_lock lock(listMx_, std::try_to_lock);
        if(lock) { // skip update on high contention
            // insert or erase in process
            if(node.inList()) {
                node.unlink();
                pushFront(node);
            }
        }
    }

    void eraseLru() {
        Node* node{};
        { // unlink lru list
            std::unique_lock lock(listMx_);
            node = tail_.prev;
            if(node == &head_) // empty list
                return;

            node->unlink();
        }
        map_.erase(node->key);
    }

    void pushFront(Node& node) {
        node.prev = &head_;
        node.next = head_.next;
        head_.next->prev = &node;
        head_.next = &node;
    }

    bool listEmpty() const {
        return tail_.prev == &head_;
    }

private:
    size_t maxSize_;
    std::atomic<size_t> size_{0};
    Node head_;
    Node tail_;
    std::mutex listMx_;
    HashMap map_;
};

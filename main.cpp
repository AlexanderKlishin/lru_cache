#include "lru.hpp"

int main(int /*argc*/, const char** /*argv*/) {
    auto& cache = LruCache::get(100);

    std::string res;

    res = cache.resolve("msk");
    if(!res.empty()) {
        printf("error not empty");
        return 1;
    }

    cache.update("msk", "127.0.0.1");

    res = cache.resolve("msk");
    if(res.empty()) {
        printf("error empty");
        return 1;
    }

    return 0;
}

_Pragma("once");
#include<vector>
struct BitVector{
    std::vector<bool> data_;
    BitVector(size_t size);
    bool next();
};
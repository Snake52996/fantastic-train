#include<includes/BitVector.hpp>
BitVector::BitVector(size_t size): data_(size, false){}
bool BitVector::next(){
    for(size_t i = 0; i < data_.size(); ++i){
        data_.at(i) = !data_.at(i);
        if(data_.at(i)) break;
    }
    for(size_t i = 0; i < data_.size(); ++i){
        if(data_.at(i)) return true;
    }
    return false;
}
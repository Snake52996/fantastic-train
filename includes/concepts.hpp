#include <concepts>
#include <type_traits>
template<typename T>
concept MCTreeSearchActionEncode = requires(T a){
    T::min;
    T::max;
    a.encode;
    std::is_same_v<decltype(T::min), decltype(T::max)>;
    std::is_same_v<decltype(T::min), decltype(a.encode)>;
    std::totally_ordered<decltype(T::min)>;
};
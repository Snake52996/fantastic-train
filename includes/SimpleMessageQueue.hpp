_Pragma("once");
#include<queue>
#include<mutex>
#include<semaphore>
template<typename Item_t>
class SimpleMessageQueue{
  private:
    std::counting_semaphore<> avaliable_items_;
    std::mutex access_mutex_;
    std::queue<Item_t> items_;
  public:
    SimpleMessageQueue(): avaliable_items_(0){}
    // disallow copy
    SimpleMessageQueue(const SimpleMessageQueue& other) = delete;
    SimpleMessageQueue& operator=(const SimpleMessageQueue& other) = delete;
    Item_t get(){
        this->avaliable_items_.acquire();
        std::scoped_lock lock(this->access_mutex_);
        Item_t result{this->items_.front()};
        this->items_.pop();
        return result;
    }
    void put(Item_t&& item){
        this->avaliable_items_.release();
        std::scoped_lock lock(this->access_mutex_);
        this->items_.emplace(item);
    }
};
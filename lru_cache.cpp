#include <iostream>
#include <unordered_map>
#include <vector>
#include <memory>
#include <stdexcept>
#include <utility>
#include <list>

template<typename T>
class LRUOrder{
  public:
    void put(int key, int value){
      auto it = cache_.find(key);
      if(it != cache_.end()){
      // this exits in the cache
        it->second.value_ = value;
        touch(key);
        return;
      }
      auto [it, inserted ] = cache_.emplace(key, Node{value, order_.begin()});
    
    }

  private:
    struct Node{
      T value_;
      std::list<int>::iterator list_iter_;
    };

    std::unordered_map<int, Node>cache_;
    std::list<int> order_;
    
    void touch(int key){
    // goal of touch is make it MRU 
    // prereq is we know this key exists in order
      auto it = cache_.find(key);
      if(it == cache_.end()) return;
      //order_.erase(key);
      //order_.push_front(key);
      order_.splice(order_.begin(), order_, it->second.value_);
      it->second.list_iter_ = order_.begin();
    }

};

template<typename T>
class LRU{
private:
  struct Task{
    T task_type_;
    int index_;
    Task* next = nullptr;
    Task* prev = nullptr;
  };
  size_t size_;
  size_t capacity_;
  
  Task* head_; Task* tail_;
  std::unordered_map<int, std::unique_ptr<Task>> Cache_;
  // head is MRU - RIGHT
  // tail is LRU - LEFT
  void removeNode(Task* task){
     

  }

  void addToFront(Task* task){

  }
  
  void moveToFront(Task* task){


  }

  void evictLRU();


public:
  LRU(size_t capacity) : size_(0), capacity_(capacity){}
  LRU(LRU &&) = default;
  LRU(const LRU &) = default;
  LRU &operator=(LRU &&) = default;
  LRU &operator=(const LRU &) = default;
  ~LRU();

 

  void put(int key, T value){


  }

  const T* get(int key){


  }
  



};



int main(int argc, char* argv[]){



}

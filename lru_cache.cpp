#include <iostream>
#include <unordered_map>
#include <vector>
#include <memory>
#include <stdexcept>
#include <utility>




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

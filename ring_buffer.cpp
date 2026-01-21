#include <iostream>
#include <vector>



template<typename T>
class RingBuffer{
  private:
    struct Slot{
      T value_;
      int index_;
    };
  
  Slot* buff_;
  size_t capacity_;
  int size_; 
  int head_;
  int tail_;  
  
  public:
  RingBuffer(size_t capacity) : size_(0), capacity_(capacity),
    head_(0), tail_(0)
  {   
    buff_ = new Slot[capacity_];
  }

  void push(T value){
    head_ = (head_ + 1) % capacity_;
    if(size_ == capacity_){
      tail_ = (tail_ + 1) % capacity_;
    }
    else{
      size_++;
    }
    buff_[head_].value_ = value;  
    buff_[head_].index_ = size_;
  }

  T pop(){
    if(size_ == 0 ){
      throw std::runtime_error("[pop] eror size == 0");
    }
    T val = buff_[tail_].value;
    tail_ = (tail_ + 1) % capacity_;
    size_--;
    return val;
  } 
  
  const T& front(){
    if(size_ == 0){
      throw std::runtime_error("[pop] eror size == 0");
    }
    T curr = buff_[head_].value;
    return curr;
  }
};


int main(int argc, char* argv[]){
    

  std::cout << "hello" << std::endl;
  return 0;
}

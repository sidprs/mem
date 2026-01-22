#include <iostream>


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
    if(size_ == capacity_){
      tail_ = (tail_ + 1) % capacity_;
    }
    else{
      size_++;
    }
    buff_[head_].value_ = value;  
    buff_[head_].index_ = size_;
    head_ = (head_ + 1) % capacity_;
  }

  T pop(){
    if(size_ == 0 ){
      throw std::runtime_error("[pop] eror size == 0");
    }
    T val = buff_[tail_].value_;
    tail_ = (tail_ + 1) % capacity_;
    size_--;
    return val;
  } 
  
  const T& front() const{
    if(size_ == 0){
      throw std::runtime_error("[pop] eror size == 0");
    }
    return buff_[head_].value_;
  }
  const T& back() const{
    if(size_ == 0){
      throw std::runtime_error("[pop] eror size == 0");
    }
    int last = (head_ - 1 + capacity_) % capacity_;
    return buff_[head_].value_;
;
  }
  
  RingBuffer& operator=(const RingBuffer&) = delete;
  RingBuffer(const RingBuffer&) = delete;


  RingBuffer(RingBuffer&& other) noexcept 
    :buff_(other.buff_), head_(other.head_), tail_(other.tail_),
     size_(other.size_), capacity_(other.capacity_)
  {
    other.buff_ = nullptr;
    other.size_ = 0;
    other.capacity_ = 0;
  }
  
  RingBuffer& operator=(RingBuffer&& other){
    if(this == &other) return *this;
   
    cleanup();
    //delete []buff_; // deleting the pointer variable to the heap
    this->buff_ = other.buff_;
    other.buff_ = nullptr;
    capacity_ = other.capacity_;
    head_ = other.head_;
    tail_ = other.tail_;
    size_ = other.size_;
    other.capacity_ = 0;
    other.head_ = other.tail_ = other.size_ = 0;

    return *this;
  }  

  void cleanup(){
    delete[] buff_;
    buff_ = nullptr;
  }
  ~RingBuffer() { cleanup(); }

};


int main(int argc, char* argv[]){
    

  std::cout << "hello" << std::endl;
  return 0;
}

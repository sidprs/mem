#include <iostream>

template<typename T>
class DynamicArray{
  private:
    T* buff_;
    size_t size_;
    size_t capacity_;
    
    bool check(int i){
      if(i > capacity_ || i < 0){
        return false;
      }
      return true; 
    }
    void resize(){
      capacity_ *= 2;
      T* newBuff = new T[capacity_];
      for (int i{}; i < size_; i++) {
          newBuff[i] = buff_[i];
      }
      delete[] buff_;
      buff_ = newBuff;
  
    }

public:
  explicit DynamicArray(int capacity): capacity_(capacity),
    size_(0)
  {
    buff_ = new T[(int)capacity_];
  }
  
  T get(int i){
    check(i);        
    return buff_[i];
  }
  void set(int i, int n){
    check(i);
    buff_[i] = n;
  }
  
  void pushback(int n){
    if(size_ == capacity_){
      resize();
    } 
    
    buff_[size_] = n;
    check(size_);
    size_++;
  }

  T popBack(){
   if(size_ > 0){
      size_--;
    }
    return buff_[size_];

  }

  size_t getSize(){
    return size_;
  }
  size_t getCapacity(){
    return capacity_;
  }


  DynamicArray(const DynamicArray&) = delete;
  DynamicArray(DynamicArray&&) = default;
  

  DynamicArray& operator=(const DynamicArray&) = delete;
  DynamicArray& operator=(DynamicArray&&) = default;


};



int main(int argc, char* argv[]){



  return 0;
}

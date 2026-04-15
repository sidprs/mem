#include <iostream>
#include <vector>
#include <unordered_map>




// jank detection 
// input is samples
// output is metrics
//


struct Sample{
  int64_t ts_us;   // monotonically increasing timestamp
  int32_t dur_us;  // gpu frame duration
};


struct Metrics {
  int64_t count = 0;
  int64_t jank_count = 0;
  double avg_dur_us = 0.0;
  int32_t p95_dur_us = 0;
  int32_t max_dur_us = 0;
};



class FrameTimeMonitor {
public:
  FrameTimeMonitor(int32_t window_ms, int32_t budget_us) : window_ms_(window_ms),
    budget_us_(budget_us), time_t(0)
  {
    


  }

  void addSample(const Sample& s){
    if(!Samples_.empty() && ) 


  }

  // Compute metrics over the current window (ending at newest sample).
  Metrics getMetrics() const{


  }

private:
  int64_t window_us_;
  int32_t budget_us_;
  size_t time_t;
  std::deque<Sample> Samples_;
  // You choose internal state.
};


int main(int argc, char* argv[]){



  return 0;
}

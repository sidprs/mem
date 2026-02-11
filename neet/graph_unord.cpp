#include <iostream>
#include <vector>
#include <unordered_map>

/*


    [implementation design]
  - using a unordered_map for network telemetry
  - making DFS, BFS and topological sort methods
  - lets make a packet a template of payload of type T 



*/


template<typename NodeT, typename WeightT = int>
class NetworkGraph{




public:
  struct Edge {  // This stays struct - it's pure data
        NodeT src;
        NodeT dst;
        WeightT weight;
        
        Edge(const NodeT& s, const NodeT& d, WeightT w = WeightT{}) 
            : src(s), dst(d), weight(w) {}
  };

  struct NodeInfo {
      NodeT id;
      std::string label;
      std::unordered_map<std::string, std::string> metadata;
      
      NodeInfo() = default;
      explicit NodeInfo(const NodeT& node_id, const std::string& node_label = "") 
          : id(node_id), label(node_label) {}
  };

    
    explicit NetworkGraph(bool directed = true) : is_directed(directed) {}
    
    void addNode(const NodeT& node, const std::string& label = "") {
        if (nodes.find(node) == nodes.end()) {
            nodes[node] = NodeInfo(node, label);
            adj_list[node]; // Ensure entry exists
        }
    }
    void addEdge(const NodeT& src, const NodeT& dst, WeightT weight){
      addNode(src);
      addNode(dst);
      
      adj_list[src][dst] = weight;
      if(is_directed){
        adj_list[dst][src] = weight;
      }
    }
  


private:
    // source : destination , weight
    std::unordered_map<NodeT, std::unordered_map<NodeT, WeightT>> adj_list;
    std::unordered_map<NodeT, NodeInfo> nodes;
    std::unordered_map<std::string, std::string> graph_metadata;
    bool is_directed;




};


int main(int argc, char* argv[]){
    NetworkGraph<std::string, double> network(true);
    
    // Add nodes with labels
    network.addNode("A", "Server A");
    network.addNode("B", "Server B");
    network.addNode("C", "Server C");
    network.addNode("D", "Server D");
    
    // Add weighted edges (latency in ms)
    network.addEdge("A", "B", 10.5);
    network.addEdge("B", "C", 5.2);
    network.addEdge("A", "C", 20.1);
    network.addEdge("C", "D", 3.7);
    


  std::cout << " hello " << std::endl;
  return 0;
}

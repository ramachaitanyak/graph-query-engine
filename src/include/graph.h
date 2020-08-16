#pragma once

#include <map>
#include <vector>
#include <functional>
#include <string>
#include <mutex>

#ifdef BAZEL_BUILD
#include "protos/graph.grpc.pb.h"
#else
#include "graph.grpc.pb.h"
#endif

using graph::Request;

namespace GraphQueryEngine {

class Graph {
  public:
    Graph(int nodes, std::vector<std::vector<uint32_t>> adj_list)
      : num_nodes(nodes), adjacency_list(adj_list) {}
    ~Graph() = default;
    class Edge {
      public:
        Edge(int in_src, int in_dest)
          :src(in_src), dest(in_dest){}
        int src;
        int dest;

        ~Edge() = default;
    };

    /*
     * Compute minimum edges between src and dest nodes
     * @param src, uint32_t representation of source node
     * @param dest, uint32_t representation of destination node
     * @return uint32_t number of minimum edges between src & dest
     */
    uint32_t MinEdgeBfs(int src, int dest);

  private:
    // Total number of nodes in the graph
    uint32_t num_nodes;
    // Adjacency list of edges for graph
    std::vector<std::vector<uint32_t>> adjacency_list;

};

using GraphSharedPtr = std::shared_ptr<Graph>;

class GraphEngine {

  public:
    ~GraphEngine() = default;
    
    std::string ProcessRequest(graph::Request& request);
  private:
    std::hash<std::string> hash_fn;
    std::string PostGraphRequest(graph::Request& request);
    std::string DeleteGraphRequest(graph::Request& request);
    std::string MinDistanceGraphRequest(graph::Request& request);
    std::mutex graph_db_mutex;
    std::map<uint64_t, GraphSharedPtr> graph_db;
};

using GraphEngineSharedPtr = std::shared_ptr<GraphEngine>;

} // end GraphQueryEngine

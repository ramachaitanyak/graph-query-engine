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
    Graph(int nodes, std::vector<std::vector<uint32_t>> adj_list, std::string name)
      : num_nodes(nodes), adjacency_list(adj_list), graph_name(name) {}
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
    // Name of the graph
    std::string graph_name;

};

using GraphSharedPtr = std::shared_ptr<Graph>;

class GraphEngine {

  public:
    ~GraphEngine() = default;
    
    std::string ProcessRequest(graph::Request& request);
  private:
    /*
     * Hash function to generate graph-ids based on graph names
     */
    std::hash<std::string> hash_fn;
    /*
     * Post graph request to submit a graph to server
     * @param request, contains type of request, number of nodes and adjancency list
     * @return returns a string indicating the state of operation
     */
    std::string PostGraphRequest(graph::Request& request);
    /*
     * Delete graph request to delete a graph from server
     * @param request, consisting the graph id to be deleted
     * @return returns a string indicating the state of operation
     *         success, collision and inexistent graphs are
     *         appropriately returned in the message
     */
    std::string DeleteGraphRequest(graph::Request& request);
    /*
     * Compute minimum distance between 2 nodes of a posted graph
     * @param request, consists of graph id, source and destination nodes
     * @return returns a string indicating the state of operation
     */
    std::string MinDistanceGraphRequest(graph::Request& request);
    // Mutex to guard graphdb against concurrent operations
    std::mutex graph_db_mutex;
    // graph db consisting of the graph id as key and the graph
    // as the value
    std::map<uint64_t, GraphSharedPtr> graph_db;
};

using GraphEngineSharedPtr = std::shared_ptr<GraphEngine>;

} // end GraphQueryEngine

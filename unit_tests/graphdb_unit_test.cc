#include "src/include/graph.h"
#include <iostream>
#include <limits>

#ifdef BAZEL_BUILD
#include "protos/graph.grpc.pb.h"
#else
#include "graph.grpc.pb.h"
#endif

using graph::GraphEngine;
using graph::Request;

using namespace GraphQueryEngine;

int main() {
  // Create a test graph engine client
  GraphEngineSharedPtr test_graph_engine =
      std::make_shared<GraphQueryEngine::GraphEngine>();

  {
    /*
     * Testcase-1 Post duplicate graphs
     */
    Request request;
    request.set_graph_name("test_graph1");
    request.set_graph_total_nodes(2);
    request.set_request_type(graph::POST_GRAPH);
    // Construct adjacency list for test_graph1
    graph::Edges *edge1 = request.add_adjacency_list();
    edge1->set_src(0);
    edge1->set_dest(1);

    graph::Edges *edge2 = request.add_adjacency_list();
    edge2->set_src(1);
    edge2->set_dest(0);

    test_graph_engine->ProcessRequest(request);
    // Requesting to add for the same graph must fail
    std::string result = test_graph_engine->ProcessRequest(request);
    if (result.compare("ERROR: Graph already in DB") == 0) {
      std::cout << "Testcase-1, Post duplicate graphs passed" << std::endl;
    } else {
      std::cout << "Testcase-1, Post duplicate graphs failed" << std::endl;
    }
  }

  {
    /*
     * Testcase-2 Delete non-existing graph
     */
    Request request;
    request.set_request_type(graph::DELETE_GRAPH);
    request.mutable_delete_graph()->set_map_id(1234); // Not there

    test_graph_engine->ProcessRequest(request);
    // Requesting to delete a non-existent graph must fail
    std::string result = test_graph_engine->ProcessRequest(request);
    if (result.compare("ERROR: Graph not present in DB") == 0) {
      std::cout << "Testcase-2, Delete non-existent graph passed" << std::endl;
    } else {
      std::cout << "Testcase-2, Delete non-existent graph failed" << std::endl;
    }
  }

  {
    /*
     * Testcase-3 Minimum distance from a node to itself is 0
     */
    Request request;
    request.set_graph_name("min_dest_graph");
    request.set_graph_total_nodes(2);
    request.set_request_type(graph::POST_GRAPH);
    // Construct adjacency list for min_dest_graph
    graph::Edges *edge1 = request.add_adjacency_list();
    edge1->set_src(0);
    edge1->set_dest(1);

    graph::Edges *edge2 = request.add_adjacency_list();
    edge2->set_src(1);
    edge2->set_dest(0);

    std::string result = test_graph_engine->ProcessRequest(request);
    uint64_t graph_id = std::stoull(result);

    // Request min distance from 0 to 0
    Request min_request;
    min_request.set_request_type(graph::GET_MIN_DISTANCE);
    min_request.mutable_min_distance()->set_begin_node(0);
    min_request.mutable_min_distance()->set_end_node(0);
    min_request.mutable_min_distance()->set_map_id(graph_id);

    std::string min_result = test_graph_engine->ProcessRequest(min_request);
    if (min_result.compare("OK, found minimum distance between 0 0 to be 0") ==
        0) {
      std::cout << "Testcase-3, Minimum distance from a node to itself passed"
                << std::endl;
    } else {
      std::cout << "Testcase-3, Minimum distance from a node to itself failed"
                << std::endl;
    }
  }

  {
    /*
     * Testcase-4 Minimum distance in a disconnected graph
     */
    Request request;
    request.set_graph_name("disconnected_graph");
    request.set_graph_total_nodes(4);
    request.set_request_type(graph::POST_GRAPH);
    // Construct adjacency list for disconnected graph
    graph::Edges *edge1 = request.add_adjacency_list();
    edge1->set_src(0);
    edge1->set_dest(1);

    graph::Edges *edge2 = request.add_adjacency_list();
    edge2->set_src(1);
    edge2->set_dest(0);

    graph::Edges *edge3 = request.add_adjacency_list();
    edge3->set_src(2);
    edge3->set_dest(3);

    graph::Edges *edge4 = request.add_adjacency_list();
    edge4->set_src(3);
    edge4->set_dest(2);

    std::string result = test_graph_engine->ProcessRequest(request);
    uint64_t graph_id = std::stoull(result);

    // Request min distance from 0 to 3, which must be
    // `std::numeric_limits<int>::max()`
    Request min_request;
    min_request.set_request_type(graph::GET_MIN_DISTANCE);
    min_request.mutable_min_distance()->set_begin_node(0);
    min_request.mutable_min_distance()->set_end_node(3);
    min_request.mutable_min_distance()->set_map_id(graph_id);

    std::string min_result = test_graph_engine->ProcessRequest(min_request);
    if (min_result.compare(
            "OK, found minimum distance between 0 3 to be 2147483647") == 0) {
      std::cout << "Testcase-4, Minimum distance in a disconnected graph passed"
                << std::endl;
    } else {
      std::cout << "Testcase-4, Minimum distance in a disconnected graph failed"
                << std::endl;
    }
  }
}

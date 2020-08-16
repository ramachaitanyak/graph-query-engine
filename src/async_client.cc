/*
 *
 * Copyright 2015 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <grpcpp/grpcpp.h>
#include <grpc/support/log.h>
#include <thread>

#include "include/graph.h"

#ifdef BAZEL_BUILD
#include "protos/graph.grpc.pb.h"
#else
#include "graph.grpc.pb.h"
#endif

using grpc::Channel;
using grpc::ClientAsyncResponseReader;
using grpc::ClientContext;
using grpc::CompletionQueue;
using grpc::Status;
using graph::Request;
using graph::Response;
using graph::GraphEngine;

class GraphEngineClient {
  public:
    explicit GraphEngineClient(std::shared_ptr<Channel> channel)
            : stub_(GraphEngine::NewStub(channel)) {}

    // Assembles the client's payload and sends it to the server.
    void PostGraphRequest(const std::string& graph_name,
        std::vector<GraphQueryEngine::Graph::Edge>& adj_list) {

        std::cout<<"adjlist size"<<adj_list.size()<<std::endl;
        // Data we are sending to the server.
        Request request;
        request.set_graph_name(graph_name);
        request.set_request_type(graph::POST_GRAPH);

        // Construct the adjacency list in protobuf format
        for (auto input_edge : adj_list) {
            graph::Edges* edge = request.add_adjacency_list();
            edge->set_src(input_edge.src);
            edge->set_dest(input_edge.dest);
        }

        // Call object to store rpc data
        AsyncClientCall* call = new AsyncClientCall;

        // stub_->PrepareAsyncSayHello() creates an RPC object, returning
        // an instance to store in "call" but does not actually start the RPC
        // Because we are using the asynchronous API, we need to hold on to
        // the "call" instance in order to get updates on the ongoing RPC.
        call->response_reader =
            stub_->PrepareAsyncGraphEngineRequest(&call->context, request, &cq_);

        // StartCall initiates the RPC call
        call->response_reader->StartCall();

        // Request that, upon completion of the RPC, "reply" be updated with the
        // server's response; "status" with the indication of whether the operation
        // was successful. Tag the request with the memory address of the call object.
        call->response_reader->Finish(&call->reply, &call->status, (void*)call);

    }

    // Assembles the client's payload for deleting a stored graph and sends it to server
    void DeleteGraphRequest(const uint32_t& graph_id) {
        Request request;
        request.set_request_type(graph::DELETE_GRAPH);
        request.mutable_delete_graph()->set_map_id(graph_id);

        // Call object to store rpc data
        AsyncClientCall* call = new AsyncClientCall;

        // stub_->PrepareAsyncSayHello() creates an RPC object, returning
        // an instance to store in "call" but does not actually start the RPC
        // Because we are using the asynchronous API, we need to hold on to
        // the "call" instance in order to get updates on the ongoing RPC.
        call->response_reader =
            stub_->PrepareAsyncGraphEngineRequest(&call->context, request, &cq_);

        // StartCall initiates the RPC call
        call->response_reader->StartCall();

        // Request that, upon completion of the RPC, "reply" be updated with the
        // server's response; "status" with the indication of whether the operation
        // was successful. Tag the request with the memory address of the call object.
        call->response_reader->Finish(&call->reply, &call->status, (void*)call);
    }

    // Assembles the client's payload for calculating the minimum distance between two
    // nodes in a stored graph, identified by graph_id
    void CalculateMinDistance(const uint32_t& graph_id,
        const uint32_t src, const uint32_t dest) {
        Request request;
        request.set_request_type(graph::GET_MIN_DISTANCE);
        request.mutable_min_distance()->set_begin_node(src);
        request.mutable_min_distance()->set_end_node(dest);
        request.mutable_min_distance()->set_map_id(graph_id);

        // Call object to store rpc data
        AsyncClientCall* call = new AsyncClientCall;

        // stub_->PrepareAsyncSayHello() creates an RPC object, returning
        // an instance to store in "call" but does not actually start the RPC
        // Because we are using the asynchronous API, we need to hold on to
        // the "call" instance in order to get updates on the ongoing RPC.
        call->response_reader =
            stub_->PrepareAsyncGraphEngineRequest(&call->context, request, &cq_);

        // StartCall initiates the RPC call
        call->response_reader->StartCall();

        // Request that, upon completion of the RPC, "reply" be updated with the
        // server's response; "status" with the indication of whether the operation
        // was successful. Tag the request with the memory address of the call object.
        call->response_reader->Finish(&call->reply, &call->status, (void*)call);
    }

    // Loop while listening for completed responses.
    // Prints out the response from the server.
    void AsyncCompleteRpc() {
        void* got_tag;
        bool ok = false;

        // Block until the next result is available in the completion queue "cq".
        while (cq_.Next(&got_tag, &ok)) {
            // The tag in this example is the memory location of the call object
            AsyncClientCall* call = static_cast<AsyncClientCall*>(got_tag);

            // Verify that the request was completed successfully. Note that "ok"
            // corresponds solely to the request for updates introduced by Finish().
            GPR_ASSERT(ok);

            if (call->status.ok())
                std::cout << "Greeter received: " << call->reply.message() << std::endl;
            else
                std::cout << "RPC failed" << std::endl;

            // Once we're complete, deallocate the call object.
            delete call;
        }
    }

  private:

    // struct for keeping state and data information
    struct AsyncClientCall {
        // Container for the data we expect from the server.
        Response reply;

        // Context for the client. It could be used to convey extra information to
        // the server and/or tweak certain RPC behaviors.
        ClientContext context;

        // Storage for the status of the RPC upon completion.
        Status status;


        std::unique_ptr<ClientAsyncResponseReader<Response>> response_reader;
    };

    // Out of the passed in Channel comes the stub, stored here, our view of the
    // server's exposed services.
    std::unique_ptr<GraphEngine::Stub> stub_;

    // The producer-consumer queue we use to communicate asynchronously with the
    // gRPC runtime.
    CompletionQueue cq_;
};

int main(int argc, char** argv) {


    // Instantiate the client. It requires a channel, out of which the actual RPCs
    // are created. This channel models a connection to an endpoint (in this case,
    // localhost at port 50051). We indicate that the channel isn't authenticated
    // (use of InsecureChannelCredentials()).
    GraphEngineClient graph_client(grpc::CreateChannel(
            "localhost:50051", grpc::InsecureChannelCredentials()));

    // Spawn reader thread that loops indefinitely
    std::thread thread_ = std::thread(&GraphEngineClient::AsyncCompleteRpc, &graph_client);

    // Post graph request
    // Adjacency List
    std::vector<GraphQueryEngine::Graph::Edge> adj_list;
    adj_list.push_back(GraphQueryEngine::Graph::Edge(0,1));
    adj_list.push_back(GraphQueryEngine::Graph::Edge(1,2));

    std::string graph_name = "site_network";
    graph_client.PostGraphRequest(graph_name, adj_list); // The actual RPC call!

    /*
    for (int i = 0; i < 100; i++) {
        std::string user("world " + std::to_string(i));
        graph_client.PostGraphRequest(user, adj_list);  // The actual RPC call!
    }
    */

    std::cout << "Press control-c to quit" << std::endl << std::endl;
    thread_.join();  //blocks forever

    return 0;
}

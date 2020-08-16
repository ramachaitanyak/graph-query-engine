# graph_query_engine

## Overview
Graph Query Engine with grpc (in C++).

Building with bazel.

## Building and Running the Code

To only build the server and client, run the following:
```
bazel build :async_server
bazel build :async_client
```

To build and run the server and client, run the following:
```
bazel run :async_server
bazel run :async_client
```

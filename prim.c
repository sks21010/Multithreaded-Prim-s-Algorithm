#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <limits.h>

// to compile: ` gcc -g -o test prim_starter.c `
// to run: ` ./test `

#define NUM_NODES 5          // number nodes in graph
#define MAX_WEIGHT 10        // max wt of an edge

// note: you do not need to use all of these variables. Please feel free to make your own vars and functions!

/* repr an edge in the graph */
typedef struct Edge {
    int dest;               // dest node ID
    int weight;             // wt of edge
    struct Edge* next;      // ptr to the next edge
} Edge;

/* repr a node in the graph */
typedef struct Node {
    int id;                 // node id
    Edge* edges;            // adj list of edges
    int in_mst;             // flag indicating if node in MST
    pthread_mutex_t lock;   // mutex to protect node
} Node;

/* thread arguments */
typedef struct {
    int node_id;            // node id to process
} ThreadArg;

/* candidate edges */
typedef struct {
    int src;
    int dest;
    int weight;
} CandidateEdge;

/* glob vars */
Node graph[NUM_NODES];                      // graph repr as array of nodes
int mst_edges[NUM_NODES - 1][2];            // edges in MST
int mst_weights[NUM_NODES - 1];             // wts of edges in MST
int mst_edge_count = 0;                     // num edges in MST
int total_mst_weight = 0;                   // tot wt of MST

CandidateEdge candidate_edges[NUM_NODES * NUM_NODES];
int candidate_count = 0;
pthread_mutex_t candidate_mutex = PTHREAD_MUTEX_INITIALIZER;

/* add edge to graph */
void add_edge(int src, int dest, int weight) {
    Edge* edge = (Edge*)malloc(sizeof(Edge));
    edge->dest = dest;
    edge->weight = weight;
    edge->next = graph[src].edges;
    graph[src].edges = edge;
}

/* initialize the graph */
void init_graph() {
    // init nodes
    for (int i = 0; i < NUM_NODES; i++) {
        graph[i].id = i;
        graph[i].edges = NULL;
        graph[i].in_mst = 0;
        pthread_mutex_init(&graph[i].lock, NULL);  // init mutex for each node
    }

    // init edges, undirected
    add_edge(0, 2, 9);
    add_edge(0, 1, 5);

    add_edge(1, 0, 5); 
    add_edge(1, 2, 10);
    add_edge(1, 3, 4);

    add_edge(2, 0, 9);
    add_edge(2, 1, 10);
    add_edge(2, 3, 7);
    add_edge(2, 4, 8);

    add_edge(3, 1, 4);
    add_edge(3, 2, 7);
    add_edge(3, 4, 9);

    add_edge(4, 2, 8);
    add_edge(4, 3, 9);
}

/* executed by each thread to find best candidate edge */
void* find_candidate_edge(void* arg) {

    /** 
    TODO:
        - Implement a multithreaded function to identify the best candidate edge for each node in the MST.
        - For the given node, check all edges to find the minimum-weight edge connecting to a node outside the MST.
        - Synchronize access to shared resources (e.g., candidate edges array) to update the global best candidate edge across all threads safely.
    */

    ThreadArg* thread_arg = (ThreadArg*)arg;
    int node_id = thread_arg->node_id;

    Edge* edge = graph[node_id].edges;
    CandidateEdge best_edge;
    best_edge.src = -1;
    best_edge.dest = -1;
    best_edge.weight = INT_MAX;

    while (edge != NULL) {
        if (!graph[edge->dest].in_mst && edge->weight < best_edge.weight) {
            best_edge.src = node_id;
            best_edge.dest = edge->dest;
            best_edge.weight = edge->weight;
        }
        edge = edge->next;
    }

    if (best_edge.src != -1) {
        pthread_mutex_lock(&candidate_mutex);
        candidate_edges[candidate_count++] = best_edge;
        pthread_mutex_unlock(&candidate_mutex);
    }

    return NULL;
}

/* perform Prim's algorithm to find MST */
void prim_mst(int start_node) {

    /** 
    TODO:
        - Implement Prim's algorithm to construct the Minimum Spanning Tree (MST) by repeatedly finding the minimum-weight edge that connects a node inside the MST to a node outside it.
        - Use multithreading to parallelize the edge selection process for nodes currently in the MST, synchronize access to shared resources, and update the MST incrementally until all nodes are included.
    **/

    printf("Starting Prim's algorithm from node %d\n", start_node);

    graph[start_node].in_mst = 1;
    int mst_nodes = 1;

    while (mst_nodes < NUM_NODES) {
        candidate_count = 0;

        pthread_t threads[NUM_NODES];
        ThreadArg thread_args[NUM_NODES];
        int thread_count = 0;

        for (int i = 0; i < NUM_NODES; i++) {
            if (graph[i].in_mst) {
                thread_args[thread_count].node_id = i;
                pthread_create(&threads[thread_count], NULL, find_candidate_edge, &thread_args[thread_count]);
                thread_count++;
            }
        }

        for (int i = 0; i < thread_count; i++) {
            pthread_join(threads[i], NULL);
        }

        CandidateEdge best_edge = candidate_edges[0];
        for (int i = 1; i < candidate_count; i++) {
            if (candidate_edges[i].weight < best_edge.weight) {
                best_edge = candidate_edges[i];
            }
        }

        if (best_edge.src != -1) {
            printf("Edge selected: %d --(%d)--> %d\n", best_edge.src, best_edge.weight, best_edge.dest);
            mst_edges[mst_edge_count][0] = best_edge.src;
            mst_edges[mst_edge_count][1] = best_edge.dest;
            mst_weights[mst_edge_count] = best_edge.weight;
            mst_edge_count++;
            total_mst_weight += best_edge.weight;

            graph[best_edge.dest].in_mst = 1;
            mst_nodes++;
        }
    }

    printf("\nMinimum Spanning Tree constructed. Total weight: %d\n", total_mst_weight);

}


int main() {
    init_graph();

    // graph structure
    printf("Graph:\n");
    for (int i = 0; i < NUM_NODES; i++) {
        printf("Node %d:", i);
        Edge* edge = graph[i].edges;
        while (edge != NULL) {
            printf(" -> %d(w=%d)", edge->dest, edge->weight);
            edge = edge->next;
        }
        printf("\n");
    }
    printf("\n");

    // start from node 0 (arbitrary)
    prim_mst(0);

    printf("Edges in MST:\n");
    for (int i = 0; i < mst_edge_count; i++) {
        printf("%d --(%d)--> %d\n", mst_edges[i][0], mst_weights[i], mst_edges[i][1]);
    }

    // clean up
    for (int i = 0; i < NUM_NODES; i++) {
        pthread_mutex_destroy(&graph[i].lock);

        Edge* edge = graph[i].edges;
        while (edge != NULL) {
            Edge* temp = edge;
            edge = edge->next;
            free(temp);
        }
    }
    pthread_mutex_destroy(&candidate_mutex);

    return 0;
}

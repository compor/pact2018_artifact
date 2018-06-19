#include "generate-samples.h"

#include <iostream>
#include <map>
#include <vector>
#include <sstream>

using namespace std;

std::map<std::string, int> recur_names;
int nrecurs = 0;


void print_graph(map<int, vector<EdgeInfo>> &g) {
	for (auto &gg: g) {
		for (auto &ggg: gg.second) {
			cout << gg.first << " " << ggg.dest_id << " " << ggg.type << endl;
		}
	}
}


int main() {

  string line;
  stringstream sin(line);

  map<int, vector<EdgeInfo>> graph;

  while (getline(cin, line)) {
    stringstream sin1(line);
    string from, to, type, coef, origin;
    sin1 >> from >> to >> type >> coef >> origin;

    int from_idx, to_idx;
    if (recur_names.find(from) == recur_names.end()) {
	    recur_names[from] = nrecurs++;
    } 
    from_idx = recur_names[from];

    if (recur_names.find(to) == recur_names.end()) {
	    recur_names[to] = nrecurs++;
    } 
    to_idx = recur_names[to];

    //cout << from_idx << " "  << to_idx << " " << type << " " << coef  << " " << origin << endl;
    EdgeInfo edge(to_idx, type, coef, origin);

    if (graph.find(from_idx) == graph.end()) {
    	graph[from_idx] = vector<EdgeInfo>();
    }


    graph[from_idx].push_back(edge);
  }

  print_graph(graph);

/*
  // Create test samples.
  vector<EdgeInfo> outgoing_edges_for_0;
  EdgeInfo edge00(0, "FST", "NULL", "0");
  EdgeInfo edge01(1, "FST", "NULL", "0");
  outgoing_edges_for_0.push_back(edge00);
  outgoing_edges_for_0.push_back(edge01);

  vector<EdgeInfo> outgoing_edges_for_1;
  EdgeInfo edge11(1, "LINR_P", "NULL", "0");
  outgoing_edges_for_1.push_back(edge11);

  map<int, vector<EdgeInfo>> graph;
  graph[0] = outgoing_edges_for_0;
  graph[1] = outgoing_edges_for_1;


  print_graph(graph);
  */

  map<int, vector<vector<SampleInfo>>> sample_results;
  GenerateSamples(graph, &sample_results);

//  print_results(sample_results);

  vector<vector<SampleInfo>> merged_samples;
  GenerateUnionResults(sample_results, &merged_samples);

  vector<vector<double>> matrix;

  ConvertToMatrix(merged_samples, &matrix);

  print_results(sample_results);
}

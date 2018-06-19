#ifndef GENERATE_SAMPLES_H_
#define GENERATE_SAMPLES_H_

#include <map>
#include <set>
#include <vector>
#include <string>

#define MY_TINY -999999
#define MY_SMALL -888888
#define MY_LARGE 888888
#define MY_HUGE 999999
#define PADDING_VALUE -1


struct EdgeInfo {
  int dest_id;
  std::string type;
  double *coefficient = nullptr;
  bool origin;

  EdgeInfo() {}

  EdgeInfo(int d, std::string t, std::string coef, std::string pass_origin) {
      dest_id = d;
      type = t;
      if (coef != "NULL") 
	      coefficient = new double(std::stod(coef));
      if (pass_origin == "1")
	      origin = true; 
      else origin = false;
  }
};

struct SampleInfo {
  int vertex_id;
  int sample_value;
};

// Generate incoming edges mapping for a graph.
void GenerateIncomingEdges(const std::map<int, std::vector<EdgeInfo>>& graph,
                           std::map<int, std::vector<int>>* incoming_edges);

// Generate samples for each outgoing edge, based on the edge info.
void GenerateSamplesForEdge(const EdgeInfo& edge, std::vector<double>* samples);

// Cross product all incoming edges' samples.
void GenerateCrossProductsForAllVertices(
    const std::map<int, std::set<double>>& samples,
    const std::map<int, std::vector<int>>& graph,
    std::vector<std::vector<SampleInfo>>* cross_samples);

// DS for the graph:
// vertex_id -> [EdgeInfo{dest_vertex, function_type, coefficient}]
// Return value:
//
// rec_v1|rec_v2|rec_v3...
//
// v1_s1  v2_s1  v3_s1
// v1_s2  v2_s2  v3_s2
// v1_s3  v2_s3  v3_s3
// ...
void GenerateSamples(const std::map<int, std::vector<EdgeInfo>>& graph,
                     std::map<int, std::vector<std::vector<SampleInfo>>>* samples);

// Merge two sample sets.
void MergeTwoSampleSets(const std::vector<std::vector<SampleInfo>>& left,
                        const std::vector<std::vector<SampleInfo>>& right,
                        std::vector<std::vector<SampleInfo>>* merged_samples);

// Generate merged results out of all sample sets.
void GenerateUnionResults(const std::map<int, std::vector<std::vector<SampleInfo>>>& samples,
                          std::vector<std::vector<SampleInfo>>* results);

// Convert the results to a matrix. The row number indicates the vertex ID.
void ConvertToMatrix(const std::vector<std::vector<SampleInfo>>& samples,
                     std::vector<std::vector<double>>* results);

void print_results(const std::vector<std::vector<double>>& samples);

void print_results(const std::vector<std::vector<SampleInfo>>& samples);

void print_results(const std::map<int, std::vector<std::vector<SampleInfo>>>& samples);

void print_results(const std::map<int, std::vector<int>>& samples);

void print_results(const std::map<int, std::set<double>>& samples);

#endif

#include "generate-samples.h"
#include <algorithm>
#include <assert.h>
#include <iostream>



using namespace std;

// Generate incoming edges mapping for a graph.
void GenerateIncomingEdges(const map<int, vector<EdgeInfo>>& graph,
                           map<int, vector<int>>* incoming_edges) {
    for (const auto& pair : graph) {
        int source = pair.first;
        const vector<EdgeInfo>& edge_vector = pair.second;
        for (const EdgeInfo& edge : edge_vector) {
            int dest = edge.dest_id;
            if (incoming_edges->find(dest) == incoming_edges->end()) {
                (*incoming_edges)[dest] = vector<int>();
            }
            (*incoming_edges)[dest].push_back(source);
        }
    }
}

void GenerateSamplesForEdge(const EdgeInfo& edge, vector<double>* samples) {
    if ((edge.type.find("LINR_P") != string::npos && edge.coefficient) || (edge.type.find("LINR_N") != string::npos && edge.coefficient)) {
        samples->push_back(MY_SMALL);
    } else if (edge.type.find("LINR_") != string::npos) {
        samples->push_back(MY_SMALL);
        samples->push_back(MY_LARGE);
    } else if (edge.type.find("RECT_") != string::npos) {

        if (edge.type.find(',') == string::npos) {
            if (edge.coefficient == nullptr) {
                if (!edge.origin) {  // the general case
                    samples->push_back(MY_TINY);
                    samples->push_back(MY_SMALL);
                    samples->push_back(MY_LARGE);
                    samples->push_back(MY_HUGE);
                } else {
                    samples->push_back(MY_SMALL);
                    samples->push_back(MY_LARGE);
                }
            } else if (!edge.origin) {
                samples->push_back(MY_SMALL);
                samples->push_back(MY_LARGE);
            } else if (edge.type == "RECT_A" || edge.type == "RECT_B"  ) {
                samples->push_back(MY_SMALL);
            } else if (edge.type == "RECT_C" || edge.type == "RECT_D"  ) {
                samples->push_back(MY_LARGE);
            } else {
                samples->push_back(MY_SMALL);
                samples->push_back(MY_LARGE);
            }
        } else {
            samples->push_back(MY_TINY);
            samples->push_back(MY_SMALL);
            samples->push_back(MY_LARGE);
            samples->push_back(MY_HUGE);
        }
    } else if (edge.type.find("FST") != string::npos) {
        samples->push_back(1);
        samples->push_back(0);
    }
}

void GenerateCrossProductsForAllVertices(
    const map<int, set<int>>& samples,
    const map<int, vector<int>>& graph, // graph represented as incoming edges
    map<int, vector<vector<SampleInfo>>>* resulting_samples) {
    for (const auto& pair : graph) {
        int dest = pair.first;
        const vector<int>& incoming_edges = pair.second;

        vector<vector<SampleInfo>> cross_samples[2];
        cross_samples[0].push_back(vector<SampleInfo>());

        // indicates which cross_samples is being read. !read is the one that is
        // being writen.
        int read = 0;

        for (int incoming_edge : incoming_edges) {
            const set<int>& edge_samples = samples.find(incoming_edge)->second;
            cross_samples[!read].clear();
            for (const auto& sample_vec : cross_samples[read]) {
                for (const auto& sample : edge_samples) {
                    SampleInfo info = {incoming_edge, sample};
                    vector<SampleInfo> sample_vec_tmp(sample_vec);
                    sample_vec_tmp.push_back(info);
                    cross_samples[!read].push_back(sample_vec_tmp);
                }
            }
            read = !read;
        }

        resulting_samples->insert(std::make_pair(dest, cross_samples[read]));
    }
}

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
void GenerateSamples(const map<int, vector<EdgeInfo>>& graph,
                     map<int, vector<vector<SampleInfo>>>* sample_results) {
    // 1. Generate samples for each outgoing edge.
    // 2. Union the samples associated to every vertex.
    // samples are keyed by the vertex ID
    map<int, set<int>> samples;
    for (const auto& pair : graph) {
        samples[pair.first] = set<int>();
        for (const auto& edge : pair.second) {
            vector<double> edge_samples;
            GenerateSamplesForEdge(edge, &edge_samples);
            samples[pair.first].insert(edge_samples.begin(), edge_samples.end());
        }
    }

    // print_results(samples);

    // 3. Get incoming edges for every vertex, and Cartesian product the samples
    // from all incoming edges, for each vertex.
    map<int, vector<int>> incoming_edges;
    GenerateIncomingEdges(graph, &incoming_edges);

    // cout << "Incoming edges: " << endl;
    // print_results(incoming_edges);

    GenerateCrossProductsForAllVertices(samples, incoming_edges, sample_results);

    vector<int> only_for_linr;
    for (auto& pair : graph) {
        bool only_linr = true;
        for (auto &st : pair.second) {
            if (st.type.find("LINR_") == string::npos) {
                only_linr = false;
                break;
            }
        }
        only_for_linr.push_back(pair.first);
    }

    for (auto& v : only_for_linr) {
        for (auto &sr : *sample_results) {
            for (auto &ss : sr.second) {
				for (auto &sss : ss) {
					if (sss.vertex_id == v) {
						if (sss.sample_value == MY_LARGE) sss.sample_value = 1;
						else if (sss.sample_value == MY_SMALL) sss.sample_value = 0;
					}	
				}
            }

        }
    }

}

// Tells whether two samples match by the columns that are indicated.
bool AreTwoSamplesMatch(const std::vector<SampleInfo>& left,
                        const std::vector<SampleInfo>& right,
                        const set<int>& columns) {
    map<int, double> sample_values_in_left;
    map<int, double> sample_values_in_right;
    for (const auto& left_sample : left) {
        if (columns.find(left_sample.vertex_id) != columns.end()) {
            sample_values_in_left.insert(std::make_pair(left_sample.vertex_id,
                                         left_sample.sample_value));
        }
    }
    for (const auto& right_sample : right) {
        if (columns.find(right_sample.vertex_id) != columns.end()) {
            sample_values_in_right.insert(std::make_pair(right_sample.vertex_id,
                                          right_sample.sample_value));
        }
    }
    return sample_values_in_left.size() == sample_values_in_right.size()
           && std::equal(sample_values_in_left.begin(), sample_values_in_left.end(),
                         sample_values_in_right.begin());
}

set<int> GetVerticesInCommon(const std::vector<SampleInfo>& line,
                             const std::vector<std::vector<SampleInfo>>& samples) {
    set<int> vertex_set_in_line;
    set<int> vertex_set_in_samples;

    for (const auto& sample_info : line) {
        vertex_set_in_line.insert(sample_info.vertex_id);
    }

    for (const auto& sample_info : samples[0]) {
        vertex_set_in_samples.insert(sample_info.vertex_id);
    }

    set<int> intersection;

    vector<int> vertices_in_common;

    // Get vertices in common.
    std::set_intersection(vertex_set_in_line.begin(), vertex_set_in_line.end(),
                          vertex_set_in_samples.begin(), vertex_set_in_samples.end(),
                          std::back_inserter(vertices_in_common));

    intersection.insert(vertices_in_common.begin(), vertices_in_common.end());

    return intersection;
}

// Get the line ID where line is to be inserted into in samples.
int GetInsertionLineNumber(const std::vector<SampleInfo>& line,
                           const std::vector<std::vector<SampleInfo>>& samples,
                           set<int>* lines_already_used) {
    assert(!samples.empty());

    set<int> intersection = GetVerticesInCommon(line, samples);

    for (unsigned i = 0; i < samples.size(); ++i) {
        if (lines_already_used->find(i) != lines_already_used->end()) {
            continue;
        }

        if (intersection.empty()) {
            lines_already_used->insert(i);
            return i;
        }

        // Match sample values against the columns in the intersection.
        if (AreTwoSamplesMatch(line, samples[i], intersection)) {
            lines_already_used->insert(i);
            return i;
        }
    }

    return -1;
}

void MergeTwoSamples(const std::vector<std::vector<SampleInfo>>& left,
                     const std::vector<std::vector<SampleInfo>>& right,
                     std::vector<std::vector<SampleInfo>>* merged_samples) {
    const auto& longer = left.size() > right.size() ? left : right;
    const auto& shorter = left.size() <= right.size() ? left : right;

    *merged_samples = longer;

    if (shorter.empty()) {
        return;
    }

    set<int> intersection = GetVerticesInCommon(shorter[0], longer);

    // Set including all lines that we already used.
    set<int> lines_already_used;

    for (const auto& shorter_entry : shorter) {
        int line_number = GetInsertionLineNumber(shorter_entry, longer, &lines_already_used);
        // Append shorter_entry to the entry in longer sample set, with line number
        // of line_number.
        for (const auto& sample_info : shorter_entry) {
            // If sample for a vertex is already there, don't append.
            if (intersection.find(sample_info.vertex_id) == intersection.end()) {
                (*merged_samples)[line_number].push_back(sample_info);
            }
        }
    }
}

// Assuming results is empty.
void GenerateUnionResults(const std::map<int, std::vector<std::vector<SampleInfo>>>& samples,
                          vector<vector<SampleInfo>>* results) {
    vector<vector<SampleInfo>> accumulated_results;
    for (const auto& sample : samples) {
        vector<vector<SampleInfo>> temp_results;
        MergeTwoSamples(sample.second, accumulated_results, &temp_results);
        accumulated_results = temp_results;
    }
    *results = accumulated_results;
}

void ConvertToMatrix(const vector<vector<SampleInfo>>& samples,
                     vector<vector<double>>* results) {
    int max_length = 0;
    for (const auto& sample : samples) {
        max_length = max(max_length, (int)(sample.size()));
    }
    for (const auto& sample : samples) {
        map<int, double> row_map;
        for (const auto& sample_info : sample) {
            row_map.insert(make_pair(sample_info.vertex_id,
                                     sample_info.sample_value));
        }
        vector<double> row;
        for (const auto& pair : row_map) {
            row.push_back(pair.second);
        }
        for (int i = row.size(); i < max_length; ++i) {
            row.push_back(PADDING_VALUE);
        }
        results->push_back(row);
    }
}

void print_results(const vector<vector<double>>& samples) {
    for (const auto& row : samples) {
        for (const auto& sample : row) {
            cout << sample << ", ";
        }
        cout << endl;
    }
}

void print_results(const vector<vector<SampleInfo>>& samples) {
    for (const auto& row : samples) {
        for (const auto& sample_info : row) {
            cout << "vertex" << sample_info.vertex_id;
            cout << " sample_value: " << sample_info.sample_value << ", ";
        }
        cout << endl;
    }
}

void print_results(const map<int, vector<vector<SampleInfo>>>& samples) {
    for (const auto& pair : samples) {
        cout << "Vertex" << pair.first << ": " << endl;
        for (const auto& sample_row : pair.second) {
            for (const auto& sample_info : sample_row) {
                cout << "vertex" << sample_info.vertex_id
                     << " sample_value: " << sample_info.sample_value << ", ";
            }
            cout << endl;
        }
        cout << endl;
        cout << endl;
    }
}

void print_results(const map<int, vector<int>>& samples) {
    for (const auto& pair : samples) {
        cout << "Vertex: " << pair.first << ": " << endl;
        for (const auto& sample : pair.second) {
            cout << sample << ", ";
        }
        cout << endl;
    }
}

void print_results(const map<int, set<double>>& samples) {
    for (const auto& pair : samples) {
        cout << "Vertex: " << pair.first << ": " << endl;
        for (const auto& sample : pair.second) {
            cout << sample << ", ";
        }
        cout << endl;
    }
}

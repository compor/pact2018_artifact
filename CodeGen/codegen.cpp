#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/raw_ostream.h"
#include "clang/AST/ASTContext.h"
#include "generate-samples.h"
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <map>
#include <cassert>

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::driver;
using namespace clang::tooling;
using namespace std;

static llvm::cl::OptionCategory MatcherSampleCategory("Matcher Sample");

std::map<std::string, int> recur_names;
int nrecurs = 0;
map<int, vector<EdgeInfo>> graph;
map<int, vector<vector<SampleInfo>>> sample_results;


map<string, string> name2type;

vector<vector<SampleInfo>> merged_samples;
int smpl_m = -1;


class ExprHandler : public MatchFinder::MatchCallback {
public:
    ExprHandler(Rewriter &Rewrite, string vname) : Rewrite(Rewrite), vname(vname) {}

    virtual void run(const MatchFinder::MatchResult &Result) {
        const DeclRefExpr *e = Result.Nodes.getNodeAs<DeclRefExpr>(vname);
        Rewrite.ReplaceText(SourceRange(e->getLocStart(), e->getLocEnd()),  "V" + vname + "[_i_s]");
    }

private:
    Rewriter &Rewrite;
    string vname;
};

class FuncHandler : public MatchFinder::MatchCallback {
public:
    FuncHandler(Rewriter &Rewrite) : Rewrite(Rewrite) {}


    string num_to_string(int num) {
        if (num == MY_TINY) return "MY_TINY";
        if (num == MY_SMALL) return "MY_SMALL";
        if (num == MY_LARGE) return "MY_LARGE";
        if (num == MY_HUGE) return "MY_HUGE";
        if (num == 1) return "true";
        if (num == 0) return "false";
        return "";
    }

    string getNamebyIdx(int idx) {
        for (auto &tt : recur_names) {
            if (tt.second == idx) return tt.first;
        }
        return "";
    }

    EdgeInfo getEdge(int incoming_node, int rv_idx) {
        for (auto &e : graph[incoming_node]) {
            if (e.dest_id == rv_idx) {
                return e;
            }
        }
        return EdgeInfo();
    }

    bool isConsistent(int r1, int r2, vector<int> &all_incoming_nodes, unsigned except) {
        if (r1 == r2) return false;
        for (unsigned i = 0; i < all_incoming_nodes.size(); i++) {
            if (i == except) {
                if (merged_samples[r1][i].sample_value == merged_samples[r2][i].sample_value) return false;
            } else {
                if (merged_samples[r1][i].sample_value != merged_samples[r2][i].sample_value) return false;
            }
        }
        return true;
    }

    bool isConsistent(int r1, int r2, vector<int> &all_incoming_nodes, unsigned except, vector<vector<SampleInfo>> &reduced_samples) {
        if (r1 == r2) return false;
        for (unsigned i = 0; i < all_incoming_nodes.size(); i++) {
            if (i == except) {
                if (reduced_samples[r1][i].sample_value == reduced_samples[r2][i].sample_value) return false;
            } else {
                if (reduced_samples[r1][i].sample_value != reduced_samples[r2][i].sample_value) return false;
            }
        }
        return true;
    }
    virtual void run(const MatchFinder::MatchResult &Result) {

        string original_decls = "";

        if ( const FunctionDecl *f = Result.Nodes.getNodeAs<FunctionDecl>("func")) {
            for (auto *p : f->parameters()) {
                for (auto &ss : recur_names) {
                    const string &s = ss.first;
                    if (s.compare(p->getName()) == 0) {
                        name2type[s] = p->getOriginalType().getAsString();
                        break;
                    }
                }
            }
            for (auto *p : f->getBody()->children()) {
                if (auto *d = dyn_cast<DeclStmt>(p)) {
                    if (auto *dd = dyn_cast<ValueDecl>(d->getSingleDecl())) {
                        for (auto &ss : recur_names) {
                            const string &s = ss.first;
                            if (s.compare(dd->getNameAsString()) == 0) {
                                name2type[s] = dd->getType().getAsString();
                                original_decls += Rewrite.getRewrittenText(d->getSourceRange());
                                original_decls += "\n";
                                Rewrite.RemoveText(d->getSourceRange());
                            }
                        }
                    }
                }
            }

            string decl = "/* Part One: Parallel Sampling  */\n";
            decl += string("const int _N_SAMP = ") + to_string(merged_samples.size()) + ";\n";


            for (auto &ss : recur_names) {
                decl += name2type[ss.first] + " S_V" + ss.first + "[_N_THREADS][_N_SAMP];\n";
            }

            decl += "int BSIZE = N / _N_THREADS;\n";
            decl += "thread *workers = new thread[_N_THREADS];\n";

            decl += "for (int tid = 0; tid < _N_THREADS; tid++) {\n";

            decl += "workers[tid] = thread([=] {\n";

            for (auto &ss : recur_names) {
                decl += name2type[ss.first] + " V" + ss.first + "[_N_THREADS] = {";

                bool first = true;
                bool found = false;
                for (auto & sm : merged_samples) {
                    if (!first && found) decl += ", ";
                    first = false;

                    for (auto &st : sm) {
                        if (st.vertex_id != recur_names[ss.first]) continue;
                        found = true;
                        decl += num_to_string(sm[recur_names[ss.first]].sample_value);
                        break;
                    }
                }
                if (!found) {
                    first = true;
                    for (int i = 0; i < smpl_m; i++) {
                        if (!first) decl += ", ";
                        first = false;
                        decl += "0";


                    }
                }
                decl += "}; \n";
            }

            decl += "int start_pos = tid * BSIZE;\n";
            decl += "int end_pos = min(tid * BSIZE, N);\n";




            Rewrite.ReplaceText(SourceRange(f->getNameInfo().getLocStart(), f->getNameInfo().getLocEnd()), "parallel");
            Rewrite.InsertTextBefore(f->getBody()->child_begin()->getLocStart(), decl);
        }

        if (const ForStmt *FS = Result.Nodes.getNodeAs<clang::ForStmt>("forLoop")) {
            string innerfor = "{ \n for (int _i_s=0; _i_s< _N_SAMP;  _i_s++) \n";


            const Stmt* tt;
            for ( auto *v : FS->getInit()->children()) {
                tt = v;
            }

            Rewrite.ReplaceText(SourceRange(tt->getLocStart(), tt->getLocEnd()), "start_pos");


            for (auto *v : FS->getCond()->children()) {
                tt = v;
            }

            Rewrite.ReplaceText(SourceRange(tt->getLocStart(), tt->getLocEnd()), "end_pos");


            Rewrite.InsertTextBefore(FS->getBody()->getLocStart(), innerfor);
            Rewrite.InsertTextAfter(FS->getBody()->getLocEnd(), "} // end sampling \n");


        }

        if ( const FunctionDecl *f = Result.Nodes.getNodeAs<FunctionDecl>("func")) {

            string mcopy = "";
            for (auto &ss : recur_names) {
                mcopy += "memcpy(S_V" + ss.first + "[tid], " + "V" + ss.first + ", sizeof(V" + ss.first + "));\n";
            }

            mcopy += "});\n";
            mcopy += "} // end threading \n";

            mcopy += "\n/* Part Two: Sequential Propagation */\n";

            mcopy += original_decls;

            mcopy += "for (tid=0; tid<_N_THREADS; tid++) {\n";
            mcopy += "workers[tid].join();\n";

            map<string, string> last_update;

            for (auto &rv : recur_names) {
                const string &rv_name = rv.first;
                int rv_idx = rv.second;

              //  cout << "rv_name: " << rv_name << endl;
		//		cout << "rv_idx: " << rv_idx << endl;

				// sampling points for rv
                auto &samples = sample_results[rv_idx];

                if (!samples.empty()) {

                    // get all variables that rv dependents on 
                    vector<int> all_incoming_nodes;
                    for (unsigned ts = 0; ts < samples[0].size(); ts++) {
                        all_incoming_nodes.push_back(samples[0][ts].vertex_id);
                    }

                    // find the corresponding edge in the graph, find the first edge
                    int incoming_node = all_incoming_nodes[0];
                    string incoming_rvname = getNamebyIdx(incoming_node);
                    EdgeInfo edge = getEdge(incoming_node, rv_idx);
		//			cout << "edge: " << incoming_node << " " << rv_idx << " " << edge.dest_id << " " << edge.type << endl;

                    vector<map<int, int>> groups;
                    for (unsigned i = 0; i < merged_samples.size(); i++) {
                        assert(merged_samples[i][incoming_node].vertex_id == incoming_node);
                        // the ith sample in the merged_samples matches this sample value, for
                        int svalue = merged_samples[i][incoming_node].sample_value;
                        if (svalue == PADDING_VALUE) continue;

          //              cout << "svalue: " << svalue << endl;
			//			cout << "all incoming nodes: ";
			//			for (auto &tn: all_incoming_nodes) cout << tn << " ";
			//			cout << endl;

                        bool inserted = false;
                        for (auto &g : groups) {
                            if (g.find(svalue) == g.end() && isConsistent(g.begin()->second, i, all_incoming_nodes, incoming_node)) {
                                g[svalue] = i;
                                inserted = true;
								break;
                            }
                        }
                        if (!inserted) {
                            map<int, int> g;
                            g[svalue] = i;
                            groups.push_back(g);
                        }
						/*
						cout << "groups: " << endl;
						for (auto &g: groups) {
							for (auto &gg: g) {
								cout << gg.first << " " << gg.second << endl;
							}		
							cout << endl;
						}*/
                    }

					// if a variable has only one incoming node, we only need to keep one sample group
					if (all_incoming_nodes.size() == 1) {
						for (unsigned ii=0; ii<groups.size()-1; ii++) {
							groups.pop_back();
						}
					}


                    //cout << "edge type: " << edge.type << endl;
                    //cout << "num samples: " << merged_samples.size() << endl;

              //      cout << "num groups: " << groups.size() << endl;


                    int tmp_idx = 0;
                    for (auto &sample_rows : groups) {

                        last_update[rv_name] = rv_name + incoming_rvname + to_string(tmp_idx);

				//		cout << edge.type << endl;

                        if ((edge.type.find("LINR_P") != string::npos && edge.coefficient) || (edge.type.find("LINR_N") != string::npos && edge.coefficient)) {
                            assert(sample_rows.find(MY_LARGE) != sample_rows.end());
                            mcopy += name2type[rv_name] + " " + rv_name + incoming_rvname + to_string(tmp_idx) + " = " + to_string(*edge.coefficient)  + " * " + incoming_rvname + "  + S_V" + rv_name + "[tid][" + to_string(sample_rows[MY_LARGE]) + "] - MY_LARGE;\n";

                        } else if (edge.type.find("LINR_") != string::npos) {
                            assert(sample_rows.size() == 2);
                            mcopy += name2type[rv_name] + " " + rv_name + incoming_rvname + to_string(tmp_idx) + " = " + "Linr(S_V" + rv_name + "[tid][" + to_string(sample_rows[MY_SMALL]) + "], S_V" + rv_name + "[tid][" + to_string(sample_rows[MY_LARGE]) + "], MY_SMALL, MY_LARGE, " + incoming_rvname + ");\n";
                        } else if (edge.type.find("RECT_") != string::npos) {

                            if (edge.type.find(',') == string::npos) {
                                if (edge.coefficient == nullptr) {
                                    if (!edge.origin) {  // the general case
                                        assert(sample_rows.size() == 4);
                                        mcopy += name2type[rv_name] + " " + rv_name + incoming_rvname + to_string(tmp_idx) + " = " + "Rectf(MY_TINY, S_V " + rv_name + "[tid][" + to_string(sample_rows[MY_TINY]) + "], MY_SMALL, S_V" + rv_name + "[tid][" + to_string(sample_rows[MY_SMALL]) + "], MY_LARGE, S_V" + rv_name + "[tid][" + to_string(sample_rows[MY_LARGE]) + "], MY_HUGE, S_V" + rv_name + "[tid][" + to_string(sample_rows[MY_LARGE]) + "], " + incoming_rvname + ");\n";

                                    } else {
                                        // passes origin, unkown coef
                                        assert(sample_rows.size() == 2);
                                        if (edge.type == "RECT_A") {
                                            mcopy += name2type[rv_name] + " " + rv_name + incoming_rvname + to_string(tmp_idx) + " = " + "Rectf_max(" + "S_V" + rv_name + "[tid][" + to_string(sample_rows[MY_SMALL]) + "], (" + incoming_rvname + " * " + "S_V" + rv_name + "[tid][" + to_string(sample_rows[MY_LARGE]) + "] / MY_LARGE));\n";

                                        } else if (edge.type == "RECT_B") {
                                            mcopy += name2type[rv_name] + " " + rv_name + incoming_rvname + to_string(tmp_idx) + " = " + "Rectf_min(" + "S_V" + rv_name + "[tid][" + to_string(sample_rows[MY_SMALL]) + "], (" + incoming_rvname + " * " + "S_V" + rv_name + "[tid][" + to_string(sample_rows[MY_LARGE]) + "] / MY_LARGE));\n";

                                        } else if (edge.type == "RECT_C") {
                                            mcopy += name2type[rv_name] + " " + rv_name + incoming_rvname + to_string(tmp_idx) + " = " + "Rectf_max(" + "S_V" + rv_name + "[tid][" + to_string(sample_rows[MY_LARGE]) + "], (" + incoming_rvname + " * " + "S_V" + rv_name + "[tid][" + to_string(sample_rows[MY_SMALL]) + "] / MY_SMALL));\n";

                                        } else if (edge.type == "RECT_D") {
                                            mcopy += name2type[rv_name] + " " + rv_name + incoming_rvname + to_string(tmp_idx) + " = " + "Rectf_min(" + "S_V" + rv_name + "[tid][" + to_string(sample_rows[MY_LARGE]) + "], (" + incoming_rvname + " * " + "S_V" + rv_name + "[tid][" + to_string(sample_rows[MY_SMALL]) + "] / MY_SMALL));\n";
                                        } else {
                                            cerr << "rect e or f, but the coefficent is unknown!" << endl;
                                            exit(-1);
                                        }
                                    }
                                } else if (!edge.origin) {
                                    // known coef but not pass origin
                                    if (edge.type == "RECT_A") {

                                        mcopy +=  name2type[rv_name] + " " + rv_name + incoming_rvname + to_string(tmp_idx) + " = " + "Rectf_max(" + "S_V" + rv_name + "[tid][" + to_string(sample_rows[MY_SMALL]) + "], (" + incoming_rvname + " + " + "S_V" + rv_name + "[tid][" + to_string(sample_rows[MY_LARGE]) + "] - " + to_string(*edge.coefficient) + " * MY_LARGE));\n";

                                    } else if (edge.type == "RECT_B") {

                                        mcopy += name2type[rv_name] + " " + rv_name + incoming_rvname + to_string(tmp_idx) + " = " + "Rectf_min(" + "S_V" + rv_name + "[tid][" + to_string(sample_rows[MY_SMALL]) + "], (" + incoming_rvname + " + " + "S_V" + rv_name + "[tid][" + to_string(sample_rows[MY_LARGE]) + "] - " + to_string(*edge.coefficient) + " * MY_LARGE));\n";

                                    } else if (edge.type == "RECT_C") {
                                        mcopy += name2type[rv_name] + " " + rv_name + incoming_rvname + to_string(tmp_idx) + " = " + "Rectf_max(" + "S_V" + rv_name + "[tid][" + to_string(sample_rows[MY_LARGE]) + "], (" + incoming_rvname + " + " + "S_V" + rv_name + "[tid][" + to_string(sample_rows[MY_SMALL]) + "] - " + to_string(*edge.coefficient) + " * MY_SMALL));\n";

                                    } else if (edge.type == "RECT_D") {
                                        mcopy += name2type[rv_name] + " " + rv_name + incoming_rvname + to_string(tmp_idx) + " = " + "Rectf_min(" + "S_V" + rv_name + "[tid][" + to_string(sample_rows[MY_LARGE]) + "], (" + incoming_rvname + " + " + "S_V" + rv_name + "[tid][" + to_string(sample_rows[MY_SMALL]) + "] - " + to_string(*edge.coefficient) + " * MY_SMALL));\n";

                                    } else {
                                        cerr << "rect e or f, but do not know if it passes origin!" << endl;
                                        exit(-1);
                                    }
                                    // known ceof, and passes origin
                                } else if (edge.type == "RECT_A") {
                                    mcopy += name2type[rv_name] + " " + rv_name + incoming_rvname + to_string(tmp_idx) + " = " + "Rectf_max(" + "S_V" + rv_name + "[tid][" + to_string(sample_rows[MY_SMALL]) + "], (" + to_string(*edge.coefficient) + " * " + incoming_rvname +     "));\n";
                                } else if (edge.type == "RECT_B") {
                                    mcopy += name2type[rv_name] + " " + rv_name + incoming_rvname + to_string(tmp_idx) + " = " + "Rectf_min(" + "S_V" + rv_name + "[tid][" + to_string(sample_rows[MY_SMALL]) + "], (" + to_string(*edge.coefficient) + " * " + incoming_rvname +     "));\n";
                                } else if (edge.type == "RECT_C") {
                                    mcopy +=  name2type[rv_name] + " " + rv_name + incoming_rvname + to_string(tmp_idx) + " = " + "Rectf_max(" + "S_V" + rv_name + "[tid][" + to_string(sample_rows[MY_LARGE]) + "], (" + to_string(*edge.coefficient) + " * " + incoming_rvname +     "));\n";
                                } else if (edge.type == "RECT_D") {
                                    mcopy += name2type[rv_name] + " " + rv_name + incoming_rvname + to_string(tmp_idx) + " = " + "Rectf_min(" + "S_V" + rv_name + "[tid][" + to_string(sample_rows[MY_LARGE]) + "], (" + to_string(*edge.coefficient) + " * " + incoming_rvname +     "));\n";
                                } else {
                                    assert(*edge.coefficient == 1);
                                    if (edge.type == "RECT_E") {
                                        mcopy +=  name2type[rv_name] + " " + rv_name + incoming_rvname + to_string(tmp_idx) + " = (" + incoming_rvname + " > 0 ? " + "Rectf_min(" + "S_V" + rv_name + "[tid][" + to_string(sample_rows[MY_LARGE]) + "], " + incoming_rvname + ") : Rectf_max(S_V" + rv_name + "[tid][" + to_string(sample_rows[MY_SMALL]) +  "], " + incoming_rvname +  "));\n";

                                    } else if (edge.type == "RECT_F") {
                                        mcopy +=  name2type[rv_name] + " " + rv_name + incoming_rvname + to_string(tmp_idx) + " = (" + incoming_rvname + " > 0 ? " + "Rectf_max(" + "S_V" + rv_name + "[tid][" + to_string(sample_rows[MY_LARGE]) + "], " + incoming_rvname + ") : Rectf_min(S_V" + rv_name + "[tid][" + to_string(sample_rows[MY_SMALL]) +  "], " + incoming_rvname +  "));\n";
                                    }
                                }
                            } else if (edge.type.find("RECT_E") == string::npos && edge.type.find("RECT_F") == string::npos ) {
                                assert(sample_rows.size() == 4);
                                mcopy += name2type[rv_name] + " " + rv_name + incoming_rvname + to_string(tmp_idx) + " = " + "Rectf(MY_TINY, S_V " + rv_name + "[tid][" + to_string(sample_rows[MY_TINY]) + "], MY_SMALL, S_V" + rv_name + "[tid][" + to_string(sample_rows[MY_SMALL]) + "], MY_LARGE, S_V" + rv_name + "[tid][" + to_string(sample_rows[MY_LARGE]) + "], MY_HUGE, S_V" + rv_name + "[tid][" + to_string(sample_rows[MY_LARGE]) + "], " + incoming_rvname + ");\n";
                            }
                        } else if (edge.type.find("FST") != string::npos) {
                            //assert(sample_rows.size() == 2);
                            mcopy +=  name2type[rv_name] + " " + rv_name + incoming_rvname + to_string(tmp_idx) + ";\n" +  "if (" + incoming_rvname + " == 0) {" + rv_name + incoming_rvname + to_string(tmp_idx)  + " = S_V" + rv_name + "[tid][" + to_string(sample_rows[0]) + "];  }\n " +
                                      "else {" + rv_name + incoming_rvname + to_string(tmp_idx) + " = S_V" + rv_name + "[tid][" + to_string(sample_rows[1]) + "];  }\n " ;
                        }
                        tmp_idx++;

                    }



                    // cout << "num reduce samples: " << reduced_samples.size() << endl;
                    //print_results(reduced_samples);

                    vector<map<int, int>> groups_local = groups;
                    for (unsigned ts = 1; ts < all_incoming_nodes.size(); ts++) {
                        // find the corresponding edge in the graph
                        int incoming_node = all_incoming_nodes[ts];
                        string incoming_rvname = getNamebyIdx(incoming_node);
                        EdgeInfo edge = getEdge(incoming_node, rv_idx);
                        string previous_rv = getNamebyIdx(all_incoming_nodes[ts - 1]);
                        vector<vector<SampleInfo>> reduced_samples;

                        for (auto &g : groups_local) {
                            vector<SampleInfo> t;
                            for (auto &ss : merged_samples[g.begin()->second]) {
                                t.push_back(ss);
                            }
                            reduced_samples.push_back(t);
                        }
                        groups_local.clear();
                        for (unsigned i = 0; i < reduced_samples.size(); i++) {
                            assert(reduced_samples[i][incoming_node].vertex_id == incoming_node);
                            int svalue = reduced_samples[i][incoming_node].sample_value;
                            if (svalue == PADDING_VALUE) continue;

                            bool inserted = false;
                            for (auto &g : groups_local) {
                                if (g.find(svalue) == g.end() && isConsistent(g.begin()->second, i, all_incoming_nodes, ts, reduced_samples)) {
                                    g[svalue] = i;
                                    inserted = true;
									break;
                                }
                            }
                            if (!inserted) {
                                map<int, int> g;
                                g[svalue] = i;
                                groups_local.push_back(g);
                            }
                        }




                        int tmp_idx = 0;

                        for (auto &sample_rows : groups_local) {

                            last_update[rv_name] = rv_name + incoming_rvname + to_string(tmp_idx);


                            if ((edge.type.find("LINR_P") != string::npos && edge.coefficient) || (edge.type.find("LINR_N") != string::npos && edge.coefficient)) {
                                assert(sample_rows.size() == 1 && sample_rows.find(MY_LARGE) != sample_rows.end());
                                mcopy += name2type[rv_name] + " " + rv_name + incoming_rvname + to_string(tmp_idx) + " = " + to_string(*edge.coefficient)  + " * " + incoming_rvname + "  + " + rv_name + previous_rv + to_string(sample_rows[MY_LARGE]) + " - MY_LARGE;\n";

                            } else if (edge.type.find("LINR_") != string::npos) {
                                assert(sample_rows.size() == 2);
                                mcopy += name2type[rv_name] + " " + rv_name + incoming_rvname + to_string(tmp_idx) + " = " + "Linr(" + rv_name + previous_rv + to_string(sample_rows[MY_SMALL]) + ", " + rv_name + previous_rv  + to_string(sample_rows[MY_LARGE]) + ", MY_SMALL, MY_LARGE, " + incoming_rvname + ");\n";
                            } else if (edge.type.find("RECT_") != string::npos) {

                                if (edge.type.find(',') == string::npos) {
                                    if (edge.coefficient == nullptr) {
                                        if (!edge.origin) {  // the general case
                                            assert(sample_rows.size() == 4);
                                            mcopy += name2type[rv_name] + " " + rv_name + incoming_rvname + to_string(tmp_idx) + " = " + "Rectf(MY_TINY, " + rv_name + previous_rv + to_string(sample_rows[MY_TINY]) + ", MY_SMALL, " + rv_name + previous_rv + to_string(sample_rows[MY_SMALL]) + ", MY_LARGE, " + rv_name + previous_rv + to_string(sample_rows[MY_LARGE]) + ", MY_HUGE, " + rv_name + previous_rv + to_string(sample_rows[MY_LARGE]) + ", " + incoming_rvname + ");\n";

                                        } else {
                                            // passes origin, unkown coef
                                            assert(sample_rows.size() == 2);
                                            if (edge.type == "RECT_A") {
                                                mcopy +=  name2type[rv_name] + " " + rv_name + incoming_rvname + to_string(tmp_idx) + " = " + "Rectf_max(" + rv_name + previous_rv + to_string(sample_rows[MY_SMALL]) + ", (" + incoming_rvname + " * " + rv_name + previous_rv + to_string(sample_rows[MY_LARGE]) + " / MY_LARGE));\n";

                                            } else if (edge.type == "RECT_B") {
                                                mcopy += name2type[rv_name] + " " + rv_name + incoming_rvname + to_string(tmp_idx) + " = " + "Rectf_min(" + rv_name + previous_rv + to_string(sample_rows[MY_SMALL]) + ", (" + incoming_rvname + " * " + rv_name + previous_rv + to_string(sample_rows[MY_LARGE]) + " / MY_LARGE));\n";

                                            } else if (edge.type == "RECT_C") {
                                                mcopy +=  name2type[rv_name] + " " + rv_name + incoming_rvname + to_string(tmp_idx) + " = " + "Rectf_max(" + rv_name + previous_rv + to_string(sample_rows[MY_LARGE]) + ", (" + incoming_rvname + " * " + rv_name + previous_rv + to_string(sample_rows[MY_SMALL]) + " / MY_SMALL));\n";

                                            } else if (edge.type == "RECT_D") {
                                                mcopy += name2type[rv_name] + " " + rv_name + incoming_rvname + to_string(tmp_idx) + " = " + "Rectf_min(" + rv_name + previous_rv + to_string(sample_rows[MY_LARGE]) + ", (" + incoming_rvname + " * " + rv_name + previous_rv + to_string(sample_rows[MY_SMALL]) + " / MY_SMALL));\n";
                                            } else {
                                                cerr << "rect e or f, but the coefficent is unknown!" << endl;
                                                exit(-1);
                                            }
                                        }
                                    } else if (!edge.origin) {
                                        // known coef but not pass origin
                                        if (edge.type == "RECT_A") {

                                            mcopy += name2type[rv_name] + " " + rv_name + incoming_rvname + to_string(tmp_idx) + " = " + "Rectf_max(" + rv_name + previous_rv + to_string(sample_rows[MY_SMALL]) + ", (" + incoming_rvname + " + " + rv_name + previous_rv + to_string(sample_rows[MY_LARGE]) + " - " + to_string(*edge.coefficient) + " * MY_LARGE));\n";

                                        } else if (edge.type == "RECT_B") {

                                            mcopy +=  name2type[rv_name] + " " + rv_name + incoming_rvname + to_string(tmp_idx) + " = " + "Rectf_min(" + rv_name + previous_rv + to_string(sample_rows[MY_SMALL]) + ", (" + incoming_rvname + " + " + rv_name + previous_rv + to_string(sample_rows[MY_LARGE]) + " - " + to_string(*edge.coefficient) + " * MY_LARGE));\n";

                                        } else if (edge.type == "RECT_C") {
                                            mcopy += name2type[rv_name] + " " + rv_name + incoming_rvname + to_string(tmp_idx) + " = " + "Rectf_max(" + rv_name + previous_rv + to_string(sample_rows[MY_LARGE]) + ", (" + incoming_rvname + " + " + rv_name + previous_rv + to_string(sample_rows[MY_SMALL]) + " - " + to_string(*edge.coefficient) + " * MY_SMALL));\n";

                                        } else if (edge.type == "RECT_D") {
                                            mcopy += name2type[rv_name] + " " + rv_name + incoming_rvname + to_string(tmp_idx) + " = " + "Rectf_min(" + rv_name + previous_rv + to_string(sample_rows[MY_LARGE]) + ", (" + incoming_rvname + " + " + rv_name + previous_rv + to_string(sample_rows[MY_SMALL]) + " - " + to_string(*edge.coefficient) + " * MY_SMALL));\n";

                                        } else {
                                            cerr << "rect e or f, but do not know if it passes origin!" << endl;
                                            exit(-1);
                                        }
                                        // known ceof, and passes origin
                                    } else if (edge.type == "RECT_A") {
                                        mcopy += name2type[rv_name] + " " + rv_name + incoming_rvname + to_string(tmp_idx) + " = " + "Rectf_max(" + rv_name + previous_rv + to_string(sample_rows[MY_SMALL]) + ", (" + to_string(*edge.coefficient) + " * " + incoming_rvname +     "));\n";
                                    } else if (edge.type == "RECT_B") {
                                        mcopy += name2type[rv_name] + " " + rv_name + incoming_rvname + to_string(tmp_idx) + " = " + "Rectf_min(" + rv_name + previous_rv + to_string(sample_rows[MY_SMALL]) + ", (" + to_string(*edge.coefficient) + " * " + incoming_rvname +     "));\n";
                                    } else if (edge.type == "RECT_C") {
                                        mcopy += name2type[rv_name] + " " + rv_name + incoming_rvname + to_string(tmp_idx) + " = " + "Rectf_max(" + rv_name + previous_rv + to_string(sample_rows[MY_LARGE]) + ", (" + to_string(*edge.coefficient) + " * " + incoming_rvname +     "));\n";
                                    } else if (edge.type == "RECT_D") {
                                        mcopy += name2type[rv_name] + " " + rv_name + incoming_rvname + to_string(tmp_idx) + " = " + "Rectf_min(" + rv_name + previous_rv + to_string(sample_rows[MY_LARGE]) + ", (" + to_string(*edge.coefficient) + " * " + incoming_rvname +     "));\n";
                                    } else {
                                        assert(*edge.coefficient == 1);
                                        if (edge.type == "RECT_E") {
                                            mcopy += name2type[rv_name] + " " + rv_name + incoming_rvname + to_string(tmp_idx) + " = (" + incoming_rvname + " > 0 ? " + "Rectf_min(" + rv_name + previous_rv + to_string(sample_rows[MY_LARGE]) + ", " + incoming_rvname + ") : Rectf_max(" + rv_name + previous_rv + to_string(sample_rows[MY_SMALL]) +  ", " + incoming_rvname +  "));\n";

                                        } else if (edge.type == "RECT_F") {
                                            mcopy += name2type[rv_name] + " " + rv_name + incoming_rvname + to_string(tmp_idx) + " = (" + incoming_rvname + " > 0 ? " + "Rectf_max(" + rv_name + previous_rv + to_string(sample_rows[MY_LARGE]) + ", " + incoming_rvname + ") : Rectf_min(" + rv_name + previous_rv + to_string(sample_rows[MY_SMALL]) +  ", " + incoming_rvname +  "));\n";
                                        }
                                    }
                                } else if (edge.type.find("RECT_E") == string::npos && edge.type.find("RECT_F") == string::npos ) {
                                    assert(sample_rows.size() == 4);
                                    mcopy += name2type[rv_name] + " " + rv_name + incoming_rvname + to_string(tmp_idx) + " = " + "Rectf(MY_TINY, " + rv_name + previous_rv + to_string(sample_rows[MY_TINY]) + ", MY_SMALL, " + rv_name + previous_rv + to_string(sample_rows[MY_SMALL]) + ", MY_LARGE, " + rv_name + previous_rv + to_string(sample_rows[MY_LARGE]) + ", MY_HUGE, " + rv_name + previous_rv + to_string(sample_rows[MY_LARGE]) + ", " + incoming_rvname + ");\n";
                                }
                            } else if (edge.type.find("FST") != string::npos) {
                                assert(sample_rows.size() == 2);
                                mcopy += name2type[rv_name] + " " + rv_name + ";\n" + "if (" + incoming_rvname + " == 0) {" + rv_name + incoming_rvname + to_string(tmp_idx)  + "= " + rv_name + previous_rv + to_string(sample_rows[0]) + ";  }\n " +
                                         "else {" + rv_name + incoming_rvname + to_string(tmp_idx) + " = " + rv_name + previous_rv + to_string(sample_rows[1]) + ";  }\n " ;
                            }
                            tmp_idx++;

                        }






                    }
                }
            }


            for (auto &rv : recur_names) {
                mcopy += rv.first + " = " + last_update[rv.first] + ";\n";


            }


            mcopy += "} // end propagation \n";



            Rewrite.InsertTextAfter(f->getLocEnd(), mcopy);
        }




    }

private:

    Rewriter &Rewrite;
};




// Implementation of the ASTConsumer interface for reading an AST produced
// by the Clang parser. It registers a couple of matchers and runs them on
// the AST.
class MyASTConsumer : public ASTConsumer {
public:
    MyASTConsumer(Rewriter &R) :  HandlerForFunc(R) {
        for (auto &ss : recur_names) {
            auto *t = new ExprHandler(R, ss.first);
            HandlerForExpr[ss.first] =  t;
        }


        for (auto &ss : recur_names) {
            Matcher.addMatcher(
                declRefExpr(hasDeclaration(decl(namedDecl(hasName(ss.first))))).bind(ss.first), HandlerForExpr[ss.first]);
        }

        Matcher.addMatcher(
            functionDecl(namedDecl(hasName("serial")), hasBody(compoundStmt(hasAnySubstatement(forStmt().bind("forLoop"))))).bind("func"), &HandlerForFunc);

    }

    void HandleTranslationUnit(ASTContext &Context) override {
        // Run the matchers when we have the whole TU parsed.
        Matcher.matchAST(Context);
    }

private:
    std::map<std::string, ExprHandler*> HandlerForExpr;
    FuncHandler HandlerForFunc;
    MatchFinder Matcher;
};

// For each source file provided to the tool, a new FrontendAction is created.
class MyFrontendAction : public ASTFrontendAction {
public:
    MyFrontendAction() {}
    void EndSourceFileAction() override {
        TheRewriter.getEditBuffer(TheRewriter.getSourceMgr().getMainFileID())
        .write(llvm::outs());
    }

    std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
            StringRef file) override {
        TheRewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
        return llvm::make_unique<MyASTConsumer>(TheRewriter);
    }

private:
    Rewriter TheRewriter;
};


int main(int argc, const char **argv) {
    CommonOptionsParser op(argc, argv, MatcherSampleCategory);
    ClangTool Tool(op.getCompilations(), op.getSourcePathList());

    string line;
    stringstream sin(line);


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

   /* for (auto &gg: graph) {
      cout << gg.first << " ";
      for (auto &ggg: gg.second) {
      cout << ggg.dest_id << " " << ggg.type << " " << ggg.coefficient << " " << ggg.origin << endl;

      }
      }*/


    GenerateSamples(graph, &sample_results);

    //  print_results(sample_results);

    GenerateUnionResults(sample_results, &merged_samples);

    //print_results(merged_samples);
    smpl_m = merged_samples.size();

    return Tool.run(newFrontendActionFactory<MyFrontendAction>().get());
}

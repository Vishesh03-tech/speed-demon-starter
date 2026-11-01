#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <sstream>

using namespace std;

vector<string> tokenize(const string& text) {
    vector<string> tokens;
    string current = "";
    for (char c : text) {
        unsigned char uc = (unsigned char)c;
        if (uc >= 'A' && uc <= 'Z') uc += 32; 
        if ((uc >= 'a' && uc <= 'z') || (uc >= '0' && uc <= '9')) {
            current += (char)uc;
        } else {
            if (!current.empty()) { tokens.push_back(current); current = ""; }
        }
    }
    if (!current.empty()) tokens.push_back(current);
    return tokens;
}

vector<int> op_and(const vector<int>& R, const vector<int>& S) {
    vector<int> res;
    set_intersection(R.begin(), R.end(), S.begin(), S.end(), back_inserter(res));
    return res;
}
vector<int> op_or(const vector<int>& R, const vector<int>& S) {
    vector<int> res;
    set_union(R.begin(), R.end(), S.begin(), S.end(), back_inserter(res));
    return res;
}
vector<int> op_not(const vector<int>& R, const vector<int>& S) {
    vector<int> res;
    set_difference(R.begin(), R.end(), S.begin(), S.end(), back_inserter(res));
    return res;
}

int main(int argc, char* argv[]) {
    string data_dir = "./data";
    string out_dir = "./out";
    
    for (int i = 1; i < argc; i++) {
        if (string(argv[i]) == "--data" && i + 1 < argc) data_dir = argv[++i];
        else if (string(argv[i]) == "--out" && i + 1 < argc) out_dir = argv[++i];
    }

    unordered_map<string, vector<int>> inverted_index;
    unordered_map<int, vector<string>> doc_tokens;

    ifstream docs_file(data_dir + "/docs.txt", ios::binary);
    if (docs_file.is_open()) {
        string line;
        while (getline(docs_file, line)) {
            if (line.empty()) continue;
            if (line.back() == '\r') line.pop_back();
            
            size_t tab_pos = line.find('\t');
            if (tab_pos == string::npos) continue;
            
            int doc_id = stoi(line.substr(0, tab_pos));
            vector<string> tokens = tokenize(line.substr(tab_pos + 1));
            doc_tokens[doc_id] = tokens; 
            
            for (const string& token : tokens) {
                if (inverted_index[token].empty() || inverted_index[token].back() != doc_id) {
                    inverted_index[token].push_back(doc_id);
                }
            }
        }
        docs_file.close();
    }

    ifstream queries_file(data_dir + "/queries.txt", ios::binary);
    ofstream results_file(out_dir + "/results.txt", ios::binary);
    
    if (queries_file.is_open() && results_file.is_open()) {
        string q_line;
        while (getline(queries_file, q_line)) {
            if (q_line.empty()) continue;
            if (q_line.back() == '\r') q_line.pop_back();

            vector<int> R;
            bool is_phrase = (q_line.find("PHRASE") == 0);
            
            if (is_phrase) {
                size_t first_quote = q_line.find('"');
                size_t last_quote = q_line.rfind('"');
                if (first_quote != string::npos && last_quote != string::npos) {
                    string phrase_text = q_line.substr(first_quote + 1, last_quote - first_quote - 1);
                    vector<string> p_terms = tokenize(phrase_text);
                    
                    if (!p_terms.empty()) {
                        vector<int> candidates = inverted_index[p_terms[0]];
                        for(size_t i = 1; i < p_terms.size(); ++i) {
                            candidates = op_and(candidates, inverted_index[p_terms[i]]);
                        }
                        
                        for (int doc_id : candidates) {
                            const vector<string>& dt = doc_tokens[doc_id];
                            bool found = false;
                            for (size_t i = 0; i + p_terms.size() <= dt.size(); ++i) {
                                bool match = true;
                                for (size_t j = 0; j < p_terms.size(); ++j) {
                                    if (dt[i+j] != p_terms[j]) { match = false; break; }
                                }
                                if (match) { found = true; break; }
                            }
                            if (found) R.push_back(doc_id);
                        }
                    }
                }
            } else {
                string bool_content = q_line.substr(5);
                stringstream ss(bool_content);
                string word;
                vector<string> q_parts;
                while(ss >> word) q_parts.push_back(word);
                
                if (!q_parts.empty()) {
                    R = inverted_index[q_parts[0]];
                    for (size_t i = 1; i + 1 < q_parts.size(); i += 2) {
                        string op = q_parts[i];
                        string term = q_parts[i+1];
                        vector<int> S = inverted_index[term];
                        if (op == "AND") R = op_and(R, S);
                        else if (op == "OR") R = op_or(R, S);
                        else if (op == "NOT") R = op_not(R, S);
                    }
                }
            }

            if (R.empty()) {
                results_file << "0 0\n";
            } else {
                long long sum = 0;
                for (int id : R) sum += id;
                results_file << R.size() << " " << sum;
                int limit = min((int)R.size(), 10);
                for (int i = 0; i < limit; i++) results_file << " " << R[i];
                results_file << "\n";
            }
        }
        queries_file.close();
        results_file.close();
    }
    return 0;
}
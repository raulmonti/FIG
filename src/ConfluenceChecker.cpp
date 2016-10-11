/* Leonardo Rodríguez */

#include <sstream>

#include "ConfluenceChecker.h"

namespace iosa {

void ConfluenceChecker::visit(std::shared_ptr<Model> node) {
    for (auto module : node->get_modules()) {
        ModuleIOSA iosa(module);
        iosa.search_non_confluents(non_confluents);
        iosa.search_triggering_pairs(tr);
        iosa.search_initially_enabled(initials);
        iosa.search_spontaneous(spontaneous);
    }    
    //print_debug_info();
    prepare_matrix();
    warshall();
    confluence_check();
}

void ConfluenceChecker::debug_matrix() {
    for (auto entry : position) {
        std::cout << entry.first << "-> " << entry.second << std::endl;
    }
    unsigned int size = position.size();
    for (unsigned int i = 0 ; i < size; i++) {
        for (unsigned int j = 0; j < size; j++) {
           std::cout << matrix[i][j] << " ";
        }
        std::cout << std::endl;
    }
}

//Raul's paper algorithm
bool ConfluenceChecker::confluence_check() {
    for (NonConfluentPair &pair : this->non_confluents) {
        string a = pair.first.get_data().get_label_id();
        string b = pair.second.get_data().get_label_id();
        for (IEdge &edge1 : this->initials) {
            string c = edge1.get_data().get_label_id();
            if (indirectly_triggers(c,a)) {
                for (IEdge &edge2 : this->initials) {
                    string d = edge2.get_data().get_label_id();
                    if (indirectly_triggers(d, b)) {
                        initial_non_deterministic_msg(pair, edge1, edge2);
                        return (true);
                    }
                }
            }
        }
        for (IEdge &edge1 : this->spontaneous) {
            string c = edge1.get_data().get_label_id();
            if (indirectly_triggers(c,a) && indirectly_triggers(c,b)) {
                spontaneous_non_deterministic_msg(pair, edge1);
                return (true);
            }
        }
    }
    return (false);
}

void ConfluenceChecker::
initial_non_deterministic_msg(NonConfluentPair& pair, IEdge& edge1,
                              IEdge& edge2) {
    string a = pair.first.get_data().get_label_id();
    string b = pair.second.get_data().get_label_id();
    string c = edge1.get_data().get_label_id();
    string d = edge2.get_data().get_label_id();
    std::stringstream ss;
    ss << "Potential source of non-determinism." << std::endl;
    ss << "Initial action \"" << c << "\"";
    ss << " indirectly triggers action \"" << a << "\"." << std::endl;
    ss << "Initial state: [";
    edge1.get_src()->print_state(ss);
    ss << "]" << std::endl;
    ss << "Initial action \"" << d << "\"";
    ss << " indirectly triggers action \"" << b << "\"." << std::endl;
    ss << "Initial state: [";
    edge2.get_src()->print_state(ss);
    ss << "]" << std::endl;
    ss << "Actions \"" << a << "\" and \"" << b << "\" are non confluent";
    ss << " on state: [";
    pair.first.get_src()->print_state(ss);
    ss << "]" << std::endl;
    put_error(ss.str());
}

void ConfluenceChecker::
spontaneous_non_deterministic_msg(NonConfluentPair &pair, IEdge& edge) {
    string a = pair.first.get_data().get_label_id();
    string b = pair.second.get_data().get_label_id();
    string c = edge.get_data().get_label_id();
    std::stringstream ss;
    ss << "Potential source of non-determinism." << std::endl;
    ss << "Spontaneous action \"" << c << "\"";
    ss << " indirectly triggers actions \"" << a << "\" and \"" << b << "\"";
    ss << std::endl;
    ss << "Actions \"" << a << "\" and \"" << b << "\" are non confluent ";
    ss << " on state: [";
    pair.first.get_src()->print_state(ss);
    ss << "]" << std::endl;
    ss << "Action \"" << c << "\" is enabled on stable state: [";
    edge.get_src()->print_state(ss);
    ss <<  "]" << std::endl;
    put_error(ss.str());
}

bool ConfluenceChecker::indirectly_triggers(const string &label1,
                                            const string &label2) {
    if (position.find(label1) == position.end() ||
            position.find(label2) == position.end()) {
        return (false);
    }
    unsigned int i = position.at(label1);
    unsigned int j = position.at(label2);
    return matrix[i][j];
}

void ConfluenceChecker::prepare_matrix() {
    unsigned int i = 0;
    for (TriggeringPair& entry : tr) {
        string label1 = entry.first.get_data().get_label_id();
        string label2 = entry.second.get_data().get_label_id();
        if (position.find(label1) == position.end()) {
            position[label1] = i;
            i++;
        }
        if (position.find(label2) == position.end()) {
            position[label2] = i;
            i++;
        }
    }
    unsigned int size = position.size();
    matrix.resize(size);
    for (std::vector<bool>& v : matrix) {
        v.resize(size, false);
    }
    for (TriggeringPair& entry : tr) {
        string label1 = entry.first.get_data().get_label_id();
        string label2 = entry.second.get_data().get_label_id();
        unsigned int pos1 = position.at(label1);
        unsigned int pos2 = position.at(label2);
        matrix[pos1][pos2] = true;
    }
}

void ConfluenceChecker::warshall() {
    unsigned int size = position.size();
    //transitive closure
    for (unsigned int k = 0; k < size; k++) {
        for (unsigned int i = 0; i < size; i++) {
            for (unsigned int j = 0; j < size; j++) {
                matrix[i][j] = matrix[i][j] || (matrix[i][k] && matrix[k][j]);
            }
        }
    }
    //make it reflexive, sure?
    for (unsigned int i = 0 ; i < size ; i++) {
        matrix[i][i] = true;
    }
}

void ConfluenceChecker::print_debug_info() {
    for (IEdge &edge : initials) {
        std::cout << "initial: " << edge.get_data().get_label_id();
        std::cout << std::endl;
    }
    for (NonConfluentPair pair : non_confluents) {
        std::cout << "(" << pair.first.get_data().get_label_id();
        std::cout << "," << pair.second.get_data().get_label_id();
        std::cout << ") on state: ";
        pair.first.get_src()->print_state(std::cout);
        std::cout << " non confluent." << std::endl;

    }
    for (TriggeringPair pair : tr) {
        std::cout << "[" << pair.first.get_data().get_label_id();
        std::cout << "] triggers [" << pair.second.get_data().get_label_id();
        std::cout << "]";
        std::cout << std::endl;
    }
    for (IEdge& edge : spontaneous) {
        std::cout << "[" << edge.get_data().get_label_id();
        std::cout << "]" << " spontanous on state: ";
        edge.get_src()->print_state(std::cout);
        std::cout << std::endl;
    }
}

}//

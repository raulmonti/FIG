#include <fstream>
#include <string>

#include "Clock.h"
#include "State.h"
#include "Precondition.h"
#include "Postcondition.h"
#include "Transition.h"
#include "ModuleInstance.h"
#include "ModuleNetwork.h"
#include "ModelSuite.h"
#include "PropertyTransient.h"
#include "PropertyRate.h"

using std::string;

namespace {

template<typename Container>
void print_vec(std::ostream &out, const string &vec_name,
               const Container &vec) {
    out << vec_name;
    for (const auto &x : vec) {
        out << x << ",";
    }
    out << std::endl;
}

template<typename Iterator>
void print_vec (std::ostream &out, const string &name,
                Iterator first, Iterator last) {
    out << name;
    auto &it = first;
    while(it != last) {
        out << *it << ",";
        it++;
    }
    out << std::endl;
}

} //namespace

namespace fig {

void Clock::print_info(std::ostream &out) const {
    out << "CLOCK" << std::endl;
    out << "NAME:" << name_ << std::endl;
    out << "DISTNAME:" << distName_ << std::endl;
    out << "CLOCK-SEED:" << this->rng_seed() << std::endl;
    ::print_vec(out, "DISTPARAMETERS:", distParams_.begin(), distParams_.end());
    out << "ENDOF-CLOCK:" << name_ << std::endl;
}

template<typename T_>
void State<T_>::print_info(std::ostream &out) const {
    out << "STATE" << std::endl;
    out << "NVARS:" << this->size() << std::endl;
    ::print_vec(out, "VARIABLES:", this->varnames());
    this->print_out(out);
    out << "ENDOF-STATE" << std::endl;
}

void Precondition::print_info(std::ostream &out) const {
    out << "PRECONDITION" << std::endl;
    out << "EXPRESSION:" << this->expression() << std::endl;
    out << "NVAR:" << this->NVARS_ << std::endl;
    ::print_vec(out, "NAMES:", this->varsNames_);
    ::print_vec(out, "POSITIONS:", this->varsPos_);
    ::print_vec(out, "VALUES:", this->varsValues_);
    out << "ENDOF-PRECONDITION" << std::endl;
}

void Postcondition::print_info(std::ostream &out) const {
    out << "POSTCONDITION" << std::endl;
    out << "EXPRESSION:" << this->exprStr_ << std::endl;
    out << "NUM-VAR-UPDATES:" << this->NUPDATES_ << std::endl;
    out << "NUM-VAR:" << this->NVARS_ << std::endl;
    ::print_vec(out, "UPDATE-NAMES:", this->updatesNames_);
    ::print_vec(out, "UPDATE-POSITIONS:", this->updatesPos_);
    ::print_vec(out, "NAMES:", this->varsNames_);
    ::print_vec(out, "POSITIONS:", this->varsPos_);
    ::print_vec(out, "VALUES:", this->varsValues_);
    out << "ENDOF-POSTCONDITION" << std::endl;
}

void Transition::print_info(std::ostream &out) const {
    out << "TRANSITION" << std::endl;
    out << "LABEL:" << this->label().str << std::endl;
    out << "TRIGGER-CLOCK:" << this->triggeringClock << std::endl;
    this->pre.print_info(out);
    this->pos.print_info(out);
    if (this->resetClocksData_ == CRYSTAL) {
        out << "RESET-CLOCKS-ENCODED:"
            << this->resetClocks_.to_ullong()
            << std::endl;
    } else {
        ::print_vec(out, "RESET-CLOCKS:", this->resetClocksList_);
    }
    out << "ENDOF-TRANSITION" << std::endl;
}

void ModuleInstance::print_info(std::ostream &out) const {
    out << "MODULE" << std::endl;
    out << "NAME:" << this->name << std::endl;
    out << "STATE-SIZE:" << this->state_size() << std::endl;
    out << "NUM-VAR:" << this->num_vars() << std::endl;
    out << "NUM-CLOCK:" << this->num_clocks() << std::endl;
    out << "NUM-TRANSITIONS:" << this->num_transitions() << std::endl;
    out << "GLOBAL-INDEX:" << this->global_index() << std::endl;
    out << "FIRST-CLOCK:" << this->firstClock_ << std::endl;
    out << "FIRST-VAR:" << this->firstVar_ << std::endl;
    out << "LOCAL-STATE:" << std::endl;
    this->lState_.print_info(out);
    for (const auto &clock : this->lClocks_) {
        clock.print_info(out);
    }
    for (const auto &transition : this->transitions_) {
        transition.print_info(out);
    }
    out << "ENDOF-MODULE " << this->name << std::endl;
}

void ModuleNetwork::print_info(std::ostream &out) const {
    out << "MODULENETWORK" << std::endl;
    out << "NUM-MODULES:" << this->num_modules() << std::endl;
    out << "NUM-TRANSITIONS:" << this->num_transitions() << std::endl;
    out << "STATE-SIZE:" << this->state_size() << std::endl;
    out << "CONCRETE-STATE-SIZE:" << this->concrete_state_size() << std::endl;
    out << "INITIAL-STATE-SIZE:" << this->initial_concrete_state() << std::endl;
    out << "INITIAL-STATE:" << std::endl;
    this->initial_state().print_info(out);
    out << "INITIAL-CLOCKS:";
    for (auto &entry : this->initialClocks) {
        const Clock &clock = entry.second;
        int size = entry.first;
        out << clock.name() << "[pos=" << size << "],";
    }
    out << std::endl;
    out << "GLOBAL-STATE:" << std::endl;
    this->global_state().print_info(out);
    for (const auto& module : this->modules) {
        module->print_info(out);
    }
    for (const auto &clock : this->clocks()) {
        clock.get().print_info(out);
    }
    out << "ENDOF-MODULENETWORK" << std::endl;
}

void PropertyTransient::print_info(std::ostream &out) const {
    out << "PROPERTY-TRANSIENT" << std::endl;
    out << "EXPRESSION:" << this->expression << std::endl;
    out << "LEFT-EXPRESSION:" << std::endl;
    this->expr1_.print_info(out);
    out << "RIGHT-EXPRESSION:" << std::endl;
    this->expr2_.print_info(out);
    out << "ENDOF-PROPERTY-TRANSIENT" << std::endl;
}

void PropertyRate::print_info(std::ostream &out) const {
    out << "PROPERTY-RATE" << std::endl;
    out << "EXPRESSION:" << this->expression() << std::endl;
    out << "LEFT-EXPRESSION:" << std::endl;
    this->expr_.print_info(out);
    out << "ENDOF-PROPERTY-RATE" << std::endl;
}

void ModelSuite::print_info(std::ostream &out) const {
    out << "MODEL" << std::endl;
    out << "NUM-MODULES:" << this->num_modules() << std::endl;
    out << "NUM-TRANSITION:" << this->num_clocks() << std::endl;
    out << "NUM-PROPERTIES:" << this->num_properties() << std::endl;
    out << "NUM-CLOCKS:" << this->num_clocks() << std::endl;
    this->model->print_info(out);
    out << "PROPERTIES:" << std::endl;
    for (const auto &prop : this->properties) {
        prop->print_info(out);
    }
    out << "ENDOF-MODEL" << std::endl;
}

void ModelSuite::print_importance_function(std::ostream &out,
                                           const ImportanceFunction &imf) const {
    out << "IMPORTANCE-FUNCTION" << std::endl;
    imf.print_out(out, this->modules_network()->global_state());
    out << "ENDOF-IMPORTANCE-FUNCTION" << std::endl;
}

}

#include "ModelAST.h"
class ExpStringBuilder : public Visitor {
    vector<std::string> names;
    std::string result;
    bool should_enclose;

public:
    ExpStringBuilder() : result {""}, should_enclose {false} {}
    void visit(shared_ptr<IConst> node);
    void visit(shared_ptr<BConst> node);
    void visit(shared_ptr<FConst> node);
    void visit(shared_ptr<LocExp> node);
    void visit(shared_ptr<OpExp> node);
    const vector<std::string>& get_names();
    std::string str();

    //converts a vector of expressions [e1, e2, e3, ...]
    //into the string "str(e1) & str(e2) & str(e3) & ...."
    //and returns also the vector with all the names that occur
    //in the expressions
    static std::pair<std::string, std::vector<std::string>>
    make_conjunction_str(const vector<shared_ptr<Exp>>& expvec);
};

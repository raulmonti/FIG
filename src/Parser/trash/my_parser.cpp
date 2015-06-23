/*
    My Parser 19-06-2015
    Raul Monti
*/
#include <iostream>
#include <string>
#include <stack>
#include "my_lexer.h"
#include "exceptions.h"

using namespace std;


char table[][6] = {"NUM", "NAME", "WS", "NL", "INT", "MEOF"};

string stringtoparse = string("aaab33");


class Parser
{ 
    stack<int> stk;
    Symbol sym;
    int pos = -1;
    int lastpos = pos;
    bool skipws = false;

public:

    int
    getLineNum(int p){
        int result = 0;
        for(int i = 0; i < p; i++){
            if(vec[i] == NL) result++;
        }
        return result;
    }

    inline int 
    isw(){
        return (vec[pos] == WS || vec[pos] == NL);
    }

    void nextsym(void){

        pos ++;
        if (skipws){
            while( isw() && pos < vec.size()){                
                pos++;
                lastpos++;
            }
        } 

        
        if (pos == vec.size()){
            sym = MEOF;
        }else{
            sym = (Symbol)vec[pos];
        }
    }
 
    int accept(Symbol s) {
        if (sym == s) {
            lastpos = pos;  
            nextsym();
            return 1;
        }
        return 0;
    }
 
    int expect(Symbol s) {
        if (accept(s))
            return 1;

        string ss = string("Expected ") + string(table[s]) 
                  + string(" type, got ") + ii[pos] 
                  + string(", of type ") + string(table[sym]);
        throw(new SyntaxError(ss));
    }

    int 
    saveLocation(){
        stk.push(pos);
    }


    int 
    loadLocation(){
        pos = stk.top();
        stk.pop();
        sym = (Symbol)vec[pos];
    }

    int
    removeLocation(){
        stk.pop();
    }


    int 
    lookahead(){
        if(output()){
            ;
        }else if(input()){
            ;
        }else{
            return 0;
        }
        return 1;
    }

    int
    output(){
        saveLocation();
        //accept(WS);  // initial whites
        if (accept(NUM)){
            accept(WS);
            if (accept(NUM)){
                accept(WS);
                expect(NAME);
                //accept(WS); // ending whites
                removeLocation();
                return 1;            
            }
        }
        loadLocation();
        return 0;
    }


    int
    input(){
        saveLocation();
        //accept(WS);  // initial whites
        if (accept(NUM)){
            accept(WS);
            if (accept(NAME)){
                accept(WS);
                expect(NUM);
                //accept(WS); // ending whites
                removeLocation();
                return 1;            
            }
        }
        loadLocation();
        return 0;
    }
    

    int 
    grammar(){
        if (!lookahead()) {
            throw (new SyntaxError((string("Syntax error at line ")) 
                      + string(to_string(getLineNum(lastpos+1))) +string(": '") 
                      + ii[lastpos+1]+string("'")));
        }
        return 1;
    } 

    int
    parse(){
        nextsym();
        try{
            while(!ended()){
                grammar();
            }
        }catch(SyntaxError* e){
            cout << "Parser: " << e->what() << endl;
            return 0;
        }
        return 1;
    }

    int
    ended(){
        return pos == vec.size();
    }

};



int main (int argv, char **argc)
{

    cout << "MY PARSER IS RUNNING\n" << endl;

    /*Lex*/
    Lexer *lexx = new Lexer;
    lexx->lex(stringtoparse);

    /*Print the lexed vector*/
    cout << "[";
    for(int i = 0; i < vec.size()-1; i++){
        std::cout << vec[i] << ",";
    }
    cout << vec[vec.size()-1] << "]" << std::endl;

    /*Parse*/
    Parser *parser = new Parser;
    parser->parse();

    return 0;
}

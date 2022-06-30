

#include <exception>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <fstream>
#include <unistd.h>

#include "lc.h"

using namespace std;
using namespace lc;
int main(){
Scanner scanner{cin};
Parser parser(scanner);
//parser.consume();
map<string, string> decls;
//cerr << parser.parseName();
cout << "#include <functional>\n";
cout << "#include <any>\n";
cout << "struct o: std::function<o(const o&)>{std::any raw;using std::function<o(const o&)>::function;";
cout << "o(unsigned long x){*this=[=](o const &zero) -> o{return [=](o const &succ) -> o{auto z = zero;for(unsigned long i = 0;i < x;i++)z = succ(z);return z;};};};";
cout << "unsigned long scint() const{return std::any_cast<unsigned long>((*this)(o{.raw = (unsigned long)0})([=](o const &tosucc) -> o{return o{.raw = std::any_cast<unsigned long>(tosucc.raw) + 1};}).raw);};";
cout << "};";
cout << "inline o app(const o &a,const o &b){return o([=](const o &c) -> o{return a(b)(c);});};";
while(scanner.curr() != Token::eof){
auto g = parser.parseName();
decls[g] = parser.parseLine()->emit();
cerr << g;
};
for(auto d: decls)cerr << d.first;
for(auto d: decls)cout << "extern o o_" << d.first << ";\n";
cout << "int main(){o_tgt(o([=](o const &a) -> o{return a;}));};";
for(auto d: decls)cout << "o o__" << d.first << "(){return " << d.second << ";};o o_" << d.first << " = o__" << d.first << "();\n";
}
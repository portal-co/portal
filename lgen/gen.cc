

#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
#include <unistd.h>

#include "lc.h"

using namespace std;
using namespace lc;
int main(){
    char c;
string inp;while(cin >> c)inp += c;
stringstream ss(inp);
Scanner scanner{ss};
Parser parser(scanner);
auto ast = parser.parseLine();
auto e = ast->emit();
cout << "#include <functional>\n";
cout << "#include <any>\n";
cout << "struct o: std::function<o(const o&)>{std::any raw;using std::function<o(const o&)>::function;";
cout << "o(unsigned long x){if(x == 0){*this=[=](o const &zero) -> o{return [=](o const &succ) -> o{return zero;};};return;};*this=[=](o const &zero) -> o{return [=](o const &succ) -> o{return succ(x - 1);};};};";
cout << "unsigned long scint() const{return std::any_cast<unsigned long>((*this)(o{.raw = (unsigned long)0})([=](o const &tosucc) -> o{return o{.raw = tosucc.scint() + 1};}).raw);};";
cout << "};";
cout << "inline o app(const o &a,const o &b){return o([=](const o &c) -> o{return a(b)(c);});};";
cout << "o tgt(){return " << e << ";};";
cout << "int main(){tgt()(o([=](o const &a) -> o{return a;}));};";
}
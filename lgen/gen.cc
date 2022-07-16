

#include <exception>
#include <fstream>
#include <iostream>
#include <istream>
#include <map>
#include <sstream>
#include <string>
#include <unistd.h>

#include "csb.h"
#include "lc.h"

using namespace std;
using namespace lc;
int main() {
  Scanner scanner{cin};
  Parser parser(scanner);
  // parser.consume();
  map<string, string> decls;
  // cerr << parser.parseName();
  cout << "#include <functional>\n";
  cout << "#include <any>\n";
  cout << "#include <limits>\n";
  cout << "#include \"InfInt.h\"\n";
  cout << "struct o: std::function<o(const o&)>{std::any raw;using "
          "std::function<o(const o&)>::function;";
  // cout << "o(unsigned long x){*this=[=](o const &zero) -> o{return [=](o
  // const &succ) -> o{auto z = zero;for(unsigned long i = 0;i < x;i++)z =
  // succ(z);return z;};};};"; cout << "unsigned long scint() const{return
  // std::any_cast<unsigned long>((*this)(o{.raw = (unsigned long)0})([=](o
  // const &tosucc) -> o{return o{.raw = std::any_cast<unsigned
  // long>(tosucc.raw) + 1};}).raw);};"; cout << "o(InfInt x){*this=[=](o const
  // &zero) -> o{return [=](o const &succ) -> o{auto z = zero;for(InfInt i = 0;i
  // < x;i++)z = succ(z);return z;};};};"; cout << "InfInt sciint() const{return
  // std::any_cast<InfInt>((*this)(o{.raw = InfInt(0)})([=](o const &tosucc) ->
  // o{return o{.raw = std::any_cast<InfInt>(tosucc.raw) + 1};}).raw);};";
  cout << "};\n";
  cout << "struct ox: std::exception{o val;};\n";
  cout << "struct oxt: std::exception{o val;};\n";
  cout << "template<typename T> T p(const T &x){return x;};";
  cout << "inline o app(const o &a,const o &b){return o([=](const o &c) -> "
          "o{return a(b)(c);});};";
  while (scanner.curr() != Token::eof) {
    auto g = parser.parseName();
    if(g == "rawglobal"){
      cout << parser.parseRaw();
      continue;
    };
    auto l = parser.parseLine();
    decls[g] = l->emit("cc");
    cerr << g << l->str() << endl;
  };
  decls["todo"] = "o([=](const o &x) -> o{oxt ex;ex.val = x;throw ex;})";
  decls["fix"] = "o([=](const o &f) -> o{return app(f,o_fix(f));})";
  for (auto d : decls)
    cerr << d.first;
  for (auto d : decls)
    cout << "extern o o_" << d.first << ";\n";
  // cout << "o o_p_wsize = 65536;\n"; // 16 bit for now
  cout << "int main(){o_tgt(o([=](o const &a) -> o{return a;}));};\n";
  for (auto d : decls)
    cout << "o o__" << d.first << "(){return " << d.second << ";};o o_"
         << d.first << " = o__" << d.first << "();\n";
}
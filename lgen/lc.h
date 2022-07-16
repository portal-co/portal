#ifndef __LC_H__
#define __LC_H__

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <cwctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <locale>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <deque>

namespace lc {

using std::string;
using std::ostream;
using std::istream;
using std::stringstream;
using std::locale;

using std::exception;

using std::unique_ptr;
using std::make_unique;
using std::move;

using std::quoted;
using std::to_string;

using std::vector;

// Stringer
struct Stringer {
  virtual ~Stringer() {}

  virtual string str() const = 0;
};

inline ostream &operator<<(ostream &out, const Stringer &stringer) {
  return out << stringer.str();
}

// Describer
struct Describer {
  virtual ~Describer() {}

  virtual string describe(const string &prefix) const = 0;
};

// Error
struct Error : virtual public Stringer, virtual public exception {

  Error() = delete;
  Error(const string &prefix, const string &message)
      : Message{prefix + " : " + message} {}

  Error(const Error &) = delete;
  Error &operator=(const Error &) = delete;

  Error(Error &&) = default;
  Error &operator=(Error &&) = default;

  virtual ~Error() {}

  const char *what() const noexcept override { return Message.c_str(); }

  string str() const { return Message; }

protected:
  const string Message{};
};

// Position
struct Position : virtual public Stringer {
  static const int defaultLine = 1;
  static const int defaultColumn = 0;
  static const int defaultOffset = 0;

  Position(int line = defaultLine, int column = defaultColumn,
           int offset = defaultOffset)
      : Line{line}, Column{column}, Offset{offset} {}

  Position(const Position &pos) = default;
  Position(Position &&pos) = default;

  Position &operator=(const Position &pos) = default;
  Position &operator=(Position &&pos) = default;
  virtual ~Position() {}

  void incr() {
    ++Column;
    ++Offset;
  }

  void nl(bool updateOffset = true) {
    Column = defaultColumn;
    ++Line;
    if (updateOffset)
      ++Offset;
  }

  bool operator==(const Position &pos) const {
    return Line == pos.Line && Column == pos.Column && Offset == pos.Offset;
  }

  virtual string str() const override {
    return to_string(Line) + ":" + to_string(Column);
  }

  int Line{defaultLine};
  int Column{defaultColumn};
  int Offset{defaultOffset};
};

const int Position::defaultColumn;
const int Position::defaultLine;
const int Position::defaultOffset;

// Errors
struct ScanError : virtual public Error {
  ScanError(const string &message) : Error{description(), message} {}
  ScanError(const string &filename, const string &message, const Position &pos)
      : Error{description() + " " + filename + ":" + pos.str(), message} {}

protected:
  virtual const string description() const {
    static const string desc = "scanner error";
    return desc;
  }
};

struct ParseError : virtual public Error {
  ParseError(const string &message) : Error{description(), message} {}
  ParseError(const string &filename, const string &message, const Position &pos)
      : Error{description() + " " + filename + ":" + pos.str(), message} {}

protected:
  virtual const string description() const {
    static const string desc = "parser error";
    return desc;
  }
};

struct EvalError : virtual public Error {
  EvalError(const string &message) : Error{description(), message} {}
  EvalError(const string &filename, const string &message, const Position &pos)
      : Error{description() + " " + filename + ":" + pos.str(), message} {}

protected:
  virtual const string description() const {
    const string desc = "eval error";
    return desc;
  }
};

struct Token : virtual public Stringer {
  enum Type {
    undefined,
    raw,
    eof,
    lambda,
    dot,
    id,
    number,
    lparen,
    rparen,
    nl,
  };

  Token() = delete;
  Token(Type type, const Position &pos = {}) : Pos{pos}, ValueType{type} {}
  Token(Type type, const string &s, const Position &pos = {})
      : Pos{pos}, ValueType{type}, ValueString{s} {}

  Token(Type type, long long i, const Position &pos = {})
      : Pos{pos}, ValueType{type}, ValueNumber{i} {}

  Token(const Token &tok)
      : Pos{tok.Pos}, ValueType{tok.ValueType}, ValueString{tok.ValueString},
        ValueNumber{tok.ValueNumber} {}

  Token &operator=(const Token &tok) {
    if (this != &tok) {
      Pos = tok.Pos;
      ValueType = tok.ValueType;
      ValueString = tok.ValueString;
      ValueNumber = tok.ValueNumber;
    }
    return *this;
  };

  Token(Token &&tok)
      : Pos{move(tok.Pos)}, ValueType{tok.ValueType},
        ValueString{move(tok.ValueString)}, ValueNumber{tok.ValueNumber} {}

  Token &operator=(Token &&tok) {
    if (this != &tok) {
      Pos = move(tok.Pos);
      ValueType = tok.ValueType;
      ValueString = move(tok.ValueString);
      ValueNumber = tok.ValueNumber;
    }
    return *this;
  }

  virtual ~Token() {}

  bool is(const Type &type) const { return ValueType == type; }

  bool operator==(const Type &type) const { return is(type); }

  bool operator!=(const Type &type) const { return !is(type); }

  bool operator==(const Token &tok) const {
    return Pos == tok.Pos && ValueType == tok.ValueType &&
           ValueString == tok.ValueString && ValueNumber == tok.ValueNumber;
  }

  bool operator!=(const Token &tok) const { return !(*this == tok); }

  virtual string str() const override {
    stringstream ss;

    ss << "(";
    switch (ValueType) {
    case eof: {
      ss << "'eof'";
    } break;
    case lambda: {
      ss << "λ";
    } break;
    case dot: {
      ss << "'.'";
    } break;
    case id: {
      ss << "id"
         << " " << quoted(ValueString);
    } break;
    case number: {
      ss << ValueNumber;
    } break;
    case lparen: {
      ss << "'('";
    } break;
    case rparen: {
      ss << "')'";
    } break;
    case nl: {
      ss << "'\\n'";
    } break;
    case undefined:
    default: { ss << "undefined"; }
    }

    ss << " " << Pos << ")";

    return ss.str();
  }

  Position Pos{};
  Type ValueType{undefined};
  string ValueString{};
  long long ValueNumber{};
};

struct AST : virtual public Stringer, virtual public Describer {
  friend class Interpreter;

public:
  enum Type {
    expr,
    name,
    function,
    application,
  };

  virtual ~AST() {}

  virtual Type type() const = 0;
  virtual unique_ptr<AST> clone() const = 0;
  virtual string emit(string ty) const = 0;
};

// <name> := [a-zA-Z][a-zA-Z0-9]*
class NameAST : public AST {
  friend class Interpreter;

  static const int defaultIndex = -1;

public:
  explicit NameAST(const string &name, int index, bool bound)
      : Name{name}, Index{index}, Bound{bound} {}

  const string name() const { return Name; }

  int index() const { return Index; }

  bool bound() const { return Bound; }

  virtual Type type() const override { return Type::name; }

  virtual unique_ptr<AST> clone() const override {
    return make_unique<NameAST>(Name, Index, Bound);
  }

  bool operator==(const NameAST &n) const { return Name == n.Name; }

  NameAST &operator+=(int n) {
    Index += n;
    return *this;
  }

  virtual string str() const override { return Name; }

  virtual string describe(const string &prefix) const override {
    stringstream ss;

    ss << prefix << "(" << quoted(Name) << ' ' << Index << ' ' << std::boolalpha
       << Bound << ")";

    return ss.str();
  }

  virtual string emit(string ty) const override{
    return string("o_") + name();
  }

protected:
  const string Name{};
  int Index{defaultIndex};
  const bool Bound{true};
};
const int NameAST::defaultIndex;

// <function> := λ <name>.<expression>
class FunctionAST : public AST {
  friend class Interpreter;

public:
  FunctionAST(const string &argument, unique_ptr<AST> body)
      : Argument{argument}, Body{move(body)} {
    if (!Body)
      throw ParseError{"Invalid Function"};
  }

  virtual Type type() const override { return Type::function; }

  virtual unique_ptr<AST> clone() const override {
    return make_unique<FunctionAST>(Argument, move(Body->clone()));
  }

  AST *body() const { return Body.get(); }

  virtual string str() const override {
    stringstream ss;
    ss << "λ " << Argument;

    AST *childBody = Body.get();
    while (dynamic_cast<FunctionAST *>(childBody)) {
      auto fn = dynamic_cast<FunctionAST *>(childBody);
      ss << " " << fn->Argument;
      childBody = fn->body();
    }

    ss << "." << *childBody;
    return ss.str();
  }

  virtual string describe(const string &prefix) const override {
    stringstream ss;
    ss << prefix << "[ " << quoted(Argument) << '\n'
       << prefix << Body->describe(prefix + "  ") << '\n'
       << prefix << "]";
    return ss.str();
  }

  virtual string emit(string ty) const override{
    return string("o([=](const o &") + (string("o_") + Argument) + ") -> o{return " + Body->emit(ty) + ";})";
  }

protected:
  const string Argument{};
  unique_ptr<AST> Body{nullptr};
};

// <application> := <expression><expression>
class ApplicationAST : public AST {

  friend class Interpreter;

public:
  ApplicationAST(unique_ptr<AST> lhs, unique_ptr<AST> rhs)
      : Lhs{move(lhs)}, Rhs{move(rhs)} {
    if (!Lhs || !Rhs)
      throw ParseError{"Invalid Application"};
  }

  virtual Type type() const override { return Type::application; }

  virtual unique_ptr<AST> clone() const override {
    return make_unique<ApplicationAST>(move(Lhs->clone()), move(Rhs->clone()));
  }

  AST *lhs() const { return Lhs.get(); }
  AST *rhs() const { return Rhs.get(); }

  virtual string str() const override {
    stringstream ss;
    ss << *Lhs << "(" << *Rhs << ")";
    return ss.str();
  }

  virtual string describe(const string &prefix) const override {
    stringstream ss;

    ss << prefix << "{" << '\n'
       << prefix << Lhs->describe(prefix + "  ") << '\n'
       << prefix << Rhs->describe(prefix + "  ") << '\n'
       << prefix << "}";
    return ss.str();
  }
  string emit(string x) const override{
    return string("app(") + Lhs->emit(x) + "," + Rhs->emit(x) + ")";
  }

protected:
  unique_ptr<AST> Lhs, Rhs;
};

class RawAST: public AST{
  public:
  string raw;
  RawAST(string x): raw(x){};
  string emit(string ty) const override{
    return raw;
  }
  virtual unique_ptr<AST> clone() const override {
    return make_unique<RawAST>(raw);
  }
  virtual Type type() const override {throw this;}
  virtual string str() const override {return string("`") + raw + "`";}
  virtual string describe(const string &p) const override {return string("`") + raw + "`";}
};

// boost::locale::generator generator;
const string defaultLocale = "en_US.UTF-8";

class Scanner {
public:
  static const Position initialPosition;

protected:
std::deque<char> q;
  char get() {
    if(q.size()){
      char c = q.front();
      q.pop_front();
      return c;
    };
    Ch = Input.get();
    Pos.incr();
    return Ch;
  }

  void skipWhitespace() {
    while (isspace(Ch) && Ch != '\n') {
      get();
    }
  }

  Token getNext() {
    skipWhitespace();

    Position currentPos = Pos;

    if(Ch == '`'){
      char old = Ch;
      std::cerr << int(old);
      get();
      string s{};
      do {
        s += Ch;
        std::cerr << int(Ch) << "`" << s << "`";
                std::cerr << std::endl;
        if(Ch != old){
         get();
        }else{
          
          goto out;
        }
      } while (Ch != old && Ch != -1);
      out:
        std::cerr << int(get()) << std::endl;
      return Token(Token::raw, s, currentPos);
    }

    if (isalnum(Ch)) {
      string s{};

      do {
        s += Ch;
        get();
      } while (isalnum(Ch) || Ch == '$');

      return Token(Token::id, s, currentPos);
    }
/*
    if (isalnum(Ch)) {
      string s{};

      do {
        s += Ch;
        get();
      } while (isalnum(Ch));

      long long i = strtoll(s.c_str(), nullptr, 10);

      return Token(Token::number, i, currentPos);
    }
*/
    switch (Ch) {
    case '\\': {
      get();
      return Token(Token::lambda, currentPos);
    }
    case '.': {
      get();
      return Token(Token::dot, currentPos);
    }
    case '(': {
      get();
      return Token(Token::lparen, currentPos);
    }
    case ')': {
      get();
      return Token(Token::rparen, currentPos);
    }
    case '\n': {
      get();
      Pos.nl(false);
      return Token(Token::nl, currentPos);
    }
    }

    if (eof()) {
      return Token(Token::eof, currentPos);
    }

    stringstream ss;
    ss << "Unknown token '" << Ch << "'";

    throw ScanError(Filename, ss.str(), currentPos);
  }

public:
  Scanner(istream &input, const string &filename = "<in>",
          const string &loc = defaultLocale)
      : Input{input}, Locale{locale(loc)}, Filename{filename},
        Pos{initialPosition}, Ch{}, currentToken{Token::undefined,
                                                 initialPosition},
        nextToken{Token::undefined, initialPosition} {
    input.imbue(Locale);
    get();
  }

  Scanner(const Scanner &) = delete;
  Scanner &operator=(const Scanner &) = delete;

  Scanner(Scanner &&) = delete;            // TODO
  Scanner &operator=(Scanner &&) = delete; // TODO

  ~Scanner() {}

  locale loc() const { return Locale; }

  string fileName() const { return Filename; }

  const Token &curr() const { return currentToken; }
  const Token &peek() const { return nextToken; }

  Token next() {
    if (nextToken == Token::undefined) {
      nextToken = getNext();
    }

    currentToken = nextToken;
    nextToken = getNext();
    return currentToken;
  }

  bool eof() const { return Input.eof(); }

  Position position() const { return Pos; }

protected:
  istream &Input;
  locale Locale{};
  string Filename{};

  Position Pos{initialPosition};
  char Ch{};

  Token currentToken, nextToken;
};

const Position Scanner::initialPosition{1, -1, -1};

/*
 *
 * Grammar:
 *
 * <prog>         := <expression> '\n'
 * <application>  := <expression> [ <expression> ]+
 * <expression>   := <name> | <function> | <application> | <p-expression>
 * <p-expression> := '(' <expression> ')'
 * <function>     := 'λ' <name> '.' <expression>
 * <name>         := [a-zA-Z][a-zA-Z0-9]*
 *
 * */

class Parser {

  friend class Interpreter;

protected:
  Scanner &scanner;
public:
  Token consume() { return scanner.next(); }
private:

  // <application> := <expression><expression>
  unique_ptr<AST> parseApplicationExpr(vector<string> scope,
                                       unique_ptr<AST> expr = nullptr) {
    if (expr == nullptr) {
      expr = parseExpression(scope);
      if (!expr) {
        return nullptr;
      }
    }

    auto arg = parseExpression(scope);
    if (!arg) {
      return expr;
    }

    auto app = make_unique<ApplicationAST>(move(expr), move(arg));
    std::cerr << scanner.curr();
    return parseApplicationExpr(scope, move(app));
  }

  // <expression> := <name> | <function> | <application> | <p-expression>
  unique_ptr<AST> parseExpression(vector<string> &scope) {
    switch (scanner.curr().ValueType) {
    case Token::raw:
    {
    auto a = std::make_unique<RawAST>(scanner.curr().ValueString);
    consume();
    return a;
    }
    case Token::id:
      return parseNameExpr(scope);
    case Token::lambda:
      return parseFunctionExpr(scope);
    case Token::lparen:
      return parseParenExpr(scope);
    case Token::eof:
    case Token::undefined:
    default:
      return nullptr;
    }
  }

  // <p-expression> := '(' <expression> ')'
  unique_ptr<AST> parseParenExpr(vector<string> scope) {
    consume(); // consume '('
    auto expr = parseApplicationExpr(scope, nullptr);
    if (!expr) {
      return nullptr;
    }

    if (scanner.curr() != Token::rparen) {
      throw ParseError(scanner.fileName(), "parser: Expected ')'",
                       scanner.position());
    }
    consume(); // consume ')'
    return expr;
  }

  // <function> := λ <name>.<expression>
  unique_ptr<AST> parseFunctionExpr(vector<string> scope) {
    if (scanner.curr() == Token::lambda) {
      consume(); // consume 'λ'
    }
    if (scanner.curr() != Token::id) {
      throw ParseError(scanner.fileName(), "Expected identifier",
                       scanner.position());
    }

    string argument = scanner.curr().ValueString;
    consume(); // consume token::id

    if (scanner.curr() == Token::dot) {
      vector<string> newScope{argument};

      newScope.insert(newScope.end(), scope.begin(), scope.end());

      consume(); // consume '.'
      auto expr = parseApplicationExpr(newScope, nullptr);
      if (!expr) {
        return nullptr;
      }

      return make_unique<FunctionAST>(argument, move(expr));
    } else if (scanner.curr() == Token::id) {
      auto body = parseFunctionExpr(scope);
      return make_unique<FunctionAST>(argument, move(body));
    }

    throw ParseError(scanner.fileName(), "Expected '.'", scanner.position());
  }

  // <name> := [a-zA-Z][a-zA-Z0-9]*
  unique_ptr<NameAST> parseNameExpr(vector<string> scope) {
    if (scanner.curr() != Token::id) {
      throw ParseError(scanner.fileName(), "Expected identifier",
                       scanner.position());
    }

    int index = -1;
    bool bound = false;
    for (auto it = scope.begin(); it != scope.end(); ++it) {
      index++;
      if (*it == scanner.curr().ValueString) {
        bound = true;
        break;
      }
    }
    if (!bound)
      index++;

    auto name = make_unique<NameAST>(scanner.curr().ValueString, index, bound);
    consume();
    return name;
  }

public:
  Parser(Scanner &scanner) : scanner{scanner} { scanner.next(); }

  // <prog> := <expression> '\n' | <eof>
  vector<unique_ptr<AST>> parse() {
    vector<unique_ptr<AST>> program{};

    while (scanner.curr() != Token::eof) {
      if (scanner.curr() == Token::undefined) {
        stringstream ss;
        ss << "Unexpected Token " << scanner.curr();
        throw ParseError(scanner.fileName(), ss.str(), scanner.position());
      }

      auto ptr = parseLine();
      program.push_back(move(ptr));

      if (scanner.curr() != Token::eof && scanner.curr() != Token::nl) {
        throw ParseError(scanner.fileName(), "Expected newline",
                         scanner.position());
      }
      consume(); // consume '\n'
    }

    return program;
  }

  unique_ptr<AST> parseLine() {
        std::cerr << scanner.curr();
        vector<string> sc;
    return move(parseExpression(sc));
  }
  string parseRaw(){
        std::cerr << scanner.curr();
    if(scanner.curr() == Token::raw){auto s = scanner.curr().ValueString;consume();return s;};
    if(scanner.curr() == Token::eof)return "end";
  }
  string parseName(){
    std::cerr << scanner.curr();
    if(scanner.curr() == Token::id){auto s = scanner.curr().ValueString;consume();return s;};
    if(scanner.curr() == Token::eof)return "end";
    //throw "not id";
  }
};

class Interpreter {
  bool isValue(AST *node) { return node && isFunction(node); }

  bool isApplication(AST *node) {
    return node && dynamic_cast<ApplicationAST *>(node);
  }

  bool isName(AST *node) { return node && dynamic_cast<NameAST *>(node); }

  bool isFunction(AST *node) {
    return node && dynamic_cast<FunctionAST *>(node);
  }

  void shift(int n, AST *node) {
    if (!node)
      return;

    if (isName(node)) {
      auto name = dynamic_cast<NameAST *>(node);
      if (!name->bound()) {
        *name += n;
      }
    } else if (isFunction(node)) {
      auto fn = dynamic_cast<FunctionAST *>(node);
      shift(n, fn->body());
    } else if (isApplication(node)) {
      auto app = dynamic_cast<ApplicationAST *>(node);
      shift(n, app->lhs());
      shift(n, app->rhs());
    }
  }

  // subst(value, node): substitutes all the variables in node that have a De
  // Bruijn index of zero by value.
  unique_ptr<AST> subst(int n, AST *value, unique_ptr<AST> node) {
    if (!value || !node)
      return nullptr;

    if (isName(node.get())) {
      unique_ptr<AST> name{move(node)};

      if (dynamic_cast<NameAST *>(name.get())->index() == n) {
        name = unique_ptr<AST>(value->clone());
        shift(n, name.get());
      }
      node = move(name);
    } else if (isFunction(node.get())) {
      auto body = move(dynamic_cast<FunctionAST *>(node.get())->Body);
      dynamic_cast<FunctionAST *>(node.get())->Body =
          subst(n + 1, value, move(body));
    } else if (isApplication(node.get())) {
      auto lhs = move(dynamic_cast<ApplicationAST *>(node.get())->Lhs);
      auto rhs = move(dynamic_cast<ApplicationAST *>(node.get())->Rhs);
      dynamic_cast<ApplicationAST *>(node.get())->Lhs =
          subst(n, value, move(lhs));
      dynamic_cast<ApplicationAST *>(node.get())->Rhs =
          subst(n, value, move(rhs));
    }

    return node;
  }

  unique_ptr<AST> substitute(unique_ptr<FunctionAST> lhs, unique_ptr<AST> rhs) {
    shift(1, rhs.get());

    unique_ptr<AST> result = move(subst(-1, rhs.get(), move(lhs)));

    shift(-1, result.get());

    if (!isFunction(result.get())) {
      throw EvalError("eval: expected function");
    }

    unique_ptr<FunctionAST> fn{dynamic_cast<FunctionAST *>(result.release())};

    return move(fn->Body);
  };

  unique_ptr<AST> evalExpr(unique_ptr<AST> ast) {

    while (true) {
      if (isApplication(ast.get())) {
        unique_ptr<ApplicationAST> app{
            dynamic_cast<ApplicationAST *>(ast.release())};

        // Rule 1
        // t1 t2 → t1' t2 iff t1 → t1'
        if (!isValue(app->lhs())) {
          app->Lhs = move(evalExpr(move(app->Lhs)));
        }

        // std::cout << "LHS: " << *app->lhs << std::endl;

        // Rule 2
        // given v1 isValue: v1 t2 → v1 t2' iff t2 -> t2'
        if (!isValue(app->rhs())) {
          app->Rhs = move(evalExpr(move(app->Rhs)));
        }

        // std::cout << "RHS: " << *app->rhs << std::endl;

        // Rule 3
        // (λx. t12) v2 → [x ↦ v2]t12
        if (dynamic_cast<FunctionAST *>(app->lhs())) {
          unique_ptr<FunctionAST> lhs{
              dynamic_cast<FunctionAST *>(app->Lhs.release())};
          unique_ptr<AST> rhs{dynamic_cast<AST *>(app->Rhs.release())};
          ast = substitute(move(lhs), move(rhs));
          continue;
        } else {
          ast = move(app);
        }
      } else if (isFunction(ast.get())) {
        unique_ptr<FunctionAST> fn{dynamic_cast<FunctionAST *>(ast.release())};
        fn->Body = move(evalExpr(move(fn->Body)));

        ast = move(fn);
      }
      break;
    }

    return ast;
  }

public:
  unique_ptr<AST> eval(unique_ptr<AST> ast) {
    if (ast != nullptr) {
      return move(evalExpr(move(ast)));
    }

    return nullptr;
  }
};

} // namespace lc
#endif

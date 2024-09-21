/*
Simple calculator

Revision history:
Revised by Seth September 2024
Revised by Seth August 2004
Originally written August 2024.

This program implements a basic expression calculator.
Input from cin; output to cout.

The grammar for input is:

Calculation:
	Statement
	Print
	Quit
	Help
	Calculation Statement
Help:
	"help"
Symbols:
	"symbols"
Print:
	";"
	"\n"
Quit:
	"q"
	"exit"
Statement:
	Declaration
	Assignment
	Expression
Declaration:
	"let" Name "=" Expression
	"#" Name "=" Expression
	"const" Name "=" Expression
Assignment:
	Name "=" Expression
Expression:
	Term
	Expression "+" Term
	Expression "-" Term
Term:
	Secondary
	Term "*" Secondary
	Term "/" Secondary
	Term "%" Secondary
Secondary:
	Primary
	Secondary "!"
Primary:
	Number
	"(" Expression ")"
	"{" Expression "}"
	"-" Primary
	"+" Primary
	Name
	Function "(" Argument ")"
Function:
	"sqrt"
	"pow"
Argument:
	Expression
	Argument "," Expression
Name:
	[alphabetic-char]
	Name [alphabetic-char]
	Name [digit-char]
	Name "_"
Number:
	[floating-point-literal]

Input comes from cin through the Token_stream called ts.
*/

#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <stdexcept>
#include <cmath>
#include <memory>
#include <algorithm>

using namespace std;

// models a grammar token
class Token {
public:
	char kind;
	double value{};						// if kind is number then store actual numerical value here
	string name;
	Token()											// default constructor
		:kind{0} {}
	explicit Token(const char ch)
		:kind{ch} {}
	Token(const char ch, const double val)
		:kind{ch}, value{val} {}
	Token(const char ch, string  n)
		:kind{ch}, name{std::move(n)} {}
};

// models cin as a Token stream
class Token_stream {
public:
	Token get();									// get a Token
	void putback(const Token& t);					// put a token back
	void ignore(char c);							// discard characters up to and including a c
	explicit Token_stream(istream& ii)
		: is{ii} { }								// constructor, takes istream
private:
	vector<Token> buffer;							// store tokens
	istream& is;									// istream we will use
};

// defined (name, value) pair
class Variable {
public:
	string name;
	double value;
	bool constant;
};

// defined variables and constants
class Symbol_table {
public:
	double get_value(const string&);
	void set_value(const string&, double);
	double define_name(const string&, double, bool);
	bool is_declared(const string&);
	void print();
private:
	vector<Variable> var_table;
};

// globals and forward declarations
double expression(Token_stream&);
Symbol_table symbols;

// token kinds
constexpr char t_number = '8';
constexpr char t_print = ';';
constexpr char t_name = 'a';
constexpr char t_quit = 'q';
constexpr char t_sqrt = 'S';
constexpr char t_pow = 'P';
constexpr char t_decl = '#';
constexpr char t_assign = '=';
constexpr char t_const = 'C';
constexpr char t_help = 'h';
constexpr char t_symbols = '$';

// keywords
const string quitkey = "quit";
const string declkey = "let";
const string constkey = "const";
const string helpkey = "help";
const string symbkey = "symbols";

// calculator functions
const string sqrtkey = "sqrt";
const string powkey = "pow";


// put Token t back into Token_stream buffer
void Token_stream::putback(const Token& t) {
	buffer.push_back(t);
}

// reads from cin to make Tokens
Token Token_stream::get() {
	// Use token from buffer if available, FIFO
	if (!buffer.empty()) {
		Token t = buffer.back();
		buffer.pop_back();
		return t;
	}

	char ch = ' ';
	while (isspace(ch) && ch != '\n')			// ignore whitespace except newline
		cin.get(ch);

	switch (ch) {
		case t_print:
		case '\n':
			return Token{t_print};
		case t_decl:
		case t_quit:
		case t_assign:
		case '(': case ')':
		case '{': case '}':
		case ',':								// separation of args in pow function
		case '+':
		case '-':
		case '*':
		case '/':
		case '!':
			return Token{ch};					// let each character represent itself
		case '.':								// floating-point literal can start with dot
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
		{
			cin.putback(ch);
			double val = 0;
			cin >> val;							// reads entire double, not just first digit
			return Token{t_number, val};
		}
		default:
			if (isalpha(ch)) {					// can also expect strings
				string s;
				s += ch;
				while (cin.get(ch) && (isalpha(ch) || isdigit(ch) || ch == '_'))
					s += ch;					// accumulate letters and numbers in string
				cin.putback(ch);

				if (s == constkey)
					return Token{t_const};
				if (s == declkey)
					return Token{t_decl};
				if (s == sqrtkey)
					return Token{t_sqrt};
				if (s == powkey)
					return Token{t_pow};
				if (s == helpkey)
					return Token{t_help};
				if (s == symbkey)
					return Token{t_symbols};
				if (s == quitkey)
					return Token(t_quit);
				return Token{t_name, s};
			}
		throw runtime_error("bad token");
	}
}

// clear input until next instance of 'c' in cin (or buffer)
void Token_stream::ignore(const char c) {
	// first look in buffer, remove all non c kind tokens
	while (!buffer.empty() && buffer.back().kind != c)
		buffer.pop_back();

	if (!buffer.empty())				// contains a c kind token
		return;

	char ch = 0;
	while (cin >> ch)					// process cin directly
		if (ch == c)
			return;
}

// return the value of the Variable named s
double Symbol_table::get_value(const string& s) {
	for (const auto&[name, value, constant] : var_table)
		if (name == s)
			return value;
	throw runtime_error("trying to read undefined variable " + s);
}

// set the value of Variable named s to d
void Symbol_table::set_value(const string& s, const double d) {
	for (auto&[name, value, constant] : var_table)
		if (name == s) {
			if (constant == true)
				throw runtime_error("trying to write to constant");
			value = d;
			return;
		}
	throw runtime_error("trying to write undefined variable " + s);
}

// is var already in var_table?
bool Symbol_table::is_declared(const string& var) {
	return ranges::any_of(
		var_table,
		[var](const Variable& v) { return v.name == var; });
}

// add {var, val} to var_table
double Symbol_table::define_name(const string& var, const double val, const bool constant) {
	if (is_declared(var))
		throw runtime_error(var + " declared twice");
	var_table.push_back(Variable{var, val, constant});
	return val;
}

void Symbol_table::print() {
	cout << "\nSymbols:\n";
	for (const auto&[name, value, constant] : var_table)
		cout << name << '\t' << value << '\n';
	cout << '\n';
}

// return result of factorial of arg x
double factorial(int x) {
	if (x < 0)
		throw runtime_error("cannot get factorial of negative number.");

	if (x == 0)
		x = 1;

	for (int i = x-1; i > 0; --i) {
		const int prev = x;
		x *= i;

		if (prev != 0 && x/prev != i)
			throw runtime_error("overflow occurred in int.");
	}

	return x;
}

double eval_function(Token_stream& ts, Token t) {
	switch (t.kind) {
		case t_sqrt:
		{
			t = ts.get();
			double inner;
			switch (t.kind) {
				case '(':
				{
					const double d = expression(ts);
					t = ts.get();
					if (t.kind != ')')
						throw runtime_error("sqrt: ')' expected");
					if (d < 0)
						throw runtime_error("cannot get square root of negative number");
					inner = d;
					break;
				}
				default:
					throw runtime_error("sqrt: primary expected");
			}
			return sqrt(inner);
		}
		case t_pow:
		{
			t = ts.get();
			double exp1, exp2;
			switch(t.kind) {
				case '(':
				{
					double d = expression(ts);
					t = ts.get();
					if (t.kind != ',')
						throw runtime_error("pow: ',' expected");
					exp1 = d;

					d = expression(ts);
					t = ts.get();
					if (t.kind != ')')
						throw runtime_error("pow: ')' expected");
					exp2 = d;

					break;
				}
				default:
					throw runtime_error("pow: primary expected");
			}
			return pow(exp1, exp2);
		}
		default:
			throw runtime_error("function not implemented");
	}
}

// deal with numbers, signage, names, functions, assignment, and parentheses/braces
double primary(Token_stream& ts) {
	switch (Token t = ts.get(); t.kind) {
		case '(':
		{
			const double d = expression(ts);
			t = ts.get();
			if (t.kind != ')')
				throw runtime_error("')' expected");
			return d;
		}
		case '{':
		{
			const double d = expression(ts);
			t = ts.get();
			if (t.kind != '}')
				throw runtime_error("'}' expected");
			return d;
		}
		case t_sqrt:
		case t_pow:
			return eval_function(ts, t);
		case t_number:
			return t.value;
		case '-':
			return - primary(ts);
		case '+':
			return + primary(ts);
		case t_name: {
			return symbols.get_value(t.name);
		}
		default:
			throw runtime_error("primary expected");
	}
}

// deal with factorials, '!'
double secondary(Token_stream& ts) {
	double left = primary(ts);
	Token t = ts.get();
	while (true) {
		switch (t.kind) {
			case '!':
				left = factorial(static_cast<int>(left));
				t = ts.get();
				break;
			default:
				ts.putback(t);
				return left;
		}
	}
}

// deal with '*', '/', and '%'
double term(Token_stream& ts) {
	double left = secondary(ts);
	Token t = ts.get();
	while (true) {
		switch (t.kind) {
			case '*':
				left *= secondary(ts);
				t = ts.get();
				break;
			case '/':
			{
				const double d = secondary(ts);
				if (d == 0)
					throw runtime_error("divide by zero");
				left /= d;
				t = ts.get();
				break;
			}
			case '%':
			{
				const double d = secondary(ts);
				if (d == 0)
					throw runtime_error("%: divide by zero");
				left = fmod(left, d);
				t = ts.get();
				break;
			}
			default:
				ts.putback(t);
				return left;
		}
	}
}

// deal with '+' and '-'
double expression(Token_stream& ts) {
	double left = term(ts);
	Token t = ts.get();
	while (true) {
		switch (t.kind) {
			case '+':
				left += term(ts);
				t = ts.get();
				break;
			case '-':
				left -= term(ts);
				t = ts.get();
				break;
			default:
				ts.putback(t);
				return left;
		}
	}
}

// declare a variable called 'name' with the initial value 'expression'
double declaration(Token_stream& ts, const bool constant) {
	const Token t = ts.get();
	if (t.kind != t_name)
		throw runtime_error("name expected in declaration");

	if (const Token t2 = ts.get(); t2.kind != '=')
		throw runtime_error("'=' missing in declaration of " + t.name);
	const double d = expression(ts);
	symbols.define_name(t.name, d, constant);
	return d;
}

// give new value to named variable
double assignment(Token_stream& ts) {
	const Token t = ts.get();
	const string var_name = t.name;

	if (!symbols.is_declared(var_name))
		throw runtime_error(var_name + " has not been declared");

	ts.get();								// skip the '='
	const double d = expression(ts);
	symbols.set_value(var_name, d);
	return d;
}

// deal with 'let'
double statement(Token_stream& ts) {
	switch (const Token t = ts.get(); t.kind) {
		case t_const:
			return declaration(ts, true);
		case t_decl:
			return declaration(ts, false);
		case t_name: {
			const Token t2 = ts.get();
			ts.putback(t2);				// need to rollback tokens to be usable
			ts.putback(t);

			if (t2.kind == t_assign)
				return assignment(ts);
			break;
		}
		default:
			ts.putback(t);
	}
	return expression(ts);
}

// move to start of next expression
void clean_up(Token_stream& ts) {
	ts.ignore(t_print);
}

void print_intro() {
	cout << "Welcome to Simple Calc.\n"
	<< "Enter '" << helpkey << "' to learn how to use this program.\n\n";
}

void print_help() {
	cout << "\nSimple Calc Help\n"
	<< "\n\tBasic Syntax:\n"
	<< "\t\tEnter '" << helpkey << "' to see this message.\n"
	<< "\t\tEnter '" << quitkey << "' or '" << t_quit << "' to exit the program.\n"
	<< "\t\tEnter '" << t_print << "' or a new line to print the results.\n"
	<< "\t\tSupported operands: '*', '/', '%', '!', '+', '-', '=' (assignment).\n"
	<< "\t\tBrackets and braces can be used to group expressions: '4*(2+3)'.\n"
	<< "\n\tFunctions:\n"
	<< "\t\t" << sqrtkey << "(n)\t\t\tsquare root of n.\n"
	<< "\t\t" << powkey << "(n, e)\t\te power of n.\n"
	<< "\n\tUser Variables:\n"
	<< "\t\tVariables names must be composed of alphanumerical characters and '_',\n"
	<< "\t\tand must start with an alphabetical character: 'a_var3', 'X', or 'y2'.\n"
	<< "\t\t" << declkey << " var = expr\t\t\tdeclare a variable named var and initializes it.\n"
	<< "\t\t" << t_decl << " var = expr\t\t\twith evaluation value of expression expr.\n"
	<< "\t\t" << constkey << " var = expr\t\tdeclare and initialize a constant named var.\n"
	<< "\t\tvar " << t_assign << " expr\t\t\t\tassign new value to previously declared variable var.\n"
	<< "\t\tEnter '" << symbkey << "' to see all variables in the program.\n"
	<< "\n\tPredefined Variables:\n"
	<< "\t\tpi\t\t3.1415926535 (constant)\n"
	<< "\t\te\t\t2.7182818284 (constant)\n"
	<< "\t\tk\t\t1000\n"
	<< "\n";
}

constexpr string prompt = "> ";
constexpr string result = "= ";         // indicates that what follows is a result

// handle main loop, any commands, calculation, and input/output prompts/messages
void calculate(Token_stream& ts) {
	while(cin) {
		try {
			cout << prompt;
			Token t = ts.get();
			while (t.kind == t_print)						// first discard all 'prints'
				t = ts.get();

			switch (t.kind) {
				case t_quit:
					return;
				case t_help:
					print_help();
					break;
				case t_symbols:
					symbols.print();
					break;
				default:									// if no commands, do and show calc
					ts.putback(t);
					cout << result << statement(ts) << "\n";
			}
		}
		catch (exception& e) {
			cerr << "error: " << e.what() << '\n';			// write error message
			clean_up(ts);
		}
	}
}

int main()
try
{
	Token_stream ts {cin}; // construct Token_stream using cin as the input stream

	// predefine names:
	symbols.define_name("pi", 3.1415926535, true);
	symbols.define_name("e", 2.7182818284, true);
	symbols.define_name("k", 1000, false);

	print_intro();

	calculate(ts);
	return 0;
}
catch (exception& e) {
	cerr << "error: " << e.what() << '\n';
	return 1;
}
catch (...) {
	cerr << "Oops: unknown exception!\n";
	return 2;
}

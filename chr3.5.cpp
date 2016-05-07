#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <cassert>
#include <string>
#include <vector>
#include <istream>
#include <fstream>
#include <iostream>

#include "lexer.h"

using namespace std;

enum EXPR {
	INT, // 0
	BOOL,
	LAMBDA, // 2
	NIL,
	PAIR,
	FUNC_CALL, // 5
	LET,
	DEFINE,
	NAME, // 8
	ARITH,
	COND,
	COMPARE,
};

struct expr_t;
struct env_entry_t;
void print_ast(expr_t *e, bool in_list);
struct env_t
{
	string name;
	expr_t *value;
	env_t *parent;

	env_t(string n):name(n),parent(NULL),value(NULL) {}

	expr_t *lookup(const string name) {
		// cout<<"lookup "<<name<<", compare "<<this->name<<endl;
		assert(parent != this);
		if (this->name == name) {
			// printf("%s = (%lu)", name.c_str(), value);
			// print_ast(value, false);
			// printf("\n");
			return value;
		}
		if (parent == NULL) return NULL;
		return parent->lookup(name);
	}
	env_t *extend(string name, expr_t *value)
	{
		// printf("extend %s\n", name.c_str());
		env_t *child = new env_t(name);
		child->value = value;
		child->parent = this;
		return child;
	}
};

// function call: (func value)
// lambda: (\ name . body)
// let: (let (pairs) body)
// arithmetic: (value name value2)
// compare: (value name value2)
// cond: (cond (pairs))
struct expr_t
{
	int type;
	int ivalue; // for int
	string str;  // for name

	expr_t *name;
	expr_t *func;
	expr_t *value;
	expr_t *value2; // only for + - * /
	expr_t *body;

	expr_t *left;
	expr_t *right;

	env_t *env; // only for lambda

	vector<pair<expr_t*,expr_t*>> pairs; // for cond and let

	expr_t() {}
	expr_t(int t):type(t) {
	}
};

// ==== parser ====

expr_t *make_name(string str)
{
	expr_t *atom;
	if (str == "true" || str == "else") {
		atom = new expr_t(BOOL);
		atom->ivalue = 1;
		return atom;
	}
	if (str == "false") {
		atom = new expr_t(BOOL);
		atom->ivalue = 1;
		return atom;
	}
	atom = new expr_t(NAME);
	atom->str = str;
	return atom;
}
expr_t *parse_atom(string str)
{
	// printf("parse_atom %s\n", str.c_str());
	if (isdigit(str[0])) {
		expr_t *atom = new expr_t(INT);
		atom->ivalue = stoi(str);
		// printf("int: %d\n", atom->ivalue);
		return atom;
	}
	if (str == "nil") {
		// printf("build nil\n");
		expr_t *atom = new expr_t(NIL);
		atom->ivalue = 0;
		return atom;
	}
	return make_name(str);
}
expr_t *parser(list_t list)
{
	if (list.is_atom) {
		// printf("atom\n");
		return parse_atom(list.atom);
	}
	expr_t *ret   = NULL;
	int size = list.list.size();
	// printf("size (%d)\n", size);
	if (size == 1) {
		return parser(list.list[0]);
	} else if (size == 2) {
		list_t first = list.list[0];
		list_t second = list.list[1];
		if (first.is_atom && first.atom == "cond") {
			assert(!second.is_atom);
			ret = new expr_t(EXPR::COND);
			for (auto t : second.list) {
				auto c = parser(t.list[0]);
				// c ? r
				t.list.erase(t.list.begin(), t.list.begin()+2);
				auto r = parser(t);
				ret->pairs.push_back(make_pair(c, r));
			}
			return ret;
		}
		// printf("FUNC_CALL (%d)\n", list.size);
		ret = new expr_t(EXPR::FUNC_CALL);
		ret->func = parser(list.list[0]);
		ret->value = parser(list.list[1]);
		return ret;
	} else if (size >= 3) {
		list_t first = list.list[0];
		list_t second = list.list[1];
		if (first.is_atom) {
			if (first.atom == "\\") {
				// cout<<"lambda"<<endl;
				ret = new expr_t(EXPR::LAMBDA);
				ret->name = make_name(list.list[1].atom);
				// printf("name is %s\n", ret->name.str);
				list.list.erase(list.list.begin(), list.list.begin()+3);
				ret->body = parser(list);
				return ret;
			}
			if (first.atom == "let") {
				ret = new expr_t(EXPR::LET);
				for (auto t : second.list) {
					assert(t.list[0].is_atom);
					auto a = make_name(t.list[0].atom);
					// a : e
					t.list.erase(t.list.begin(), t.list.begin()+2);
					auto e = parser(t);
					ret->pairs.push_back(make_pair(a, e));
				}
				list.list.erase(list.list.begin(), list.list.begin()+2);
				ret->body = parser(list);
				return ret;
			}

		}
		if (second.is_atom) {
			if (second.atom == ".") {
				list_t third = list.list[2];
				ret = new expr_t(EXPR::PAIR);
				ret->left = parser(first);
				list.list.erase(list.list.begin(), list.list.begin()+2);
				ret->right = parser(list);
				return ret;
			}
			if (second.atom == ":") {
				// cout<<"="<<"LET"<<endl;
				ret = new expr_t(EXPR::DEFINE);
				ret->name = make_name(list.list[0].atom);
				list.list.erase(list.list.begin(), list.list.begin()+2);
				ret->value = parser(list);
				// cout<<"LET end!"<<endl;
				return ret;
			} else if (second.atom.size() == 1) {
				char op = second.atom[0];
				if (op == '+' || op == '-' || op == '*' || op == '/') {
					ret = new expr_t(EXPR::ARITH);
					ret->func = parser(list.list[1]);
					ret->value = parser(list.list[0]);
					ret->value2 = parser(list.list[2]);
					return ret;
				}
				if (op == '>' || op == '<' || op == '=') {
					ret = new expr_t(EXPR::COMPARE);
					ret->func = parser(list.list[1]);
					ret->value = parser(list.list[0]);
					ret->value2 = parser(list.list[2]);
					return ret;
				}
			}
		}
	}
	return ret;
}
void print_ast(expr_t *e, bool in_list = false)
{
	// printf("(0x%lx) type: %d\n", e, e->type);
	assert(e->type >= 0);
	switch (e->type) {
		case EXPR::NIL:
			printf("nil");
			break;
		case EXPR::INT:
			printf("%d", e->ivalue);
			break;
		case EXPR::BOOL:
			printf("%s", e->ivalue ? "true" : "false");
			break;
		case EXPR::PAIR:
			if (!in_list) printf("[ ");
			print_ast(e->left);
			if (e->right->type != NIL) {
				if (e->right->type == PAIR) {
					printf(" ");
					print_ast(e->right, true);
				} else {
					printf(" . ");
					print_ast(e->right);
				}
			}
			if (!in_list) printf(" ]\n");
			break;
		case EXPR::NAME:
			printf("%s:%d", e->str.c_str(), e->type);
			break;
		case EXPR::LAMBDA:
			printf("LD(lambda (%s) ", e->name->str.c_str());
			print_ast(e->body);
			printf(")");
			break;
		case EXPR::DEFINE:
			printf("DF(%s ", e->name->str.c_str());
			print_ast(e->value);
			printf(")\n");
			break;
		case EXPR::LET:
			printf("LT(\n");
			for (auto i = e->pairs.begin(); i != e->pairs.end(); ++i)
			{
				printf("\t(");
				print_ast(i->first);
				printf(":\t");
				print_ast(i->second);
				printf(")\n");
			}
			print_ast(e->body);
			printf(")\n");
			break;
		case EXPR::FUNC_CALL:
			printf("FC(");
			print_ast(e->func);
			printf(" ");
			print_ast(e->value);
			printf(")");
			break;
		case EXPR::ARITH:
			printf("AR(");
			print_ast(e->func);
			printf(" ");
			print_ast(e->value);
			printf(" ");
			print_ast(e->value2);
			printf(")");
			break;
		case EXPR::COMPARE:
			printf("CP(");
			print_ast(e->func);
			printf(" ");
			print_ast(e->value);
			printf(" ");
			print_ast(e->value2);
			printf(")");
			break;
		case EXPR::COND:
			printf("CD(\n");
			for (auto i = e->pairs.begin(); i != e->pairs.end(); ++i)
			{
				print_ast(i->first);
				printf("?\t");
				print_ast(i->second);
				printf("\n");
			}
			printf(")\n");
	}
}

// ==== interpreter ====

env_t *env0;

expr_t *interp(expr_t *e, env_t *env)
{
	env_t *new_env;
	expr_t *ret;
	expr_t *v1, *v2;
	int v;
	switch (e->type) {
		case EXPR::NAME:
			// printf("name of '%s' in %lu\n", e->str.c_str(), &env);
			ret = env->lookup(e->str);
			if (!ret) {
				// printf("lookup %s in env0\n", e->str.c_str());
				ret = env0->lookup(e->str);
				if (!ret) {
					printf("undefined variable %s\n", e->str.c_str());
					exit(1);
				}
			}
			return ret;
		case EXPR::BOOL:
		case EXPR::NIL:
		case EXPR::INT:
			// printf("int %d\n", e->ivalue);
			return e;
		case EXPR::LAMBDA:
			// printf("lambda (%s)\n", e->name->str);
			e->env = env;
			return e;
		case EXPR::DEFINE:
			// printf("let (%s)\n", e->name->str.c_str());
			v1 = interp(e->value, env);
			env0 = env->extend(e->name->str, v1);
			return v1;
		case EXPR::LET:
			new_env = env;
			for (auto p : e->pairs) {
				v1 = interp(p.second, env);
				// printf("let extend %s\n", p.first->str.c_str());
				new_env = new_env->extend(p.first->str, v1);
			}
			return interp(e->body, new_env);
		case EXPR::ARITH:
			// printf("v1:%d, v2:%d in env(%ld)\n",
			// 	e->value->type, e->value2->type, env);
			v1 = interp(e->value, env);
			v2 = interp(e->value2, env);
			ret = new expr_t(INT);
			assert(e->func->type == NAME);
			switch (e->func->str[0]) {
				case '+':
					v = v1->ivalue + v2->ivalue;
					break;
				case '-':
					v = v1->ivalue - v2->ivalue;
					break;
				case '*':
					v = v1->ivalue * v2->ivalue;
					break;
				case '/':
					v = v1->ivalue / v2->ivalue;
					break;
			}
			ret->ivalue = v;
			return ret;
		case EXPR::COMPARE:
			v1 = interp(e->value, env);
			v2 = interp(e->value2, env);
			ret = new expr_t(BOOL);
			assert(e->func->type == NAME);
			switch (e->func->str[0]) {
				case '=':
					v = v1->ivalue == v2->ivalue;
					break;
				case '<':
					v = v1->ivalue < v2->ivalue;
					break;
				case '>':
					v = v1->ivalue > v2->ivalue;
					break;
			}
			ret->ivalue = v;
			return ret;
		case EXPR::FUNC_CALL:
			// printf("FUNC_CALL (:%d :%d) \n", e->func->type, e->value->type);
			v2 = interp(e->value, env);
			if (e->func->type == NAME) {
				string func_str = e->func->str;
				if (func_str == "head") {
					return v2->left;
				}
				if (func_str == "rest") {
					return v2->right;
				}
				if (func_str == "null") {
					ret = new expr_t(BOOL);
					ret->ivalue = v2->type == NIL ? 1 : 0;
					return ret;
				}
				if (func_str == "pair") {
					ret = new expr_t(BOOL);
					ret->ivalue = v2->type == PAIR ? 1 : 0;
					return ret;
				}
			}
			v1 = interp(e->func, env);
			// printf("FC body ");
			// print_ast(v1->body);
			// printf(" param %s = ", v1->name->str.c_str());
			// print_ast(v2);
			// printf("\n");
			new_env = v1->env->extend(v1->name->str, v2);
			ret = interp(v1->body, new_env);
			// new_env could not be delete
			return ret;
		case EXPR::COND:
			for (auto cp : e->pairs) {
				v1 = interp(cp.first, env);
				if (v1->ivalue) {
					return interp(cp.second, env);
				}
			}
			printf("cond all false\n");
			exit(1);
			break;
		case EXPR::PAIR:
			// make a pair
			v1 = interp(e->left, env);
			v2 = interp(e->right, env);
			ret = new expr_t(PAIR);
			ret->left = v1;
			ret->right = v2;
			return ret;
	}
}

int main(int argc, char const *argv[])
{
	string code;
	list_t list;
	expr_t *e;
	char const * file = "test.chr3.5";
	if (argc >= 2) {
		file = argv[1];
	}
	ifstream ifs(file, ifstream::in);
	env0 = new env_t("");
	while (ifs.good()) {
		// 补全括号规则
		// 如果某一行最后一个字符是左括号
		// 将接下来的每一行都用括号括起来
		// 直到遇见一个行首即为右括号且行末不是左括号的行
		getline(ifs, code);
		if (!code.empty() && code[code.size()-1] == '(') {
			string more_code;
			while (ifs.good()) {
				getline(ifs, more_code);
				if (more_code.empty())
				{
					continue;
				}
				if (more_code[0] == ')' && more_code[more_code.size()-1] == '(') {
					code += more_code;
				} else if (!more_code.empty() && more_code[0] == ')') {
					code += more_code;
					break;
				} else {
					code += ("(" + more_code + ")");
				}
			}
		}
		cout << "> " << code << endl;
		if (code.size() == 0) continue;
		if (code[0] == ';') continue;
		list = lexer(code.c_str());
		if (!list.is_atom && list.list.empty())
			continue;
		// print_lexer_tree(list);
		e = parser(list);
		// print_ast(e);
		e = interp(e, env0);
		printf("==> ");
		print_ast(e);
		printf("\n");
	}
}

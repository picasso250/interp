#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <cassert>
#include <string>
#include <vector>
#include <istream>
#include <fstream>
#include <iostream>

using namespace std;

struct list_t {
	bool is_atom;
	vector<list_t> list; // array
	string atom;

	list_t():is_atom(false) {}
	void set_atom(string s) {
		is_atom = true;
		atom = s;
	}
};

// ==== lexer ====

char const *_g_code;

// a list is a line or (...)
// the end is ')' or '\n'
// operator is a word or a one length char ~!@#$%^&*()_+-={}|[]\:";'<>?,./
list_t lexer_list()
{
	list_t list;
	string word;
	list_t child;
	while (1) {
		if (isalnum(*_g_code)) {
			word += *_g_code;
			_g_code++;
		} else {
			if (!word.empty()) {
				// cout<<"word:"<<word<<endl;
				child.set_atom(word);
				list.list.push_back(child);
				word.clear();
			}
			if (*_g_code && isspace(*_g_code)) {
				_g_code++;
			} else if (*_g_code == 0 || *_g_code == ')') {
				_g_code++;
				break;
			} else if (*_g_code == '(') {
				_g_code++;
				list.list.push_back(lexer_list());
			} else { // operators
				word += *_g_code;
				// cout<<"oprt:"<<word<<endl;
				child.set_atom(word);
				list.list.push_back(child);
				word.clear();
				_g_code++;
			}
		}
	}
	// cout<<"	list end!"<<endl;
	return list;
}

list_t lexer(char const *code)
{
	_g_code = code;
	return lexer_list();
}
void print_lexer_tree(list_t &list)
{
	if (list.is_atom) {
		printf("%s ", list.atom.c_str());
		return;
	}
	printf("(");
	for (auto i : list.list)
	{
		print_lexer_tree(i);
	}
	printf(")\n");
}

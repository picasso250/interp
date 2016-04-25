#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

// end is the end of current expression
int calc_iter(char *code, char **end)
{
	char numbuf[100];
	if (code[0] != '(') {
		// we are reading a number
		int i = 0;
		for (; code[i] && isdigit(code[i]); i++) {
			numbuf[i] = code[i];
		}
		numbuf[i] = 0;
		*end = code + i;
		return atoi(numbuf);
	}
	int v1, v2;
	char *end1, *end2;
	// skip '(+ ', so code + 3
	v1 = calc_iter(code+3, &end1);
	// skip the space between 2 expressions
	v2 = calc_iter(end1+1, &end2);
	// skip ')'
	*end = end2 + 1;
	// see the operator
	switch(code[1]) {
		case '+':
			return v1 + v2;
		case '-':
			return v1 - v2;
		case '*':
			return v1 * v2;
		case '/':
			return v1 / v2;		
	}
}

int calc(char *code)
{
	char *end;
	return calc_iter(code, &end);
}

int main(int argc, char const *argv[])
{
	char *code;

	// ----- examples -----
	code = "(+ 1 2)";
	printf("%s => %d\n", code, calc(code));
	// => 3

	code = "(* 2 3)";
	printf("%s => %d\n", code, calc(code));
	// => 6

	code = "(* (+ 1 2) (+ 3 4))";
	printf("%s => %d\n", code, calc(code));
	// => 21
	return 0;
}
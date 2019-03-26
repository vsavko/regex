#include <stdio.h>
#include <string.h>

#define TRUE 1
#define FALSE 0

void print_mistake(char * expression, int length);
int check_syntax (char * regex);

void main(int argc, char* argv[])
{
	int len, test;
	len = strlen(argv[1]); 
	
	if (argc < 2) {
		printf("Regular expression needed\n");
		return;
	}
	
	/*if (argc < 3) {
		printf("Search file location needed\n");
		return;
	}*/
	
	if ( check_syntax(argv[1]) == FALSE) return;


}

int check_syntax (char * regex)
{
	int * stack;
	int len, left_par, i, j;
	char prev_char = '0';
	
	left_par = 0;
	len = strlen(regex);
	
	for (i = 0; i < len; i++){
		if ( (prev_char == '*' || prev_char == '(' ||  prev_char == '+') && regex[i] == '+') {
			printf("wrong syntax with '+' at length %d\n", i);
			print_mistake(regex,i);
			return FALSE;
		}	
		if ( (prev_char == '+' || prev_char == '(' || prev_char == '*') && regex[i] == '*') {
			printf("wrong syntax with '*' at length %d\n", i);
			print_mistake(regex,i);
			return FALSE;
		}
		if ( (prev_char == '+' ) && regex[i] == ')') {
			printf("wrong syntax with ')' at length %d\n", i);
			print_mistake(regex,i);
			return FALSE;
		}				
		if (regex[i] == '(') ++left_par;
		if (regex[i] == ')') --left_par;
		if (left_par < 0){
			printf("Incorrect brackets at length %d\n", i);
			print_mistake(regex,i);
			return FALSE;
		}
		prev_char = regex[i];	
	}
	
	if (left_par != 0){
		printf("Incorrect closing brackets \n");
		return FALSE;
	}
	else 
		return TRUE;	
}

void print_mistake(char * expression, int length)
{
	int j;
	printf("%s\n",expression);
	for (j = 0 ; j < length ; j++) printf(" ");
	printf("^");
}
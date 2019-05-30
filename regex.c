#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>


#define TRUE 1
#define FALSE 0

typedef struct{
int state_in;
int num_of_transitions;
int * transitions;
} nfa_table_record;

void nfa_record_add(nfa_table_record * record, int transition_to_state){
	if (record -> num_of_transitions == 0){
		(record -> transitions) = malloc(sizeof(int));
	}
	else{
		(record -> transitions) = realloc(record -> transitions, (record -> num_of_transitions + 1) * sizeof(int));
	}
	++(record -> num_of_transitions);
	*(record -> transitions + (record -> num_of_transitions - 1)) = transition_to_state;
}

void nfa_record_delete(nfa_table_record * record, int tran_to_delete){
	int i, num_of_tr, j;
	num_of_tr = record -> num_of_transitions;
	
	for (i=0; i< num_of_tr; i++){
		if (*(record-> transitions + i ) == tran_to_delete){
			memmove(record -> transitions + i,record -> transitions + i +1, num_of_tr - i -1);
			break;
		}
	}
	--(record -> num_of_transitions);
	record -> transitions = realloc(record -> transitions, (record -> num_of_transitions) * sizeof(int));
}

int count_symbols(char * regex){
	int len, i, count = 0;
	len = strlen(regex);
	char ascii_table[256];

	for (i=0; i<256; i++){
		ascii_table[i] = '\0';
	}

	for (i=0; i<len; i++){
		if (regex[i] == '+' || regex[i] == '*' || regex[i] == '(' || regex[i] == ')')
			continue;
		if (ascii_table[regex[i]] == 0){
			ascii_table[regex[i]] = 1;
			++count;
		}
	}
	return count;
}

int count_symbols2(char * regex){
	int len, i, count = 0;
	len = strlen(regex);
	char ascii_table[256];

	for (i=0; i<256; i++){
		ascii_table[i] = '\0';
	}

	for (i=0; i<len; i++){
		if (regex[i] == '+' )
			continue;
		if (regex[i] == '*' )
			count +=2;
		else
			++count;
	}
	++count; //for one extra state, ie ab - 3 states
	return count;
}

void assign_col_to_letters (char * regex, int num_symbols, char table[]){
	int len, i, j;
	len = strlen(regex);

	for (j = 0; j < (num_symbols + 1); j++)
	table[j] = '\0';

	for (i = 0 ; i < len; i++){
		if (regex[i] == '+' || regex[i] == '*' || regex[i] == '(' || regex[i] == ')')
			continue;
		for (j = 1; j < num_symbols+1; j++){ //0 for epsilon transition
			if(regex[i] == table[j])
				break;
			if(table[j] == '\0'){
				table[j] = regex[i];
				break;
			}
		}
	}
}

int search_symbol_col (char letter, char table[], int num_symbols){
	int i;

	for (i = 0 ; i < num_symbols; i++) {
		if (letter == table[i])
			return i;	
	}	
}

int reg_to_nfa (int symbols_num2,int count_symbols, nfa_table_record  * table[][count_symbols+2], char * regex){
	int len, i;

	int state_to_add_transition; // to which row of the table (state) we will add transitions when we see a letter from sigma
	int state_start_tab[symbols_num2]; //state to return to when we see + or * after brackets
	int state_in = 0; //current number of row in state_start_tab table
	int num_tran; //number of transitions inside one cell - nfa_table_record - in the table of transitions
	int free_state = 1; //add new state to the table
	int current_state = 0; // state where we are currently
	int brackets_out_state[symbols_num2];
	int brackets_out_cur=0;
	char symbols_table[count_symbols+1];
	int symbol_col;

	/*
	table of the form:

	e   A   B   C ... e- epsilon transition, A B C - alphabet letters
	q1
	q2
	q3
	....

	*/

	state_start_tab[0] = 0;
	len = strlen(regex);	
	assign_col_to_letters(regex, count_symbols, symbols_table);


	for (i = 0; i < len; i++){	
		if (regex[i] == '('){
			nfa_record_add(table[current_state][1], free_state ); 
			current_state = free_state;
			++free_state;
			state_start_tab[++state_in] = current_state; // _( 
			brackets_out_state[++brackets_out_cur] = free_state;
			++free_state;
		}
		else if (regex[i] == '+'){
		if (state_in != 0){
			nfa_record_add(table[current_state][1], brackets_out_state[brackets_out_cur] );
		}
		current_state = state_start_tab[state_in];
		}
		else if (regex[i] == '*'){
			if (regex[i-1] == ')'){
				++state_in;
				nfa_record_add(table[current_state][1], state_start_tab[state_in] ); // 0 - epsilon transition to position before opening brackets
				nfa_record_add(table[current_state][1], free_state );
				--state_in;
				current_state = free_state;
				++free_state;
			}
			else{
				nfa_record_add(table[current_state-1][1], current_state );
				symbol_col = search_symbol_col(regex[i-1], symbols_table, count_symbols);
				nfa_record_add(table[current_state][symbol_col+1], current_state );
			}
		}
		else if (regex[i] == ')'){
			--state_in;
			nfa_record_add(table[current_state][1], brackets_out_state[brackets_out_cur] );
			current_state = brackets_out_state[brackets_out_cur--];
		}
		else {
		symbol_col = search_symbol_col(regex[i], symbols_table, count_symbols);
		nfa_record_add(table[current_state][symbol_col+1], free_state);
		current_state = free_state;
		free_state++;
		}	
	}
	printf("NFA:\n");
	printf("\tF\te\t");
	for (i= 1; i < count_symbols +1 ; i++)
	printf("%c\t", symbols_table[i]);	
	printf("\n");	
	
	return free_state;
}

void print_mistake(char * expression, int length)
{
	int j;
	printf("%s\n",expression);
	for (j = 0 ; j < length ; j++) printf(" ");
	printf("^");
}

int check_syntax (char * regex)
{
	int * stack;
	int len, left_par, i, j;
	char prev_char = '\0';
	
	left_par = 0;
	len = strlen(regex);
	
	for (i = 0; i < len; i++){
		if ( (prev_char == '(' ||  prev_char == '+' ||  prev_char == '\0') && regex[i] == '+') {
			printf("Wrong syntax with '+' at length %d\n", i);
			print_mistake(regex,i);
			return FALSE;
		}	
		if ( (prev_char == '+' || prev_char == '(' || prev_char == '*' ||  prev_char == '\0' ) && regex[i] == '*') {
			printf("Wrong syntax with '*' at length %d\n", i);
			print_mistake(regex,i);
			return FALSE;
		}
		if ( (prev_char == '+' ) && regex[i] == ')') {
			printf("Wrong syntax with ')' at length %d\n", i);
			print_mistake(regex,i);
			return FALSE;
		}	
		if (  regex[i] == '+' && i == len - 1 ) {
			printf("Wrong syntax with '+' at length %d\n", i);
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

char * RemoveExccessBrackets(char * argv)
{
	int len, i ,j;
	int *stack;
	int stackpointer = 0;
	char * new_regex;
	char * temp;
	
	len = strlen(argv);
	new_regex = malloc(len * sizeof(char) + 1 );
	temp = malloc(len * sizeof(char) + 1 );
	
	for (i=0 ; i < len; i++)
		new_regex[i] = 1;

	stack = malloc(len * sizeof(int));
	
	for (i = 0; i < len; i++){
		
		if (stackpointer >= 0 && argv[i] == '(' )
			stack[stackpointer++] = i;
		
		if (stackpointer > 0 && argv[i] == '+')
			stack[stackpointer-1] = -1;
		
		if (stackpointer > 0 && argv[i] == ')')
		{
			if(stack[stackpointer-1] >= 0 && argv[i+1] != '*'){
				new_regex[i] = 0;
				new_regex[stack[stackpointer-1]] = 0;
			}
			--stackpointer;
		}
	}
	
	for (i=0, j=0; i < len; i++){
		if (new_regex[i] != 0)
			temp[j++] = argv[i];
	}
	temp[j] = '\0';
	
	temp = realloc(temp, j);
	free(new_regex);
	free(stack);
	return temp;
}

void find_end_states(int symbols_num2, int symbols_num, nfa_table_record * table[symbols_num2][symbols_num+2], int free_states){ //symbols2 - states, symbols_num -alphabet
	int i,j,k,state,num_of_transitions, end_state;
	int * transitions;
	
	for (i=0; i<free_states; i++){
		end_state = TRUE;
		for (j=1; j<symbols_num+2 && end_state == TRUE; j++){
			if ( (num_of_transitions = table[i][j]->num_of_transitions) == 0){
				continue;
			}
			transitions = table[i][j]-> transitions;
			for (k=0; k < num_of_transitions; k++){
				if ( *(transitions+k) != i){ //if current state has transitions to other states its not final
					end_state = FALSE;
					break;
				}
			}
		}
		nfa_record_add(table[i][0], end_state );
	}
}

int find_state(nfa_table_record * record, int state){
	int i;
	
	for (i=0; i< record->num_of_transitions; i++){
		if (state == *(record->transitions + i) ){
			return 1;
		}
	}
	return 0;	
}

int * simplify_e_transitions(int symbols_num2, int symbols_num, nfa_table_record * table[symbols_num2][symbols_num+2]){
	int i,j,k,x,y,z,h, num_of_tran, transition_to_state, found;
	int * delete_states;
	
	delete_states = malloc(sizeof(int)*symbols_num2);
	
	for (i=0; i < symbols_num2; i++)
		delete_states[i] = 1;
	
	for (i = 0; i < symbols_num2; i++){
		found = 0;
		if (table[i][1] -> num_of_transitions == 0)
			continue;
		for (j = 2; j < symbols_num+2; j++){ //find state with only epsilon transitions
			if (table[i][j] -> num_of_transitions > 0){
				goto next;
			}
		}
		
		// if q1 has only epsilon transitions, find all qn that flow into q1, 
		//add transitions from q1 to all qn, then delete transitions from qn to q1 and delete q1
		
		for (x = 0; x < symbols_num2; x++){ //search rows to see which states flow into one being deleted
			if (x == i)
				continue;
			
			for (y = 1; y < symbols_num+2; y++){ //search cols - alphabet 
				num_of_tran = table[x][y] -> num_of_transitions;
				if (num_of_tran == 0)
					continue;
				
				for (k=0; k < num_of_tran; k++){ //search transitions within a state
					transition_to_state = *(table[x][y] -> transitions +k);
					
					if (transition_to_state == i){ //if found transition to state
						nfa_record_delete(table[x][y], i); //delete transition to the state that we simplify
						for (z = 0; z < table[i][1] -> num_of_transitions; z++){ //add transitions from q1 to qn and delete transitions from qn to q1
							if (find_state(table[x][y], *(table[i][1] ->transitions + z)) == 0 ){	
								nfa_record_add(table[x][y], *(table[i][1] ->transitions + z));	
							}
						}
						for (h=0; h<table[x][y] -> num_of_transitions; h++){
							}
						found = 1;
					}
				}
			}
		}	
		
		if (found == 1){ 
			table[i][1] -> num_of_transitions = 0;
			delete_states[i]=0;
			i = 0;
		}

	next:
	;
	}
	return delete_states;
}

void ECLOSE(int nfa_state, int symbols_num, int states_in[], nfa_table_record * nfa_table[][symbols_num+2]){
	int i, transition;
	
	states_in[nfa_state] = 1; //ECLOSE always transitions to itself
	
	for (i = 0; i < nfa_table[nfa_state][1] -> num_of_transitions; i++){
		transition = *(nfa_table[nfa_state][1] -> transitions + i);
			states_in[transition] = 1;
		ECLOSE(transition, symbols_num, states_in, nfa_table);
	}
}

typedef struct{
int *line_num;
int *char_num;
} text_output;

void set_table(int table[], int length){
	int i;
	for (i=0; i<length; i++){
		table[i] = 0;
	}
}

//prints the line and char number
text_output search_text(char * text, int symbols_num, int symbols_num2, nfa_table_record * nfa_table[][symbols_num+2]){
	int states_in_A[symbols_num2], states_in_B[symbols_num2];
	int i, symb_count, char_nr = 0, line_nr = 0, found_cases = 0;
	text_output output;
	
	set_table(states_in_A, symbols_num2);
	ECLOSE(0, symbols_num, states_in_A, nfa_table);
	
	while (text[symb_count++] != '\0'){ //!= EOF
		if (text[symb_count] = '\n'){
			++line_nr;
			char_nr = 0;
		}
		
		// find transitions from current states on input symbol 
		for (){
			
		}
		
		++char_nr;
	}
	
	for(i = 0 ; i < symbols_num2; i++)
		printf("%d %d\n",i,states_in_A[i]);
	
	output.line_num = malloc(sizeof(int)*found_cases+1);
	output.char_num = malloc(sizeof(int)*found_cases+1);
	
	if (found_cases == 0)
		*(output.line_num) = -1;

	return output;
}

void main(int argc, char * argv[]){
	int i, j, k, argv_char_count = 0;
	char * regex ="a+b";
	int argv_end = 0, len, symbols_num, symbols_num2, free_state;
	int * delete_states;
	int dfa_states;
	char * text = "abcd or abgfd\n abcdgsg \n rtrtew\n";
	text_output output;
	
	printf("%s",argv[1]);
	
	if (argc > 1){
		if (*(argv[1]) != '"'){
			printf("regular expression must begin by quotes - \\\"");
			return;
		}
		for (i = 1; i <= (argc-1); i++){
			len = strlen(argv[i]);
			if (*(argv[i]+len-1) == '"'){
				argv_end = i;
				break;
			}	
		}
		if (argv_end == 0){
			printf("regular expression must end by quotes - \\\"");
			return;
		}	
		for (i = 1; i <= argv_end; i++){
			argv_char_count += strlen(argv[i]);
		}
		argv_char_count += (argv_end-2) - 2;	//?
		
		printf("%d\n",argv_char_count);
		
		regex = malloc(sizeof(char)*argv_char_count);
		regex[0] = '\0';	
		for (i = 1; i <= (argv_end); i++){
			if (i == 1 && i == argv_end)
				strncat(regex,argv[i]+1, strlen(argv[i])-2); //delete quotes
			else if(i == 1)
				strcat(regex,argv[i]+1);
			else if(i == argv_end)
				strncat(regex,argv[i], strlen(argv[i])-1);
			else
				strcat(regex,argv[i]);
			if (i < argv_end)
				strcat(regex," ");
		}
	}
	if ( check_syntax(regex) == FALSE) return;
	regex = RemoveExccessBrackets(regex);
	symbols_num = count_symbols(regex);
	symbols_num2 = count_symbols2(regex);
	
	nfa_table_record * table[symbols_num2][symbols_num+2];
	
	printf("REGEX: %s %d\n",regex,symbols_num2);

	
	for (i= 0; i < symbols_num2;  i++){
		for (j = 0; j<symbols_num+2; j++){
			table[i][j] = malloc(sizeof(nfa_table_record));
			table[i][j] -> num_of_transitions = 0;
		}
	}

	free_state = reg_to_nfa(symbols_num2, symbols_num, table, regex);
	find_end_states(symbols_num2, symbols_num, table, free_state);
	
	/*for (i= 0; i < free_state; i++){
		printf("%d\t",i);
		for (j = 0; j < symbols_num + 2 ; j++){
			if(table[i][j] -> num_of_transitions > 0){
				for (k = 0 ; k < table[i][j] -> num_of_transitions; k++) 
					printf("%d,", *(table[i][j] -> transitions + k) );
				printf(" \t" );
			}
			else
				printf("n\t" );
		}
		printf("\n");
	}*/
	
	printf("\n");
	delete_states = simplify_e_transitions(symbols_num2, symbols_num, table);
	
	for (i= 0; i < free_state; i++){
		if (*(delete_states + i) == 0)
			continue;
		printf("%d\t",i);
		for (j = 0; j < symbols_num + 2 ; j++){
			if(table[i][j] -> num_of_transitions > 0){
				for (k = 0 ; k < table[i][j] -> num_of_transitions; k++) 
					printf("%d,", *(table[i][j] -> transitions + k) );
				printf(" \t" );
			}
			else
				printf("n\t" );
		}
		printf("\n");
	}

	output = search_text(text, symbols_num,  symbols_num2, table);
	
	if (*(output.line_num) == -1)
		printf("No mathces found!");
		

	for (i= 0; i < symbols_num2;  i++){
		for (j = 0; j<symbols_num+2; j++){
			free(table[i][j]);
		}
	}
	free(regex);

}

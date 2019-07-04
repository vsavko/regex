#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#define TRUE 1
#define FALSE 0
#if defined(__linux__)
    #define PLATFORM_NAME 1
#else 
	#define PLATFORM_NAME 0
#endif
#define KRED  "\x1B[31m"
#define KWHT  "\x1B[37m"

typedef struct{
char state_name;
int state_in;
int num_of_transitions;
int * transitions;
} nfa_table_record;

typedef struct{
int found;
int *line_num;
int *char_num_begin;
int *char_num_end;
int found_lines;
char **line_text;
} text_output;

void check_malloc(void *pointer, int line){
	if (pointer == NULL)
	{
		printf("Malloc error on %d", line);
		exit(EXIT_FAILURE);
	}	
}

void nfa_record_add(nfa_table_record * record, int transition_to_state){
	if (record -> num_of_transitions == 0){
		(record -> transitions) = malloc(sizeof(int));
		check_malloc((void*)record -> transitions,__LINE__);
	}
	else{
		(record -> transitions) = realloc(record -> transitions, (record -> num_of_transitions + 1) * sizeof(int));
		check_malloc((void*)record -> transitions,__LINE__);
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
	if (record -> num_of_transitions >0)
		check_malloc((void*)(record -> transitions),__LINE__);
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
	int len, i, count = 2; //from 2 because we always have state Q0 and an e-transition to state Q1
	len = strlen(regex);

	for (i=0; i<len; i++){
		if (regex[i] == '+' )
			++count;
		if (regex[i] == '*' )
			continue;
		if (regex[i] == '(' )
			count += 2; //state_startt and state out
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
	int len, i, x ,j, k;
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

	
	//table of the form:
	//	F	e   A   B   C ... e- epsilon transition, A B C - alphabet letters
	//	q1	0	1,2	n	2	n
	//	q2	1	2	n	1	n
	//	q3	0	n	n	n	4
	//....
	
	state_start_tab[0] = 0;
	len = strlen(regex);	
	assign_col_to_letters(regex, count_symbols, symbols_table);
	
	table[0][0] -> state_name = 'F'; //Final states
	table[0][1] -> state_name = 'e'; //Epsilon transitions 
	
	for (i = 2 ; i <= count_symbols+1; i++){
		table[0][i] -> state_name = symbols_table[i-1];
	}

	//initialize accepting  - F stastes to zero
	for (i=0; i<symbols_num2; i++)
		nfa_record_add(table[i][0], 0);
	

	current_state = free_state;
	nfa_record_add(table[0][1], current_state); //epsilon transition from initial state
	++free_state;
	
	//making the NFA table
	for (i = 0; i < len; i++){	

		if (regex[i] == '('){
			nfa_record_add(table[current_state][1], free_state ); 
			state_start_tab[++state_in] = current_state; // _( 
			current_state = free_state;
			++free_state;
			brackets_out_state[++brackets_out_cur] = free_state;
			++free_state;
		}
		else if (regex[i] == '+'){
			if (state_in != 0){
				nfa_record_add(table[current_state][1], brackets_out_state[brackets_out_cur] );
			}
			if (state_in == 0){
				*(table[current_state][0]->transitions) = 1 ;//final state found
			}
			current_state = free_state;
			++free_state;
			nfa_record_add(table[state_start_tab[state_in]][1], current_state ); 
			
		}
		else if (regex[i] == '*'){
			if (regex[i-1] == ')'){
				++state_in;
				nfa_record_add(table[current_state][1], state_start_tab[state_in] ); // 0 - epsilon transition to position before opening brackets
				nfa_record_add(table[current_state-1][1], current_state ); // epsilon transition to next state
				nfa_record_add(table[current_state][1], free_state );
				--state_in;
				current_state = free_state;
				++free_state;
			}
			else{
				if (brackets_out_cur > 0 && current_state-1 ==  brackets_out_state[brackets_out_cur]){
					nfa_record_add(table[current_state][1], current_state-2 ); //epsilon transition to the state from
					nfa_record_add(table[current_state-2][1], current_state ); // epsilon transition to next state in case of brackets brackets out state
				}
				else{
					nfa_record_add(table[current_state][1], current_state-1 );	
					nfa_record_add(table[current_state-1][1], current_state ); // epsilon transition to next state
				}
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
	
	*(table[current_state][0]->transitions) = 1 ;//final state 


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
	check_malloc((void*)new_regex, __LINE__);
	
	temp = malloc(len * sizeof(char) + 1 );
	check_malloc((void*)temp, __LINE__);
	
	for (i=0 ; i < len; i++)
		new_regex[i] = 1;

	stack = malloc(len * sizeof(int));
	check_malloc((void*)stack, __LINE__);
	
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
		
	temp = realloc(temp, (j+1)*sizeof(char));
	if (j > 0)
		check_malloc((void*)temp,__LINE__);

	free(new_regex);
	free(stack);
	return temp;
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
	check_malloc((void*)delete_states,__LINE__);
	
	for (i=0; i < symbols_num2; i++)
		delete_states[i] = 1;
	
	
	for (i = 0; i < symbols_num2; i++){
		found = 0;
		if (table[i][1] -> num_of_transitions == 0 || *(table[i][0] ->transitions) == 1)
			continue;
		
		for (j = 2; j < symbols_num+2; j++){ //find state with only epsilon transitions
		
			if (table[i][j] -> num_of_transitions > 0 ){
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

void ECLOSE(int nfa_state, int symbols_num, int states_in[][2], nfa_table_record * nfa_table[][symbols_num+2], int closed_states[]){
	int i, transition;
	
	states_in[nfa_state][0] = 1; //ECLOSE always transitions to itself

	for (i = 0; i < nfa_table[nfa_state][1] -> num_of_transitions; i++){
		transition = *(nfa_table[nfa_state][1] -> transitions + i);
			states_in[transition][0] = 1;
			states_in[transition][1] = states_in[nfa_state][1];
		if (closed_states[transition] == 0){
			closed_states[transition] = 1;
			ECLOSE(transition, symbols_num, states_in, nfa_table, closed_states);
		}
	}
}

void set_table(int table[][2], int length){
	int i;
	table[0][0] = TRUE; //initial state always true as we cycle among the text
	table[0][1] = -1;
	for (i=1; i<length; i++){
		table[i][0] = FALSE;
		table[i][1] = -1;
	}
}

//prints the line and char number
text_output search_text(FILE * search_text_input, int symbols_num, int symbols_num2, nfa_table_record * nfa_table[][symbols_num+2]){
	int states_in_A[symbols_num2][2], states_in_B[symbols_num2][2], symbol_col, NFA_empty = TRUE, found_lines = 0, new_found_line = FALSE ;
	int i,j,k, symb_count, char_nr = 0, line_nr = 0, found_cases = 0, closed_states[symbols_num2], transition;
	char text;
	text_output output;
	
	// states_in_A[symbols_num2][2] - current states that NFA is in, x-dimention 1/0 if is in state, y-dim the number of begin_char from text line
		
	output.line_num = malloc(sizeof(int));
	check_malloc((void*)output.line_num,__LINE__);
	
	output.char_num_begin = malloc(sizeof(int));
	check_malloc((void*)output.char_num_begin,__LINE__);
	
	output.char_num_end = malloc(sizeof(int));
	check_malloc((void*)output.char_num_end,__LINE__);
	
	output.line_text = malloc(100 * sizeof(char*));
	check_malloc((void*)output.line_text,__LINE__);
	
	output.line_text[found_lines] = malloc(100 * sizeof(char));
	check_malloc((void*)output.line_text[found_lines],__LINE__);
	
	*(output.char_num_begin) = -1;
	*(output.line_num) = -1;
	
	set_table(states_in_A, symbols_num2);
	set_table(states_in_B, symbols_num2);
	
	while ((text = getc(search_text_input)) != '\0' && text != EOF){ 	
			
		if (text == '\n'){
			++line_nr;
			if (new_found_line == TRUE){
				*(output.line_text[found_lines] + char_nr) = '\0';
				++found_lines;
				if (found_lines > 99){
					output.line_text = realloc(output.line_text, sizeof(char*)*(found_lines+1));
					check_malloc((void*)output.line_text,__LINE__);
				}
				output.line_text[found_lines] = malloc(100 * sizeof(char));
				check_malloc((void*)output.line_text[found_lines],__LINE__);
				
				new_found_line = FALSE;
			}
			char_nr = 0;
			continue;
		}
		
		*(output.line_text[found_lines] + char_nr) = text;	
		symbol_col = 0;
			
		//find symbol column
		for (i = 2; i < symbols_num+2; i++){
			if (nfa_table[0][i] -> state_name == text){
				symbol_col = i;
				states_in_A[0][1] = char_nr;
				break;
			}
		}
		
		for (i = 0; i < symbols_num2; i++)
			closed_states[i] = 0;
	
		ECLOSE(0, symbols_num, states_in_A, nfa_table, closed_states);	
		
		if (symbol_col >= 2){
			// find transitions from current states on input symbol 
			// symbols_num2 - rows 
			for (k = 0; k < symbols_num2; k++)
				closed_states[k] = 0;

			for (i = 0; i < symbols_num2 ; i++){ //search through states of NFA we are at and add transitions to new ones on current aplhabet symball 
				if (states_in_A[i][0] == TRUE){					
					for (j = 0 ; j < nfa_table[i][symbol_col] -> num_of_transitions; j++){
						transition = *(nfa_table[i][symbol_col] ->transitions + j);
						states_in_B [transition][0] = TRUE;
						if (states_in_B [transition][1] == -1)
							states_in_B [transition][1] = states_in_A[i][1];
						
						ECLOSE(transition, symbols_num, states_in_B, nfa_table, closed_states);
					}
				}
			}
						
			//check if we ended up in a final state
			for (i = 1; i < symbols_num2 ;i++){
				if (states_in_B[i][0] == 1 && *(nfa_table[i][0]->transitions) == 1){
					++found_cases;
					
					output.line_num = realloc(output.line_num, sizeof(int)*(found_cases+1) );
					check_malloc((void*)output.line_num,__LINE__);
					
					output.char_num_end = realloc(output.char_num_end, sizeof(int)*(found_cases+1) );
					check_malloc((void*)output.char_num_end,__LINE__);
					
					output.char_num_begin = realloc(output.char_num_begin, sizeof(int)*(found_cases+1) );
					check_malloc((void*)output.char_num_begin,__LINE__);
					
					*(output.line_num + found_cases-1) = line_nr;
					*(output.char_num_end + found_cases-1) = char_nr;
					*(output.char_num_begin + found_cases-1) = states_in_B[i][1];
					new_found_line = TRUE;
				}
			}
			//copy B to A and delete B
			states_in_A[0][0] = TRUE;
			for (i = 1; i < symbols_num2 ;i++){
				states_in_A[i][0] = states_in_B[i][0];
				states_in_A[i][1] = states_in_B[i][1];
				states_in_B[i][0] = FALSE;
				states_in_B[i][1] = -1;
			}
		}
		else{
			for (i = 1; i < symbols_num2; i++){
				states_in_A[i][0] = FALSE;
				states_in_A[i][1] = -1;
				states_in_B[i][0] = FALSE;
				states_in_A[i][1] = -1;
				NFA_empty = TRUE;
			}	
		}
		++char_nr;
		if (char_nr > 99){
			output.line_text[found_lines] = realloc(output.line_text[found_lines], sizeof(char)*(char_nr+1));
			check_malloc((void*)output.line_text[found_lines],__LINE__);
		}
	}
	
	if (new_found_line == TRUE){
		*(output.line_text[found_lines] + char_nr) = '\0';
	}
		
	output.found = found_cases;
	output.found_lines = found_lines;

	return output;
}

void print_lines(char * str, char * str2, int len, int line, int window_size){
	int char_left, x ,y, stop = FALSE;
	
	printf("\nLine %d:\n",line);				
	for (x = 0; x < len; x += window_size-1){
		char_left = (len - x < window_size - 1) ? len - x : window_size-1;
		for (y = 0; y < char_left; y++){
			printf("%c",*(str+x+y));
		}
		printf("\n");
		for (y = 0; y < char_left && stop == FALSE; y++){
			printf("%c",*(str2+x+y));
			if (*(str2+x+y) == '\0'){
				stop = TRUE;
				printf("\n");
			}
		}
		if (stop == FALSE) printf("\n");
	}
	
}

void print_lines_linux(char * str, char * str2, int len, int line, int window_size){
	int char_left, x ,y;
	
	printf("\nLine %d:\n",line);				
	for (x = 0; x < len; x += window_size-1){
		char_left = (len - x < window_size - 1) ? len - x : window_size-1;
		for (y = 0; y < char_left; y++){
			if (*(str2+x+y) =='^' || *(str2+x+y) =='-'){
				printf("%s%c",KRED,*(str+x+y));
			}
			else{
				printf("%s%c", KWHT,*(str+x+y));
			}	
		}
		printf("%s\n",KWHT);
	}
	
}

void main(int argc, char * argv[]){
	int i, j, k, y, x, argv_char_count = 0, window_size = 100, char_left;
	char *regex, *str2, *temp ;
	int argv_end = 0, len, symbols_num, symbols_num2, free_state;
	int *delete_states;
	int dfa_states, previos_line;
	int line, char_beg, char_end;
	FILE *search_text_input;
	text_output output;

	//check enetered parameters
	if (argc > 1){
		//check that regex is in quotes
		if (*(argv[1]) != '"'){ 
			printf("regular expression must begin by quotes - \\\"");
			return;
		}
		//search quoted text for the argument with end quotes
		for (i = 1; i <= (argc-1); i++){
			len = strlen(argv[i]);
			if (*(argv[i]+len-1) == '"'){
				argv_end = i;
				break;
			}	
		}
		//check if ending quotes are present
		if (argv_end == 0){
			printf("regular expression must end by quotes - \\\"");
			return;
		}	
		
		//find the character length of the regex
		for (i = 1; i <= argv_end; i++){
			argv_char_count += strlen(argv[i]);
		}
		argv_char_count += (argv_end-1) - 2;	//sum up argv_end because we are not counting the spaces between arguments, 
												//-1 because there is one less space than arguments
												// -2 is removing the quotes. 
		
		regex = malloc(sizeof(char)*(argv_char_count+1));
		check_malloc((void*)regex,__LINE__);
		
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
	else {
		printf("Enter a regular expression as a 2nd argument of the program\n");
		return;
	}
	
	if(argc <= argv_end){
		printf("Enter a file to search for as an argument after the regex expression.\n");
		return;
	}
	else {
		
		search_text_input = fopen(argv[argv_end+1], "r");	
		if (search_text_input == NULL)
		   {
			  perror("Error while opening the file.\n");
			  exit(EXIT_FAILURE);
		   }
	}
	printf("argc %d argv_end+1 %d\n",argc,argv_end+1);
	
	if (argc > argv_end+2 && atoi(argv[argv_end+2]) > 1 )
		window_size = atoi(argv[argv_end+2]);
	
	if ( check_syntax(regex) == FALSE) return;
	
	temp = regex;
	regex = RemoveExccessBrackets(temp);
	free(temp);
	symbols_num = count_symbols(regex);
	symbols_num2 = count_symbols2(regex) +1; //+1 is for header alphabet symbols
	
	nfa_table_record * table[symbols_num2][symbols_num+2];

	printf("REGEX: \"%s\", num of unique char: %d\n",regex,symbols_num);
	
	for (i= 0; i < symbols_num2;  i++){
		for (j = 0; j<symbols_num+2; j++){
			table[i][j] = malloc(sizeof(nfa_table_record));
			check_malloc((void*)table[i][j],__LINE__);
			
			table[i][j] -> num_of_transitions = 0;
		}
	}
	
	free_state = reg_to_nfa(symbols_num2, symbols_num, table, regex);	
	printf("\n");
	
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
	delete_states = simplify_e_transitions(symbols_num2, symbols_num, table);
	printf("NFA:\n");
	
	//print NFA table
	for (i= 0; i < free_state; i++){
		if (*(delete_states + i) == 0)
			continue;
		if ( i > 0 )
			printf("%d\t",i);
		else
			printf("\t");
		if (i == 0){
			for (j = 0; j < symbols_num + 2 ; j++){
				printf("%c\t",table[0][j] -> state_name);
			}
			printf("\n");
		printf("%d\t",i);
		}
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

	output = search_text(search_text_input, symbols_num,  symbols_num2, table);

	fclose(search_text_input);
	//print out search results
	if (output.found == 0)
		printf("\nNo matches found!");
	
	else{
		previos_line = -1;
		printf("Output found: %d\n",(output.found==1)?output.found:output.found-1 );	
		str2 = malloc(sizeof(char));
		
		for(i = 0, j = 0; i < output.found; i++){
			
			line = *(output.line_num + i);
			char_beg = *(output.char_num_begin + i);
			char_end = *(output.char_num_end + i);
			
			if (previos_line != *(output.line_num + i)) {
				if (previos_line > -1){		
					if(PLATFORM_NAME == 0){
						print_lines(output.line_text[j++], str2, len, line-1, window_size);
					}
					else
						print_lines_linux(output.line_text[j++], str2, len, line-1, window_size);
				}
				previos_line = *(output.line_num + i);
				len = strlen(output.line_text[j]);
				
				str2 = realloc(str2,sizeof(char)*(len+1));
				check_malloc((void*)str2,__LINE__);
				
				k = 0;
			}
			for (; k < len; k++){
				if (k == char_beg) {
					*(str2+k) = '^';
					if(k == char_end){
						++k;
						break;
					}
				}
				else if(k == char_end){
					*(str2+k) = '^';
					++k;
					break;
				}
				else if(k > char_beg && k < char_end){
					*(str2+k) = '-';
				}
				else{
					*(str2+k) = ' ';
				}
			}
			*(str2+k) = '\0';
		}
		
		if(PLATFORM_NAME == 0)
			print_lines(output.line_text[j], str2, len, line, window_size);
		else
			print_lines_linux(output.line_text[j], str2, len, line, window_size);
	}


	for (i= 0; i < symbols_num2;  i++){
		for (j = 0; j<symbols_num+2; j++){
			if (table[i][j] ->num_of_transitions >0)
				free(table[i][j] ->transitions);
			free(table[i][j]);
		}
	}

	//DONT FORGET TO FREE 
	if (output.found > 0){
		for (i=0; i <= output.found_lines; i++)
			free(output.line_text[i]);
		free(output.line_text);
		free(output.line_num);
		free(output.char_num_begin);
		free(output.char_num_end);	
		free(str2);		
	}

	free(regex);
	free(delete_states);

}

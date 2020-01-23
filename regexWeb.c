#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#define TRUE 1
#define FALSE 0

typedef struct{
unsigned char state_name;
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
unsigned char **line_text;
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

int count_symbols( unsigned char * regex){
	int len, i, count = 0;
	len = strlen(regex);
	unsigned char ascii_table[256];

	for (i=0; i<256; i++){
		ascii_table[i] = 0;
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

int count_symbols2( unsigned char * regex){
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

unsigned char find_escaped_char(unsigned char regex){
	char text;
			switch (regex){
			case 1:
				text = 43; // +
				break;
				
			case 2:
				text = 42; // *
				break;
				
			case 3:
				text = 40; // (
				break;
				
			case 4:
				text = 41; // )
				break;
			default:
				text = regex;
				break;
		}
	return text;
}

void assign_col_to_letters ( unsigned char * regex, int num_symbols,  unsigned char table[]){
	int len, i, j;
	len = strlen(regex);
	unsigned char text;
	
	for (j = 0; j < (num_symbols + 1); j++)
	table[j] = '\0';

	for (i = 0 ; i < len; i++){
		if (regex[i] == '+' || regex[i] == '*' || regex[i] == '(' || regex[i] == ')')
			continue;
		
		//add escaped characters back
		text = find_escaped_char(regex[i]);
		
		for (j = 1; j < num_symbols+1; j++){ //0 for epsilon transition
			if(text == table[j])
				break;
			if(table[j] == '\0'){
				table[j] = text;
				break;
			}
		}
	}
}

int search_symbol_col ( unsigned char letter, unsigned char table[], int num_symbols){
	int i;
	
	for (i = 0 ; i < num_symbols +1; i++) {
		if (letter == table[i]){
			return i;	
		}
	}	
	return -1;
}

int reg_to_nfa (int symbols_num2,int count_symbols, nfa_table_record  * table[][count_symbols+2],  unsigned char * regex){
	int len, i, x ,j, k;
	int state_to_add_transition; // to which row of the table (state) we will add transitions when we see a letter from sigma
	int state_start_tab[symbols_num2]; //state to return to when we see + or * after brackets
	int state_in = 0; //current number of row in state_start_tab table
	int num_tran; //number of transitions inside one cell - nfa_table_record - in the table of transitions
	int free_state = 1; //add new state to the table
	int current_state = 0; // state where we are currently
	int brackets_out_state[symbols_num2];
	int brackets_out_cur=0;
	unsigned char symbols_table[count_symbols+1];
	int symbol_col;
	unsigned char text;
	
	
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
		text = find_escaped_char(regex[i]);
		if ( (symbol_col = search_symbol_col(text, symbols_table, count_symbols)) <0){
			printf("Wrong symbol in search_symbol_col line %d",__LINE__);
			exit(EXIT_FAILURE);
		}
		nfa_record_add(table[current_state][symbol_col+1], free_state);
		current_state = free_state;
		free_state++;
		}	
	}
	
	*(table[current_state][0]->transitions) = 1 ;//final state 

	return free_state;
}


void print_mistake( unsigned char * expression, int length)
{
	int y,z = 0;
	char * print_line;
	
	print_line = malloc(sizeof(char)*length*40);

	for (y = 0; y <= length; y++){
		if ( y == length){
				sprintf(print_line+z, "<font color=\"red\">");
				z += 18;

			sprintf(print_line+z,"%c",*(expression+y)),
			++z;
		}
		else{
			sprintf(print_line+z, "%c",*(expression+y));
			++z;
		}	
	}
	sprintf(print_line+z, "</font>");
	z += 7;
	*(print_line+z) = '\0';
	
	printf("%s<br>",print_line);
	free(print_line);
	
}

int check_syntax ( unsigned char * regex)
{
	int * stack;
	int len, left_par, i, j;
	unsigned char prev_char = '\0';
	
	left_par = 0;
	len = strlen(regex);
	
	for (i = 0; i < len; i++){
		if ( (prev_char == '(' ||  prev_char == '+' ||  prev_char == '\0') && regex[i] == '+') {
			printf("Wrong syntax with '+' at length %d<br>", i);
			print_mistake(regex,i);
			return FALSE;
		}	
		if ( (prev_char == '+' || prev_char == '(' || prev_char == '*' ||  prev_char == '\0' ) && regex[i] == '*') {
			printf("Wrong syntax with '*' at length %d<br>", i);
			print_mistake(regex,i);
			return FALSE;
		}
		if ( (prev_char == '+' ) && regex[i] == ')') {
			printf("Wrong syntax with ')' at length %d<br>", i);
			print_mistake(regex,i);
			return FALSE;
		}	
		if (  regex[i] == '+' && i == len - 1 ) {
			printf("Wrong syntax with '+' at length %d<br>", i);
			print_mistake(regex,i);
			return FALSE;
		}			
		if (regex[i] == '(') ++left_par;
		if (regex[i] == ')') --left_par;
		if (left_par < 0){
			printf("Incorrect brackets at length %d<br>", i);
			print_mistake(regex,i);
			return FALSE;
		}
		prev_char = regex[i];	
	}
	
	if (left_par != 0){
		printf("Incorrect closing brackets <br>");
		return FALSE;
	}
	else 
		return TRUE;	
}

 unsigned char * RemoveExccessBrackets( unsigned char * argv)
{
	int len, i ,j;
	int *stack;
	int stackpointer = 0;
	 unsigned char * new_regex;
	 unsigned char * temp;
	
	len = strlen(argv);
	
	new_regex = malloc(len * sizeof(unsigned char) + 1 );
	check_malloc((void*)new_regex, __LINE__);
	
	temp = malloc(len * sizeof(unsigned char) + 1 );
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
		
	temp = realloc(temp, (j+1)*sizeof(unsigned char));
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
			if (states_in[transition][1] == -1) //if allready holds a start state dont replace with a new one
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

//prints the line and unsigned char number
text_output search_text( unsigned char * search_text_input, int symbols_num, int symbols_num2, nfa_table_record * nfa_table[][symbols_num+2]){
	int states_in_A[symbols_num2][2], states_in_B[symbols_num2][2], symbol_col, NFA_empty = TRUE, found_lines = 0, new_found_line = FALSE ;
	int i,j,k,x=0, symb_count, char_nr = 0, line_nr = 0, found_cases = 0, closed_states[symbols_num2], transition;
	 unsigned char text;
	text_output output;
	
	// states_in_A[symbols_num2][2] - current states that NFA is in, x-dimention 1/0 if is in state, y-dim the number of begin_char from text line
		
	output.line_num = malloc(sizeof(int));
	check_malloc((void*)output.line_num,__LINE__);
	
	output.char_num_begin = malloc(sizeof(int));
	check_malloc((void*)output.char_num_begin,__LINE__);
	
	output.char_num_end = malloc(sizeof(int));
	check_malloc((void*)output.char_num_end,__LINE__);
	
	output.line_text = malloc(100 * sizeof(unsigned char*));
	check_malloc((void*)output.line_text,__LINE__);
	
	output.line_text[found_lines] = malloc(100 * sizeof(unsigned char));
	check_malloc((void*)output.line_text[found_lines],__LINE__);
	
	*(output.char_num_begin) = -1;
	*(output.line_num) = -1;
	
	set_table(states_in_A, symbols_num2);
	set_table(states_in_B, symbols_num2);
	
	while ((text = search_text_input[x++]) != '\0' && text != EOF){ 	
		if (text == '\n'){
			++line_nr;
			if (new_found_line == TRUE){
				*(output.line_text[found_lines] + char_nr) = '\0';
				++found_lines;
				if (found_lines > 99){
					output.line_text = realloc(output.line_text, sizeof(unsigned char*)*(found_lines+1));
					check_malloc((void*)output.line_text,__LINE__);
				}
				output.line_text[found_lines] = malloc(100 * sizeof(unsigned char));
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
						if (states_in_B [transition][1] == -1){
							states_in_B [transition][1] = states_in_A[i][1];
						}
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
			output.line_text[found_lines] = realloc(output.line_text[found_lines], sizeof(unsigned char)*(char_nr+1));
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


void print_lines_web( unsigned char * str,  unsigned char * str2, int len, int line, int window_size){
	int char_left, x ,y,z = 0;
	char * print_line;
	int color = 0;
	
	print_line = malloc(sizeof(char)*len*40);
	
	printf("<br>Line %d:<br>",line);	

	for (x = 0; x < len; x += window_size-1){
		char_left = (len - x < window_size - 1) ? len - x : window_size-1;
		for (y = 0; y < char_left; y++){
			if (*(str2+x+y) =='^' || *(str2+x+y) =='-'){
				if (color == 0){
					sprintf(print_line+z, "<font color=\"red\">");
					z += 18;
					color = 1;
				}
				sprintf(print_line+z,"%c",*(str+x+y)),
				++z;
			}
			else{
				if (color == 1){
				sprintf(print_line+z, "</font>");
				z += 7;
				color = 0;
				}
				sprintf(print_line+z, "%c",*(str+x+y));
				++z;
			}	
		}
	}
	if (color == 1){
		sprintf(print_line+z, "</font>");
		z += 7;
		color = 0;
	}
	*(print_line+z) = '\0';
	
	printf("%s<br>",print_line);
	free(print_line);
}

//web part
 int * find_form_text_char_nr( unsigned char * text, int len){
	int first_end, first_start, second_start, second_end, j;
	int *output = malloc(sizeof(int)*5);
	
	output[4] = 0;
	
	first_end = len;
	second_start = len;
	second_end = len;
	
	for (j = 0; j < len; j++){
		if  (text[j] == '=' && j < first_end){
			first_start = j;
		}
		if (text[j] == '&' && j < second_start){
			first_end = j;
		}
		if (text[j] == '&' && j > second_start){
			second_end = j;
			output[4] = 1;
		}
		if  (text[j] == '=' && j > first_end && j < second_end){
			second_start = j;
		}
	}
	
	output[0] = first_start;
	output[1] = first_end;
	output[2] = second_start;
	output[3] = second_end;
	
	
	return output;
 }
 
 unsigned char * format_text( unsigned char* text_data, int text_len){
	int i, j, k;
	int states[6] ={0};
	int states_tmp[6] = {0};
	 unsigned char* text_data2 = malloc(sizeof(unsigned char)*(text_len));
	
	/* NFA for recognizing %0D%0A - newline
		%	0	D	A	rest
	a	0,a a	a	a	a
	0	-	1	-	-	-
	1	-	-	2	-	-
	2	3	-	-	-	-
	3	-	4	-	-	-
	4	-	-	-	5	-
	5	-	-	-	-	-
	
	*/
	
	for (i=0, j=0; i<text_len; i++){
		
		if (text_data[i] == '+')
			text_data2[j++] = ' ';
		else 
			text_data2[j++] = text_data[i];
		
		if (text_data[i] == '%'){
			states_tmp[0] = 1;
			if (states[2] == 1)
				states_tmp[3] = 1;
		}
		
		if (text_data[i] == '0'){
			if (states[0] == 1)
				states_tmp[1] = 1;
			if (states[3] == 1)
				states_tmp[4] = 1;
		}
		
		if (text_data[i] == 'D'){
			if (states[1] == 1)
				states_tmp[2] = 1;
		}
		
		if (text_data[i] == 'A'){
			if (states[4] == 1)
				states_tmp[5] = 1;
		}
		
		for (k = 0; k < 6 ; k++){
			states[k] = states_tmp[k];
			states_tmp[k] = 0;
		}
		
		if (states[5] == 1){
			j = j-6; 
			text_data2[j++] = '\n';
		}
	}
	text_data2[j] = '\0';
	
	//text_data2 = realloc(text_data2, j);
	return text_data2;
 }
 
typedef struct{
 unsigned char * text;
 unsigned char * regex;
 int show_nfa;
} web_regex;
 
web_regex web(){
    int len=atoi(getenv("CONTENT_LENGTH"));
	int j, size,text_len, regex_len, i;
	int *text_char_nr; //text delimiting characters from form input
	 unsigned char * text_data, * text_data2;
	 unsigned char * regex1, *regex2;
     unsigned char *tmp, *tmp2;
	web_regex output;	
	
	printf("Content-type: text/html\n\n");

	if (len == 0)
		len =1;

    tmp = malloc((len)*sizeof(unsigned char));
    fread(tmp,len,1,stdin); //read from stdin something of i bytes to tmp

	text_char_nr = find_form_text_char_nr(tmp, len);
	
	//get text
	text_len = text_char_nr[1]-text_char_nr[0]-1;
	text_data= malloc(sizeof(unsigned char)*(text_len+1));
	strncpy(text_data,tmp+text_char_nr[0]+1,text_len); //segfault - why?
	text_data[text_len] = '\0';
	
	//format text for new line %0D%0A and spaces +
	text_data2 = format_text(text_data,text_len+1);
	free(text_data);
	
	//get regex	
	regex_len = text_char_nr[3] - text_char_nr[2]-1;	
	regex1 = malloc(sizeof(unsigned char)*(regex_len+1));
	if (regex1 == NULL){
		printf("malloc fail");
		exit(EXIT_FAILURE);
	}
	
	strncpy(regex1,tmp+text_char_nr[2]+1,regex_len);
	regex1[regex_len] = '\0';
	
	//format + to space
	regex2 = format_text(regex1,regex_len+1);
	free(regex1);
	
	output.show_nfa = text_char_nr[4];
	output.text = text_data2;
	output.regex = regex2;
	free(text_char_nr);
	free(tmp);
    return output;
}

int percent_decode( unsigned char* out, const  unsigned char* in) {

	
    static const unsigned char tbl[256] = {
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
         0, 1, 2, 3, 4, 5, 6, 7,  8, 9,-1,-1,-1,-1,-1,-1,
        -1,10,11,12,13,14,15,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,10,11,12,13,14,15,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1
    };
    unsigned char c, v1, v2, *beg=out;
	int len =0;
	
    if(in != NULL) {
        while((c=*in++) != '\0') {
            if(c == '%') {
                if((v1=tbl[( unsigned char)*in++])<0 || (v2=tbl[( unsigned char)*in++])<0) {
                    *beg = '\0';
                    return -1;
                }
                c = (v1<<4)|v2;
            }
            *(out+len) = c;
			++len;
        }
    }
    *(out+len) = '\0';
    return len;
}

unsigned char * check_star(unsigned char * regex){
	//1 byte UTF-8 is in the format of 0xxx xxxx < 128 
	//2 byte UTF-8 is in the format of 191 < 110x xxxx < 224  192 > 10xx xxxx > 127 
	//3 byte UTF-8 is in the format of 223 < 1110 xxxx < 240  192 > 10xx xxxx > 127  192 > 10xx xxxx > 127 
	//4 byte UTF-8 is in the format of 239 < 1111 0xxx < 248  192 > 10xx xxxx > 127  192 > 10xx xxxx > 127  192 > 10xx xxxx > 127
	
	int i, j, count = 0;
	unsigned char * output;
	int len;

	len = strlen(regex);
	
	for (i=0; i<len; i++)
		if (regex[i] == '*' && i > 2 && regex[i-1] != ')' )
			++count;
				
	output = malloc(sizeof(char)*(len+1+count*2));
	
	for (i = 0, j = 0 ; i < len+1; i++){
		
		//find kleene star after a letter that is possibly not in ascii format
		if (regex[i] == '*' && regex[i-1] != ')' ){
			//case ascii, 1 byte, do nothing
			if (regex[i-1] < 128){
				output[j++] = regex[i];
				continue;
			}
			if (regex[i-1] < 192 && regex[i-1] > 127){
				//two bytes utf-8
				if (regex[i-2] > 191 && regex[i-2] <224){
					j -= 2;
					output[j++] = '(';
					output[j++] = regex[i-2];
					output[j++] = regex[i-1];
					output[j++] = ')';
					output[j++] = regex[i];
				}
				//3 bytes
				else if(regex[i-3] > 223 && regex[i-3] < 240){
					j -= 3;
					output[j++] = '(';
					output[j++] = regex[i-3];
					output[j++] = regex[i-2];
					output[j++] = regex[i-1];
					output[j++] = ')';
					output[j++] = regex[i];
					
				}
				//4 bytes
				else if(regex[i-4] > 239 && regex[i-4] < 248){
					j -= 4;
					output[j++] = '(';
					output[j++] = regex[i-4];
					output[j++] = regex[i-3];
					output[j++] = regex[i-2];
					output[j++] = regex[i-1];
					output[j++] = ')';
					output[j++] = regex[i];
					
				}
				//more than 4 bytes or other errors
				else{
					printf("<br>utf-8 mistake on line %d", __LINE__);
					exit(EXIT_FAILURE);	
				}
			}
		}
		else
			output[j++] = regex[i];
	}
	
	return output;

}
//end web part


void show_nfa(int symbols_num, int free_state,int * delete_states, nfa_table_record  * table[][symbols_num+2]){
	int i, j, k;
	
	printf("NFA:<br>");
	
	//print NFA table
	printf("<style> table, td, th {  border: 1px solid #ddd;  text-align: left; } table { border-collapse: collapse;  width: 100%%;} th, td {  padding: 1px;}</style>");
	printf("<table>");
	for (i= 0; i < free_state; i++){
		if (*(delete_states + i) == 0)
			continue;
		if ( i > 0 )
			printf("<td>%d</td>",i);
		else
			printf("<td></td>");
		if (i == 0){
			for (j = 0; j < symbols_num + 2 ; j++){
				if (j < 2)
				printf("<td>%c</td>",table[0][j] -> state_name);
				else
				printf("<td>%d</td>",table[0][j] -> state_name);
			}
			printf("<tr>");
		printf("<td>%d </td>",i);
		}
		for (j = 0; j < symbols_num + 2 ; j++){
			if(table[i][j] -> num_of_transitions > 0){
				printf("<td>" );
				for (k = 0 ; k < table[i][j] -> num_of_transitions; k++){
					printf("%d", *(table[i][j] -> transitions + k) );
					if (k+1 < table[i][j] -> num_of_transitions)
						printf(",");
				}
				printf("</td>" );
			}
			else
				printf("<td>n</td>" );
		}
		printf("</tr><tr>");
	}
	printf("</tr></table>");
}

char * ChangeEscapedChar(char * regex){
	int len, i, j ;
	char * output;
	
	len = strlen(regex);
	output = malloc(sizeof(char)*len);
	
	for (i =0, j = 0; i < len; i++){
		if (regex[i] == '\\'){
			if (i == len -1){
				printf("Wrong use of escape character - \\ at the end");
				exit(EXIT_FAILURE);
			}
			
			++i;
			switch (regex[i]){
				case '+':
					output[j++] = 1;
					break;
					
				case '*':
					output[j++] = 2;
					break;
					
				case '(':
					output[j++] = 3;
					break;
					
				case ')':
					output[j++] = 4;
					break;
				default:
					output[j++] = regex[i];
					break;
			}
						
		}
		else{
			output[j++] = regex[i];
		}	
	}
	
	output[j] = '\0';
	output = realloc(output, j);
	
	return output;	
}

void main(){
	int i, j, k, y, x, argv_char_count = 0, window_size = 100, char_left;
	unsigned char *regex, *str2, *temp ;
	int argv_end = 0, len, symbols_num, symbols_num2, free_state;
	int *delete_states;
	int dfa_states, previos_line;
	int line, char_beg, char_end;
	unsigned char *search_text_input;
	text_output output;
	web_regex web_input;
	
	web_input = web();
		
	temp = web_input.regex;
	
	if (strlen(temp) == 0){
		printf("Enter a regular expression!");
		exit(EXIT_FAILURE);
	}
	
	len = strlen(temp);
	regex = malloc((len+1)*sizeof(unsigned char));
	len = percent_decode(regex,temp);
	
	temp = web_input.text;
	len = strlen(temp);
	search_text_input = malloc((len+1)*sizeof(unsigned char));
	len = percent_decode(search_text_input,temp);
	
	free(web_input.regex);
	free(web_input.text);
	
	temp = regex;
	regex = ChangeEscapedChar(temp);
	free(temp);
	
	if ( check_syntax(regex) == FALSE) return;
	
	temp = regex;
	regex = RemoveExccessBrackets(temp);
	free(temp);
	
	temp = regex;
	regex = check_star(temp);
	free(temp);
	
	symbols_num = count_symbols(regex);
	symbols_num2 = count_symbols2(regex) +1; //+1 is for header alphabet symbols
	
	nfa_table_record * table[symbols_num2][symbols_num+2];

	//printf("REGEX: \"%s\", num of unique unsigned char: %d<br>",regex,symbols_num);
	
	for (i= 0; i < symbols_num2;  i++){
		for (j = 0; j<symbols_num+2; j++){
			table[i][j] = malloc(sizeof(nfa_table_record));
			check_malloc((void*)table[i][j],__LINE__);
			
			table[i][j] -> num_of_transitions = 0;
		}
	}
	
	free_state = reg_to_nfa(symbols_num2, symbols_num, table, regex);	
	delete_states = simplify_e_transitions(symbols_num2, symbols_num, table);
	
	if (web_input.show_nfa == 1){
		show_nfa(symbols_num,free_state,delete_states,table);
	}

	output = search_text(search_text_input, symbols_num,  symbols_num2, table);
	
	//print out search results
	if (output.found == 0)
		printf("No matches found!");
	
	else{
		previos_line = -1;
		printf("Output found: %d<br>",output.found);	
		str2 = malloc(sizeof(unsigned char));
		
		int previous_end = -1, previous_line = -1;
		
		for(i = 0, j = 0; i < output.found; i++){

			line = *(output.line_num + i);
			char_beg = *(output.char_num_begin + i);
			char_end = *(output.char_num_end + i);
			
		//	printf("previous_end %d ,previous_line %d ,line %d ,char_beg %d ,char_end %d <br>",previous_end,previous_line,line,char_beg,char_end);
			
			if (char_end <= previous_end && previous_line >= line)
				continue;
			
			previous_end = char_end;
			previous_line = line;
			
			if (previos_line != line) {
				if (previos_line > -1){	
					print_lines_web(output.line_text[j++], str2, len, previos_line, window_size);
				}
				previos_line = line;
				len = strlen(output.line_text[j]);
				
				str2 = realloc(str2,sizeof(unsigned char)*(len+1));
				check_malloc((void*)str2,__LINE__);
				
				for (k = 0; k < len; k++)
					str2[k] = ' ';
				
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
		print_lines_web(output.line_text[j], str2, len, line, window_size);
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
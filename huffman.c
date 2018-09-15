#include <stdlib.h>
#include <stdio.h>

struct tree_node{
	char c;
	int frequency;
	struct tree_node *left, *right;
};

struct queue_node{
	struct tree_node *node;
	struct queue_node *next;
};

struct binary_table_node{
	char c;
	unsigned char *binary;
	int binary_length;
	int total_bits;
};

int is_encoded(FILE *file);
void encode(FILE *file, char *output_file_name);
void decode(FILE *file, char *output_file_name);
void count_char(struct tree_node ***nodes,int *char_count, char c);
void push(struct queue_node **queue_head, struct tree_node *node);
struct tree_node *pop(struct queue_node **queue_head);
struct tree_node *merge_nodes(struct tree_node *node1, struct tree_node *node2);
struct tree_node *create_tree(FILE *file, int *char_count);
struct binary_table_node *create_binary_table(struct tree_node *head, int char_count);
void get_binary_from_tree(struct binary_table_node **table, int *table_node_count, struct tree_node *tree_node, int depth, unsigned char *binary);
void write_header(unsigned char *file_text,struct tree_node *node, int *current_bit);
void encode_text(FILE *file, unsigned char *file_text, struct binary_table_node *table, int *current_bit);
void read_header(struct tree_node **node, FILE *file, int *current_bit, unsigned char *buffer);
void decode_text(FILE*input_file, FILE *output_file, struct tree_node *head, int bit_count, int *current_bit, unsigned char *buffer);
void delete_tree(struct tree_node **node);
void delete_table(struct binary_table_node **table, int char_count);
void print_table(struct binary_table_node *table, int char_count);
void print_tree(struct tree_node *bst, int indent);


void main(int argc, char **argv){
	//variable declaration
	FILE *file;
	char *output_file_name = NULL;

	//check if a file is provided
	if(argc < 2){
		printf("Please provide a file to encode or decode as a command line argument.\n");
		return;
	}

	if(argc > 2){
		output_file_name = argv[2];
	}else{
		output_file_name = "output_file";
	}

	//open file, check if file exists
	if(!(file = fopen(argv[1],"r"))){
		printf("The specified file, \"%s\", does not exist or could not be opened.\n", argv[1]);
		return;
	}

	//check if file is empty
	if(getc(file)== EOF){
		printf("The specified file, \"%s\", is empty.\n", argv[1]);
		fclose(file);
		return;
	}

	rewind(file);

	if(is_encoded(file)){
		decode(file, output_file_name);
	}else{
		encode(file, output_file_name);
	}

	fclose(file);

	return;
}

int is_encoded(FILE *file){
	if(!file)return 0;
	//magic string to check if the file is already encoded.
	char magic_string[4] = ".8\nz";

	//check if the first 4 characters in the file match the magic string
	for(int i=0; i <4; i++){
		if(magic_string[i] != getc(file)){
			rewind(file);
			//printf("This file has not been previously encoded, it will be encoded.\n");
			return 0; 
		}
	}

	//printf("This file has been previously encoded, it will be decoded.\n");
	return 1;
}

void encode(FILE *file, char *output_file_name){
	if(!file) return;
	//the number of distinct chars in the original file, required for memory allocation 
	unsigned int char_count = 1;
	//length of the encoded file
	unsigned int file_length_bits = 0;
	int file_length_bytes;
	int file_lenght_bytes;
	//string of text of the encrypted file.
	unsigned char *file_text;
	struct tree_node *head = create_tree(file, &char_count);
	//print_tree(head, 0);
	struct binary_table_node *table = create_binary_table(head, char_count);
	//Allocate buffer for encoding
	//The number of nodes in a huffman tree is n+(n-1) (n being the number of end nodes).So, using my approach, n+(n*8)+(n-1)bits are required to 
	//encode the header.
	//The number of bits required for encoding the actual characters can be calculated by adding the total bits for each character calculated earlier.
	//The maximum required size of the buffer (in bytes) is the sum of the total bits for each character plus n+(n*8)+(n-1) divided by 8.
	file_length_bits += char_count+(char_count*8)+(char_count-1);	
	
	for(int i = 0; i < char_count; i++) file_length_bits += (table+i)->total_bits;
	//convert from bits to bytes
	if(!(file_length_bits %8)){
		file_length_bytes = file_length_bits/8;
	}else{
		file_length_bytes = file_length_bits/8+1;
	}
	file_text = (unsigned char *)malloc(sizeof(unsigned char)*file_length_bytes);
	//set all chars in file_text equal to 0 (00000000)
	for(int i = 0; i < file_length_bytes; i++) *(file_text+i) = 0;
	int current_bit = 0;
	write_header(file_text, head, &current_bit);
	encode_text(file, file_text, table, &current_bit);
	//print_table(table,char_count);
	FILE *output_file = fopen(output_file_name, "w+");
	fwrite(".8\nz", sizeof(char), 4, output_file);
	//length in bits of header and encoded text
	fwrite(&file_length_bits, sizeof(int), 1, output_file);
	//number of distinct chars in the file
	fwrite(file_text, sizeof(char), file_length_bytes, output_file);
	fclose(output_file);
	delete_tree(&head);
	delete_table(&table, char_count);
	free(file_text);


	return;
}

void decode(FILE *file, char * output_file_name){
	if(!file) return;
	int bit_count;
	unsigned char buffer;
	char *text;
	struct tree_node *head;
	int current_bit = 0;
	fread(&bit_count, sizeof(int), 1, file);
	//printf("\nThe file to be decoded is %i bits (not including magic string, char count, and bit count). \n", bit_count);
	read_header(&head, file, &current_bit, &buffer);
	FILE *output = fopen(output_file_name, "w+");
	//print_tree(head, 0);
	decode_text(file, output, head, bit_count, &current_bit, &buffer);
	delete_tree(&head);
	fclose(output);
}

void count_char(struct tree_node ***nodes,int *char_count, char c){
	if(!nodes) return;
	//first node
	//printf("\n\nNumber of unique chars so far: %i\n\n", *char_count);
	//printf("\n\nCounting char: %c\n\n", c);
	if(!(*nodes)){
		//allocate memory
		if(!(*nodes = (struct tree_node **)malloc(sizeof(struct tree_node*)*128))) printf("Failed to allocate memory.\n"); // 128 for full ascii set, not all will be used in most cases.
		if(!(**nodes = (struct tree_node *)malloc(sizeof(struct tree_node)))) printf("Failed to allocate memory.\n");

		(**nodes)->c = c;
		(**nodes)->frequency = 1;
		(**nodes)->right = NULL;
		(**nodes)->left = NULL;
		return;
	}
	
	for(int i=0; i < *char_count; i++){
		if((*(*nodes+i))->c == c){
			(*(*nodes+i))->frequency++;
			return;
		}
	}

	//allocate memory
	if(!(*((*nodes)+(*char_count)) = (struct tree_node *)malloc(sizeof(struct tree_node)))) printf("Failed to allocate memory.\n");

	(*((*nodes)+(*char_count)))->c = c;
	(*((*nodes)+(*char_count)))->frequency = 1;
	(*((*nodes)+(*char_count)))->right = NULL;
	(*((*nodes)+(*char_count)))->left = NULL;
	(*char_count)++;
	return;
}


void push(struct queue_node **queue_head, struct tree_node *node){
	if(!queue_head||!node)return;
	struct queue_node *probe;
	struct queue_node *new_node;

	//First node
	if(!(*queue_head)){
		if(!(*queue_head = (struct queue_node *)malloc(sizeof(struct queue_node)))) printf("Failed to allocate memory.\n");
		(*queue_head)->node = node;
		(*queue_head)->next = NULL;
		return;
	}
	

	probe = *queue_head;

	//Find place
	while((probe)->next && (node)->frequency > (probe)->next->node->frequency) (probe) = (probe)->next;

	//Allocate memory
	if(!(new_node = (struct queue_node *)malloc(sizeof(struct queue_node)))) printf("Failed to allocate memory.\n");

	new_node->node = node;
	new_node->next = (probe)->next;
	(probe)->next = new_node;
}

struct tree_node *pop(struct queue_node **queue_head){
	if(!queue_head)return NULL;
	struct tree_node *return_node = (*queue_head)->node;
	struct queue_node *node_to_be_freed = *queue_head;
	*queue_head = (*queue_head)->next;
	free(node_to_be_freed);
	return return_node;
}

struct tree_node *merge_nodes(struct tree_node *node1, struct tree_node *node2){
	if(!node1||!node2)return NULL;
	struct tree_node *new_node;

	//Allocate memory
	if(!(new_node = (struct tree_node *)malloc(sizeof(struct tree_node)))) printf("Failed to allocate memory.\n");

	new_node->c = 0;
	new_node->left = node1;
	new_node->right = node2;
	new_node->frequency = (node1)->frequency + (node2)->frequency;
	return new_node;
}

struct tree_node *create_tree(FILE *file, int *char_count){
	if(!file)return NULL;
	struct tree_node **nodes = NULL;
	struct tree_node *bst_head;
	struct queue_node *queue_head = NULL;
	char c;

	//create an array of node pointers from the chars in the file.
	while((c = getc(file)) != EOF){
		count_char(&nodes, char_count, c);
	}

	//for edge case where file contains only \n's
	if(*char_count == 1){ 
		count_char(&nodes, char_count, 'a');
		(*(nodes+1))->frequency = 0;
	}

	//push the node pointers onto a priority queue.
	for(int i = 0; i < *char_count; i++){
		push(&queue_head, *(nodes+i));
	}

	//merge nodes until one remains using the priority queue.
	while(queue_head->next) push(&queue_head,(merge_nodes((pop(&queue_head)),(pop(&queue_head)))));
	bst_head = queue_head->node;

	//free allocated memory
	pop(&queue_head);
	free(nodes);
	return bst_head;
}

struct binary_table_node *create_binary_table(struct tree_node *head, int char_count){
	if(!head)return NULL;
	struct binary_table_node *table = (struct binary_table_node *)malloc(sizeof(struct binary_table_node)*char_count);
	int table_node_count = 0;
	//required for get_binary_from_tree
	unsigned char *binary = (unsigned char *)malloc(sizeof(unsigned char));
	*binary = 0;
	//create a table of binary codes using a recursive function
	get_binary_from_tree(&table,&table_node_count, head, 0, binary);
	return table;
}

void get_binary_from_tree(struct binary_table_node **table, int *table_node_count, struct tree_node *tree_node, int depth, unsigned char *binary){
	if(!*table||!tree_node||!binary)return;
	//if this is a char-containing leaf-node, then add it's data to the table	
	if(!(tree_node->left)){
		((*table)+*table_node_count)->c = tree_node->c;
		((*table)+*table_node_count)->binary = binary;
		((*table)+*table_node_count)->binary_length = depth;
		((*table)+*table_node_count)->total_bits = (depth)*tree_node->frequency;
		(*table_node_count)++;
		return;
	}
	//seperate binary code for divergent path.
	unsigned char * right_binary = (unsigned char *)malloc(sizeof(unsigned char)*((int)(depth/8)+1));
	//if the binary code is too long to be stored in the previously allocated  char array, then add another char.
	if(depth && !(depth % 8)){
		if(!(binary = realloc(binary, sizeof(unsigned char)*((depth/8)+1)))) printf("Failed to reallocate memory.");
		*(binary+depth/8) = 0;
	}
	//shift the bits of the current char left 1 to allow for another bit to be coded.
	(*(binary+(depth/8))) = (*(binary+(depth/8)))<<1;
	for(int i = 0; i < ((depth/8)+1); i++){
		*(right_binary+i) = *(binary+i);
	}
	//add a '1' to right_binary
	(*(right_binary+(depth/8))) = (*(right_binary+(depth/8)))|1;

	//recurse down the tree
	get_binary_from_tree(table, table_node_count, tree_node->right, depth+1, right_binary);
	get_binary_from_tree(table, table_node_count, tree_node->left, depth+1, binary);
}

void write_header(unsigned char *file_text,struct tree_node *node, int *current_bit){
	if(!file_text||!node) return;
	
	unsigned char bit = 1<<(7-(*current_bit%8));
	char c;
	char c_2;

	if(node->c){
		//add 1 to signify node with char
		*(file_text+(*current_bit/8)) |= bit;
		(*current_bit)++;
		c = node->c;

		//if the current byte has 8 bits remaining
		if(!(*current_bit%8)){
			*(file_text+(*current_bit/8)) = c;
		}else{
			c_2 = c<<(8-(*current_bit%8));
			c = c>>(*current_bit%8);
			*(file_text+(*current_bit/8)) |= c;
			*(file_text+(*current_bit/8+1)) |=c_2;
		}

		(*current_bit) += 8;
		return;
	}

	(*current_bit)++;

	//recurse down tree
	write_header(file_text, node->left, current_bit);
	write_header(file_text, node->right, current_bit);
	return;
}

void encode_text(FILE *file, unsigned char *file_text, struct binary_table_node *table, int *current_bit){
	if(!file||!file_text||!table) return;
		
	char c;
	unsigned char binary;
	unsigned char binary_2;
	int counter;

	rewind(file);

	while((c = getc(file)) != EOF){
		//printf("encoding letter: %c\n", c);
		counter = 0;

		//find char in table
		while((table+counter)->c != c) counter++;

		//add binary code to file_text
		for(int i = 0; i <= ((table+counter)->binary_length-1)/8; i++){
			binary = *((table+counter)->binary+i);
			if(i<(table+counter)->binary_length/8 || !((table+counter)->binary_length % 8)){
				if(!(*current_bit % 8)){
					*(file_text+*current_bit/8) = binary;
				}else{
					binary_2 = binary<<(8-(*current_bit%8));
					binary = binary>>(*current_bit%8);
					*(file_text+(*current_bit/8)) |= binary;
					if((table+counter)->binary_length > 8-(*current_bit%8))*(file_text+(*current_bit/8+1)) |= binary_2;	
				}
				*current_bit += 8;
			}else{
				binary = binary<<8-((table+counter)->binary_length % 8);
				if(!(*current_bit % 8)){
					*(file_text+*current_bit/8) = binary;
				}else{
					binary_2 = binary<<(8-(*current_bit%8));
					binary = binary>>(*current_bit%8);
					*(file_text+(*current_bit/8)) |= binary;
					if((table+counter)->binary_length > 8-(*current_bit%8))*(file_text+(*current_bit/8+1)) |= binary_2;	
				}
				*current_bit +=  ((table+counter)->binary_length % 8);
			}
		}
	}
	return;
}

void read_header(struct tree_node ** node, FILE *file, int *current_bit, unsigned char *buffer){
	if(!file||!buffer)return;
	int end_node_count;
	unsigned char bit;

	bit = 1;
	// '1' at the current bit location in byte
	bit = bit<<(7-(*current_bit%8));

	//read a new byte
	if(!(*current_bit%8))fread(buffer, sizeof(char), 1, file);

	(*current_bit)++;

	//allocate memory
	*node = (struct tree_node *)malloc(sizeof(struct tree_node));

	//if the current bit is a '0', then add an empty node.
	if(*buffer != (bit|=*buffer)){
		(*node)->c = 0;
		(*node)->left = NULL;
		(*node)->right = NULL;
		read_header(&((*node)->left), file, current_bit, buffer);
		read_header(&((*node)->right), file, current_bit, buffer);
	}else{ // if the current bit is a '1', then add read the next 8 bits as a char for a char node
		if(!(*current_bit%8)){
			fread(&((*node)->c), sizeof(char), 1, file);
		}else{
			(*node)->c = 0;
			(*node)->c = *buffer<<(*current_bit%8);
			fread(buffer, sizeof(char), 1, file);
			(*node)->c |= *buffer>>(8-(*current_bit%8));
		}

		(*node)->left = NULL;
		(*node)->right = NULL;
		*current_bit +=8;
		return;
	}
}

void decode_text(FILE* input_file, FILE* output_file, struct tree_node *head, int bit_count, int *current_bit, unsigned char * buffer){
	if(!input_file||!output_file||!head||!buffer) return;
	unsigned char bit;
	struct tree_node *probe = head;

	//read bits until the last encoded bit.
	while(*current_bit < bit_count){
		bit = 1;
		bit = bit<<(7-(*current_bit%8));
		if(!(*current_bit%8))fread(buffer, sizeof(char), 1, input_file);
		(*current_bit)++;

		if(*buffer != (bit|=*buffer)){
			probe = probe->left;
		}else{
			probe = probe->right;
		}

		if(probe->c){
			fwrite(&(probe->c), sizeof(char), 1, output_file);
			probe = head;
		}
	}
}

void delete_tree(struct tree_node ** node){
	if(!*node) return;

	//recurse down tree
	if((*node)->right) delete_tree(&((*node)->right));
	if((*node)->left) delete_tree(&((*node)->left));

	//free node
	free(*node);
}

void delete_table(struct binary_table_node **table, int char_count){
	if(!*table)return;
	for(int i = 0; i < char_count; i++) free(((*table)+i)->binary);
	free(*table);
}


//FOR TESTING PURPOSES
void print_table(struct binary_table_node *table, int char_count){
	if(!table)return;
	printf("\nCHAR\t\tINT\t\tBINARY LENGTH\n");
	for(int i = 0; i < char_count; i++) printf("%c\t\t%i\t\t%i\n", (int)(table+i)->c,*((table+i)->binary),(table+i)->binary_length);
	return;
}

//FOR TESTING PURPOSES
void print_tree(struct tree_node *bst, int indent){
    if(!bst) return;
    for(int i = 0; i < indent; i++)
    {
                        printf( "\t");
    }
    fprintf(stdout,"%c\n", bst->c);
    if(bst->left)
    {
                print_tree(bst->left, indent + 1);
    }
    if(bst->right)
    {
            print_tree(bst->right, indent + 1);
    }
}



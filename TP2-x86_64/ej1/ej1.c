#include "ej1.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>


string_proc_list* string_proc_list_create(void) {
    string_proc_list* list = malloc(sizeof(string_proc_list));
    if (list) {
        list->first = NULL;
        list->last = NULL;
    }
    return list;
}

string_proc_node* string_proc_node_create(uint8_t type, char* hash) {
    string_proc_node* node = malloc(sizeof(string_proc_node));
    if (node) {
        node->type = type;
        node->hash = hash;      // No copiamos el hash, simplemente apuntamos.
        node->next = NULL;
        node->previous = NULL;
    }
    return node;
}

void string_proc_list_add_node(string_proc_list* list, uint8_t type, char* hash) {
    if (!list) return;

    string_proc_node* new_node = string_proc_node_create(type, hash);
    if (!new_node) return;

    if (list->last == NULL) {
        // Lista vacía
        list->first = new_node;
        list->last = new_node;
    } else {
        // Lista con elementos
        new_node->previous = list->last;
        list->last->next = new_node;
        list->last = new_node;
    }
}

char* string_proc_list_concat(string_proc_list* list, uint8_t type, char* hash) {
    if (!list || !hash) return NULL;

    // Primero calculamos el tamaño total necesario
    size_t total_length = strlen(hash);
    string_proc_node* current = list->first;

    while (current) {
        if (current->type == type && current->hash) {
            total_length += strlen(current->hash);
        }
        current = current->next;
    }

    // Reservamos memoria para el string resultante (+1 para el null terminator)
    char* result = malloc(total_length + 1);
    if (!result) return NULL;

    // Comenzamos copiando el hash recibido
    strcpy(result, hash);

    // Concatenamos los hashes de los nodos del mismo tipo
    current = list->first;
    while (current) {
        if (current->type == type && current->hash) {
            strcat(result, current->hash);
        }
        current = current->next;
    }

    return result;
}



/** AUX FUNCTIONS **/

void string_proc_list_destroy(string_proc_list* list){

	/* borro los nodos: */
	string_proc_node* current_node	= list->first;
	string_proc_node* next_node		= NULL;
	while(current_node != NULL){
		next_node = current_node->next;
		string_proc_node_destroy(current_node);
		current_node	= next_node;
	}
	/*borro la lista:*/
	list->first = NULL;
	list->last  = NULL;
	free(list);
}
void string_proc_node_destroy(string_proc_node* node){
	node->next      = NULL;
	node->previous	= NULL;
	node->hash		= NULL;
	node->type      = 0;			
	free(node);
}


char* str_concat(char* a, char* b) {
	int len1 = strlen(a);
    int len2 = strlen(b);
	int totalLength = len1 + len2;
    char *result = (char *)malloc(totalLength + 1); 
    strcpy(result, a);
    strcat(result, b);
    return result;  
}

void string_proc_list_print(string_proc_list* list, FILE* file){
        uint32_t length = 0;
        string_proc_node* current_node  = list->first;
        while(current_node != NULL){
                length++;
                current_node = current_node->next;
        }
        fprintf( file, "List length: %d\n", length );
		current_node    = list->first;
        while(current_node != NULL){
                fprintf(file, "\tnode hash: %s | type: %d\n", current_node->hash, current_node->type);
                current_node = current_node->next;
        }
}


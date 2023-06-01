// Rafael Augusto Boschi de Campos GRR20182984
// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.4 -- Janeiro de 2022
// Definição e operações em uma fila genérica.

// Importações
#include "queue.h"
#include <stdio.h>
#include <stdlib.h>

// Funções

int queue_size (queue_t *queue) {
    queue_t *andador = queue;
    if(!andador){
        return 0; //Fila vazia.
    }
    int cont = 1;
    queue_t *ini = queue;
    while(andador->next != ini){ //Faz a iteração até achar o ultimo elemento da fila.
        cont += 1;
        andador = andador->next;
    }
    return cont;
}

void queue_print (char *name, queue_t *queue, void print_elem (void*) ) {
    queue_t *andador = queue; 
    if(!andador){
        printf("%s: []\n", name); //Se a fila for vazia então o print será padrão.
    }
    else{
        printf("%s: [", name); //Inicia o print com a string passada de acordo com os testes.
        queue_t *ini = queue;
        while(andador->next != ini){ //Faz a iteração até achar o ultimo elemento da fila.
            print_elem(andador);
            andador = andador->next;
            printf(" "); //Espaço entre os valores da fila.
        }
        print_elem(andador);
        printf("]\n"); //Fecha o print da fila.
    }
}

int queue_append (queue_t **queue, queue_t *elem) {
    queue_t *novo = elem;
    if(!novo){
        //perror("Erro de elemento não existe"); //fica a escolha do usuario.
        fprintf(stderr, "##Erro de elemento não existe\n");
        return -2;
        //saida de erro para caso elemento nao exista.
    }
    if(novo->next || novo->prev){
        //perror("Erro de elemento existe e não está isolado"); //fica a escolha do usuario.
        fprintf(stderr, "##Erro de elemento existe e não está isolado\n");
        return -4;
        //saida de erro para caso o elemento não seja isolado.
    }
    if(!*queue){ //Caso a fila esteja vazia insere o primeiro elemento.
        novo->next = novo;
        novo->prev = novo;
        *queue = novo;
        return 0;
    }
    //Caso a fila não seja vazia insere no final da fila.
    queue_t *ini = *queue;
    queue_t *andar = *queue;
    while(andar->next != ini){ //Faz a iteração até achar o ultimo elemento.
        andar = andar->next;
    } 
    andar->next = novo;
    novo->prev = andar;
    novo->next = ini;
    ini->prev = novo;
    return 0;
}

int queue_remove (queue_t **queue, queue_t *elem) {
    queue_t *exc = elem;
    if(!exc){
        //perror("Erro de elemento não existe"); //fica a escolha do usuario.
        fprintf(stderr, "##Erro de elemento não existe\n");
        return -2;
        //Saida de erro para caso elemento nao exista.
    }
    if(!(exc->next && exc->prev)){
        //perror("Erro de elemento existe e está isolado"); //fica a escolha do usuario.
        fprintf(stderr, "##Erro de elemento existe e está isolado\n");
        return -3;
        //Saida de erro para caso o elemento nao aponte para nada.
    }
    if(!*queue){
        //perror("Erro de fila vazia"); //fica a escolha do usuario.
        fprintf(stderr, "##Erro de fila vazia\n");
        return -1;
        //Erro fila vazia.
    }
    //Caso a fila não seja vazia começam os testes.
    queue_t *ini = *queue;
    queue_t *andar = *queue;
    if((andar == exc && andar->next == andar)){ //Descobre se o elemento a ser excluído é o primeiro e único elemento da fila.
        exc->prev = NULL;
        exc->next = NULL;
        *queue = NULL;
        return 0;
    }
    if(andar == exc){ //Descobre se o elemento a ser excluído é o primeiro elemento da fila porém não é o único.
        exc->next->prev = exc->prev;
        exc->prev->next = exc->next;
        *queue = exc->next;
        exc->prev = NULL;
        exc->next = NULL;
        return 0;   
    }
    //Último caso é para qualquer outro elemento da fila.
    andar = andar->next; //A fila precisa começar do segundo elemento para que os testes funcionem (afinal o primeiro elemento ja foi testado).
    while(andar != ini){ //Faz a iteração até achar o primeiro elemento da fila.
        if(andar == exc){
            andar = andar->prev;
            andar->next = exc->next;
            exc->next->prev = andar;
            exc->prev = NULL;
            exc->next = NULL;
            return 0;
        }
        andar = andar->next;
    }

    //Não achou o elemento na fila.
    //perror("Erro de elemento não está na fila"); //fica a escolha do usuario.
    fprintf(stderr, "Erro de elemento não está na fila\n");
    return -5;
}

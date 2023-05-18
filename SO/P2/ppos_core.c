// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.4 -- Janeiro de 2022

// Demonstração das funções POSIX de troca de contexto (ucontext.h).


#include "ppos.h"
#include "queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>

#define STACKSIZE 64*1024

int tasks_ids = 0;
task_t t_main, *task_anterior, t_dispatcher, *fila_tasks ;

void ppos_init(){

    setvbuf (stdout, 0, _IONBF, 0) ;

    t_main.id = 0;
    t_main.status = 1;

    getcontext(&(t_main.context));

    task_anterior = &t_main;

    queue_append((queue_t **) &fila_tasks, (queue_t*) &t_main);

    task_init(&t_dispatcher, dispatcher, "TASK YIELD");

    task_yield();
}

int task_init (task_t *task, void  (*start_func)(void *), void   *arg){

    char *stack ;

    getcontext (&(task->context)) ;

    stack = malloc (STACKSIZE) ;
    if (stack)
    {
       task->context.uc_stack.ss_sp = stack ;
       task->context.uc_stack.ss_size = STACKSIZE ;
       task->context.uc_stack.ss_flags = 0 ;
       task->context.uc_link = 0 ;
    }
    else
    {
       perror ("Erro na criação da pilha: ") ;
       exit (1) ;
    }

    makecontext (&(task->context), (void*)(*start_func), 1, arg) ;

    tasks_ids+=1;
    task->id = tasks_ids;
    task->status = 1;

    #ifdef DEBUG
    printf ("task_init: %d\n", task->id) ;
    #endif

    if(&t_dispatcher != task){
        if(!(queue_append((queue_t **) &fila_tasks, (queue_t*) task))){
            #ifdef DEBUG
                printf ("Task %d adicionada a fila\n", task->id);
            #endif
        }
    }

    return task->id;
}

int task_switch (task_t *task){

    if(!task)
        exit(-1);

    task_t *aux = task_anterior;

    task_anterior = task;

    #ifdef DEBUG
        printf("saindo da tarefa %d e indo para tarefa %d \n", aux->id, task->id);
    #endif

    swapcontext(&(aux->context), &(task->context));

    return (0);
}

void task_exit (int exit_code){

    task_anterior->status = 3;

    if(task_anterior->id == 1){
        exit(0);
    }
    else{
        task_yield();
    }
}

void task_yield (){
    task_switch(&t_dispatcher);
}

task_t *scheduler(){

    task_t *aux = fila_tasks;

    fila_tasks = fila_tasks->next;
    
    return aux;
}

void dispatcher(){
   // enquanto houverem tarefas de usuário

    task_t *proxima_tarefa = NULL;

    while(queue_size((queue_t*) fila_tasks) > 0){

      // escolhe a próxima tarefa a executar
        proxima_tarefa = scheduler();

      // escalonador escolheu uma tarefa?      
        if(proxima_tarefa != NULL){

         // transfere controle para a próxima tarefa
            task_switch (proxima_tarefa);
         
         // voltando ao dispatcher, trata a tarefa de acordo com seu estado
            switch (proxima_tarefa->status)
            {
                case 1:
                    break;
                case 3:
                    queue_remove ((queue_t **)&fila_tasks, (queue_t *) proxima_tarefa);
                    break;
                default:
            }        
        }
        #ifdef DEBUG
            printf ("%d elementos na fila \n", queue_size((queue_t*) fila_tasks)) ;
        #endif
    }

   task_exit(0); // encerra a tarefa dispatcher

}
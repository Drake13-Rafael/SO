// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.4 -- Janeiro de 2022

// Demonstração das funções POSIX de troca de contexto (ucontext.h).

// operating system check
#if defined(_WIN32) || (!defined(__unix__) && !defined(__unix) && (!defined(__APPLE__) || !defined(__MACH__)))
#warning Este codigo foi planejado para ambientes UNIX (LInux, *BSD, MacOS). A compilacao e execucao em outros ambientes e responsabilidade do usuario.
#endif

#define _XOPEN_SOURCE 600	/* para compilar no MacOS */

//#ifdef DEBUG
//printf ("task_init: iniciada tarefa %d\n", task->id) ;
//#endif

#include "ppos_data.h"
#include "ppos.h"
#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>

#define STACKSIZE 64*1024

int tasks_ids = 0;
task_t t_main, *task_antiga, t_dispatcher, *proxima_tarefa, *fila_tasks ;

void ppos_init(){

    getcontext(&t_main.context);

    t_main.id = 0;

    task_antiga = &t_main;

    queue_append((queue_t **) &fila_tasks, (queue_t*) &t_main);

    task_init(&t_dispatcher, dispatcher, "TASK YIELD");

    setvbuf (stdout, 0, _IONBF, 0) ;
}

int task_init (task_t *task, void  (*start_func)(void *), void   *arg){

    char *stack ;

    getcontext (&task->context) ;

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

    makecontext (&task->context, (void*)(*start_func), 1, arg) ;

    tasks_ids+=1;
    task->id = tasks_ids;
    task->status = 1;

    #ifdef DEBUG
    printf ("task_init: %d\n", task->id) ;
    #endif

    queue_append((queue_t **) &fila_tasks, (queue_t*) task);

    return task->id;
}

int task_switch (task_t *task){

    if(!task)
        exit(-1);

    #ifdef DEBUG
    printf ("task_switch: %d para %d\n", task_antiga->id, task->id) ;
    #endif

   task_antiga = task;

    swapcontext(&task_antiga->context, &task->context);
    return (0);
}

void task_exit (int exit_code){

    task_antiga->status = 3;

    if(task_antiga->id == 0)
        task_yield();
    else if(task_antiga->id == 1)
        exit(0);

}

void task_yield (){
    task_switch(&t_dispatcher);
}

task_t *scheduler(){
    task_t *andador = fila_tasks;
    if(!andador){
        return NULL; //Fila vazia.
    }
    task_t *ini = fila_tasks;
    while(andador->next != ini){ //Faz a iteração até achar o ultimo elemento da fila.
        if((andador->status == 1) && ((andador->id != 1)&&(andador->id != 0)))
            return andador;
        andador = andador->next;
    }
    return ini;
}

void dispatcher(){
   // enquanto houverem tarefas de usuário
    #ifdef DEBUG
        printf ("dispatcher to aqui com %d elementos na fila \n", queue_size((queue_t*) &fila_tasks)) ;
    #endif
    while(queue_size((queue_t*) &fila_tasks) > 1){

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
                case 2:
                    proxima_tarefa->status = 1;
                    break;
            }        
        }
    }

   // encerra a tarefa dispatcher
   //task_exit(0);

}

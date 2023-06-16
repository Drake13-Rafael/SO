// Rafael Augusto Boschi de Campos GRR20182984
// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.4 -- Janeiro de 2022

// Demonstração das funções POSIX de troca de contexto (ucontext.h).

// cc -Wall -o teste -DDEBUG ppos_core.c teste.c queue.c -lm

#include "ppos.h"
#include "queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <signal.h>
#include <sys/time.h>
#include <string.h>

#define STACKSIZE 64*1024
#define MAXQUANTUM 20
#define MAIORPRIO -20
#define MENORPRIO 20

// Cabeçalhos de funções internas
void dispatcher();
void tratador(int);
task_t *scheduler();
void task_suspend(task_t **queue);
void task_resume(task_t *task, task_t **queue);
void testa_suspensas();
void testa_adormecidas();
void acordar_tarefas_semaforo(task_t *fila);


//quantum é variavel limite de tempo para todas as tarefas de usuario
// a flag é para saber se quem está executando é o usuário ou o sistema
int tasks_ids = 0, flag_sistema_usuario = 0, quantum = MAXQUANTUM, saida, semaforos_existem; 
// Relogio global do sistema
unsigned int relogio = 0;

// task_atual é a tarefa que está em execução no momento, t_dispatcher é a tarefa do dispatcher, fila_tasks é a fila de tarefas
// t_main é a tarefa da main
task_t t_main, *task_atual, t_dispatcher, *fila_tasks, *fila_suspensas, *fila_adormecidas;

// Semaforos
semaphore_t s_buffer, s_receiver, s_send;

// estrutura que define um tratador de sinal (deve ser global ou static)
struct sigaction action ;
// estrutura de inicialização do timer
struct itimerval timer;

// tratador do sinal
void tratador (int signum)
{
    relogio += 1;
    task_atual->tempo_processamento += 1; // Aumenta o tempo de processamento das tarefas
    if(flag_sistema_usuario){ //Testa se a interrupção está numa tarefa de sistema(0) ou de usuário(1)
        if(quantum > 1){    // Drecementa o quantum caso o quantum não tenha chego em zero
            quantum -= 1; 
        }
        else{   // Quantum em zero sai da tarefa atual setando a prioridade dela pra estatica e resetando os parametros globais 
            quantum = MAXQUANTUM; // reseta o quantum
            flag_sistema_usuario = 0; // seta a flag para que a partir daqui sejam só tarefas de sistema
            task_yield();
        }
    }
}

// Retorna o tempo do sistema
unsigned int systime (){
    return relogio; 
}

// Inicializa o sistema e as variaveis globais com seus respectivos valores setados
void ppos_init(){

    setvbuf (stdout, 0, _IONBF, 0) ;

// Set da tarefa main
    t_main.id = 0;
    t_main.status = 1;
    t_main.prioridade_estatica = 0;
    t_main.prioridade_dinamica = t_main.prioridade_estatica;
    t_main.ativacoes = 0;
    t_main.tempo_execucao = 0;
    t_main.tempo_processamento = 0;

    getcontext(&(t_main.context));
    
    task_atual = &t_main;

    sem_init(&s_buffer, 1);
    sem_init(&s_receiver, 0);
    sem_init(&s_send, 1);

    // Adiciona a main como tarefa na fila de tarefas
    queue_append((queue_t **) &fila_tasks, (queue_t*) &t_main);

    // Inicia a tarefa dispatcher
    task_init(&t_dispatcher, dispatcher, "TASK YIELD");

    // funções do interruptor
    // registra a ação para o sinal de timer SIGALRM (sinal do timer)
    action.sa_handler = tratador ;
    sigemptyset (&action.sa_mask) ;
    action.sa_flags = 0 ;
    if (sigaction (SIGALRM, &action, 0) < 0)
    {
        perror ("Erro em sigaction: ") ;
        exit (1) ;
    }

    // ajusta valores do temporizador
    timer.it_value.tv_usec = 1000 ;      // primeiro disparo, em micro-segundos
    timer.it_interval.tv_usec = 1000 ;   // disparos subsequentes, em micro-segundos

    // arma o temporizador ITIMER_REAL
    if (setitimer (ITIMER_REAL, &timer, 0) < 0)
    {
        perror ("Erro em setitimer: ") ;
        exit (1) ;
    }

    // Chama o dispatcher para direcionar a próxima tarefa (só existe a main aqui)
    task_yield();
}

// Inicializa a tarefa passada com sua determinada função
int task_init (task_t *task, void  (*start_func)(void *), void   *arg){

    char *stack ;

    // Pega o contexto da tarefa
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

    // Cria o contexto
    makecontext (&(task->context), (void*)(*start_func), 1, arg) ;

    // Seta todos os valores das tarefas
    tasks_ids+=1; // Variavel global para saber o id da tarefa (main é 0 e dispatcher 1)
    task->id = tasks_ids;
    task->status = 1; // Status 1 é PRONTA
    task->prioridade_estatica = 0; // Prioridade default(0)
    task->prioridade_dinamica = task->prioridade_estatica;
    task->ativacoes = 0; // Seta as ativações da tarefa para nenhuma
    task->tempo_execucao = systime(); // Começo de cada tarefa no momento do relogio
    task->tempo_processamento = 0; // Processamento de cada tarefa começa em zero

    #ifdef DEBUG
    printf ("task_init: %d\n", task->id) ;
    #endif

    if(&t_dispatcher != task){
        if((queue_append((queue_t **) &fila_tasks, (queue_t*) task)) == 0){ // Adiciona a tarefa passada a fila e testa se tudo certo
            #ifdef DEBUG
                printf ("Task %d adicionada a fila\n", task->id); 
            #endif
        }
    }

    return task->id;
}

// Troca a tarefa atual pela tarefa passada
int task_switch (task_t *task){

    if(!task) // Se a tarefa passada nao existir da erro
        exit(-1);

    task->ativacoes += 1; // Aumenta a ativação de cada tarefa cada vez que ela é chamada(troca o contexto para ela)
    task_t *aux = task_atual; // Seta um auxiliar para salvar qual é a tarefa antiga

    task_atual = task;

    /*#ifdef DEBUG
        printf("saindo da tarefa %d e indo para tarefa %d \n", aux->id, task->id);
    #endif*/
    
    if(task->id != 1){ // Se a tarefa passada for de usuário (tarefas de sistema é 1 que é main e dispatcher)
        flag_sistema_usuario = 1; // Seta a flag para tarefas de usuario
    }
    swapcontext(&(aux->context), &(task->context)); // Troca o contexto

    return (0);
}

// Ao terminar de executar a tarefa chama o exit que trata de acordo com o tipo de tarefa
void task_exit (int exit_code){

    task_atual->tempo_execucao = systime() - task_atual->tempo_execucao;

    // Mostra o tempo de execução, processamento e as ativações de cada tarefa
    printf("task %d exit: execution time %ld ms, processor time %ld ms, %ld activations\n", task_atual->id, task_atual->tempo_execucao, task_atual->tempo_processamento, task_atual->ativacoes);

    flag_sistema_usuario = 0; // Volta a flag para tarefa de sistema
    quantum = MAXQUANTUM; // Reseta o quantum para a proxima tarefa

    task_atual->status = 3; // Seta o status da tarefa que foi encerrada como TERMINADA(3) 

    if(task_atual->id == 1){ // Caso a tarefa seja o dispatcher ele sai do programa sem erros
        exit(0);
    }
    else{
        testa_suspensas();
        saida = exit_code;
        task_yield(); // Caso a tarefa não seja o dispatcher o contexto será redirecionado pra ele
    }
}

// Chama o dispatcher
void task_yield (){
    task_switch(&t_dispatcher);
}

// Seta a prioridade estática de uma tarefa
void task_setprio (task_t *task, int prio){

    if (!task) // Se não tiver passado nenhuma tarefa então seta a prioridade da tarefa atual
    {
        if(prio > MENORPRIO){ // Testes para saber se a prioridade foi passada corretamente
            fprintf(stderr, "##Erro de tamanho da prioridade, será setada para 20\n");
            task_atual->prioridade_estatica = MENORPRIO;
        }else if(prio < MAIORPRIO){
            fprintf(stderr, "##Erro de tamanho da prioridade, será setada para -20\n");
            task_atual->prioridade_estatica = MAIORPRIO;
        }else
            task_atual->prioridade_estatica = prio;
    }
    else{
        if(prio > MENORPRIO){ // Testes para saber se a prioridade foi passada corretamente
            fprintf(stderr, "##Erro de tamanho da prioridade, será setada para 20\n");
            task->prioridade_estatica = MENORPRIO;
        }else if(prio < MAIORPRIO){
            fprintf(stderr, "##Erro de tamanho da prioridade, será setada para -20\n");
            task->prioridade_estatica = MAIORPRIO;
        }else
            task->prioridade_estatica = prio;
    }
    

}

// Retorna a prioridade estática de uma tarefa 
int task_getprio (task_t *task){
    
    if (!task) // Se a tarefa passada for nula (não existe tarefa) então retorna a prioridade estática da tarefa atual  
        return task_atual->prioridade_estatica;
    else
        return task->prioridade_estatica;

}

// Define qual a tarefa a ser executada em seguida
task_t *scheduler(){
    if (fila_tasks == NULL)
    {
        return NULL;
    }

    // Aux é a variavel que retornará a proxima tarefa pra execução
    task_t *aux = fila_tasks;
    // Ini é a variavel que aponta para o começo da fila
    task_t *ini = fila_tasks;
    // começa as verificações com o item 2 da fila e não o item 1
    fila_tasks = fila_tasks->next;

    while(fila_tasks != ini){ //Faz a iteração até achar o ultimo elemento da fila.
        if(fila_tasks->prioridade_dinamica < aux->prioridade_dinamica){ // Caso a prioridade do item atual seja menor que o menor
            if(aux->prioridade_dinamica > MAIORPRIO) // Teste para saber o limite de prioridade
                aux->prioridade_dinamica -= 1; // Diminui a prioridade do ex-menor
            aux = fila_tasks; // Troca o menor para atual
        }
        else{ 
            if(fila_tasks->prioridade_dinamica > MAIORPRIO) // Teste para saber o limite de prioridade
                fila_tasks->prioridade_dinamica -= 1; // Caso não seja menor apenas envelhece sua prioridade
        }
        fila_tasks = fila_tasks->next; // Faz a próxima iteração
    }

    aux->prioridade_dinamica = aux->prioridade_estatica; // Seta a prioridade da tarefa a ser executada para sua prioridade estática
    fila_tasks = aux; // A fila pega sua nova cabeça

    return aux; 
}

void dispatcher(){

    task_t *proxima_tarefa = NULL; // Prox tarefa a ser executada

    while((queue_size((queue_t*) fila_tasks) > 0) || semaforos_existem || (queue_size((queue_t*) fila_adormecidas) > 0) || (queue_size((queue_t*) fila_suspensas) > 0)){ // Enquanto houver tarefas de usuário

      // escolhe a próxima tarefa a executar
        proxima_tarefa = scheduler();

      // escalonador escolheu uma tarefa?      
        if(proxima_tarefa != NULL){

         // transfere controle para a próxima tarefa
            task_switch (proxima_tarefa);
         
         // voltando ao dispatcher, trata a tarefa de acordo com seu estado
            switch (proxima_tarefa->status)
            {
                case 1: // Tarefa PRONTA
                    break;
                case 3: // Tarefa TERMINADA
                    queue_remove ((queue_t **)&fila_tasks, (queue_t *) proxima_tarefa); // Remove da fila de PRONTAS
                    break;
            }        
        }

        testa_adormecidas();
        
    }

   task_exit(0); // encerra a tarefa dispatcher

}

// Retorna o ID da tarefa atual
int task_id() {
    return task_atual->id; 
}

int task_wait(task_t *task){

    if (!task)
    {
        fprintf(stderr, "##Erro de tarefa não existe\n");
        return -2;
    }

    if(task->status != 3){
        flag_sistema_usuario = 0; // Volta a flag para tarefa de sistema
        quantum = MAXQUANTUM; // Reseta o quantum para a proxima tarefa

        task_atual->tarefa_suspendeu = task->id;
        task_suspend(&fila_suspensas);

        return saida;
    }
    return -1;
}

void task_suspend(task_t **queue){

    if(!queue)
        exit(-1);
    
    if(fila_tasks == task_atual){
        task_atual->status = 2;
        queue_remove((queue_t **) &fila_tasks, (queue_t *) task_atual);
        queue_append((queue_t **) queue, (queue_t *) task_atual);
    }
    task_yield();
}

void task_resume(task_t *task, task_t **queue){

    if(!queue)
        exit(-1);

    task->status = 1;
    queue_remove ((queue_t **) queue, (queue_t *) task);
    queue_append((queue_t **) &fila_tasks, (queue_t *) task);
}

void testa_suspensas(){

    task_t *aux = fila_suspensas;
    if(fila_suspensas){
        do{
            if (aux->tarefa_suspendeu == task_atual->id){
                aux = aux->next;
                task_resume(aux->prev, &fila_suspensas);
            }else{
                aux = aux->next;
            }
        }while((fila_suspensas != NULL)&&(aux != fila_suspensas));
    }
}

void testa_adormecidas(){

    task_t *aux = fila_adormecidas;
    if(fila_adormecidas){
        do{
            if (aux->tempo_adormecida == systime()){
                aux = aux->next;
                task_resume(aux->prev, &fila_adormecidas);
            }else{
                aux = aux->next;
            }
        }while((fila_adormecidas != NULL)&&(aux != fila_adormecidas));
    }
}

void acordar_tarefas_semaforo(task_t *fila){

    task_t *aux = fila;
    
    if(fila){
        while(queue_size((queue_t*) fila) > 0){
            task_resume(aux->prev, &fila);
            aux = aux->next;
        }
    }
}

void task_sleep (int t){

    #ifdef DEBUG
        printf("Tarefa %d indo a mimir por %d\n", task_atual->id, t);
    #endif

    task_atual->tempo_adormecida = t+systime();

    task_suspend(&fila_adormecidas);
}

int sem_down(semaphore_t *s){

    if(!s || s->destruido)
        return -1;
    
    if(s->lock <= 0)
        task_suspend(&s->fila_sem);

    s->lock -= 1;

    while (__sync_fetch_and_or (&s->lock, 0));

    if(!s)
        return -1;
    return 0;
}

int sem_up(semaphore_t *s){

    if(!s || s->destruido)
        return -1;

    if(queue_size((queue_t*) s->fila_sem) > 0)
        task_resume(s->fila_sem, &s->fila_sem);

    (s->lock) += 1 ;
    return 0;
}

int sem_init (semaphore_t *s, int value){
    
    if(!s)
        return -1;

    semaforos_existem += 1;

    s->destruido = 0;
    s->lock = value;
    s->fila_sem = NULL;

    return 0;
}

int sem_destroy (semaphore_t *s){

    if(!s || s->destruido)
        return -1;

    s->lock += queue_size((queue_t*) s->fila_sem);

    acordar_tarefas_semaforo(s->fila_sem);

    semaforos_existem -= 1;

    s->destruido = 1;
    s->fila_sem = NULL;
    s = NULL;

    return 0;

}

int mqueue_init (mqueue_t *queue, int max, int size){
    
    if(!queue)
        return -1;
    if(queue->destruido)
        return -2;

    queue->max_msgs = max;
    queue->msg_size = size;
    queue->destruido = 0;

    return 0;

}

int mqueue_destroy (mqueue_t *queue){

    if(!queue)
        return -1;
    if(queue->destruido)
        return -2;
    
    queue->destruido = 1;
    queue->fila = NULL;
    queue->max_msgs = 0;
    queue->msg_size = 0;

    return 0;

}

int mqueue_send (mqueue_t *queue, void *msg){

    fila_msg_t *mensagem = malloc(sizeof(fila_msg_t));
    mensagem->msg = malloc(queue->msg_size);

    if(!queue)
        return -1;
    if(queue->destruido)
        return -2;

    if(queue_size((queue_t *) queue->fila) > queue->max_msgs){
        sem_down(&s_send);

        sem_down(&s_buffer);
        bcopy(msg, mensagem->msg, queue->msg_size);
        queue_append((queue_t **) &queue->fila, (queue_t *) mensagem);
        sem_up(&s_buffer);
    }else{
        sem_down(&s_buffer);
        bcopy(msg, mensagem->msg, queue->msg_size);
        //printf("%p, %p\n", mensagem->next, mensagem->prev);
        queue_append((queue_t **) &queue->fila, (queue_t *) mensagem);
        sem_up(&s_buffer);
    }
    sem_up(&s_receiver);

    return 0;
}

int mqueue_recv (mqueue_t *queue, void *msg){

    if(!queue)
        return -1;
    if(queue->destruido)
        return -2;

    if(queue_size((queue_t *) queue->fila) <= 0){
        sem_down(&s_receiver);

        printf("oi : %p", queue->fila);

        sem_down(&s_buffer);
        bcopy(queue->fila->msg, msg, queue->msg_size);
        queue->fila = queue->fila->next;
        queue_remove((queue_t **) &queue->fila->prev, (queue_t *) queue->fila);
        sem_up(&s_buffer);
    }else{
        sem_down(&s_buffer);
        bcopy(queue->fila->msg, msg, queue->msg_size);
        queue->fila = queue->fila->next;
        queue_remove((queue_t **) &queue->fila->prev, (queue_t *) queue->fila->prev);
        sem_up(&s_buffer);
    }
    sem_up(&s_send);

    return 0;

}

int mqueue_msgs (mqueue_t *queue){

    if(!queue)
        return -1;
    if(queue->destruido)
        return -2;

    return (queue_size((queue_t *) queue->fila));

}

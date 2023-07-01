// Rafael Augusto Boschi de Campos GRR20182984
// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.4 -- Janeiro de 2022

// Demonstração das funções POSIX de troca de contexto (ucontext.h).

// cc -Wall -o teste queue.c ppos_core.c ppos_disk.c disk.c pingpong_disco.c -lrt -lm

#include "ppos.h"
#include "queue.h"
#include "ppos_disk.h"
#include "disk.h"

#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <signal.h>
#include <sys/time.h>
#include <string.h>

disk_t *disco;
task_t t_gerente, *aux;
semaphore_t s_disco;
pedidos_t *pedido_aux, *pedido;;
int acordado = 0, dormindo = 0, disco_livre = 1;

// estrutura que define um tratador de sinal (deve ser global ou static)
struct sigaction acao ;

task_t* acha_tarefa_por_id(task_t *fila, int id);
void gerente_de_disco ();

void tratador_disco (int signum)
{
    acordado = 1;
    disco_livre = 1;
}

task_t* acha_tarefa_por_id(task_t *fila, int id){
    task_t *andador = fila;
    task_t *ini = fila;
    while(andador->next != ini){ //Faz a iteração até achar o ultimo elemento da fila.
        if(andador->id == id)
            return andador;
        andador = andador->next;
    }

    return NULL;

}

int disk_mgr_init (int *numBlocks, int *blockSize)
{
    if(disk_cmd(DISK_CMD_INIT, 0, 0) < 0){
        return -1;
    }

    *numBlocks = disk_cmd(DISK_CMD_DISKSIZE, 0, 0);
    if(numBlocks < 0) 
        return -1;
    disco->numero_de_blocos = *numBlocks;
    printf("tchau \n");

    *blockSize = disk_cmd(DISK_CMD_BLOCKSIZE, 0, 0);
    if(blockSize < 0) 
        return -1;
    disco->tamanho_de_blocos = *blockSize;

    task_init(&t_gerente, gerente_de_disco, "TASK GERENTE");
    sem_init(&s_disco, 1);

    acao.sa_handler = tratador_disco ;
    sigemptyset (&acao.sa_mask) ;
    acao.sa_flags = 0 ;
    if (sigaction (SIGUSR1, &acao, 0) < 0)
    {
        perror ("Erro em sigaction: ") ;
        exit (1) ;
    }

    return 0;
}

void gerente_de_disco ()
{
    while (1) 
    {
        sem_down(&s_disco);

      // se foi acordado devido a um sinal do disco
        if (acordado)
        {
            acordado = 0;
            aux = (task_t *) acha_tarefa_por_id(disco->fila_disco, disco->fila_de_pedidos->tarefa_origem);
            if(!aux) exit(-6);
            task_resume(aux, &(disco->fila_disco));
            pedido_aux = disco->fila_de_pedidos;
            disco->fila_de_pedidos = disco->fila_de_pedidos->next;
            queue_remove((queue_t **) &(disco->fila_de_pedidos), (queue_t *) pedido_aux);
        }

        // se o disco estiver livre e houver pedidos de E/S na fila
        if (disco_livre && (disco->fila_de_pedidos != NULL))
        {
            disco_livre = 0;
            if(disco->fila_de_pedidos)
            {
                if(disk_cmd(DISK_CMD_READ, disco->fila_de_pedidos->bloco, disco->fila_de_pedidos->buffer_pedido) < 0) 
                {
                    exit(-1);
                }
            }
            else
            {
                if(disk_cmd(DISK_CMD_WRITE, disco->fila_de_pedidos->bloco, disco->fila_de_pedidos->buffer_pedido) < 0) 
                {
                    exit(-1);
                }
            }
        }
        sem_up(&s_disco);

        dormindo = 1;

        task_suspend(&(disco->fila_disco));
        task_yield();
    }
    task_exit(0);
}

int disk_block_read (int block, void *buffer)
{

    pedido->tipo_pedido = 1;
    pedido->bloco = block;
    pedido->buffer_pedido = buffer;
    pedido->tarefa_origem = task_id();
    sem_down(&s_disco);

    if(queue_append((queue_t **) &(disco->fila_de_pedidos), (queue_t *) pedido))
        return -1;

    if (dormindo)
    {
        task_resume(&t_gerente, &(disco->fila_disco));
    }

    sem_up(&s_disco);

    task_suspend(&(disco->fila_disco));

    task_yield();

    return 0;
}

int disk_block_write (int block, void *buffer)
{

    pedido->tipo_pedido = 0;
    pedido->bloco = block;
    pedido->buffer_pedido = buffer;
    pedido->tarefa_origem = task_id();
    sem_down(&s_disco);

    if(queue_append((queue_t **) &(disco->fila_de_pedidos), (queue_t *) pedido))
        return -1;

    if (dormindo)
    {
        task_resume(&t_gerente, &(disco->fila_disco));
    }

    sem_up(&s_disco);

    task_suspend(&(disco->fila_disco));

    task_yield();

    return 0;
}
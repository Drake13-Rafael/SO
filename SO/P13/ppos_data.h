// Rafael Augusto Boschi de Campos GRR20182984
// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.5 -- Março de 2023

// Estruturas de dados internas do sistema operacional

#ifndef __PPOS_DATA__
#define __PPOS_DATA__

#include <ucontext.h>		// biblioteca POSIX de trocas de contexto

// Estrutura que define um Task Control Block (TCB)
typedef struct task_t
{
  struct task_t *prev, *next ;		// ponteiros para usar em filas
  int id ;				// identificador da tarefa
  ucontext_t context ;			// contexto armazenado da tarefa
  short status ;			// pronta, rodando, suspensa, ...
  short prioridade_estatica;
  short prioridade_dinamica;
  unsigned long int tempo_processamento;
  unsigned long int tempo_execucao;
  unsigned long int ativacoes;
  int tarefa_suspendeu;
  int tempo_adormecida;
  // ... (outros campos serão adicionados mais tarde)
} task_t ;

// estrutura que define um semáforo
typedef struct semaphore_t
{
  int lock;
  task_t *fila_sem;
  int destruido;
} semaphore_t ;

// estrutura que define um mutex
typedef struct
{
  // preencher quando necessário
} mutex_t ;

// estrutura que define uma barreira
typedef struct
{
  // preencher quando necessário
} barrier_t ;

// Fila das mensagens
typedef struct fila_msg_t
{
  struct fila_msg_t *prev, *next;
  void *msg;
} fila_msg_t ;

// estrutura que define uma fila de mensagens
typedef struct mqueue_t
{
  fila_msg_t *fila;
  int destruido;
  unsigned long int max_msgs;
  unsigned long int msg_size;
  semaphore_t s_escrita, s_leitura, s_buffer;
} mqueue_t ;

#endif

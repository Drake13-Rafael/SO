// Rafael Augusto Boschi de Campos GRR20182984
// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.4 -- Janeiro de 2022

// interface do gerente de disco rígido (block device driver)

#ifndef __DISK_MGR__
#define __DISK_MGR__

// estruturas de dados e rotinas de inicializacao e acesso
// a um dispositivo de entrada/saida orientado a blocos,
// tipicamente um disco rigido.

// estrutura que representa um disco no sistema operacional
typedef struct disk_t
{
  int numero_de_blocos;
  int tamanho_de_blocos;
  struct pedidos_t *fila_de_pedidos;
  struct task_t *fila_disco;
} disk_t ;

typedef struct pedidos_t
{
  struct pedidos_t *prev;
  struct pedidos_t *next;
  int tipo_pedido;
  int bloco;
  void *buffer_pedido;
  int tarefa_origem;
} pedidos_t ;

// inicializacao do gerente de disco
// retorna -1 em erro ou 0 em sucesso
// numBlocks: tamanho do disco, em blocos
// blockSize: tamanho de cada bloco do disco, em bytes
int disk_mgr_init (int *numBlocks, int *blockSize) ;

// leitura de um bloco, do disco para o buffer
int disk_block_read (int block, void *buffer) ;

// escrita de um bloco, do buffer para o disco
int disk_block_write (int block, void *buffer) ;

void tratador_disco (int signum);

#endif
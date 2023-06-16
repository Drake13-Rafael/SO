// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.5 -- Março de 2023

// Teste de semáforos (light)

#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"

int item, buffer[5];
semaphore_t s_buffer, s_item, s_vaga;
task_t consumidor1, consumidor2, produtor1, produtor2, produtor3;

void consumidor();
void produtor();

// Produtor
void Produtor()
{
   while (1)
   {
      task_sleep(1000);
      item = rand() % 100;

      sem_down(&s_vaga);

      sem_down(&s_buffer);
      //insere item no buffer "produziu %d", item
      sem_up(&s_buffer);

      sem_up(&s_item);

      if (systime > 1000000)
      {
         task_exit(0);
      }
      
   }
}

// Consumidor
void Consumidor()
{
   while (1)
   {
      sem_down(&s_item);

      sem_down(&s_buffer);
      //retira item do buffer "consumiu %d" buffer[0]
      sem_up(&s_buffer);

      sem_up(&s_vaga);

      //print item
      task_sleep(1000);

      if (systime > 1000000)
      {
         task_exit(0);
      }
   }
}

int main (int argc, char *argv[])
{
   printf ("main: inicio\n") ;

   ppos_init () ;

   // inicia semaforos
   sem_init (&s_buffer, 0) ;
   sem_init (&s_vaga, 0) ;
   sem_init (&s_item, 1) ;

   // inicia tarefas
   task_init (&produtor1, Produtor, "p1") ;
   task_init (&produtor2, Produtor, "p2") ;
   task_init (&produtor3, Produtor, "p3") ;
   task_init (&consumidor1, Consumidor, "c1") ;
   task_init (&consumidor2, Consumidor, "c2") ;

   task_sleep(1000000);

   // destroi semaforos
   sem_destroy (&s_buffer) ;
   sem_destroy (&s_vaga) ;
   sem_destroy (&s_item) ;

   printf ("main: fim\n") ;
   task_exit (0) ;
}
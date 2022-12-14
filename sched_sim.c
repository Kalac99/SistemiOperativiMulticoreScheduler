#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "fake_os.h"

FakeOS os;



typedef struct {
  int quantum;
} SchedArgs;

void sched(FakeOS* os, void* args_){
  if (List_isFull(&os->running)==1) return;
  SchedArgs* args=(SchedArgs*)args_;
  
  // look for the first process in ready
  // if none, return
  if (!os->ready.first)
    return;
  FakePCB* pcb;
  switch(scheduler){
  //Se scheduler==1 allora dalla ready verrà preso il primo processo, classico round robin
  case 1:
  pcb = (FakePCB*) List_popFront(&os->ready);
  break;

  // Se scheduler == 2 allora l'utente ha scelto una politica SRJF, dalla ready verrà estratto il processo con meno tempo rimanente <- può essere
  //invocato anche prima della fine del burst -> preemption
  case 2:
    {
      ListItem* aux = os->ready.first;
      int min = 100;
      FakePCB* minpcb;
      int  concurrent;

      while(aux){
        pcb = (FakePCB*) aux;
        ProcessEvent* evento = (ProcessEvent*) pcb->events.first;
        concurrent = evento->duration;
        if(concurrent<min) {
          min = concurrent;
          minpcb = pcb;
        }
        aux = aux->next; 
      }
      pcb = (FakePCB*) List_detach(&os->ready,(ListItem*) minpcb);
      break;
    }
  // Se scheduler==3 allora l'utente ha scelto una politica di scheduling basata su priorità, verrà estratto dalla ready il processo con maggior priorità 
  // (valore numerico minore) <- può essere invocato anche prima della fine del burst -> preemption
  case 3:
    {
      ListItem* aux = os->ready.first;
      int min = 100;
      FakePCB* minpcb;
      int  concurrent;
      while(aux){
        pcb = (FakePCB*) aux;
        concurrent = pcb->temp_prio;
        //La priorità dei processi in attesa viene aumentata all'incirca ogni 4 burst (20 unità di tempo) finchè non raggiunge massima priorità o gli viene affidata la cpu,
        //terminato il burst ogni processo torna ad avere la priorità assegnatagli all'inizio -> tutto per evitare starving dei processi a bassa priorità
        //ogni volta che la priortà viene aumentata l'utente viene avvisato con un messaggio e il contatore viene azzerato
        if (pcb->counter==20*nuclei) {
          if(pcb->temp_prio>1) {
            pcb->temp_prio--;
            printf("\t\tprocess %d waited for %d time slots, his priority is now increased\n",pcb->pid,quanto-1);
            pcb->counter=0;
          }
        }
        else pcb->counter++;
        if(concurrent<min) {
          min = concurrent;
          minpcb = pcb;
        }
        aux = aux->next; 
      }
      pcb = (FakePCB*) List_detach(&os->ready,(ListItem*) minpcb);
      break;
    }
  }
  
  //Se c'è spazio nei core inserisco in running il pcb appena preso dai ready
  if (List_isFull(&os->running)==0){
    List_pushBack(&os->running,(ListItem*)pcb);  
  }
  else return;
  
  assert(pcb->events.first);
  ProcessEvent* e = (ProcessEvent*)pcb->events.first;
  assert(e->type==CPU);

  // look at the first event
  // if duration>quantum
  // push front in the list of event a CPU event of duration quantum
  // alter the duration of the old event subtracting quantum
  if (e->duration>args->quantum) {
    ProcessEvent* qe=(ProcessEvent*)malloc(sizeof(ProcessEvent));
    qe->list.prev=qe->list.next=0;
    qe->type=CPU;
    qe->duration=args->quantum;
    e->duration-=args->quantum;
    List_pushFront(&pcb->events, (ListItem*)qe);
  }
};

int main(int argc, char** argv) {

  //Questa print è molto utile per far capire all'utente come eseguire il programma, l'ordine e il significato dei parametri da passargli
  printf("\n----------------------------------------------");
  printf("\nPer l'esecuzione del programma è necessario far partire l'eseguibile passando come argomenti i seguenti: \n-Numero di core voluti(minimo 1) \n-Numero rappresentante la politica di scheduling: \n\t1->Round Robin\n\t2->Shortest Remaining Job First\n\t3->Priority Based(Assicurarsi di passare processi compatibili) \n-Dimensione del quanto voluto, maggiore o uguale a 1 (suggerito 5)\n-Processi da dare in pasto al programma\n");
  printf("----------------------------------------------\n");
  //Prendo il numero di core
  nuclei = atoi(argv[1]);
  if(nuclei<1){
    printf("\nSono stati richiesti meno di 1 core, questo è impossibile, ritenta...\n\n");
    return 0;
  }
  //Prendo la politica di scheduling
  scheduler = atoi(argv[2]);
  if(scheduler<1 || scheduler >3){
    printf("\nNessuno scheduler è stato scelto, le opzioni dispondibili sono 1, 2, 3 guarda sopra...\n\n");
    return 0;
  }
  //Prendo la grandezza del quanto
  quanto = atoi(argv[3]);
  if(quanto<1 ){
    printf("\nDimensione del quanto non specificata o minore di 1, scegliere un intero positivo prego...\n\n");
    return 0;
  }

  FakeOS_init(&os);
  SchedArgs sched_args;
  sched_args.quantum=quanto;
  os.schedule_args=&sched_args;
  os.schedule_fn=sched; 
  
  for (int i=4; i<argc; ++i){
    FakeProcess new_process;
    int num_events=FakeProcess_load(&new_process, argv[i]);
    printf("loading [%s], pid: %d, events:%d",
           argv[i], new_process.pid, num_events);
    if (num_events) {
      FakeProcess* new_process_ptr=(FakeProcess*)malloc(sizeof(FakeProcess));
      *new_process_ptr=new_process;
      List_pushBack(&os.processes, (ListItem*)new_process_ptr);
    }
  }
  printf("num processes in queue %d\n", os.processes.size);
  //Tramite os.running.first viene controllato che almeno un processo sia in esecuzione sulla cpu
  while(os.running.first
        || os.ready.first
        || os.waiting.first
        || os.processes.first){
    FakeOS_simStep(&os);
  }
  printf(" ALL PROCESSES ENDED THEIR WORK ..... BYE BYE\n");
  //FakeOS_destroy(&os);
}

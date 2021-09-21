#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "fake_os.h"

FakeOS os;



typedef struct {
  int quantum;
} SchedRRArgs;

void schedRR(FakeOS* os, void* args_){
  if (List_isFull(&os->running)==1) return;
  SchedRRArgs* args=(SchedRRArgs*)args_;
  
  // look for the first process in ready
  // if none, return
  if (!os->ready.first)
    return;
  FakePCB* pcb;

  //Se scheduler==1 allora dalla ready verrà preso il primo processo
  if (scheduler==1) pcb = (FakePCB*) List_popFront(&os->ready);

  // Se scheduler == 2 allora l'utente ha scelto una politica SRJF quando il quanto termina, dalla ready verrà estratto il processo con meno tempo rimanente 
  else if (scheduler == 2){
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

  //Prendo il numero di parametri...

  

  while(nuclei<1){
    printf("Inserisci il numero di core voluti, minimo 1: ");
    scanf("%d",&nuclei);
    if(nuclei==0) printf("Sul serio? Che ci fai con una CPU inutile?\n");
  }

  while(scheduler<1 || scheduler >2){
    printf("Inserisci il tipo di scheduler voluto 1->RR 2->SRJF: ");
    scanf("%d",&scheduler);
  }

  FakeOS_init(&os);
  SchedRRArgs srr_args;
  if(scheduler == 2) srr_args.quantum=1;
  else srr_args.quantum=5;
  os.schedule_args=&srr_args;
  os.schedule_fn=schedRR; 
  
  for (int i=1; i<argc; ++i){
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
  // qua forse sarà os.running.first, oppure un os.running1 .. os.runningN
  // OCCHIO qua c'era os.running
  //Nella mia prima versione è os.running.first
  while(os.running.first
        || os.ready.first
        || os.waiting.first
        || os.processes.first){
    FakeOS_simStep(&os);
  }
  printf(" ALL PROCESSES ENDED THEIR WORK ..... BYE BYE\n");
}

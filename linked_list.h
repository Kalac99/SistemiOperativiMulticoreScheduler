#pragma once

typedef struct ListItem {
  struct ListItem* prev;
  struct ListItem* next;
} ListItem;

typedef struct ListHead {
  ListItem* first;
  ListItem* last;
  int size;
  int maxsize;
} ListHead;

void List_init(ListHead* head);
ListItem* List_find(ListHead* head, ListItem* item);
ListItem* List_insert(ListHead* head, ListItem* previous, ListItem* item);
ListItem* List_detach(ListHead* head, ListItem* item);
ListItem* List_pushBack(ListHead* head, ListItem* item);
ListItem* List_pushFront(ListHead* head, ListItem* item);
ListItem* List_popFront(ListHead* head);
int List_isFull(ListHead* head); //Funzione aggiunta per controllare se la lista è piena -> Controllo che i processi in esecuzione non eccedano il numero di core
void List_delete(ListHead* head); //Funzione aggiunta per eliminare una lista -> Usata nel FakeOS_destroy, non ben chiaro se funzioni
//NAME: Rohan Varma
//EMAIL: rvarm1@ucla.edu
//ID: 111111111
//SortedList.h implementation with yields

#include "SortedList.h"
#include <sched.h>
#include <string.h>
#include <stdio.h>

void SortedList_insert(SortedList_t *list, SortedListElement_t *element) {
  SortedListElement_t* next = list->next;
  while(next->key != NULL && strcmp(next->key, element->key) < 0)
    next = next->next;
    
  next->prev->next = element; //link up prev's next pointer
  element->prev = next->prev; //connect element's prev to prev element in list
  if(opt_yield & INSERT_YIELD)
    sched_yield();
  //link up element's next pointer
  element->next = next;
  next->prev = element; //reassign next's pre vpointer
}

int SortedList_delete( SortedListElement_t *element){
  if(element == NULL || element->next->prev != element->prev->next) return 1;
  if(opt_yield & DELETE_YIELD)
    sched_yield();
  element->prev->next = element->next;
  element->next->prev = element->prev;
  return 0;
}

SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key){
  if(list == NULL || key == NULL)
    return NULL;
  SortedListElement_t *next = list->next;
  while(next!=list){
    if(strcmp(next->key, key) > 0){
      return NULL; //because ascending
    }
    else if(strcmp(next->key, key) == 0){
      return next;
    }
    if(opt_yield & LOOKUP_YIELD)
      sched_yield();
    next=next->next;
  }
  return NULL; //wasn't found
}

int SortedList_length(SortedList_t *list){
  if(list == NULL) return -1; //indicates corruption
  int count = 0;
  SortedListElement_t *next = list->next;
  while(next != list){
    //verify ptrs
    if(next->prev->next != next || next->next->prev != next)
      return -1;
    ++count;
    if(opt_yield & LOOKUP_YIELD)
      sched_yield();
    next=next->next;
  }
  return count;
}

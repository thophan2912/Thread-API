#define _GNU_SOURCE
#include "thread.h"
#include <ucontext.h>

#include <stdio.h>
#include <stdlib.h>

//****************************************************************************
// Private Definitions
//****************************************************************************
/**
 * The Thread Control Block.
 */
// enum thread states
typedef enum {
  READY = 1,
  RUNNING = 2,
  YIELDING = 3,
  ZOMBIE = 4,
  TERMINATED = 5,
  CREATING = 6,
} THREAD_STATE;

typedef struct thread_control_block
{
  Tid thread_id;
  THREAD_STATE thread_state;
  ucontext_t thread_context;
  char *stack_malloc;
} TCB;
typedef struct linkedlist list;
typedef struct node node;

//**************************************************************************************************
// Private Global Variables (Library State)
//**************************************************************************************************

TCB allTCBs[MAX_THREADS]; // all TCBs
list* threadList; // THREAD READY QUEUE
list* zombieList;
Tid RUNNINGid; // 0 <= RUNNINGid < 256 is the thread_id in array allTCBs;
Tid allIDs[MAX_THREADS]; // all IDs from 0 to 255: -1 is not available, else avail to set thr_id

//**************************************************************************************************
// Helper Functions
//**************************************************************************************************

void ThreadExit();
void MyThreadStub(void (*f) (void *), void *arg) {
  f(arg);
  ThreadExit();
}

Tid getAvailID() {
  for (int i=0; i<MAX_THREADS; i++) {
    if (allIDs[i] != -1) {
      return allIDs[i];
    }
  }
  return -2;
}


// struct ListNode
typedef struct node {
  Tid thread_id;
  node* next;
} node;

node* createNode(Tid new_thread) {
  node* newNode;
  newNode = (node *)malloc(sizeof(node));
  newNode->thread_id = new_thread;
  newNode->next = NULL;
  return newNode;
}

typedef struct linkedlist {
  node* first;
  int capacity;
  int size;
} list;

list* createList(int cap) {
  list* newList;
  newList = (list*)malloc(sizeof(list));
  newList->first = NULL;
  newList->capacity = cap;
  newList->size = 0;
  return newList;
}

void insertNode(list* llist, Tid newThread) {
  node* newNode = createNode(newThread);
  // insert when list is empty
  if (llist->first == NULL) {
    llist->first = newNode;
  }
  else {
    node* p = llist->first;
    // insert in the last
    while(p->next != NULL) {
      p = p->next;
    }
    p->next = newNode; // make sure newNode->next = NULL
  }
  llist->size++;
}

void insertNodeFirst(list* llist, Tid newThread) {
  node* newNode = createNode(newThread);
  if (llist->first == NULL) {
    llist->first = newNode;
  }
  else {
    newNode->next = llist->first;
    llist->first = newNode;
  }
  llist->size++;
}

void removeNode(list* llist, Tid rmNode) {
  node* p = llist->first;
  if (llist->first->thread_id == rmNode) {
    llist->first = llist->first->next;
  }
  else {
    while(p->next->thread_id != rmNode) {
      p = p->next;
    }
    node* rmNode = p->next;
    p->next = rmNode->next;
  }
  llist->size--;
}

//**************************************************************************************************
// thread.h Functions
//**************************************************************************************************
int ThreadInit()
{
  TCB first_thread;
  first_thread.thread_id=0;
  Tid first_id = 0;
  allTCBs[first_thread.thread_id] = first_thread; 
  allTCBs[first_id].thread_state = RUNNING;
  RUNNINGid = 0;
  threadList = createList(MAX_THREADS);
  zombieList = createList(MAX_THREADS);
  for (int i=0; i<MAX_THREADS; i++) {
    allIDs[i] = i;
  }
  allIDs[0] = -1; // since running kernel thread is 0.
  return 0;
}

Tid ThreadId()
{
  return RUNNINGid;
}

Tid ThreadCreate(void (*f)(void*), void* arg)
{
  if (getAvailID() == -2) {
    return ERROR_SYS_THREAD;
  }
  TCB newThread;
  Tid newID = getAvailID(); 
  allIDs[newID] = -1;
  while (zombieList->size > 0) {
    Tid zombieID = zombieList->first->thread_id;
    allTCBs[zombieID].thread_id = -1;
    free(allTCBs[zombieID].stack_malloc); // free stack
    allTCBs[zombieID].thread_state = TERMINATED;
    removeNode(zombieList, zombieList->first->thread_id);
  }
  newThread.thread_id = newID;
  allTCBs[newID] = newThread;
  allTCBs[newID].thread_state = READY;
  insertNode(threadList, allTCBs[newID].thread_id);
  getcontext(&(allTCBs[newID].thread_context));
  // temp_hello_world();

  allTCBs[newID].stack_malloc = malloc(THREAD_STACK_SIZE);
  unsigned long top_stack = (unsigned long)allTCBs[newID].stack_malloc + THREAD_STACK_SIZE;

  allTCBs[newID].thread_context.uc_mcontext.gregs[REG_RSP] = (greg_t)(top_stack - top_stack % 16 - 8);
  allTCBs[newID].thread_context.uc_mcontext.gregs[REG_RSI] = (greg_t)arg;
  allTCBs[newID].thread_context.uc_mcontext.gregs[REG_RDI] = (greg_t)f;
  allTCBs[newID].thread_context.uc_mcontext.gregs[REG_RIP] = (greg_t)&MyThreadStub;

  return newID;
}

void ThreadExit()
{
  if (threadList->size == 0) {
    exit(0);
  }
  else {
    allIDs[RUNNINGid] = RUNNINGid;
    allTCBs[RUNNINGid].thread_state = ZOMBIE;
    ThreadYield();
  }
}

Tid ThreadKill(Tid tid)
{
  if (tid < 0 || tid >= 256) {
    return ERROR_TID_INVALID;
  }
  if (RUNNINGid == tid) {
    return ERROR_THREAD_BAD;
  }
  node* p = threadList->first;
  while (p != NULL) {
    if (p->thread_id == tid) {
      allIDs[tid] = tid;
      allTCBs[tid].thread_state = TERMINATED;
      allTCBs[tid].thread_id = -1;
      removeNode(threadList, tid);
      free(allTCBs[tid].stack_malloc);
      return tid;
    }
    else {
      p = p->next;
    }
  }
  return ERROR_SYS_THREAD;
}


int ThreadYield()
{
  if (threadList->size == 0) {
    return RUNNINGid; // exit(0)
  }
  else {
    Tid yieldID = threadList->first->thread_id;
    if (allTCBs[RUNNINGid].thread_state == ZOMBIE) {
      allIDs[RUNNINGid] = RUNNINGid;
      removeNode(threadList, allTCBs[yieldID].thread_id);
      insertNode(zombieList, allTCBs[RUNNINGid].thread_id);
      RUNNINGid = allTCBs[yieldID].thread_id;
      allTCBs[yieldID].thread_state=RUNNING;
      setcontext(&(allTCBs[yieldID].thread_context));
    }
    else if (allTCBs[RUNNINGid].thread_state == RUNNING) {
      allTCBs[RUNNINGid].thread_state = YIELDING;
      insertNode(threadList, allTCBs[RUNNINGid].thread_id);

      removeNode(threadList, allTCBs[yieldID].thread_id);
      getcontext(&(allTCBs[RUNNINGid].thread_context));

      if (allTCBs[RUNNINGid].thread_state == YIELDING) {
        allTCBs[RUNNINGid].thread_state = READY;
        allTCBs[yieldID].thread_state = RUNNING;
        RUNNINGid = allTCBs[yieldID].thread_id;
        setcontext(&allTCBs[yieldID].thread_context);
      }
      else if (allTCBs[RUNNINGid].thread_state == RUNNING) {
        Tid returnID = allTCBs[yieldID].thread_id;
        if (allTCBs[yieldID].thread_state == ZOMBIE) {
          removeNode(zombieList, allTCBs[yieldID].thread_id);
          allTCBs[yieldID].thread_id = -1;
          free(allTCBs[yieldID].stack_malloc); // free stack
          allTCBs[yieldID].thread_state = TERMINATED;
        }
        return returnID;
      }
    }
  }
}

int ThreadYieldTo(Tid tid)
{
  if (tid < 0 || tid >= 256) {
    return ERROR_TID_INVALID;
  }
  if (allTCBs[tid].thread_state == RUNNING) {
    return tid;
  }
  node* p = threadList->first;
  while (p != NULL) {
    if (p->thread_id == tid) {
      removeNode(threadList, tid);
      insertNodeFirst(threadList, tid);
      return ThreadYield();
    }
    else {
      p = p->next;
    }
  }
  return ERROR_THREAD_BAD;
}
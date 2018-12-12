#include "linked_list.h"
#include "stdio.h"
#include "stdlib.h"
#include "stdbool.h"

typedef struct {
  int value;
  void* next;
} node;

node* first = NULL;
node* last = NULL;
int list_size = 0;

_Bool push(int entry)
{
  if (entry >= 0)
  {
    if(list_size == 0)
    {
      first = last = malloc(sizeof(node));
      first->value = entry;
      first->next = NULL;
      list_size = 1;
    }
    else
    {
      last->next = malloc(sizeof(node));
      last = last->next;
      last->value = entry;
      last->next = NULL;
      list_size++;
    }
    return true;
  }
  else
  {
    return false;
  }
}

int pop()
{
  if (list_size >= 2)
  {
    node* tmp = first->next;
    int ret = first->value;
    free(first);
    first = tmp;
    list_size--;
    return ret;
  }
  else if (list_size == 1)
  {
    int ret = first->value;
    free(first);
    list_size--;
    first = last = NULL;
    return ret;
  }
  else
  {
    return -1;
  }
}

_Bool insert(int entry)
{
  if (entry >= 0)
  {
    if (list_size == 0)
    {
      return push(entry);
    }
    else if (first->value <= entry)
    {
      node* tmp = first;
      first = malloc(sizeof(node));
      first->value = entry;
      first->next = tmp;
      list_size++;
      return true;
    }
    else if (list_size == 1) // and first->value >= entry
    {
      last = malloc(sizeof(node));
      first->next = last;
      last->value = entry;
      list_size++;
      return true;
    }
    else
    {
      node* prev = first;
      node* sel = first->next;
      for (int i = 1; i < list_size;i++)
      {
        if (sel->value <= entry)
        {
          prev->next = malloc(sizeof(node));
          prev = prev->next;
          prev->value = entry;
          prev->next = sel;
          list_size++;
          return true;
        }
        else if (i == list_size - 1)
        {
          return push(entry);
        }
        prev = sel;
        sel = sel->next;
      }
    }
    return false;
  }
  else
  {
    return false;
  }
}

_Bool erase(int entry)
{
  _Bool ret = false;
  if (list_size != 0)
  {
    node* tmp;
    while (list_size != 0 && first->value == entry)
    {
      tmp = first;
      if (first != last) first = first->next;
      else
      {
        first = last = NULL;
      }
      free(tmp);
      list_size--;
      ret = true;
    }
    if (list_size != 0 && list_size >= 2)
    {
      node* prev = first;
      node* sel = first->next;
      int count = list_size;
      for (int i = 1; i < count; i++)
      {
        if (sel->value == entry)
        {
          ret = true;
          if (sel->next == NULL) last = prev;
          prev->next = sel->next;
          free(sel);
          list_size--;
          if (last != prev)
          {
            sel = prev->next;
          }
        }
        else
        {
          prev = sel;
          sel = sel->next;
        }
      }
    }
  }
  return ret;
}

int getEntry(int idx)
{
  if (idx >= 0 && idx < list_size)
  {
    node* tmp = first;
    for (int i = 0; i < idx; i++)
    {
      tmp = tmp->next;
    }
    return tmp->value;
  }
  else return -1;
}

int size()
{
  return list_size;
}

void clear()
{
  if (list_size != 0)
  {
    while(first != last)
    {
      node* tmp = first->next;
      free(first);
      first = tmp;
    }
    free(first);
    first = NULL;
    last = NULL;
    list_size = 0;
  }
}

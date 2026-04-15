#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/_types/_sigaltstack.h>

/*
 *
 *  Valid Parenthesis
 *
 */

#define MAX_STACK_SIZE 1024

typedef struct {
  uint64_t arr[MAX_STACK_SIZE];
  uint64_t top;
} stack;

void init_stack(stack* stack_) { stack_->top = -1; }
void push(stack* stack, uint64_t value) {
  if (stack->top >= MAX_STACK_SIZE - 1) {
    printf("Stack Overflow");
    return;
  }
  stack->arr[++stack->top] = value;
}

uint8_t pop(stack* stack) {
  if (stack->top == -1) {
    printf("Empty Stack");
    return -1;
  }
  uint64_t popped = stack->arr[stack->top];
  stack->top--;
  return popped;
}

bool isValid(const char* input) {
  stack* stack;
  init_stack(stack);
}

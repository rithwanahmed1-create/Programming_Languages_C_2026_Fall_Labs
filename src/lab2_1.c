#include <stdio.h>

/*
    Task:
    Write a function `int sum_to_n(int n)` that computes
    the sum of all integers from 1 up to n using a for loop.

    In main():
      - Ask user for a positive integer n
      - If n < 1, print an error
      - Otherwise, call sum_to_n and print the result
*/

int sum_to_n(int n) {
  int sum = 0;
  int i;

  for (i = 1; i <= n; i++) {
    sum += i;
  }

  return sum;
}

int main(void) {
  int n;
  int result;

  printf("Enter a positive integer n: ");
  if (scanf("%d", &n) != 1) {
    printf("Error: invalid input.\n");
    return 1;
  }

  if (n < 1) {
    printf("Error: n must be positive.\n");
    return 1;
  }

  result = sum_to_n(n);
  printf("Sum from 1 to %d is %d\n", n, result);

  return 0;
}

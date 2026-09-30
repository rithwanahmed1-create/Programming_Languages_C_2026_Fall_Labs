#include <stdio.h>

/*
    Task:
    Write a function `long long factorial(int n)` that computes n!
    using a loop (not recursion).

    In main():
      - Ask user for an integer n
      - If n is negative, print an error and exit
      - Otherwise, call factorial and print the result
*/

long long factorial(int n) {
  long long result = 1;
  int i;

  for (i = 2; i <= n; i++) {
    result *= i;
  }

  return result;
}

int main(void) {
  int n;
  long long result;

  printf("Enter a non-negative integer n: ");
  if (scanf("%d", &n) != 1) {
    printf("Error: invalid input.\n");
    return 1;
  }

  if (n < 0) {
    printf("Error: n must be non-negative.\n");
    return 1;
  }

  result = factorial(n);
  printf("%d! = %lld\n", n, result);

  return 0;
}

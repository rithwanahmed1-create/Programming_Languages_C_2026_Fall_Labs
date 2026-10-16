/*
 * week4_2_struct_student.c
 * Author: [Your Name]
 * Student ID: [Your ID]
 * Description:
 *   Demonstrates defining and using a struct in C.
 *   Define a 'Student' struct with name, id and grade, create two
 *   instances with the values from the instructions, and print them.
 *
 *   This program reads no input. Output must match the format in the
 *   Week 4 instructions exactly (it is checked by the autograder).
 */

#include <stdio.h>
#include <string.h>

/* Keep each student's related fields together in one record. */
struct Student {
  char name[50];
  int id;
  float grade;
};

int main(void) {
  struct Student first_student;
  struct Student second_student;

  strcpy(first_student.name, "Alice Johnson");
  first_student.id = 1001;
  first_student.grade = 9.1f;

  strcpy(second_student.name, "Bob Smith");
  second_student.id = 1002;
  second_student.grade = 8.7f;

  printf("Student 1: %s, ID: %d, Grade: %.1f\n", first_student.name,
         first_student.id, first_student.grade);
  printf("Student 2: %s, ID: %d, Grade: %.1f\n", second_student.name,
         second_student.id, second_student.grade);

  return 0;
}

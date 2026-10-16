// week5_task4_struct_binary_io.c   (BONUS)
// Name: ________  Surname: ________  Student ID: ________
//
// Task 4: Save and load a struct in binary format with fwrite()/fread()
// Week 5 – Files & Modular Programming
//
// Build: make lab5   Run: ./bin/week5_task4_struct_binary_io

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NAME_LEN 50

typedef struct Student {
  char name[MAX_NAME_LEN];
  int age;
  float gpa;
} Student;

// Function prototypes (do not change these signatures – the autograder calls
// them directly)
int save_student_binary(const Student* s,
                        const char* filename);  // 0 on success, -1 on error
int load_student_binary(const char* filename,
                        Student* out);  // 0 on success, -1 on error
long file_size(const char* filename);   // bytes, -1 on error

int main(void) {
  Student s1 = {
      0};  // zero-initialised so unused name bytes are not garbage on disk
  strcpy(s1.name, "Alice");
  s1.age = 21;
  s1.gpa = 3.75f;

  Student loaded = {0};
  const char* bin_file = "student.bin";
  const char* txt_file = "student.txt";

  printf("Saving student in binary format...\n");
  if (save_student_binary(&s1, bin_file) != 0) {
    perror(bin_file);
    return EXIT_FAILURE;
  }

  FILE* text_fp = fopen(txt_file, "w");
  if (text_fp == NULL) {
    perror(txt_file);
    return EXIT_FAILURE;
  }
  int text_write_result =
      fprintf(text_fp, "%s %d %.2f\n", s1.name, s1.age, s1.gpa);
  int text_close_result = fclose(text_fp);
  if (text_write_result < 0 || text_close_result != 0) {
    perror(txt_file);
    return EXIT_FAILURE;
  }

  printf("Loading student from binary file...\n");
  if (load_student_binary(bin_file, &loaded) != 0) {
    perror(bin_file);
    return EXIT_FAILURE;
  }

  printf("Loaded student: %s, %d, GPA %.2f\n", loaded.name, loaded.age,
         loaded.gpa);
  long text_size = file_size(txt_file);
  long binary_size = file_size(bin_file);
  if (text_size < 0 || binary_size < 0) {
    perror("file size");
    return EXIT_FAILURE;
  }
  printf("Text file size: %ld bytes\n", text_size);
  printf("Binary file size: %ld bytes\n", binary_size);

  return EXIT_SUCCESS;
}

int save_student_binary(const Student* s, const char* filename) {
  if (s == NULL || filename == NULL) {
    return -1;
  }

  FILE* fp = fopen(filename, "wb");
  if (fp == NULL) {
    return -1;
  }

  int result = fwrite(s, sizeof(Student), 1, fp) == 1 ? 0 : -1;
  if (fclose(fp) != 0) {
    result = -1;
  }
  return result;
}

int load_student_binary(const char* filename, Student* out) {
  if (filename == NULL || out == NULL) {
    return -1;
  }

  FILE* fp = fopen(filename, "rb");
  if (fp == NULL) {
    return -1;
  }

  int result = fread(out, sizeof(Student), 1, fp) == 1 ? 0 : -1;
  if (fclose(fp) != 0) {
    result = -1;
  }
  return result;
}

long file_size(const char* filename) {
  if (filename == NULL) {
    return -1;
  }

  FILE* fp = fopen(filename, "rb");
  if (fp == NULL) {
    return -1;
  }

  long size = -1;
  if (fseek(fp, 0, SEEK_END) == 0) {
    size = ftell(fp);
  }
  if (fclose(fp) != 0) {
    size = -1;
  }
  return size;
}

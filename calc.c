// Rithwan Ahmed Jabarullah Khan 241ADB143
// Compile with: gcc -O2 -Wall -Wextra -std=c17 -o calc calc.c -lm
// Resources used: AI assistant using Copilot SDK in VS Code; prompt: "Implement the supplied graded arithmetic parser assignment"; conversation history URL was not provided.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>
#include <limits.h>
#include <dirent.h>
#include <sys/stat.h>

static const char STUDENT_NAME[] = "Rithwan Ahmed";
static const char STUDENT_LASTNAME[] = "Jabarullah Khan";
static const char STUDENT_ID[] = "241ADB143";

typedef struct {
    int is_float;
    long long integer;
    double floating;
} Value;

typedef struct {
    char *text;
    size_t length;
    size_t cursor;
    size_t line_end;
    size_t error_position;
} Parser;

typedef struct {
    char **items;
    size_t count;
} FileList;

static int is_space(char c)
{
    return c == ' ' || c == '\t' || c == '\r';
}

static void skip_space(Parser *parser)
{
    while (parser->cursor < parser->line_end &&
           is_space(parser->text[parser->cursor])) {
        parser->cursor++;
    }
}

static void set_error(Parser *parser, size_t position)
{
    // cursor and line_end are offsets in the original file buffer, so adding
    // one reports the 1-based position without losing comment/newline offsets.
    if (parser->error_position == 0) {
        parser->error_position = position;
    }
}

static Value integer_value(long long number)
{
    Value value;
    value.is_float = 0;
    value.integer = number;
    value.floating = 0.0;
    return value;
}

static Value float_value(double number)
{
    Value value;
    value.is_float = 1;
    value.integer = 0;
    value.floating = number;
    return value;
}

static double as_double(Value value)
{
    return value.is_float ? value.floating : (double)value.integer;
}

static int checked_add(long long left, long long right, long long *result)
{
    if ((right > 0 && left > LLONG_MAX - right) ||
        (right < 0 && left < LLONG_MIN - right)) {
        return 0;
    }
    *result = left + right;
    return 1;
}

static int checked_subtract(long long left, long long right,
                            long long *result)
{
    if ((right < 0 && left > LLONG_MAX + right) ||
        (right > 0 && left < LLONG_MIN + right)) {
        return 0;
    }
    *result = left - right;
    return 1;
}

static int checked_multiply(long long left, long long right,
                            long long *result)
{
    if (left == 0 || right == 0) {
        *result = 0;
        return 1;
    }
    if ((left == -1 && right == LLONG_MIN) ||
        (right == -1 && left == LLONG_MIN)) {
        return 0;
    }
    if (left > 0) {
        if ((right > 0 && left > LLONG_MAX / right) ||
            (right < 0 && right < LLONG_MIN / left)) {
            return 0;
        }
    } else {
        if ((right > 0 && left < LLONG_MIN / right) ||
            (right < 0 && left < LLONG_MAX / right)) {
            return 0;
        }
    }
    *result = left * right;
    return 1;
}

static int parse_expression(Parser *parser, Value *result);

// Parsing one digit-delimited literal directly from the shared input buffer
// avoids allocating/copying a substring for every token in large files.
static int parse_number(Parser *parser, Value *result)
{
    size_t start;
    size_t end;
    size_t cursor;
    int floating = 0;
    char saved;

    skip_space(parser);
    start = parser->cursor;
    cursor = start;

    while (cursor < parser->line_end &&
           isdigit((unsigned char)parser->text[cursor])) {
        cursor++;
    }
    if (cursor < parser->line_end && parser->text[cursor] == '.') {
        floating = 1;
        cursor++;
        if (cursor == parser->line_end ||
            !isdigit((unsigned char)parser->text[cursor])) {
            set_error(parser, cursor);
            return 0;
        }
        while (cursor < parser->line_end &&
               isdigit((unsigned char)parser->text[cursor])) {
            cursor++;
        }
    }

    end = cursor;
    saved = parser->text[end];
    parser->text[end] = '\0';
    errno = 0;
    if (floating) {
        double number = strtod(parser->text + start, NULL);
        parser->text[end] = saved;
        if (!isfinite(number)) {
            set_error(parser, start + 1);
            return 0;
        }
        *result = float_value(number);
    } else {
        long long number = strtoll(parser->text + start, NULL, 10);
        parser->text[end] = saved;
        if (errno == ERANGE) {
            set_error(parser, start + 1);
            return 0;
        }
        *result = integer_value(number);
    }
    parser->cursor = end;
    return 1;
}

// AI (Copilot SDK in VS Code), prompt: "Implement the supplied graded
// arithmetic parser assignment"; conversation history URL unavailable.
// Recursive descent mirrors the grammar: expression handles +/-, term handles
// */ and power handles **. The power operands are collected first and folded
// from right to left, so even long exponent chains remain right-associative.
static int parse_primary(Parser *parser, Value *result)
{
    size_t start;

    skip_space(parser);
    start = parser->cursor;
    if (start >= parser->line_end) {
        // The missing token is at the line break, or one past EOF on the
        // final unterminated line, rather than at the preceding operator.
        set_error(parser, parser->line_end + 1);
        return 0;
    }

    if (isdigit((unsigned char)parser->text[start])) {
        return parse_number(parser, result);
    }
    if (parser->text[start] == '(') {
        parser->cursor++;
        if (!parse_expression(parser, result)) {
            return 0;
        }
        skip_space(parser);
        if (parser->cursor >= parser->line_end) {
            // A missing closing parenthesis has the same line-end position.
            set_error(parser, parser->line_end + 1);
            return 0;
        }
        if (parser->text[parser->cursor] != ')') {
            set_error(parser, parser->cursor + 1);
            return 0;
        }
        parser->cursor++;
        return 1;
    }

    set_error(parser, start + 1);
    return 0;
}

static int integer_power(long long base, long long exponent,
                         long long *result)
{
    long long product = 1;

    while (exponent > 0) {
        if ((exponent & 1LL) != 0 &&
            !checked_multiply(product, base, &product)) {
            return 0;
        }
        exponent >>= 1;
        if (exponent > 0 && !checked_multiply(base, base, &base)) {
            return 0;
        }
    }
    *result = product;
    return 1;
}

typedef struct {
    Value value;
    size_t start;
    size_t operator_position;
} PowerOperand;

static int append_power_operand(PowerOperand **operands, size_t *count,
                               size_t *capacity, PowerOperand operand)
{
    PowerOperand *grown;
    size_t new_capacity;

    if (*count == *capacity) {
        new_capacity = *capacity == 0 ? 8 : *capacity * 2;
        if (new_capacity < *capacity ||
            new_capacity > (size_t)-1 / sizeof(**operands)) {
            return 0;
        }
        grown = realloc(*operands, new_capacity * sizeof(**operands));
        if (grown == NULL) {
            return 0;
        }
        *operands = grown;
        *capacity = new_capacity;
    }
    (*operands)[(*count)++] = operand;
    return 1;
}

static int parse_power(Parser *parser, Value *result)
{
    PowerOperand *operands = NULL;
    size_t count = 0;
    size_t capacity = 0;
    Value current;
    size_t exponent_start;
    int success = 0;

    if (!parse_primary(parser, &current)) {
        return 0;
    }
    if (!append_power_operand(&operands, &count, &capacity,
                              (PowerOperand){current, 0, 0})) {
        set_error(parser, parser->cursor + 1);
        goto done;
    }

    for (;;) {
        size_t operator_position;
        PowerOperand operand;

        skip_space(parser);
        if (parser->cursor + 1 >= parser->line_end ||
            parser->text[parser->cursor] != '*' ||
            parser->text[parser->cursor + 1] != '*') {
            break;
        }
        operator_position = parser->cursor;
        parser->cursor += 2;
        skip_space(parser);
        exponent_start = parser->cursor;
        if (!parse_primary(parser, &current)) {
            goto done;
        }
        operand.value = current;
        operand.start = exponent_start + 1;
        operand.operator_position = operator_position + 1;
        if (!append_power_operand(&operands, &count, &capacity, operand)) {
            set_error(parser, operator_position + 1);
            goto done;
        }
    }

    current = operands[count - 1].value;
    while (count > 1) {
        Value base = operands[count - 2].value;
        size_t exponent_position = operands[count - 1].start;
        size_t operator_position =
            operands[count - 1].operator_position;

        if (!base.is_float && !current.is_float) {
            long long integer_result;
            if (current.integer < 0) {
                set_error(parser, exponent_position);
                goto done;
            }
            if (!integer_power(base.integer, current.integer,
                              &integer_result)) {
                set_error(parser, exponent_position);
                goto done;
            }
            current = integer_value(integer_result);
        } else {
            double floating_result = pow(as_double(base), as_double(current));
            if (!isfinite(floating_result)) {
                set_error(parser, operator_position);
                goto done;
            }
            current = float_value(floating_result);
        }
        count--;
    }

    *result = current;
    success = 1;

done:
    free(operands);
    return success;
}

static int parse_term(Parser *parser, Value *result)
{
    Value left;

    if (!parse_power(parser, &left)) {
        return 0;
    }
    for (;;) {
        char operation;
        size_t operator_position;
        size_t right_start;
        Value right;

        skip_space(parser);
        if (parser->cursor >= parser->line_end ||
            (parser->text[parser->cursor] != '*' &&
             parser->text[parser->cursor] != '/')) {
            break;
        }
        if (parser->text[parser->cursor] == '*' &&
            parser->cursor + 1 < parser->line_end &&
            parser->text[parser->cursor + 1] == '*') {
            break;
        }
        operator_position = parser->cursor;
        operation = parser->text[parser->cursor++];
        skip_space(parser);
        right_start = parser->cursor;
        if (!parse_power(parser, &right)) {
            return 0;
        }

        if (operation == '/' && as_double(right) == 0.0) {
            // Division errors point at the first token of the divisor, even
            // when parsing that operand involved spaces or parentheses.
            set_error(parser, right_start + 1);
            return 0;
        }
        // Keep integer operations exact unless a float literal has entered
        // either side; mixed operations then use double arithmetic.
        if (!left.is_float && !right.is_float && operation == '*') {
            long long value;
            if (!checked_multiply(left.integer, right.integer, &value)) {
                set_error(parser, operator_position + 1);
                return 0;
            }
            left = integer_value(value);
        } else if (!left.is_float && !right.is_float && operation == '/') {
            if (left.integer == LLONG_MIN && right.integer == -1) {
                set_error(parser, operator_position + 1);
                return 0;
            }
            left = integer_value(left.integer / right.integer);
        } else {
            double value = operation == '*' ?
                as_double(left) * as_double(right) :
                as_double(left) / as_double(right);
            if (!isfinite(value)) {
                set_error(parser, operator_position + 1);
                return 0;
            }
            left = float_value(value);
        }
    }
    *result = left;
    return 1;
}

static int parse_expression(Parser *parser, Value *result)
{
    Value left;

    if (!parse_term(parser, &left)) {
        return 0;
    }
    for (;;) {
        char operation;
        size_t operator_position;
        Value right;

        skip_space(parser);
        if (parser->cursor >= parser->line_end ||
            (parser->text[parser->cursor] != '+' &&
             parser->text[parser->cursor] != '-')) {
            break;
        }
        operator_position = parser->cursor;
        operation = parser->text[parser->cursor++];
        if (!parse_term(parser, &right)) {
            return 0;
        }

        if (!left.is_float && !right.is_float) {
            long long value;
            int ok = operation == '+' ?
                checked_add(left.integer, right.integer, &value) :
                checked_subtract(left.integer, right.integer, &value);
            if (!ok) {
                set_error(parser, operator_position + 1);
                return 0;
            }
            left = integer_value(value);
        } else {
            double value = operation == '+' ?
                as_double(left) + as_double(right) :
                as_double(left) - as_double(right);
            if (!isfinite(value)) {
                set_error(parser, operator_position + 1);
                return 0;
            }
            left = float_value(value);
        }
    }
    *result = left;
    return 1;
}

static int evaluate_file(const char *input_path, FILE *results,
                         size_t *error_position)
{
    FILE *input;
    char *text = NULL;
    size_t length = 0;
    size_t capacity = 0;
    int ok = 0;

    input = fopen(input_path, "rb");
    if (input == NULL) {
        fprintf(stderr, "calc: cannot open input file '%s'\n", input_path);
        return 0;
    }

    for (;;) {
        size_t available;
        size_t amount;
        char *grown;

        if (capacity - length < 4096) {
            size_t new_capacity = capacity == 0 ? 8192 : capacity * 2;
            if (new_capacity < capacity ||
                new_capacity > (size_t)-1 - 1) {
                fprintf(stderr, "calc: input file is too large: '%s'\n",
                        input_path);
                goto cleanup;
            }
            grown = realloc(text, new_capacity + 1);
            if (grown == NULL) {
                fprintf(stderr, "calc: out of memory reading '%s'\n",
                        input_path);
                goto cleanup;
            }
            text = grown;
            capacity = new_capacity;
        }
        available = capacity - length;
        amount = fread(text + length, 1, available, input);
        length += amount;
        if (amount < available) {
            if (ferror(input)) {
                fprintf(stderr, "calc: cannot read input file '%s'\n",
                        input_path);
                goto cleanup;
            }
            if (feof(input)) {
                break;
            }
        }
    }
    text[length] = '\0';

    {
        size_t line_start = 0;
        FILE *staged = tmpfile();

        if (staged == NULL) {
            fprintf(stderr, "calc: cannot create temporary result storage\n");
            goto cleanup;
        }
        while (line_start < length) {
            size_t line_end = line_start;
            size_t first;
            Parser parser;
            Value value;
            int has_newline;

            while (line_end < length && text[line_end] != '\n') {
                line_end++;
            }
            has_newline = line_end < length;
            first = line_start;
            while (first < line_end && is_space(text[first])) {
                first++;
            }
            if (first < line_end && text[first] != '#') {
                parser.text = text;
                parser.length = length;
                parser.cursor = line_start;
                parser.line_end = line_end;
                parser.error_position = 0;

                if (!parse_expression(&parser, &value)) {
                    *error_position = parser.error_position;
                    fclose(staged);
                    ok = 1;
                    goto cleanup;
                }
                skip_space(&parser);
                if (parser.cursor < line_end) {
                    set_error(&parser, parser.cursor + 1);
                    *error_position = parser.error_position;
                    fclose(staged);
                    ok = 1;
                    goto cleanup;
                }
                if (value.is_float) {
                    if (fprintf(staged, "%.15g\n", value.floating) < 0) {
                        fprintf(stderr, "calc: cannot stage results\n");
                        fclose(staged);
                        goto cleanup;
                    }
                } else if (fprintf(staged, "%lld\n", value.integer) < 0) {
                    fprintf(stderr, "calc: cannot stage results\n");
                    fclose(staged);
                    goto cleanup;
                }
            }
            line_start = line_end + (has_newline ? 1 : 0);
        }
        if (fflush(staged) != 0 || fseek(staged, 0, SEEK_SET) != 0) {
            fprintf(stderr, "calc: cannot read staged results\n");
            fclose(staged);
            goto cleanup;
        }
        while (!feof(staged)) {
            char buffer[8192];
            size_t amount = fread(buffer, 1, sizeof(buffer), staged);
            if (amount > 0 && fwrite(buffer, 1, amount, results) != amount) {
                fprintf(stderr, "calc: cannot write results\n");
                fclose(staged);
                goto cleanup;
            }
            if (ferror(staged)) {
                fprintf(stderr, "calc: cannot read staged results\n");
                fclose(staged);
                goto cleanup;
            }
        }
        fclose(staged);
        *error_position = 0;
        ok = 1;
    }

cleanup:
    if (fclose(input) != 0 && ok) {
        fprintf(stderr, "calc: cannot close input file '%s'\n", input_path);
        ok = 0;
    }
    free(text);
    return ok;
}

static char *duplicate_string(const char *source)
{
    size_t length = strlen(source);
    char *copy = malloc(length + 1);

    if (copy != NULL) {
        memcpy(copy, source, length + 1);
    }
    return copy;
}

static char *join_path(const char *directory, const char *name)
{
    size_t directory_length = strlen(directory);
    size_t name_length = strlen(name);
    int separator = directory_length > 0 &&
        directory[directory_length - 1] != '/';
    char *path;

    if (directory_length > (size_t)-1 - name_length - (size_t)separator - 1) {
        return NULL;
    }
    path = malloc(directory_length + (size_t)separator + name_length + 1);
    if (path == NULL) {
        return NULL;
    }
    memcpy(path, directory, directory_length);
    if (separator) {
        path[directory_length] = '/';
    }
    memcpy(path + directory_length + (size_t)separator, name,
           name_length + 1);
    return path;
}

static char *path_basename(const char *path)
{
    size_t length = strlen(path);
    size_t end = length;
    size_t start;

    while (end > 1 && path[end - 1] == '/') {
        end--;
    }
    start = end;
    while (start > 0 && path[start - 1] != '/') {
        start--;
    }
    if (end == start) {
        return duplicate_string("root");
    }
    {
        size_t size = end - start;
        char *base = malloc(size + 1);
        if (base != NULL) {
            memcpy(base, path + start, size);
            base[size] = '\0';
        }
        return base;
    }
}

static char *input_base(const char *filename)
{
    char *base = path_basename(filename);
    size_t length;

    if (base == NULL) {
        return NULL;
    }
    length = strlen(base);
    if (length >= 4 && strcmp(base + length - 4, ".txt") == 0) {
        base[length - 4] = '\0';
    }
    return base;
}

static char *slug(const char *text)
{
    size_t length = strlen(text);
    char *result = malloc(length * 2 + 1);
    size_t used = 0;
    int pending_separator = 0;

    if (result == NULL) {
        return NULL;
    }
    for (size_t i = 0; i < length; i++) {
        unsigned char character = (unsigned char)text[i];
        if (isalnum(character)) {
            if (pending_separator && used > 0) {
                result[used++] = '_';
            }
            result[used++] = (char)tolower(character);
            pending_separator = 0;
        } else {
            pending_separator = 1;
        }
    }
    result[used] = '\0';
    return result;
}

static char *make_output_name(const char *input_name)
{
    char *base = input_base(input_name);
    char *name = slug(STUDENT_NAME);
    char *lastname = slug(STUDENT_LASTNAME);
    size_t size;
    char *result;

    if (base == NULL || name == NULL || lastname == NULL) {
        free(base);
        free(name);
        free(lastname);
        return NULL;
    }
    size = strlen(base) + strlen(name) + strlen(lastname) +
        strlen(STUDENT_ID) + 8;
    result = malloc(size);
    if (result != NULL) {
        snprintf(result, size, "%s_%s_%s_%s.txt", base, name, lastname,
                 STUDENT_ID);
    }
    free(base);
    free(name);
    free(lastname);
    return result;
}

static int ensure_output_directory(const char *directory)
{
    struct stat status;

    if (stat(directory, &status) == 0) {
        if (S_ISDIR(status.st_mode)) {
            return 1;
        }
        fprintf(stderr, "calc: output path is not a directory: '%s'\n",
                directory);
        return 0;
    }
    if (mkdir(directory, 0777) != 0) {
        fprintf(stderr, "calc: cannot create output directory '%s'\n",
                directory);
        return 0;
    }
    return 1;
}

static int compare_names(const void *left, const void *right)
{
    const char *const *a = left;
    const char *const *b = right;
    return strcmp(*a, *b);
}

static void free_file_list(FileList *files)
{
    for (size_t i = 0; i < files->count; i++) {
        free(files->items[i]);
    }
    free(files->items);
    files->items = NULL;
    files->count = 0;
}

static int collect_text_files(const char *directory, FileList *files)
{
    DIR *handle = opendir(directory);
    struct dirent *entry;
    size_t capacity = 0;

    files->items = NULL;
    files->count = 0;
    if (handle == NULL) {
        fprintf(stderr, "calc: cannot open input directory '%s'\n",
                directory);
        return 0;
    }
    while ((entry = readdir(handle)) != NULL) {
        size_t name_length = strlen(entry->d_name);
        char *path;
        struct stat status;

        if (name_length < 4 ||
            strcmp(entry->d_name + name_length - 4, ".txt") != 0) {
            continue;
        }
        path = join_path(directory, entry->d_name);
        if (path == NULL) {
            fprintf(stderr, "calc: out of memory listing input directory\n");
            closedir(handle);
            free_file_list(files);
            return 0;
        }
        if (stat(path, &status) != 0 || !S_ISREG(status.st_mode)) {
            free(path);
            continue;
        }
        free(path);
        if (files->count == capacity) {
            size_t new_capacity = capacity == 0 ? 8 : capacity * 2;
            char **grown;
            if (new_capacity < capacity ||
                new_capacity > (size_t)-1 / sizeof(*files->items)) {
                fprintf(stderr, "calc: too many input files\n");
                closedir(handle);
                free_file_list(files);
                return 0;
            }
            grown = realloc(files->items,
                            new_capacity * sizeof(*files->items));
            if (grown == NULL) {
                fprintf(stderr, "calc: out of memory listing input directory\n");
                closedir(handle);
                free_file_list(files);
                return 0;
            }
            files->items = grown;
            capacity = new_capacity;
        }
        files->items[files->count] = duplicate_string(entry->d_name);
        if (files->items[files->count] == NULL) {
            fprintf(stderr, "calc: out of memory listing input directory\n");
            closedir(handle);
            free_file_list(files);
            return 0;
        }
        files->count++;
    }
    if (closedir(handle) != 0) {
        fprintf(stderr, "calc: cannot close input directory '%s'\n",
                directory);
        free_file_list(files);
        return 0;
    }
    if (files->count > 1) {
        qsort(files->items, files->count, sizeof(*files->items),
              compare_names);
    }
    return 1;
}

static int process_one(const char *input_path, const char *input_name,
                       const char *output_directory)
{
    char *output_name = make_output_name(input_name);
    char *output_path;
    FILE *output;
    size_t error_position = 0;
    int status;

    if (output_name == NULL) {
        fprintf(stderr, "calc: out of memory creating output filename\n");
        return 0;
    }
    output_path = join_path(output_directory, output_name);
    free(output_name);
    if (output_path == NULL) {
        fprintf(stderr, "calc: out of memory creating output path\n");
        return 0;
    }
    output = fopen(output_path, "wb");
    if (output == NULL) {
        fprintf(stderr, "calc: cannot write output file '%s'\n", output_path);
        free(output_path);
        return 0;
    }
    status = evaluate_file(input_path, output, &error_position);
    if (status && error_position != 0 &&
        fprintf(output, "ERROR:%zu\n", error_position) < 0) {
        fprintf(stderr, "calc: cannot write output file '%s'\n", output_path);
        status = 0;
    }
    if (fclose(output) != 0) {
        fprintf(stderr, "calc: cannot close output file '%s'\n", output_path);
        status = 0;
    }
    free(output_path);
    return status;
}

static void usage(void)
{
    fprintf(stderr,
            "Usage: calc [-d DIR | --dir DIR] [-o OUTDIR | --output-dir OUTDIR] [input.txt]\n");
}

int main(int argc, char **argv)
{
    const char *input_file = NULL;
    const char *input_directory = NULL;
    const char *output_directory = NULL;
    char *default_output = NULL;
    int directory_mode = 0;
    int index;
    int result = 1;

    for (index = 1; index < argc; index++) {
        if (strcmp(argv[index], "-d") == 0 ||
            strcmp(argv[index], "--dir") == 0) {
            if (directory_mode || input_file != NULL ||
                index + 1 >= argc) {
                usage();
                return 2;
            }
            directory_mode = 1;
            input_directory = argv[++index];
        } else if (strcmp(argv[index], "-o") == 0 ||
                   strcmp(argv[index], "--output-dir") == 0) {
            if (output_directory != NULL || index + 1 >= argc) {
                usage();
                return 2;
            }
            output_directory = argv[++index];
        } else if (argv[index][0] == '-') {
            usage();
            return 2;
        } else if (input_file == NULL && !directory_mode) {
            input_file = argv[index];
        } else {
            usage();
            return 2;
        }
    }

    if ((directory_mode && input_file != NULL) ||
        (!directory_mode && input_file == NULL)) {
        usage();
        return 2;
    }

    if (output_directory == NULL) {
        char *base = input_base(directory_mode ? input_directory : input_file);
        char *name = slug(STUDENT_NAME);
        size_t size;

        if (base == NULL || name == NULL) {
            fprintf(stderr, "calc: out of memory creating output directory name\n");
            free(base);
            free(name);
            return 1;
        }
        size = strlen(base) + strlen(name) + strlen(STUDENT_ID) + 3;
        default_output = malloc(size);
        if (default_output == NULL) {
            fprintf(stderr, "calc: out of memory creating output directory name\n");
            free(base);
            free(name);
            return 1;
        }
        snprintf(default_output, size, "%s_%s_%s", base, name, STUDENT_ID);
        free(base);
        free(name);
        output_directory = default_output;
    }
    if (!ensure_output_directory(output_directory)) {
        goto done;
    }

    if (directory_mode) {
        FileList files;
        if (!collect_text_files(input_directory, &files)) {
            goto done;
        }
        result = 0;
        for (size_t i = 0; i < files.count; i++) {
            char *path = join_path(input_directory, files.items[i]);
            if (path == NULL) {
                fprintf(stderr, "calc: out of memory creating input path\n");
                result = 1;
                break;
            }
            if (!process_one(path, files.items[i], output_directory)) {
                result = 1;
                free(path);
                break;
            }
            free(path);
        }
        free_file_list(&files);
    } else {
        result = process_one(input_file, input_file, output_directory) ? 0 : 1;
    }

done:
    free(default_output);
    return result;
}

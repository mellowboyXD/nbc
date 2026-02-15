#include <assert.h>
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Disclaimer: The notion of creating dynamic arrays, string builders, and 
 * 		string views like this is taken from Alexey Kutepov 
 * 		youtube.com/@TsodingDaily. */

#define INIT_CAP 256 /* initial capacity of a dynamic array */
#define da_append(da, item) /* adds item into a dynamic array da */            \
	do {                                                                   \
		if ((da)->cap == 0) {                                          \
			(da)->cap = INIT_CAP;                                  \
			(da)->items = malloc(INIT_CAP * sizeof(*(da)->items)); \
		}                                                              \
		if ((da)->size == (da)->cap) {                                 \
			(da)->cap *= 2;                                        \
			(da)->items =                                          \
				realloc((da)->items,                           \
					(da)->cap * sizeof(*(da)->items));     \
		}                                                              \
		assert((da)->items != NULL);                                   \
		(da)->items[(da)->size++] = (item);                            \
	} while (0)

#define da_free(da) /* frees the dynamic array */ \
	do {                                      \
		(da)->cap = 0;                    \
		(da)->size = 0;                   \
		free((da)->items);                \
	} while (0)

typedef struct {
	size_t cap;
	size_t size;
	char *items;
} string_builder; /* size: 32 bytes */

typedef struct {
	size_t size;
	const char *cstr;
} string_view; /* size: 16 bytes */

typedef struct {
	int count;
	string_view word;
} word_freq; /* size: 20 bytes */

typedef struct {
	size_t cap;
	size_t size;
	word_freq *items;
} word_freqs; /* size: 32 bytes */

/* read_entire_file: reads an entire file into a string builder */
bool read_entire_file(const char *filepath, string_builder *sb)
{
	FILE *fp = fopen(filepath, "r");
	if (!fp) {
		fprintf(stderr, "error: could not open file '%s'\n", filepath);
		return false;
	}

	int c;
	while ((c = getc(fp)) != EOF) {
		da_append(sb, c);
	}

	fclose(fp);
	return true;
}

/* trim_left: removes whitespaces from a string view starting from the left */
string_view trim_left(string_view sv)
{
	size_t i = 0;
	while (i < sv.size && isspace(sv.cstr[i]))
		i++;

	string_view result = { .size = sv.size - i, .cstr = sv.cstr + i };

	return result;
}

/* chop_by_space: reduces a string view by chopping specific portions */
string_view chop_by_space(string_view *sv)
{
	size_t i = 0;
	while (i < sv->size && !isspace(sv->cstr[i]))
		i++;

	string_view result = { .size = i, .cstr = sv->cstr };

	if (i < sv->size) {
		sv->size -= i + 1;
		sv->cstr += i + 1;
	} else {
		sv->size -= i;
		sv->cstr += i;
	}

	return result;
}

/* find_key: returns a pointer to the word_freq (key-value pair) from the 
 * 		dynamic array word_freqs */
word_freq *find_key(word_freqs *freqs, string_view key)
{
	for (size_t i = 0; i < freqs->size; i++) {
		if (strncmp(freqs->items[i].word.cstr, key.cstr, key.size) == 0)
			return &freqs->items[i];
	}
	return NULL;
}

char *shift_argv(char **argv, int *argc)
{
	static int i = 0;
	if (*argc < i)
		return NULL;

	(*argc)--;
	char *ret = argv[i++];
	return ret;
}

int main(int argc, char **argv)
{
	const char *program_name = shift_argv(argv, &argc);
	if (argc < 1) {
		fprintf(stderr, "Usage: %s <input-file>.txt\n", program_name);
		return 1;
	}

	const char *filepath = shift_argv(argv, &argc);
	string_builder buf = { 0 };
	if (!read_entire_file(filepath, &buf))
		return 1;
	string_view content = { .size = buf.size, .cstr = buf.items };
	unsigned total_count = 0;

	word_freqs freqs = { 0 };

	for (size_t i = 0; i < content.size; i++) {
		content = trim_left(content);
		string_view token = chop_by_space(&content);
		//printf("|%.*s|\n", (int)token.count, token.data);
		word_freq *freq = find_key(&freqs, token);
		if (freq) {
			freq->count += 1;
		} else {
			word_freq f = { .count = 1, .word = token };
			da_append(&freqs, f);
		}
		total_count++;
	}

	for (size_t i = 0; i < freqs.size; i++) {
		word_freq freq = freqs.items[i];
		int n = (int)freq.word.size;
		float p = (float)freq.count / total_count;
		printf("|%.*s| => %d (%.5f)\n", n, freq.word.cstr, freq.count,
		       p);
	}

	da_free(&buf);
	da_free(&freqs);
	return 0;
}

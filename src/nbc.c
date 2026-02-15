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
		if ((da)->count == (da)->cap) {                                \
			(da)->cap *= 2;                                        \
			(da)->items =                                          \
				realloc((da)->items,                           \
					(da)->cap * sizeof(*(da)->items));     \
		}                                                              \
		assert((da)->items != NULL);                                   \
		(da)->items[(da)->count++] = (item);                           \
	} while (0)

#define da_free(da) /* frees the dynamic array */ \
	do {                                      \
		(da)->cap = 0;                    \
		(da)->count = 0;                  \
		free((da)->items);                \
	} while (0)

typedef struct {
	size_t cap;
	size_t count;
	char *items;
} string_builder; /* size: 32 bytes */

typedef struct {
	size_t count;
	const char *cstr;
} string_view; /* size: 16 bytes */

typedef struct {
	int count;
	string_view word;
} word_freq; /* size: 20 bytes */

typedef struct {
	size_t count;
	size_t cap;
	word_freq *items;
} word_freqs; /* size: 32 bytes */

/* read_entire_file: reads an entire file into a string builder */
bool read_entire_file(const char *filepath, string_builder *sb)
{
	FILE *fp = fopen(filepath, "r");
	if (!fp) {
		fprintf(stderr, "error: could not open file %s\n", filepath);
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
	while (i < sv.count && isspace(sv.cstr[i]))
		i++;

	string_view result = { .count = sv.count - i, .cstr = sv.cstr + i };

	return result;
}

/* chop_by_space: reduces a string view by chopping specific portions */
string_view chop_by_space(string_view *sv)
{
	size_t i = 0;
	while (i < sv->count && !isspace(sv->cstr[i]))
		i++;

	string_view result = { .count = i, .cstr = sv->cstr };

	if (i < sv->count) {
		sv->count -= i + 1;
		sv->cstr += i + 1;
	} else {
		sv->count -= i;
		sv->cstr += i;
	}

	return result;
}

word_freq *find_key(word_freqs *frqs, string_view key)
{
	for (size_t i = 0; i < frqs->count; i++) {
		if (strncmp(frqs->items[i].word.cstr, key.cstr, key.count) == 0)
			return &frqs->items[i];
	}
	return NULL;
}

int main()
{
	string_builder buf = { 0 };
	if (!read_entire_file("data/enron1/Summary.txt", &buf))
		return 1;
	string_view content = { .count = buf.count, .cstr = buf.items };

	word_freqs freqs = { 0 };

	for (size_t i = 0; i < content.count; i++) {
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
	}

	for (size_t i = 0; i < freqs.count; i++) {
		word_freq freq = freqs.items[i];
		int n = (int) freq.word.count;
		printf("|%.*s| => %d\n", n,freq.word.cstr, freq.count);
	}

	da_free(&buf);
	da_free(&freqs);
	return 0;
}

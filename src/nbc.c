#define _DEFAULT_SOURCE
#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* Disclaimer: The notion of creating dynamic arrays, string builders, and 
 * 		string views like this is taken from Alexey Kutepov 
 * 		youtube.com/@TsodingDaily. */

/* some common words like 'the' 'a' or 'some' appear several times in the 
 * english language. Therefore is sensible to just ignore those words entirely. 
 * only words that have a count less that this threshold will be considered. */
#define THRESHOLD 150
#define HAM_DS_PATH "data/train/enron1/ham"
#define SPAM_DS_PATH "data/train/enron1/spam"

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

#define da_reset(da) /* resets the dynamic array */ \
	do {                                        \
		(da)->cap = 0;                      \
		(da)->size = 0;                     \
		(da)->items = "";                   \
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
} bow_t; /* size: 32 bytes */

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
 * 	     dynamic array word_freqs */
word_freq *find_key(bow_t *freqs, string_view key)
{
	for (size_t i = 0; i < freqs->size; i++) {
		word_freq curword = freqs->items[i];
		if (curword.word.size != key.size)
			continue;
		if (strncmp(curword.word.cstr, key.cstr, key.size) == 0)
			return &freqs->items[i];
	}
	return NULL;
}

/* shift_argv: left shifts argv by 1 and returns a pointer to the shifted 
 * 	       string */
char *shift_argv(char **argv[], int *argc)
{
	assert(*argc > 0);
	(*argc)--;
	return *((*argv)++);
}

unsigned accumulate_words(char *filepath, bow_t *bow, string_builder *buf)
{
	printf("file: %s\n", filepath);
	da_reset(buf);
	if (!read_entire_file(filepath, buf))
		exit(EXIT_FAILURE);

	string_view content = { .size = buf->size, .cstr = buf->items };

	// since trim_left is consuming content, check if size > 0
	// using a for loop led to a segfault since trim_left removed the data
	// we were trying to access. It led to a segfault of course!
	int count = 0;
	while (content.size > 0) {
		content = trim_left(content);
		string_view token = chop_by_space(&content);
		word_freq *key = find_key(bow, token);
		if (key) {
			key->count += 1;
		} else {
			word_freq f = { .count = 1, .word = token };
			da_append(bow, f);
		}
		count++;
	}

	return count;
}

/* walk_dir: recursively opens a directory and calls callback with current path 
 * 	     and p as argument. Note that the trailing '/' should be excluded when 
 * 	     calling this function for the first time */
unsigned get_word_count_in_class(const char *dirpath, bow_t *freqs,
				 string_builder *buf, unsigned *doc_count)
{
	DIR *dir = opendir(dirpath);
	if (!dir) {
		fprintf(stderr, "error: could not open directory '%s'\n",
			dirpath);
		exit(EXIT_FAILURE);
	}

	struct stat s;
	struct dirent *entry;

	unsigned count = 0;
	while ((entry = readdir(dir))) {
		char path[1024];
		int n = snprintf(path, sizeof(path) - 1, "%s/%s", dirpath,
				 entry->d_name);
		path[n] = '\0';
		if (lstat(path, &s) == 0 && S_ISREG(s.st_mode)) {
			count += accumulate_words(path, freqs, buf);
			(*doc_count)++;
		}
	}

	closedir(dir);
	return count;
}

int main(int argc, char **argv)
{
	const char *program_name = shift_argv(&argv, &argc);
	if (argc < 1) {
		fprintf(stderr, "usage: %s <input-file>.txt\n", program_name);
		exit(EXIT_FAILURE);
	}

	const char *filepath = shift_argv(&argv, &argc);
	string_builder content = { 0 };
	if (!read_entire_file(filepath, &content))
		exit(EXIT_FAILURE);

	string_builder buf = { 0 };
	bow_t ham_bow = { 0 };
	bow_t spam_bow = { 0 };
	unsigned ham_doc_count = 0;
	unsigned spam_doc_count = 0;
	unsigned ham_word_count = get_word_count_in_class(HAM_DS_PATH, &ham_bow,
							  &buf, &ham_doc_count);
	unsigned spam_word_count = get_word_count_in_class(
		SPAM_DS_PATH, &spam_bow, &buf, &spam_doc_count);
	int total = ham_doc_count + ham_doc_count;
	printf("ham doc count: %d\n", ham_doc_count);
	printf("spam doc count: %d\n", spam_doc_count);
	printf("total docs: %d\n", total);

	double hamP = (double)ham_doc_count / total;
	double spamP = (double)spam_doc_count / total;
	printf("hamP: %f spamP: %f\n", hamP, spamP);

	string_view sv = { .cstr = content.items, .size = content.size };
	double p_ham = 0;
	double p_spam = 0;
	while (sv.size > 0) {
		sv = trim_left(sv);
		string_view token = chop_by_space(&sv);
		word_freq *key_ham = find_key(&ham_bow, token);
		word_freq *key_spam = find_key(&spam_bow, token);

		if (key_ham && key_ham->count < THRESHOLD)
			p_ham += log((double)key_ham->count / ham_word_count);
		else
			p_ham += 1.0;

		if (key_spam && key_spam->count < THRESHOLD)
			p_spam +=
				log((double)key_spam->count / spam_word_count);
		else 
			p_spam += 1.0;
	}

	p_spam += log(spamP);
	p_ham += log(hamP);

	printf("ln P(H|D): %f ln P(S|D): %f\n", p_ham, p_spam);
	if (p_spam > p_ham)
		printf("%s is a SPAM\n", filepath);
	else
		printf("%s is NOT a spam\n", filepath);

	da_free(&content);
	da_free(&ham_bow);
	da_free(&spam_bow);
	da_free(&buf);
	return 0;
}

#ifndef DISTANCE_H
#define DISTANCE_H
#include <stdio.h>

#define  max_size 2000         // max length of strings
#define N 10                 // number of closest words that will be shown
#define max_w  50              // max length of vocabulary entries

typedef struct {
	// model data
	float *M;
 	char *vocab;
 	char *bestw[N];
    	float bestd[N];
 	long long *words, *size;
 	int *a, *b;
 	float *len, *vec;

} model_data;
/*
typedef struct {
	// model data
	float *M;
 	char *vocab;
 	char *bestw[N];
    float bestd[N];
 	long long words, size;
 	int a, b;
 	float len, vec[max_size];

} model_data;*/
char** tokenize(char*, const char);

void tokenize2(char*, const char, char**);

int countTillNonNull(char** arg);

model_data* processModel(char *file_name);

float findDistance(model_data *, char *, char *);

typedef struct {
	float bestd[N];
	char *bestw[N];
} similar_words;

typedef struct {
	float distance;
	char *word;
	
} similar_word_iota;

similar_word_iota* findSimilarWords(model_data *, char *);

char* convertToString(similar_word_iota*);
#endif


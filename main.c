#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>

#include "threadpool.h"
#include "distance.h"
#include "jansson.h"
//#include "simple_parse.h"

#define THREAD 2
#define SIZE 4
#define QUEUES 1

const char *attributeMap2 = "{\"attributeMap\": {\"purpose\": [\"honeymoon\", \"business\", \"solo\", \"friends\", \"backpacking\"]}, \"hierarchicalAttributeMap\": {\"food\": [\"indian\", \"french\", \"japanese\", \"thai\", \"italian\"], \"view\": [\"mountain\", \"downtown\", \"ocean\", \"forest\"]}}";

const char *foodMap2 = "{\"foodTypeMap\": {\"indian\" : [\"dosa\", \"butter_chicken\", \"lentil\", \"dal\", \"samosa\", \"roti\", \"naan\", \"biryani\", \"momos\", \"idli\", \"masala\"], \"french\" : [\"baguette\", \"crepes\", \"croissant\", \"macarons\", \"madeleine\", \"lamb_curry\", \"patisserie\", \"quiches\", \"wine\", \"cheese\"], \"thai\": [\"papaya_salad\", \"pad_thai\", \"green_curry\", \"curry\", \"fried_rice\",\"roast_duck\", \"beef_salad\", \"coconut_cream\", \"shrimp\", \"soup\"], \"japanese\": [\"sushi\", \"ramen\", \"unagi\", \"tempura\", \"kaiseki\", \"soba\", \"teriyaki\", \"wasabi\", \"udon\", \"noodles\"], \"italian\": [\"pasta\", \"pizza\", \"spaghetti\", \"pesto\", \"bruschetta\", \"focaccia\", \"margherita\", \"risotto\", \"pomodoro\", \"tiramisu\", \"parmigiana\", \"lasagna\", \"tomato_sauce\", \"olives\"]}}";

threadpool_t *pool[QUEUES];
int tasks[SIZE];
char *sep = "/";

typedef struct {
	json_t *attributeCloud;
	json_t *subAttributeCloud;
} attjson; 

attjson *attributeCloud;

typedef struct {
	char *text;
	char *name;
} smallSt;

void iterateLevelIArrayText(json_t* node, char* att, int depth){
	int index;
	json_t *sentiment, *wordDetails, *review;
	printf("depth %d: \n", depth);
	json_array_foreach(node, index, review){
		if (depth == 1){
			sentiment = json_object_get(review, "sentiment");
			wordDetails = json_object_get(review, "worddetails");
			printf("printing sentiment and word details: \n");
			print_json(sentiment);
			print_json(wordDetails);
		}
		else{
			depth++;
			iterateLevelIArrayText(review, att, depth);
		}
	}
}

void processText(void *arg){
	int webSize = 5, index, subindex;
	char *tokens[3];
	int numTokens;
	char *locationKey;
	json_t *hotelDetails, *reviewArr, *review, *sentence, *wordDetails;
	char *hotelKey, *sentiment;
	json_t *result = json_object();
	json_error_t error;
	smallSt *val = (smallSt *)arg;
	//printf("My name is %s and my text are: %s\n", val->name, val->text);
	usleep(1000);
	// process and write to file.
	tokenize2(val->text, '\t', tokens);
	numTokens = countTillNonNull(tokens);
	locationKey = tokens[0];
	hotelDetails = json_loads(tokens[1], 0, &error);
	json_object_foreach(hotelDetails, hotelKey, reviewArr){
		iterateLevelIArrayText(reviewArr, hotelKey, 0);
	}
}

typedef struct {
	char* listOfFiles[80];
	int count;
} ListFiles;

void iterateLevelIArray(json_t* node, model_data* modelData, json_t* subAttCloud, char* att){
	int index, tmpInt;
	similar_word_iota* similar;
	json_t *similarMap, *subAtt;
	json_t *tmpJson;
	json_error_t error;
	char *subAttVal, *strForMap;
	json_array_foreach(node, index, subAtt) {
		subAttVal = json_string_value(subAtt);
		similar = findSimilarWords(modelData, subAttVal);
		strForMap = convertToString(similar);
		similarMap = json_loads(strForMap, 0, &error);
		tmpJson = json_object_get(subAttCloud, att);
		if (tmpJson == NULL){
			assert(json_object_set_new(subAttCloud, att, json_object()) == 0);
			tmpJson = json_object_get(subAttCloud, att);
			assert(tmpJson != NULL);
		}
		tmpInt = json_object_set_new(tmpJson, subAttVal, similarMap);
		assert(tmpInt == 0);
	}
}

attjson* buildAttributeCloud(model_data *modelData){
	char *line = {0}, *att = {0}, *subAtt = {0};
	json_t *attArr;
	int index;	
	int len;
	ssize_t read;
	size_t length = 0;
	json_error_t error;
	json_t *map;
	json_t *attMap, *hierMap, *foodMap;
	json_t *subAttCloud, *attCloud, *similarMap;
	similar_word_iota *similar;
	subAttCloud = json_object();
	attCloud = json_object();
	json_t *tmpJson;
	attjson *result;
	map = json_loads(attributeMap2, 0, &error);
	/* 
		
	*/
	attMap = json_object_get(map, "attributeMap");
	hierMap = json_object_get(map, "hierarchicalAttributeMap");
	map = json_loads(foodMap2, 0, &error);
	foodMap = json_object_get(map, "foodTypeMap");
	attArr = json_object_get(attMap, "purpose");
	json_object_foreach(attMap, att, attArr) {
		iterateLevelIArray(attArr, modelData, subAttCloud, att);
	}
	json_object_foreach(hierMap, att, attArr) {
		similar = findSimilarWords(modelData, att);
		similarMap = json_loads(convertToString(similar), 0, &error);
		assert(json_object_set_new(attCloud, att, similarMap) == 0);
		iterateLevelIArray(attArr, modelData, subAttCloud, att);
	}
	result = (attjson *)malloc(sizeof(attjson));
	result->attributeCloud = (json_t *)malloc(sizeof(attCloud));
	result->attributeCloud = json_deep_copy(attCloud);
	result->subAttributeCloud = (json_t *)malloc(sizeof(subAttCloud));
	result->subAttributeCloud = json_deep_copy(subAttCloud);
	return result;
}

ListFiles* getListOfFiles(const char* currentDir, const char* nextDir){
	ListFiles *list = (ListFiles *)malloc(sizeof(ListFiles));
	DIR *dir;
	struct dirent *ent;
	int count;
	char buf[160] = {0};
	strlcat(buf, currentDir, sizeof(buf));
	strlcat(buf, sep, sizeof(buf));
	strlcat(buf, nextDir, sizeof(buf));
	if ((dir = opendir (buf)) != NULL) {
  		/* print all the files and directories within directory */
		count = 0;
  		while ((ent = readdir (dir)) != NULL) {
			if (strcmp(ent->d_name, "..") == 0) { printf("continuing!"); continue;}
			if (strcmp(ent->d_name, ".") == 0) { printf("continuing!"); continue;}
			list->listOfFiles[count] = (char *)malloc(sizeof(ent->d_name));
			strlcpy(list->listOfFiles[count], ent->d_name, sizeof(ent->d_name));
			count++;
  		}
  		closedir (dir);
	} else {
  		/* could not open directory */
  		printf ("error: could not open directory: %s\n", buf);
	}
	list->count = count;
	return list;
}

void createAbsPath(char*dir1, char* dir2, char* file, char* buff){
	strcpy(buff, dir1);
	strcat(buff, sep);
	strcat(buff, dir2);
	strcat(buff, sep);
	strcat(buff, file);
	return;
}

int main(int argc, char **argv){
	char *dir = argv[1];
	char cwd[1024] = {0};
	char *tmpChar;
	char absPath[100] = {0};
	FILE *fp;
	char *line;
	int i;
	ListFiles *list;
	ssize_t read;
	size_t len = 0;
   	assert(getcwd(cwd, sizeof(cwd)) != NULL);
	smallSt *data;
	int globalCount = 0;
	char *modelFile = argv[2];
	char buf[400] = {0};
	// step1: read the model and load it.
	model_data *modelData = processModel(modelFile);
	// step2: build wordcloud 
	attributeCloud = buildAttributeCloud(modelData);
	// step 3 call individual threads.
/* 	pool[0] = threadpool_create(THREAD, SIZE, 0);
 	assert(pool[0] != NULL);
	for (i = 0; i < QUEUES; i++){
 		pool[i] = threadpool_create(THREAD, SIZE, 0);
 		assert(pool[i] != NULL);
 	}
*/	list = getListOfFiles(cwd, dir);
	globalCount = 0;
	for (i = 0; i < list->count; i++){
		tmpChar = list->listOfFiles[i];
		createAbsPath(cwd, dir, tmpChar, buf);
		fp = fopen(buf, "rb");
		if (fp == NULL) {
                	printf("Input file not found\n");
 			perror("Error");
			return -1;
 		}
		while ((read = getline(&line, &len, fp)) != -1) {
           		//printf("line text: %s :\n\n", line);
			data = (smallSt *)malloc((long long) 1 * sizeof(smallSt));
			data->text = (char *)malloc(strlen(line) + 1);
			data->name = (char *)malloc(sizeof(int));
			strcpy(data->text, line);
           		//printf("line after text: %s :\n\n", data->text);
			sprintf(data->name, "%d", globalCount++);
			processText(data);
//			assert(threadpool_add(pool[0], &processText, data, 0) == 0);
       		}
	}
	usleep(20000);
}

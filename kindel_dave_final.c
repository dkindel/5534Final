#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define DEBUG

#define V_X 2
#define V_1 1
#define V_0 0

#define MAXFUNCS 5
#define MAXCUBES 50
#define MAXLITERALS 50

//this a single function
typedef struct SFunc {
    int numin;
    int cubes[MAXCUBES][MAXLITERALS];
    int cubeCount;
} SFunc;

//This is all of the functions
typedef struct Func {
    int  numin;
    int  numout;
    SFunc  singleFuncs[MAXFUNCS]; //access the individual function
} Func;


typedef struct Kernel {
    int cubes[MAXCUBES][MAXLITERALS];
    int cubeCount;
} Kernel;

typedef struct Kernels {
    Kernel kern[100];
    int kernCount;
} Kernels;

typedef struct CoKernel {
    int cubes[MAXLITERALS];
} CoKernel;

typedef struct CoKernels {
    CoKernel cokern[100];
    int cokernCount;
} CoKernels;


Func buildFunc(char*);
void printFunc(Func* f);
void printSingleFunc(SFunc *sf);
void printCube(int* cube);
void copyCubes(int* src, int* dest);
void rKernel(SFunc *sf, Kernels *kerns, CoKernels *cokerns);
void rKernel_allFuncs(Func *f);
int  getCubesWithLiteral(SFunc *sf, int** cubes, int literal);
int findLargestSubset(int** cubes, int numCubes, int* subset);



int main(int argc, char *argv[]){
    char espName[20];

    if (argc != 2){
        fprintf(stderr, "Usage: %s <*.esp>\n", argv[0]);
        return EXIT_FAILURE;
    }
    strcpy(espName, argv[1]);
    Func f = buildFunc(espName);
    printFunc(&f);
    rKernel_allFuncs(&f);
    return EXIT_SUCCESS;
}

//loads the values from the .esp file
Func buildFunc(char* espName){
    Func f;
    FILE *inFile;
    char fName[40];

    strcpy(fName, espName);
    strcat(fName, ".esp");
    inFile = fopen(fName, "r");
    if(inFile == NULL){
        fprintf(stderr, "Can't open .esp file\n");
        exit(EXIT_FAILURE);
    }

    f.numin = 0;
    f.numout = 0;
    int i;
    for(i = 0; i < MAXFUNCS; i++){
        f.singleFuncs[i].cubeCount = 0; //keeps track of the number of cubes currently in the equation
    }
    char line[500]; 
    //first loop reads in the metadata lines
    while(fgets(line, sizeof line, inFile ) != NULL) {
        if(line[0] == '.'){
            //s is used as a junk array to read in some values
            char s[10];
            int val;
            if(line[1] == 'i'){
                sscanf(line, "%s %d", s, &val);
                f.numin = val;
            }
            else if(line[1] == 'o'){
                sscanf(line, "%s %d",s , &val);
                f.numout = val;
            }
            else if(line[1] == 'e'){
                break;
            }
            else {
                fprintf(stderr, "Unknown command provided: %c\n", line[1]);
            }
        }
    }
    fclose(inFile);

    //now reset file to read in again
    inFile = fopen(fName, "r");
    if(f.numin <= 0 || f.numout <= 0){
        return f;
    }
    //second loop starts reading in data lines
    while(fgets(line, sizeof line, inFile) != NULL) {
        if(line[0] != '.'){
            char *inputs = malloc(sizeof(char) * f.numin);
            char *outputs = malloc(sizeof(char) * f.numout);

            sscanf(line, "%s %s", inputs, outputs);

            int i; 
            for(i = 0; i < f.numout; i++){
                if(outputs[i] == '1'){
                    int currentCube = f.singleFuncs[i].cubeCount;
                    int j;
                    for(j = 0; j < f.numin; j++){
                        char inBitChar = inputs[j];
                        int inBit = V_X;
                        if(inBitChar == '0'){
                            inBit = V_0;
                        }
                        else if(inBitChar == '1'){
                            inBit = V_1;
                        }
                        f.singleFuncs[i].cubes[currentCube][j] = inBit;
                    }
                    f.singleFuncs[i].cubeCount = f.singleFuncs[i].cubeCount + 1;
                }
            }
            free(inputs);
            free(outputs);
        }
        else if(line[1] == 'e'){
            break;
        }

    }
    fclose(inFile);
    return f;
}


//used to call the recursive function on ALL the functions separately (we're going to do
//this one at a time)
void rKernel_allFuncs(Func *f){
    int i;
    Kernels *kerns = NULL;
    CoKernels *cokerns = NULL;
    for(i = 0; i < f->numout; i++){
        //I can't just malloc or work on the stack because c hates me
        kerns = (Kernels *) calloc(1, sizeof(Kernels));
        cokerns = (CoKernels *) calloc(1,sizeof(CoKernels));
        SFunc sf = f->singleFuncs[i];
        sf.numin = f->numin;
#ifdef DEBUG
        printf("For function %d: \n", i);
#endif
        rKernel(&sf, kerns, cokerns);
#ifdef DEBUG
        int j;
        printf("\tCokernels: \n");
        for(j = 0; j < cokerns->cokernCount; j++){
            printf("\t\tCoKernel %d: ", j);
            printCube(cokerns->cokern[j].cubes);
            printf("\n");
        }
#endif
        free(kerns);
        free(cokerns);
        //kerns = NULL;
        //cokerns = NULL;
    }
}

//recursive function to find the kernels
void rKernel(SFunc *sf, Kernels *kerns, CoKernels *cokerns){
    int i, j;
    for(i = 0; i < sf->numin; i++){ //i is the current literal 
        int** cubes = malloc(MAXCUBES * sizeof(int*));
        for(j = 0; j < MAXCUBES; j++){
            cubes[j] = (int*) malloc(MAXLITERALS * sizeof(int));
        }
        int numCubes = getCubesWithLiteral(sf, cubes, i);
#ifdef DEBUG
        printf("\tLiteral %d: ", i);
        for(j = 0; j < numCubes; j++){
            printCube(cubes[j]);
            printf(", ");
        }
        printf("\n");
#endif
        if(numCubes >= 2){ //if inside this loop, we know there's at least a 1 literal co-kernel (the literal itself)
            int* subset = malloc(MAXLITERALS * sizeof(int));
            findLargestSubset(cubes, numCubes, subset);
            //the largest subset is a cokernel, add it as such
            int* cokernAddr = cokerns->cokern[cokerns->cokernCount].cubes;
            copyCubes(subset, cokernAddr);
            cokerns->cokernCount = cokerns->cokernCount + 1;
#ifdef DEBUG
            printf("\t\tLargest subset (i.e. cokernel): ");
            printCube(cokernAddr);
            //printCube(subset);
            printf("\n");
#endif
        }
    }
}

//fills the largest subset into subset and returns the number of elements that were added
int findLargestSubset(int** cubes, int numCubes, int* subset){
    int i, j;
    for(j = 0; j < MAXLITERALS; j++){
        subset[j] = 1; //initialize to 1
    }
    int numLiterals = 0; //the number of literals in the largest subset
    for(i = 0; i < numCubes; i++){
        for(j = 0; j < MAXLITERALS; j++){
            subset[j] = subset[j] & cubes[i][j];
            numLiterals += subset[j];
        }
    }
    return numLiterals;
}

//gets the cubes with the literal inside
//fills the cube itself into the cubes param and returns the number cubes that have been added
int getCubesWithLiteral(SFunc *sf, int** cubes, int literal){
    int cubeCount = 0; //cubeCount is the running number of cubes being added that have the literal in it
    int i, j;
    for(i = 0; i < sf->cubeCount; i++){ //loop through all the cubes
        for(j = 0; j < MAXLITERALS; j++){ //compare all literals
            if(literal == j && sf->cubes[i][j] == V_1){
                cubes[cubeCount++] = sf->cubes[i]; //add the cube to the list and increment the cubeCount
            }
        }
    }
    return cubeCount;
}
//////////////////
//helper functions
//////////////////

void copyCubes(int* src, int* dest){
    int i;
    for(i = 0; i < MAXLITERALS; i++){
        dest[i] = src[i];
    }
}

#define intToChar(i) (char)(((int)'a')+i)

void printFunc(Func* f){
    int i, j, k;
    for(i = 0; i < f->numout; i++){
        printf("F_%d = ", i);
        for(j = 0; j < f->singleFuncs[i].cubeCount; j++){
            for(k = 0; k < MAXCUBES; k++){
                if(f->singleFuncs[i].cubes[j][k] == V_1){
                    printf("%c", intToChar(k));
                }
            }
            if(f->singleFuncs[i].cubeCount != j+1){
                printf(" + ");
            }
        }
        printf("\n");
    }
}

void printSingleFunc(SFunc *sf){
    int i, j;
    printf("F = ");
    for(i = 0; i < sf->cubeCount; i++){
        for(j = 0; j < MAXCUBES; j++){
            if(sf->cubes[i][j] == V_1){
                printf("%c", intToChar(j));
            }
        }
        if(sf->cubeCount != i+1){
            printf(" + ");
        }
    }
    printf("\n");
}

void printCube(int* cube){
    int i;
    for(i = 0; i < MAXLITERALS; i++){
        if(cube[i] == V_1){
            printf("%c", intToChar(i));
        }
    }
}

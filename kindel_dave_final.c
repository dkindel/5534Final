#include <string.h>
#include <stdio.h>
#include <stdlib.h>


#define V_X 2
#define V_1 1
#define V_0 0

#define MAXFUNCS 10
#define MAXCUBES 50
#define MAXLITERALS 100

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

typedef struct CoKernel {
    int cubes[MAXLITERALS];
} CoKernel;



Func buildFunc(char*);
void printFunc(Func* f);
void rKernel(SFunc *sf, Kernel *k, int kCount);
void rKernel_allFuncs(Func *f);



int main(int argc, char *argv[]){
    char espName[20];

    if (argc != 2){
        fprintf(stderr, "Usage: %s <*.esp>\n", argv[0]);
        return EXIT_FAILURE;
    }
    strcpy(espName, argv[1]);
    Func f = buildFunc(espName);
    printFunc(&f);
    //RKernel_allFuncs(f);
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
    for(i = 0; i < f->numout; i++){
        Kernel k[100];
        int kernelCount = 0;
        SFunc sf = f->singleFuncs[i];
        sf.numin = f->numin;
        rKernel(&sf, k, kernelCount);
    }
}

//recursive function to find the kernels
void rKernel(SFunc *sf, Kernel *k, int kCount){
    
}

//////////////////
//helper functions
//////////////////

#define intToChar(i) (char)(((int)'a')+i)

void printFunc(Func* f){
    int i, j, k;
    for(i = 0; i < f-> numout; i++){
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



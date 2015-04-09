#include <string.h>
#include <stdio.h>
#include <stdlib.h>

//#define DEBUG

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
int subtractCubes(int* cube1, int* cube2, int* result);
void logKernels(Kernels *kerns);
void logCoKernels(CoKernels *cokerns);
void printKernels(Kernels *kerns);
void printCoKernels(CoKernels *cokerns);
void copyKernToSF(Kernel *kern, SFunc *sf);
int isLastKernDup(Kernels *kerns);
int isKernDup(Kernel* kern1, Kernel* kern2);
int isCubeDup(int* cube1, int* cube2);


int main(int argc, char *argv[]){
    char espName[20];

    if (argc != 2){
        fprintf(stderr, "Usage: %s <*.esp>\n", argv[0]);
        return EXIT_FAILURE;
    }
    strcpy(espName, argv[1]);
    Func f = buildFunc(espName);
#ifdef DEBUG
    printFunc(&f);
#endif
    rKernel_allFuncs(&f);
    return EXIT_SUCCESS;
}

//loads the values from the .esp file
Func buildFunc(char* espName){
    Func f;
    //initialize func f
    int i;
    for(i = 0; i < MAXFUNCS; i++){
        SFunc *sf = &f.singleFuncs[i];
        int j,k;
        for(j = 0; j < MAXCUBES; j++){
            for(k = 0; k < MAXLITERALS; k++){
                sf->cubes[j][k] = 0;
            }
        }
        sf->numin = 0;
        sf->cubeCount = 0;
    }
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
            char *inputs = malloc(sizeof(char) * f.numin+1);
            char *outputs = malloc(sizeof(char) * f.numout+1);

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
        //I can't just malloc or work on the stack because c hates me
        Kernels *kerns = NULL;
        CoKernels *cokerns = NULL;
        kerns = (Kernels *) calloc(1, sizeof(Kernels));
        cokerns = (CoKernels *) calloc(1,sizeof(CoKernels));
        SFunc sf = f->singleFuncs[i];
        sf.numin = f->numin;
        printf("Function %d: \n", i);

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
        //we now have the kernels and cokernels
        logKernels(kerns);
        logCoKernels(cokerns);


        free(kerns);
        free(cokerns);
    }
}

void *worker_thread(void *args){
   return NULL; 
}

void logCoKernels(CoKernels *cokerns){
    printCoKernels(cokerns);
}

void logKernels(Kernels *kerns){
    printKernels(kerns);
}

//recursive function to find the kernels
void rKernel(SFunc *sf, Kernels *kerns, CoKernels *cokerns){
#ifdef DEBUG
    printf("Finding kernels for: \n");
    printSingleFunc(sf);
#endif
    int i, j;
    for(i = 0; i < sf->numin; i++){ //i is the current literal 
        int** cubes = malloc(MAXCUBES * sizeof(int*));
        int numCubes = getCubesWithLiteral(sf, cubes, i);
        kerns->kern[kerns->kernCount].cubeCount = numCubes;
#ifdef DEBUG
        printf("\tLiteral %d: ", i);
        for(j = 0; j < numCubes; j++){
            printCube(cubes[j]);
            printf(", ");
        }
        printf("\n");
#endif
        if(numCubes >= 2){ //if inside this loop, we know there's at least a 1 literal co-kernel (the literal itself)
            int* subset = calloc(MAXLITERALS, sizeof(int));
            findLargestSubset(cubes, numCubes, subset);
            
            //the largest subset is a cokernel, add it as such
            int* cokernAddr = cokerns->cokern[cokerns->cokernCount].cubes;
            copyCubes(subset, cokernAddr); //adds it directly into the cokern cubes array
            cokerns->cokernCount = cokerns->cokernCount + 1;
#ifdef DEBUG
            printf("\t\tLargest subset (i.e. cokernel): ");
            printCube(subset);
            printf("\n");
            printf("\t\t\tFound kernel: ");
#endif

            //regardless of whether the kernel will be kept, we'll add it into the array
            //There's no need to make another one.  If it won't be kept, we simply won't incrememnt the kernel count below
            for(j = 0; j < numCubes; j++){
                int* kernAddr = kerns->kern[kerns->kernCount].cubes[j];
                subtractCubes(cubes[j], subset, kernAddr); //this adds it directly into the kerns cubes array
#ifdef DEBUG
                printCube(kernAddr);
                if(j != numCubes - 1){
                    printf(" + ");
                }
#endif
            }
#ifdef DEBUG
            printf("\n");
#endif
            if(!isLastKernDup(kerns)){
                kerns->kernCount = kerns->kernCount + 1;

                //add in the subset
                //make it recursive here!
                SFunc *kernSF = calloc(1, sizeof(SFunc));
                kernSF->numin = sf->numin;
                copyKernToSF(&kerns->kern[kerns->kernCount-1], kernSF);

                rKernel(kernSF, kerns, cokerns);
                free(kernSF);
            }
            free(subset);
        }
        free(cubes);
    }
}

int isLastKernDup(Kernels *kerns){
    int numKerns = kerns->kernCount;
    if(numKerns == 0){
        return 0; //cannot have a duplicate
    }
    Kernel *lastKern = &kerns->kern[numKerns];
    int i;
    for(i = 0; i < numKerns-1; i++){ //numKerns-1 because we don't count the last one, obviously
        if(isKernDup(&kerns->kern[i], lastKern)){
            return 1;
        }
    }
    return 0; //got here because the kernel is not dupilcated
}

int isKernDup(Kernel* kern1, Kernel* kern2){
    int i, j;
    for(i = 0; i < kern1->cubeCount; i++){
        int isEqual = 0;
        for(j = 0; j < kern2->cubeCount; j++){
            if(isCubeDup(kern1->cubes[i], kern2->cubes[j])){
                isEqual = 1;
                break;
            }
        }
        if(!isEqual){
            return 0; //we know this because the cube for kern1 wasn't found in kern2
        }
    }
    //we've exhausted all of kern1 and manged to find the same cube throughout kern2
    //would return 1 now but we should do a backward search as well
    //what if kern1 = ab + ce and kern2 = ab + ce + df

    for(i = 0; i < kern2->cubeCount; i++){
        int isEqual = 0;
        for(j = 0; j < kern1->cubeCount; j++){
            if(isCubeDup(kern1->cubes[i], kern2->cubes[j])){
                isEqual = 1;
                break;
            }
        }
        if(!isEqual){
            return 0; //we know this because the cube for kern1 wasn't found in kern2
        }
    }

    return 1; //NOW we know we're done
}

int isCubeDup(int* cube1, int* cube2){
    int i;
    for(i = 0; i < MAXLITERALS; i++){
        if(cube1[i] != cube2[i]){
            return 0;   
        }
    }
    return 1; //if we've made it through, that's because every element of the cube is equal
}


//responsible for copying over the data from the kernel to the new single function
//to be used recursively
void copyKernToSF(Kernel *kern, SFunc *sf){
    sf->cubeCount = kern->cubeCount;
    int i;
    for(i = 0; i < kern->cubeCount; i++){
        copyCubes(kern->cubes[i], sf->cubes[i]);
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

//result = cube1-cube2
//the number of values left in result is returned
int subtractCubes(int* cube1, int* cube2, int* result){
    int i, numVals = 0;
    for(i = 0; i < MAXLITERALS; i++){
        if(cube1[i] == V_1 && cube2[i] == V_0){
            result[i] = V_1;
            numVals++;
        }
        else{
            result[i] = V_0;
        }
    }
    return numVals;
}



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


void printCoKernels(CoKernels *cokerns){
    printf("---------------------------\n");
    printf("Co-Kernels: \n");
    printf("\t{");
    int i;
    for(i = 0; i < cokerns->cokernCount; i++){
        printCube(cokerns->cokern[i].cubes);
        if(i != cokerns->cokernCount-1){
            printf(", ");
        }
    }
    printf("}\n");
    printf("---------------------------\n");
}

void printKernel(Kernel *kern){
    int i;
    for(i = 0; i < kern->cubeCount; i++){
        printCube(kern->cubes[i]);
        if(i != kern->cubeCount - 1){
            printf(" + ");
        }
    }
}

void printKernels(Kernels *kerns){
    printf("---------------------------\n");
    printf("Kernels: \n");
    printf("\t{");
    int i;
    for(i = 0; i < kerns->kernCount; i++){
        printKernel(&kerns->kern[i]);
        if(i < kerns->kernCount -1){
            printf(", ");
        }
    }
    printf("}\n");
    printf("---------------------------\n");
}

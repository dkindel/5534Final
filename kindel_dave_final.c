#include <string.h>
#include <stdio.h>
#include <stdlib.h>


#define V_X 2
#define V_1 1
#define V_0 0


typedef struct Func {
    int  numin;
    int  numout;
    int  cubes[20][100][200]; //first elem is func number, second is the cube, last is the literal
    //I'm not worried about space so much as simplicity so this works
    int  cubeCount[20];
} Func;



Func buildFunc(char*);
void printFunc(Func* f);

int main(int argc, char *argv[]){
    char espName[20];

    if (argc != 2){
        fprintf(stderr, "Usage: %s <*.esp>\n", argv[0]);
        return EXIT_FAILURE;
    }
    strcpy(espName, argv[1]);
    Func f = buildFunc(espName);
    printFunc(&f);
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
    for(i = 0; i < (sizeof(f.cubeCount)/sizeof(int));i++){
        f.cubeCount[i] = 0; //keeps track of the number of cubes currently in the equation
    }
    char line[500]; 
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
    while(fgets(line, sizeof line, inFile) != NULL) {
        if(line[0] != '.'){
            char *inputs = malloc(sizeof(char) * f.numin);
            char *outputs = malloc(sizeof(char) * f.numout);

            sscanf(line, "%s %s", inputs, outputs);

            int i; 
            for(i = 0; i < f.numout; i++){
                if(outputs[i] == '1'){
                    int currentCube = f.cubeCount[i];
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
                        f.cubes[i][currentCube][j] = inBit;
                    }
                    f.cubeCount[i] = f.cubeCount[i] + 1;
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

//////////////////
//helper functions
//////////////////

#define intToChar(i) (char)(((int)'a')+i)

void printFunc(Func* f){
    int i, j, k;
    for(i = 0; i < f-> numout; i++){
        printf("F_%d = ", i);
        for(j = 0; j < f->cubeCount[i]; j++){
            for(k = 0; k < (sizeof(f->cubes[i][j]) /sizeof(int)); k++){
                if(f->cubes[i][j][k] == V_1){
                    printf("%c", intToChar(k));
                }
            }
            if(f->cubeCount[i] != j+1){
                printf(" + ");
            }
        }
        printf("\n");
    }
}



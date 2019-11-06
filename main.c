//compiling: gcc -Wall main.c example_functions.c -o main

//Authors:
//Marianna Karenina de Almeida Flores - mariannakarenina@usp.br - 10821144
//Renan Peres Martins - 10716612
//Vinícius Ribeiro da Silva - vinicius.r@usp.br - 10828141

#include <stdio.h>
#include <string.h>
#include "example_functions.h"

#define INPUT_LIMIT 300
#define HEADER_SIZE 19L
#define FIXED_FIELD_SIZE 2
#define VARIABLE_FIELD_SIZE 77
#define TEST_CASE_PATH "casos-de-teste-e-binarios/caso02.bin"
#define REGISTER_SIZE 85
//the data register struct
//it holds all the information that is storaged inside of a data register
struct DataRegister{
    char *estadoOrigem;
    char *estadoDestino;
    char *cidadeOrigem;
    char *cidadeDestino;
    char *tempoViagem;

    int distancia;
};
typedef struct DataRegister DataRegister;


//the data header struct
//it holds all the information that is storaged inside of a data file header
struct DataHeader{
    char status;
    char *dataUltimaCompactacao;

    int numeroVertices;
    int numeroArestas;
};
typedef struct DataHeader DataHeader;

//given a register (reg) and its rrn, prints it
void printRegister(DataRegister reg, int rrn){
    //prints all the data that can't be null
    printf("%d %s %s %d %s %s", rrn, reg.estadoOrigem, reg.estadoDestino, reg.distancia, reg.cidadeOrigem, reg.cidadeDestino);
    
    //since tempoViagem can be null, check it...
    if(reg.tempoViagem != NULL)
        printf(" %s\n", reg.tempoViagem);
    else
        printf("\n");    
}

//Given a file pointer (fp), get a register 
//it assumes that the position indicator of the file is the begin of the register
DataRegister getRegister(FILE *fp, int rrn){
    
    
    //cursor position  
    fseek(fp, rrn*REGISTER_SIZE+HEADER_SIZE, SEEK_SET);

    DataRegister reg; //creates the register variable...
 
    //gets the "estadoOrigem" field
    //estadoOrigem is a fixed size field of size given by FIXED_FIELD_SIZE
    reg.estadoOrigem = (char*)calloc(FIXED_FIELD_SIZE + 1, sizeof(char));
    fread(reg.estadoOrigem, sizeof(char), FIXED_FIELD_SIZE, fp);
    reg.estadoOrigem[FIXED_FIELD_SIZE] = '\0';

    //gets the "estadoDestino" field
    //estadoDestino is a fixed size field of size given by FIXED_FIELD_SIZE
    reg.estadoDestino = (char*)calloc(FIXED_FIELD_SIZE + 1, sizeof(char));
    fread(reg.estadoDestino, sizeof(char), FIXED_FIELD_SIZE, fp);
    reg.estadoDestino[FIXED_FIELD_SIZE] = '\0';

    //gets the "distancia" field
    //estadoDestino is a fixed size field of size of a int
    fread(&reg.distancia, sizeof(int), 1, fp);

    //gets all the variable fields and storage it in one single string
    //since the data register is fixed size, than the size of all variable fields is the size of the register minus the size of the fixed fields
    char *variableSizeData = (char*)calloc(VARIABLE_FIELD_SIZE + 1, sizeof(char));
    fread(variableSizeData, sizeof(char), VARIABLE_FIELD_SIZE, fp);
    variableSizeData[VARIABLE_FIELD_SIZE] = '\0';

    //sepates the variableSizeData string into the required fields
    char delim[] = {'|', '\0'};
    reg.cidadeOrigem = strtok(variableSizeData, delim);
    reg.cidadeDestino = strtok(NULL, delim);
    reg.tempoViagem = strtok(NULL, delim);

    //since tempoViagem can be null, then check it if there is acctually a text in the field 
    //if there is a '#', then the field is empty.
    //isn't correct to free the field, because the actual string is variableSizeData
    if(reg.tempoViagem[0] == '#')
        reg.tempoViagem = NULL;

    //returns the reg
    return reg;
}

//given a register, free its content from primary memmory
void freeRegister(DataRegister reg){
    free(reg.estadoOrigem);
    free(reg.estadoDestino);
    free(reg.cidadeOrigem);  
    //when "cidadeOrigem" is freed, it also removes "cidadeDestino" and "tempoViagem"
    //this happens because "cidadeDestino" and "tempoViagem" where made using strtok function, and "cidadeOrigem" is the main pointer
}

//given a File pointer (fp), prints the content of the file
//the binary file must follow the data register description and variables order
void printDataFile(FILE *fp){
	size_t fl; //file lenght

    //check file lenght
    fseek(fp, 0L, SEEK_END);
    fl = ftell(fp);
    //if the file lenght is too small, return a error
    if(fl < HEADER_SIZE){
		fprintf(stderr, "Arquivo incorreto, ou sem cabecalho");
		return;
	}

    //printing the file...
    int currRRN = 0;                  //Current RRN number
    DataRegister currReg;             //Current data register
    fseek(fp, HEADER_SIZE, SEEK_SET); //sets the file position to after the header
    
    while (ftell(fp) < fl){           //keeps reading the file while there is data available
        currReg = getRegister(fp, currRRN);    //get the next register in the file and print it
        printRegister(currReg, currRRN);
        freeRegister(currReg);
        currRRN++;
    }
}

FILE* openFile(char* filename){
	FILE *fp;  //file pointer

    //check if filename is valid and if there is a file to be open
    if(filename == NULL || !(fp = fopen(filename, "rb"))) {
		fprintf(stderr, "ERRO AO ESCREVER O BINARIO NA TELA (função binarioNaTela1): não foi possível abrir o arquivo que me passou para leitura. Ele existe e você tá passando o nome certo? Você lembrou de fechar ele com fclose depois de usar?\n");
		return NULL;
	}

    return fp;
}

//given a header, print its content
void printHeader(DataHeader header){
    printf("status: %d\n", header.status);
    printf("numeroVertices: %d\n", header.numeroVertices);
    printf("numeroArestas:  %d\n", header.numeroArestas);
    printf("dataUltimaCompactacao: %s\n", header.dataUltimaCompactacao);
    printf("\n");
}

//Given a file pointer, get the data header
//the binary file must follow the data description given in the "descricao.pdf" file
DataHeader getHeader(FILE *fp){
    //creates the header variable
    DataHeader header;
    header.status = 0;
    header.numeroVertices = 0;
    header.numeroArestas  = 0;
    header.dataUltimaCompactacao = NULL;

    //if there is no file, then return the header without reading a file
    if(fp == NULL)
        return header;

    //if there is a file, alloc memory to the "dataUltimaCompactacao" string
    header.dataUltimaCompactacao = (char*)malloc(10 * sizeof(char));
    memset(header.dataUltimaCompactacao, '\0', 10 * sizeof(char));

    //set the cursor on the beginning of the file
    fseek(fp, 0L, SEEK_SET);

    //reading the header....
    fread(&(header.status), sizeof(char), 1, fp);
    fread(&(header.numeroVertices), sizeof(int), 1, fp);
    fread(&(header.numeroArestas),  sizeof(int), 1, fp);
    fread(header.dataUltimaCompactacao,  sizeof(char), 10, fp);

    return header;
}

void makingRegister(FILE *fp, char* linha){

        char* temp;
        temp =  (char*)calloc(REGISTER_SIZE, sizeof(char));

    FILE *newFile;
    newFile = fopen ("mynewfile.bin", "wb");

    //sepates the variableSizeData string into the required fields
    char delim[] = {',', '\0'};
    char barra[] = {'|', '\0'};
    temp = strtok(linha, delim);
    //saving
    fwrite(temp,sizeof(char), FIXED_FIELD_SIZE, newFile);

    temp = strtok(NULL, delim);
    fwrite(temp,sizeof(char), FIXED_FIELD_SIZE, newFile);
    
    temp = strtok(NULL, delim);
    //String to Int
    int valor = atoi (temp);
    fwrite(&valor, sizeof(int), 1 , newFile);

    temp = strtok(NULL, delim);
        fwrite(barra,sizeof(char), 1 , newFile);
        fwrite(temp, sizeof(char), VARIABLE_FIELD_SIZE, newFile);
    temp = strtok(NULL, delim);
        fwrite(barra,sizeof(char), 1 , newFile);
        fwrite(temp, sizeof(char), VARIABLE_FIELD_SIZE, newFile);
    temp = strtok(NULL, delim);
        fwrite(barra,sizeof(char), 1 , newFile);
        fwrite(temp, sizeof(char), VARIABLE_FIELD_SIZE, newFile);
    
    fclose(newFile);

    

} 

//converting csv
void DealingCSV (FILE *ptr, char* name ){

    char* linha = (char*) malloc(INPUT_LIMIT*sizeof(char));

    ptr = fopen(name, "r" );

     fseek(ptr, 17, SEEK_SET);
   
     while(fgets(linha, INPUT_LIMIT*sizeof(char), ptr) != NULL){
       makingRegister(ptr, linha);
    }
        fclose (ptr);
        binarioNaTela1("mynewfile.bin");
    }
   
   

int main(){
    /*int command = -1;
    char *args = (char*)malloc(INPUT_LIMIT * sizeof(char));
    memset(args, '\0', INPUT_LIMIT * sizeof(char));*/

    //scanf(" %d ", &command);
    //fgets(args, INPUT_LIMIT, stdin);
    //printf("command: %d, args: %s\n", command, args);

    FILE *fp = openFile(TEST_CASE_PATH);
    if(fp == NULL)
        return 0;

    DataHeader header;
    header = getHeader(fp);
    printHeader(header);
    
    printDataFile(fp);

    fclose(fp);
    //free(args);
    return 0;
}
//compiling: gcc -Wall main.c example_functions.c -o main

//Authors:
//Marianna Karenina de Almeida Flores - mariannakarenina@usp.br - 10821144
//Renan Peres Martins - 10716612
//Vinícius Ribeiro da Silva - vinicius.r@usp.br - 10828141

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "example_functions.h"

#define INPUT_LIMIT 300
#define HEADER_SIZE 19L
#define FIXED_FIELD_SIZE 2
#define VARIABLE_FIELD_SIZE 77
#define TEST_CASE_PATH "casos-de-teste-e-binarios/caso02.bin"
#define REGISTER_SIZE 85
#define STATUS_OK '1'
#define STATUS_CORRUPTED '0'

#define REMOVE_FILES 1
#define SEARCH_FILES 1
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


struct DataRegisterLinkedList{
    int rrn;
    DataRegister reg;
    struct DataRegisterLinkedList *prox;
};
typedef struct DataRegisterLinkedList DataRegisterLinkedList;


//the data header struct
//it holds all the information that is storaged inside of a data file header
struct DataHeader{
    char status;
    char *dataUltimaCompactacao;

    int numeroVertices;
    int numeroArestas;
};
typedef struct DataHeader DataHeader;

enum registerFields {ESTADO_ORIGEM, ESTADO_DESTINO, CIDADE_ORIGEM, CIDADE_DESTINO, TEMPO_VIAGEM, DISTANCIA, ERROR = -1};

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

char isRegRemoved(DataRegister reg){
    return reg.estadoOrigem[0] == '*';
}

//Given a file pointer (fp), get a register 
//it assumes that the position indicator of the file is the begin of the register
DataRegister getRegister(FILE *fp, int rrn){
    //sets the cursor position to match the given rrn 
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
void printDataFile(FILE *fp, DataHeader header){
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
    while (currRRN < header.numeroArestas){           //keeps reading the file while there is data available
        currReg = getRegister(fp, currRRN);    //get the next register in the file and print it
        if(!isRegRemoved(currReg))
            printRegister(currReg, currRRN);
        
        freeRegister(currReg);
        currRRN++;
    }
}

FILE* openFile(char* filename){
	FILE *fp;  //file pointer

    //check if filename is valid and if there is a file to be open
    if(filename == NULL || !(fp = fopen(filename, "rb+"))) {
		fprintf(stderr, "ERRO AO ESCREVER O BINARIO NA TELA (função binarioNaTela1): não foi possível abrir o arquivo que me passou para leitura. Ele existe e você tá passando o nome certo? Você lembrou de fechar ele com fclose depois de usar?\n");
		return NULL;
	}

    return fp;
}

//given a header, print its content
void printHeader(DataHeader header){
    printf("status: %c\n", header.status);
    printf("numeroVertices: %d\n", header.numeroVertices);
    printf("numeroArestas:  %d\n", header.numeroArestas);
    printf("dataUltimaCompactacao: %.10s\n", header.dataUltimaCompactacao);
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

int getFieldId(char* Field){
    if(strcmp(Field, "estadoOrigem") == 0)
        return ESTADO_ORIGEM;

    if(strcmp(Field, "estadoDestino") == 0)
        return ESTADO_DESTINO;

    if(strcmp(Field, "cidadeOrigem") == 0)
        return CIDADE_ORIGEM;

    if(strcmp(Field, "cidadeDestino") == 0)
        return CIDADE_DESTINO;

    if(strcmp(Field, "tempoViagem") == 0)
        return TEMPO_VIAGEM;
        
    if(strcmp(Field, "distancia") == 0)
        return DISTANCIA;
    
    return ERROR;
}

int compareFieldValue(DataRegister reg, char* value, int fieldId){
    if(fieldId == ESTADO_ORIGEM)
        return strcmp(reg.estadoOrigem, value) == 0;
    
    if(fieldId == ESTADO_DESTINO)
        return strcmp(reg.estadoDestino, value) == 0;

    if(fieldId == CIDADE_ORIGEM)
        return strcmp(reg.cidadeOrigem, value) == 0;

    if(fieldId == CIDADE_DESTINO)
        return strcmp(reg.cidadeDestino, value) == 0;

    if(fieldId == TEMPO_VIAGEM)
        return strcmp(reg.tempoViagem, value) == 0;

    if(fieldId == DISTANCIA){
        int value_int = atoi(value);
        return reg.distancia == value_int;
    }

    return 0;
}

//search a certain value of a field 
void searchByField(FILE *fp, DataHeader header, char* Field, char* value, int action){
    int rrn = 0;
    DataRegister reg;
    int fieldId = getFieldId(Field);

    for(rrn = 0; rrn < header.numeroArestas; rrn++){
        reg = getRegister(fp, rrn);
        if(compareFieldValue(reg, value, fieldId))
            printRegister(reg, rrn);

            //if(action == REMOVE_FILES)
    }
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
void DealingCSV (char* name ){

    char* linha = (char*) malloc(INPUT_LIMIT*sizeof(char));

    FILE *ptr = fopen(name, "r" );

     fseek(ptr, 17, SEEK_SET);
   
     while(fgets(linha, INPUT_LIMIT*sizeof(char), ptr) != NULL){
       makingRegister(ptr, linha);
    }
        fclose (ptr);
        binarioNaTela1("mynewfile.bin");
    }
   
void Insert (char *name, int num){

    FILE *ptr = fopen(name, "wb" );

    if(ptr == NULL){
        printf("Falha no processamento do arquivo.");
        return;
    }else{
        int i = 0;
        while( i!= num ){
            
            i++;

            fseek(ptr, 0, SEEK_END); 

            char*estadoOrigem;
            estadoOrigem = (char*)calloc(3, sizeof(char));
            scanf("%s", estadoOrigem);
            fwrite(estadoOrigem ,sizeof(char), 2, ptr);
          
            char*estadoDestino;
            estadoDestino = (char*)calloc(3, sizeof(char));
            scanf("%s", estadoDestino);
            fwrite(estadoDestino ,sizeof(char), 2, ptr);

            int Distancia;
            Distancia = (int )calloc(1, sizeof(int));
            scanf("%d", &Distancia);
            fwrite(&Distancia ,sizeof(int),1, ptr);

            char*cidadeOrigem;
            cidadeOrigem = (char*)calloc(85, sizeof(char));
            scanf("%s", cidadeOrigem);
            scan_quote_string(cidadeOrigem);
            fwrite(cidadeOrigem ,sizeof(char), 85, ptr);

            char*cidadeDestino;
            cidadeDestino = (char*)calloc(85, sizeof(char));
            scanf("%s", cidadeOrigem);
            scan_quote_string(cidadeDestino);
            fwrite(cidadeDestino ,sizeof(char), 85, ptr);

            char*tempoViagem;
            tempoViagem = (char*)calloc(85, sizeof(char));
            scanf("%s", tempoViagem);
            scan_quote_string(tempoViagem);
            fwrite(tempoViagem ,sizeof(char), 85, ptr);


        fclose (ptr);

        }
    }

}

void setHeaderStatus(FILE *fp, char status){
    fseek(fp, 0, SEEK_SET);
    fwrite(&status, sizeof(char), 1, fp);
}

void data(char *datafinal){
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    sprintf(datafinal, "%.2d/%.2d/%.4d", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);
}


//Save current Register
void saveRegister(FILE *fp, DataRegister reg, int rrn){
    //delimiter symbol
    char delim = '|'; 
    fseek(fp, 19, SEEK_SET);
    fwrite(reg.estadoOrigem, sizeof(char), sizeof(char)*FIXED_FIELD_SIZE, fp);
    fwrite(reg.estadoDestino, sizeof(char), sizeof(char)*FIXED_FIELD_SIZE, fp);
    fwrite(&reg.distancia, sizeof(__int16), 1*sizeof(__int16), fp);
    fwrite(reg.cidadeOrigem, sizeof(char), sizeof(char)*strlen(reg.cidadeOrigem), fp);
    fwrite(&delim, sizeof(char), 1*sizeof(char), fp);
    fwrite(reg.cidadeDestino, sizeof(char),  sizeof(char)*strlen(reg.cidadeDestino), fp);
    fwrite(&delim, sizeof(char), sizeof(char)*1, fp);
    fwrite(reg.tempoViagem, sizeof(char),  sizeof(char)*strlen(reg.tempoViagem), fp);
    fwrite(&delim, sizeof(char), sizeof(char)*1, fp);
}

//Updating a field register (Function 7)
void updateRegister(FILE *fp, DataHeader header, int size){
    //RRN Value
    int rrn = 0;
    //field identifier
    int fieldId = 0;
    //Field Name
    char *Field = (char*)calloc(14, sizeof(char));
    //Value of a field
    char* value = (char*)calloc(REGISTER_SIZE, sizeof(char));

    for(int n = 0; n < size; n++){
        DataRegister Reg;
        printf("Qual o RRN?\n");
        scanf("%d", &rrn);
        printf("Qual o campo?\n");
        scanf(" %s", Field);
        printf("Qual o valor?\n");
        scan_quote_string(value);
        fieldId = getFieldId(Field);
        Reg = getRegister(fp, rrn);
        switch (fieldId) {
            case ESTADO_ORIGEM:
                Reg.estadoOrigem = value;
                break;
            case ESTADO_DESTINO:
                Reg.estadoDestino = value;
                break;
            case CIDADE_ORIGEM:
                Reg.cidadeOrigem = value;
                break;
            case CIDADE_DESTINO:
                Reg.cidadeDestino = value;
                break;
            case TEMPO_VIAGEM:
                Reg.tempoViagem = value;
                break;
            case DISTANCIA:
                Reg.distancia = atoi(value);
                break;
            default:
                printf("Falha no processamento do arquivo.\n");
                return;
        }saveRegister(fp, Reg, rrn);
    }printDataFile(fp, header);
    
}

//defragmenting file
void defragmenter(char *in, char *out, DataHeader header){
    char file_in[strlen(in)+1], file_out[strlen(out)+1];
    snprintf(file_in, sizeof(file_in), "%s", in);
    snprintf(file_out, sizeof(file_out), "%s", out);
    FILE *file_read = fopen(file_in, "rb");
    FILE *file_write = fopen(file_out, "rb+");
    DataRegister aux;
    

    for(int i = 0, j = 0; i < header.numeroVertices; i++){
        aux = getRegister(file_read, i);
        if(isRegRemoved(aux) != '*'){
            saveRegister(file_write, aux, j);
            j++;
        }
    }data(header.dataUltimaCompactacao);
    fseek(file_write, 9, SEEK_SET);
    fwrite(header.dataUltimaCompactacao, sizeof(header.dataUltimaCompactacao), 10, file_write);
    printDataFile(file_write, header);
    fclose(file_read);
    fclose(file_write);
    
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
    
    //printDataFile(fp, header);

    printf("\n\n");
    searchByField(fp, header, "distancia", "150", SEARCH_FILES);
    updateRegister(fp, header, 2);

    fclose(fp);
    //free(args);
    return 0;
}
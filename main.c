//compiling: gcc -Wall main.c example_functions.c -o main

//Authors:
//Marianna Karenina de Almeida Flores - mariannakarenina@usp.br - 10821144
//Renan Peres Martins - 10716612
//Vinícius Ribeiro da Silva - vinicius.r@usp.br - 10828141

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "example_functions.h"

#define HASH_TABLE_SIZE 5000

#define SUCESS 0
#define FAILED 1

#define INPUT_LIMIT 300
#define HEADER_SIZE 19L
#define FIXED_FIELD_SIZE 2
#define VARIABLE_FIELD_SIZE 77
#define TEST_CASE_PATH "casos-de-teste-e-binarios/caso02.bin"
#define REGISTER_SIZE 85
#define STATUS_OK '1'
#define STATUS_CORRUPTED '0'

#define REMOVE_FILES 1
#define SEARCH_FILES 2
//the data register struct
//it holds all the information that is storaged inside of a data register
struct DataRegister{
    char *estadoOrigem;
    char *estadoDestino;
    char *cidadeOrigem;
    char *cidadeDestino;
    char *tempoViagem;

    int rrn;
    int distancia;
};
typedef struct DataRegister DataRegister;


struct DataRegisterLinkedList{
    int rrn;
    DataRegister reg;
    struct DataRegisterLinkedList *prox;
};
typedef struct DataRegisterLinkedList DataRegisterLinkedList;


struct city{
    char* name;
    int appearances;
};
typedef struct city city;

//the data header struct
//it holds all the information that is storaged inside of a data file header
struct DataHeader{
    char status;
    char *dataUltimaCompactacao;

    int numeroVertices;
    int numeroArestas;
    int maxRRN;

    long fileLength;
};
typedef struct DataHeader DataHeader;

enum registerFields {ESTADO_ORIGEM, ESTADO_DESTINO, CIDADE_ORIGEM, CIDADE_DESTINO, TEMPO_VIAGEM, DISTANCIA, ERROR = -1};

//given a register (reg) and its rrn, prints it
void printRegister(DataRegister reg){
    if(reg.rrn == FAILED)
        return;

    //prints all the data that can't be null
    printf("%d %s %s %d %s %s", reg.rrn, reg.estadoOrigem, reg.estadoDestino, reg.distancia, reg.cidadeOrigem, reg.cidadeDestino);
    
    //since tempoViagem can be null, check it...
    if(reg.tempoViagem != NULL)
        printf(" %s\n", reg.tempoViagem);
    else
        printf("\n");    
}

char isRegRemoved(DataRegister reg){
    return reg.estadoOrigem[0] == '*';
}

void overwriteFileHeader(FILE *fp, DataHeader header){
    fseek(fp, 0, SEEK_SET);
    fwrite(&(header.status), sizeof(char), 1, fp);
    fwrite(&(header.numeroVertices), sizeof(int), 1, fp);
    fwrite(&(header.numeroArestas), sizeof(int), 1, fp);
    fwrite(header.dataUltimaCompactacao, sizeof(char), 10, fp);
    fflush(fp);
}

//Given a file pointer (fp), get a register 
//it assumes that the position indicator of the file is the begin of the register
DataRegister getRegister(FILE *fp, int rrn){
    size_t fl;
    size_t seek;
    DataRegister reg; //creates the register variable...

    seek = rrn*REGISTER_SIZE+HEADER_SIZE;
    fseek(fp, 0, SEEK_END);
    fl = ftell(fp);

    //sets the cursor position to match the given rrn 
    if(seek >= fl || fseek(fp, rrn*REGISTER_SIZE+HEADER_SIZE, SEEK_SET) != 0){
        reg.rrn = FAILED;
        return reg;
    }
    reg.rrn = rrn;
 
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
    int thereIsData = 0;
	size_t fl; //file lenght

    //check file lenght
    fseek(fp, 0L, SEEK_END);
    fl = ftell(fp);
    //if the file lenght is too small, return a error
    if(fl < HEADER_SIZE){
		fprintf(stdout, "Arquivo incorreto, ou sem cabecalho");
		return;
	}

    //printing the file...
    int currRRN = 0;                  //Current RRN number
    DataRegister currReg;             //Current data register    
    while (currRRN < header.maxRRN){           //keeps reading the file while there is data available
        currReg = getRegister(fp, currRRN);    //get the next register in the file and print it

        if(currReg.rrn == FAILED){
            printf("Registro inexistente.\n");
        }
        else{
            if(!isRegRemoved(currReg)){
                printRegister(currReg);
                thereIsData = 1;
            }
            freeRegister(currReg);
        }

        currRRN++;
    }
    if(!thereIsData)
        printf("Registro inexistente.\n");
}

void printHashTable(city *hashTable, FILE *outputStream){
    int i = 0;
    for(i = 0; i < HASH_TABLE_SIZE; i++){
        if(hashTable[i].name != NULL)
            fprintf(outputStream, "%s|%d\n", hashTable[i].name, hashTable[i].appearances);
    }
}

//given a header, print its content
void printHeader(DataHeader header){
    printf("status: %c\n", header.status);
    printf("maxRRN:  %d\n", header.maxRRN);
    printf("fileLength:  %ld\n", header.fileLength);
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
    header.dataUltimaCompactacao = (char*)malloc(11 * sizeof(char));
    memset(header.dataUltimaCompactacao, '\0', 11 * sizeof(char));

    //set the cursor on the beginning of the file
    fseek(fp, 0L, SEEK_SET);

    //reading the header....
    fread(&(header.status), sizeof(char), 1, fp);
    fread(&(header.numeroVertices), sizeof(int), 1, fp);
    fread(&(header.numeroArestas),  sizeof(int), 1, fp);
    fread(header.dataUltimaCompactacao,  sizeof(char), 10, fp);

    //get file length
    fseek(fp, 0L, SEEK_END);
    header.fileLength = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    //get the rrn value of the last register
    header.maxRRN = (header.fileLength - HEADER_SIZE) / REGISTER_SIZE;

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

    if(fieldId == TEMPO_VIAGEM){
        if(value == NULL)
            return reg.tempoViagem == NULL;
        else
            return strcmp(reg.tempoViagem, value) == 0;
    }

    if(fieldId == DISTANCIA){
        int value_int = atoi(value);
        return reg.distancia == value_int;
    }

    return 0;
}

void logicalDeleteReg(FILE *fp, DataRegister reg){
    fseek(fp, reg.rrn*REGISTER_SIZE+HEADER_SIZE, SEEK_SET);
    fputc('*',fp);
}

   
int hashCode(char* string){
    int i = 0;
    int sum = 0;
    for(i = 0; string[i] != '\0' && string[i] != '\n'; i++){
        sum += string[i];
    }

    return sum % HASH_TABLE_SIZE;
}

int hashSearch(city *hashTable, city item){
    int index = hashCode(item.name);
    while(hashTable[index].name != NULL && strcmp(hashTable[index].name, item.name) != 0){
        index++;
        index = index % HASH_TABLE_SIZE;
    }

    return index;
}

void hashRemove(city *hashTable, city item, DataHeader *header){
    int index = hashSearch(hashTable, item);
    if(hashTable[index].name == NULL)
        return;
    
    if(hashTable[index].appearances == 1){
        free(hashTable[index].name);
        hashTable[index].name = NULL;
        header->numeroVertices = header->numeroVertices - 1;
    }
    else{
        hashTable[index].appearances--;
    }
}


void hashInsert(city *hashTable, city item, DataHeader *header){
    int index = hashSearch(hashTable, item);

    if(hashTable[index].name == NULL){
        hashTable[index] = item;
        hashTable[index].appearances = item.appearances;
        (header->numeroVertices)++;
        (header->maxRRN)++;
    }
    else{
        free(item.name);
        hashTable[index].appearances++;
    }
}

//search a certain value of a field 
void searchByField(FILE *fp, DataHeader *header, char* value, char* field, int action, city *hashTable){
    int rrn = 0;
    DataRegister reg;
    int fieldId = getFieldId(field);
    
    // if(value != NULL)
    //     printf("2-|v|f| |%s|%s|, FieldID: %d\n", value, field, fieldId);
    // else
    //     printf("2-|v|f| |NULL|%s|, FieldID: %d\n", field, fieldId);

    int registersRemoved = 0;
    for(rrn = 0; rrn < header->maxRRN; rrn++){
        // printf("d0.0\n");
        reg = getRegister(fp, rrn);
        if(reg.rrn == FAILED)
            continue;

        if(isRegRemoved(reg))
            continue;
            
        // printf("d0.1\n");
        // printRegister(reg);
        if(compareFieldValue(reg, value, fieldId)){
            // if(fieldId == TEMPO_VIAGEM && value != NULL)
            // printf("d1.0\n");
            if(action == SEARCH_FILES){
                printRegister(reg);
            }
            else if(action == REMOVE_FILES){

                // if(fieldId == TEMPO_VIAGEM && value != NULL){
                //     printf("d2.0, rrn: %d\n", reg.rrn);
                //     continue;
                // }
                // printf("d2.0\n");

                logicalDeleteReg(fp, reg);
                city Origem = {.name = reg.cidadeOrigem, .appearances = 1};
                city Destino = {.name = reg.cidadeDestino, .appearances = 1};
                hashRemove(hashTable, Origem, header);
                hashRemove(hashTable, Destino, header);

                (header->numeroArestas)--;
                registersRemoved = 1;
            }
        }
        // printf("d1.0\n");
    }

    if(registersRemoved == 1){
        overwriteFileHeader(fp, *header);
    }
}


void makingRegister(char* LINHA, FILE *newFile, city* hashTable, DataHeader *header){

    
    city currcity = {.name = NULL, .appearances = 1};
    char* temp;
    temp =  (char*)calloc(85, sizeof(char));
    char *cidadeOrigem = (char*)calloc(REGISTER_SIZE, sizeof(char));
    char *cidadeDestino = (char*)calloc(REGISTER_SIZE, sizeof(char));

    //separets the variableSizeData string into the required fields
    char delim[] = {',', '\0'};
  
    //saving
    //fixed Size -- estadoOrigem
    temp = strtok(LINHA, delim);
    fwrite(temp,sizeof(char), 2, newFile);


    //fixed Size -- estadoDestino
    temp = strtok(NULL, delim);
    fwrite(temp,sizeof(char), 2, newFile);
    
    //fixed Size -- Distancia
    temp = strtok(NULL, delim);
    //converting String to Int
    int valor = atoi (temp);
    fwrite(&valor, sizeof(int), 1 , newFile);

    //variable Size -- cidadeOrigem
    temp = strtok(NULL, delim);
    int lenCO;
    lenCO= 0;
    lenCO = strlen(temp);
    fwrite(temp, sizeof(char), lenCO, newFile);
    fputc('|', newFile );
    strcpy(cidadeOrigem, temp);
    currcity.name = cidadeOrigem;
    hashInsert(hashTable, currcity, header);

    //variable Size -- cidadeDestino    
    temp = strtok(NULL, delim);
    int lenCD;
    lenCD= 0;
    lenCD = strlen(temp);
    fwrite(temp, sizeof(char), lenCD, newFile);
    fputc('|', newFile );
    strcpy(cidadeDestino, temp);
    currcity.name = cidadeDestino;
    hashInsert(hashTable, currcity, header);

    //variable Size -- tempoViagem    
    temp = strtok(NULL, delim);
    temp[strlen(temp) - 1] = '\0';
    int tV;
    tV= 0;
    tV = strlen(temp);
    fwrite(temp, sizeof(char), tV, newFile);
    fputc('|', newFile );

    int espacoUsado;
    espacoUsado = 0;
    espacoUsado = lenCD + lenCO + tV + 3;

    char* completar;
    completar = (char*)malloc (REGISTER_SIZE*sizeof(char));
    memset(completar, '#', REGISTER_SIZE*sizeof(char));
    fwrite(completar,sizeof(char), VARIABLE_FIELD_SIZE - espacoUsado, newFile);
    
    (header->numeroArestas)++;
} 

//converting csv
void DealingCSV (FILE* ptr, FILE *newFile, char *outputName, city* hashTable, DataHeader *header){

    char* linha;
    linha = (char*) calloc(85,sizeof(char));
    
    // FILE *ptr = fopen(name, "r" );
   
    // if(ptr == NULL) {
    //     fprintf(stdout, "Falha no carregamento do arquivo.\n");
    //     return;
    // }

    // fseek(ptr, 77, SEEK_SET);
    // FILE *newFile;
    // newFile = fopen (novo, "wb");
    // if(newFile == NULL) {
    //     fprintf(stdout, "Falha no carregamento do arquivo.\n");
    //      return;
    // }
    // printHeader(*header);
    overwriteFileHeader(newFile, *header);
    fseek(ptr, 76, SEEK_SET);



    while(fgets(linha, REGISTER_SIZE, ptr) != NULL){
        // printHeader(*header);
        // printf("|%s|\n", linha);
        makingRegister(linha, newFile, hashTable, header);
        memset(linha, '\0',85*sizeof(char) );
    }
    

    // printHeader(*header);
}




int getBinaryFile(char *filename, FILE **fp, DataHeader *header, char* mode){
    FILE *tempFP = fopen(filename, mode);
    if(tempFP == NULL) {
        fprintf(stdout, "Falha no processamento do arquivo.\n");
        return FAILED;
    }

    *header = getHeader(tempFP);
    if(header->status == '0'){
        fprintf(stdout, "Falha no processamento do arquivo.\n");
        return FAILED;
    }

    *fp = tempFP;
    return SUCESS;
}


//Insert function   
void Insert (FILE *fp, int num, city *hashTable, DataHeader *header, char *filename){
    int i = 0;
    int Distancia = 0;
    
    char*tempoViagem = NULL;
    char*estadoOrigem = NULL;
    char*cidadeOrigem = NULL;
    char*estadoDestino = NULL;
    char*cidadeDestino = NULL;
    char* completar = (char*)malloc (REGISTER_SIZE*sizeof(char));
    memset(completar, '#', REGISTER_SIZE*sizeof(char));

    city currCity = {.name = NULL, .appearances = 1};

    // printf("d2.01\n");

    while( i!= num ){
        i++;
        int espacoUsado;


        // printf("d2.02\n");
        //fixed sixe
        estadoOrigem = (char*)calloc(3, sizeof(char));
        scanf("%s", estadoOrigem);
        // printf("%s", estadoOrigem);
        fwrite(estadoOrigem ,sizeof(char), 2, fp);

        // printf("d2.03\n");
        estadoDestino = (char*)calloc(3, sizeof(char));
        scanf("%s", estadoDestino);
        // printf("%s", estadoDestino);
        fwrite(estadoDestino ,sizeof(char), 2, fp);

        // printf("d2.04\n");
        scanf("%d", &Distancia);
        // printf ("%d", Distancia);
        getchar();
        fwrite(&Distancia ,sizeof(int),1, fp);

        // printf("d2.05\n");
        //variable size 
        int lenCO;
        lenCO= 0;
        cidadeOrigem = (char*)calloc(85, sizeof(char));
        scan_quote_string(cidadeOrigem);
        lenCO = strlen(cidadeOrigem);
        // printf("%s|", cidadeOrigem);
        fwrite(cidadeOrigem ,sizeof(char), lenCO, fp);
        fputc('|', fp);

        // printf("d2.06\n");
        int lenCD;
        lenCD = 0;
        cidadeDestino = (char*)calloc(85, sizeof(char));
        scan_quote_string(cidadeDestino);
        lenCD =  strlen(cidadeDestino);
        // printf("%s|", cidadeDestino);
        fwrite(cidadeDestino ,sizeof(char), lenCD , fp);
        fputc('|', fp);

        // printf("d2.06\n");
        int tV;
        tV = 0;
        tempoViagem = (char*)calloc(85, sizeof(char));
        scan_quote_string(tempoViagem);
        tV = strlen(tempoViagem);
        // printf("%s|\n", tempoViagem);
        fwrite(tempoViagem ,sizeof(char), tV, fp);
        fputc('|', fp);

        // printf("d2.07\n");
        espacoUsado = lenCD + lenCO + tV + 3;
        fwrite(completar,sizeof(char), VARIABLE_FIELD_SIZE - espacoUsado, fp);

        // printf("d2.08\n");
        currCity.name = cidadeOrigem;
        // printf("d2.081\n");
        hashInsert(hashTable, currCity, header);
        // printf("d2.082\n");
        currCity.name = cidadeDestino;
        // printf("d2.083\n");
        hashInsert(hashTable, currCity, header);
        header->numeroArestas = header->numeroArestas + 1;

        free(tempoViagem);
        free(estadoOrigem);
        free(estadoDestino);
    }
    overwriteFileHeader(fp, *header);
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
void saveRegister(FILE *fp, DataRegister reg){
    //delimiter symbol
    char delim = '|'; 
    // printf("d3.01\n");
    fseek(fp, reg.rrn*REGISTER_SIZE + HEADER_SIZE, SEEK_SET);
    // printf("d3.02\n");
    fwrite(reg.estadoOrigem, sizeof(char), FIXED_FIELD_SIZE, fp);
    // printf("d3.03\n");
    fwrite(reg.estadoDestino, sizeof(char), FIXED_FIELD_SIZE, fp);
    // printf("d3.04\n");
    fwrite(&(reg.distancia), sizeof(int), 1, fp);
    // printf("d3.05\n");
    fwrite(reg.cidadeOrigem, sizeof(char), strlen(reg.cidadeOrigem), fp);
    // printf("d3.06\n");
    fwrite(&(delim), sizeof(char), 1, fp);
    // printf("d3.07\n");
    fwrite(reg.cidadeDestino, sizeof(char), strlen(reg.cidadeDestino), fp);
    // printf("d3.08\n");
    fwrite(&(delim), sizeof(char), 1, fp);
    // printf("d3.09\n");
    if(reg.tempoViagem != NULL)
        fwrite(reg.tempoViagem, sizeof(char), strlen(reg.tempoViagem), fp);
    
    
    // printf("d3.10\n");
    fwrite(&(delim), sizeof(char), 1, fp);
    // printf("d3.11\n");
}

//Updating a field register (Function 7)
void updateRegister(FILE *fp, DataHeader header, int size, city* hashTable){
    //RRN Value
    int rrn = 0;
    //field identifier
    int fieldId = 0;
    //Field Name
    char *Field = (char*)calloc(REGISTER_SIZE, sizeof(char));
    //Value of a field
    char* value = (char*)calloc(REGISTER_SIZE, sizeof(char));

    city currCity = (city){.name = NULL, .appearances = 0};
    int wasHeaderModified = 0;

    DataRegister Reg;

    for(int n = 0; n < size; n++){
        // printf("d0\n");
        scanf("%d", &rrn);
        getchar();
        scanf("%s", Field);
        scan_quote_string(value);

        // printf("d1\n");

        fieldId = getFieldId(Field);
        Reg = getRegister(fp, rrn);
        if(Reg.rrn == FAILED)
            continue;

        // printf("d2 r|f|v|: %d|%s|%s|\n", rrn, Field, value);

        switch (fieldId) {
            case ESTADO_ORIGEM:
                Reg.estadoOrigem = value;
                break;
            case ESTADO_DESTINO:
                Reg.estadoDestino = value;
                break;
            case CIDADE_ORIGEM:
                currCity.name = Reg.cidadeOrigem;
                hashRemove(hashTable, currCity, &header);

                Reg.cidadeOrigem = value;

                currCity.name = value;
                hashInsert(hashTable, currCity, &header);
                wasHeaderModified = 1;
                break;
            case CIDADE_DESTINO:
                currCity.name = Reg.cidadeDestino;
                hashRemove(hashTable, currCity, &header);

                Reg.cidadeDestino = value;

                currCity.name = value;
                hashInsert(hashTable, currCity, &header);
                wasHeaderModified = 1;
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
        }
        // printf("d3\n");
        saveRegister(fp, Reg);
        // printf("d4\n");
    }
    if(wasHeaderModified)
        overwriteFileHeader(fp, header);
    //printDataFile(fp, header);
}

//defragmenting file
void defragmenter(FILE *fp, char *out, DataHeader header){
    FILE *file_read = fp;
    FILE *file_write = fopen(out, "rb+");
    DataRegister aux;

    if(file_write == NULL)
        return;    

    for(int i = 0, j = 0; i < header.maxRRN; i++){
        aux = getRegister(file_read, i);
        if(!isRegRemoved(aux)){
            aux.rrn = j;
            saveRegister(file_write, aux);
            j++;
        }
    }
    
    data(header.dataUltimaCompactacao);
    overwriteFileHeader(file_write, header);
    fclose(file_write);
}   


int recoverHashTable(city *hashTable, DataHeader *header){
    FILE *hashFile;
    hashFile = fopen("aux.bin", "rb");
    if(hashFile == NULL){
        return FAILED;
    }

    int i;    
    city currCity;
    char delim[] = {'|', '\0'};
    char *variableSizeData = (char*)malloc(REGISTER_SIZE * sizeof(char));

    for(i = 0; i < header->numeroVertices; i++){
        memset(variableSizeData, '\0', REGISTER_SIZE * sizeof(char));
        fgets(variableSizeData, REGISTER_SIZE, hashFile);

        currCity.name = (char*)calloc(VARIABLE_FIELD_SIZE, sizeof(char));
        strcpy(currCity.name, strtok(variableSizeData, delim));
        currCity.appearances = atoi(strtok(NULL, delim));

        hashInsert(hashTable, currCity, header);
    }
    return SUCESS;

}

void zeroFillHashTable(city* hashTable){
    int i = 0;
    for(i = 0; i < HASH_TABLE_SIZE; i++){
        hashTable[i] = (city){.name = NULL, .appearances = 0};
    }
}

city* createHashTable(FILE* fp, DataHeader *header){
    city *hashTable = (city*)calloc(HASH_TABLE_SIZE, sizeof(city));
    zeroFillHashTable(hashTable);

    if(header->status == '1' && recoverHashTable(hashTable, header) == SUCESS)
        return hashTable;
    
    fseek(fp, HEADER_SIZE, SEEK_SET);

    int i = 0;
    char delim[] = {'|', '\0'};
    char *variableSizeData = (char*)malloc(REGISTER_SIZE * sizeof(char));

    city cidadeOrigem;
    city cidadeDestino;
    cidadeOrigem.appearances = 1;
    cidadeDestino.appearances = 1;

    size_t aux = REGISTER_SIZE - VARIABLE_FIELD_SIZE;
	size_t seek = HEADER_SIZE;
    fseek(fp, seek, SEEK_SET);

    header->numeroVertices = 0;
    for(i = 0; i < header->maxRRN; i++){
        fseek(fp, seek + aux, SEEK_SET);
        memset(variableSizeData, '\0', REGISTER_SIZE * sizeof(char));
        fread(variableSizeData, sizeof(char), VARIABLE_FIELD_SIZE, fp);

        cidadeOrigem.name  = (char*)calloc(VARIABLE_FIELD_SIZE, sizeof(char));
        cidadeDestino.name = (char*)calloc(VARIABLE_FIELD_SIZE, sizeof(char));
        strcpy(cidadeOrigem.name, strtok(variableSizeData, delim));
        strcpy(cidadeDestino.name, strtok(NULL, delim));
        
        hashInsert(hashTable, cidadeOrigem, header);
        //if(strcmp(cidadeOrigem.name, cidadeDestino.name) != 0)
        hashInsert(hashTable, cidadeDestino, header);

        seek += REGISTER_SIZE;
    }

    return hashTable;
}


void saveHashTable(city *hashTable){
    FILE *output;
    output = fopen("aux.bin", "w+");
    printHashTable(hashTable, output);
}


DataHeader createEmptyHeader(){
    DataHeader header;
    header.maxRRN = 0;
    header.status = '1';
    header.fileLength = 0;
    header.numeroArestas = 0;
    header.numeroVertices = 0;
    header.dataUltimaCompactacao = (char*)malloc(11 * sizeof(char));
    memset(header.dataUltimaCompactacao, '#', 10 * sizeof(char));
    header.dataUltimaCompactacao[10] = '\0';

    return header;
}

DataHeader generateHeader(city* hashTable){
    int arestas = 0;
    int vertices = 0;
    int i = 0;
    for(i = 0; i < HASH_TABLE_SIZE; i++){
        if(hashTable[i].name != NULL){
            arestas += hashTable[i].appearances;
            vertices++;
        }
    }
    DataHeader header = createEmptyHeader();
    header.numeroArestas = arestas;
    header.numeroVertices = vertices;

    return header;
}  

void resetUltimaCompactacao(DataHeader *header){
    memset(header->dataUltimaCompactacao, '#', 10);
}

int main(){
    int command = -1;
    char *args = (char*)malloc(INPUT_LIMIT * sizeof(char));
    memset(args, '\0', INPUT_LIMIT * sizeof(char));

    scanf(" %d ", &command);
    fgets(args, INPUT_LIMIT, stdin);
    args[strlen(args) - 1] = '\0';
    // printf("command: %d, args: %s!\n", command, args);

    int i = 0;
    FILE *fp = NULL;
    FILE *csvP = NULL;
    char Delim[] = {' ', '\0'};
    DataHeader header;
    header = getHeader(fp);
    char *csvName;
    char *filename;
    char *outputFile;
    city *hashTable;
    char *Field = NULL;
    char *Value = NULL;

  

    switch (command){
        case 1:
            csvName = (char*)calloc(INPUT_LIMIT, sizeof(char));
            strcpy(csvName, strtok(args, Delim));
            filename = strtok(NULL, Delim);

            fp = fopen(filename, "wb");
            if(fp == NULL) {
                fprintf(stdout, "Falha no carregamento do arquivo.\n");
                return 0;
            }

            header = createEmptyHeader();
            hashTable = createHashTable(fp, &header);

            csvP = fopen(csvName, "rb");
            if(csvP == NULL){
                fprintf(stdout, "Falha no carregamento do arquivo.\n");
                return 0;
            }               

            DealingCSV (csvP,fp, csvName, hashTable, &header);
            
            fclose (csvP);
            overwriteFileHeader(fp, header);
            fclose(fp);

            binarioNaTela1(filename);

            

            break;

        case 2:
            filename = args;
            if(getBinaryFile(filename, &fp, &header, "rb+") == FAILED)
                return 0;

            printDataFile(fp, header);    
            fclose(fp);        
            break;

        case 3:
            filename = strtok(args, Delim);
            if(getBinaryFile(filename, &fp, &header, "rb+") == FAILED)
                return 0;
            searchByField(fp, &header, strtok(NULL, Delim), strtok(NULL, Delim), SEARCH_FILES, NULL);
            fclose(fp);
            break;

        case 4:
            filename = strtok(args, Delim);
            if(getBinaryFile(filename, &fp, &header, "rb+") == FAILED)
                return 0;
            printRegister(getRegister(fp, atoi(strtok(NULL, Delim))));
            fclose(fp);
            break;

        case 5:
            filename = (char*)calloc(INPUT_LIMIT, sizeof(char));
            strcpy(filename, strtok(args, Delim));
            i = atoi(strtok(NULL, Delim));

            if(getBinaryFile(filename, &fp, &header, "rb+") == FAILED)
                return 0;
            
            hashTable = createHashTable(fp, &header);

            for(; i > 0; i--){
                fgets(args, INPUT_LIMIT, stdin);
                args[strlen(args) - 1] = '\0';
                Field = strtok(args, Delim);
                Value = strtok(NULL, "\n\0");
                Value = strtok(Value, "\"\0");
                if(strcmp(Value, "NULO") == 0)
                    Value = NULL;

                searchByField(fp, &header, Value, Field, REMOVE_FILES, hashTable);
            }      

            fclose(fp); 
            binarioNaTela1(filename);
            break;

        case 6:
            filename = (char*)calloc(INPUT_LIMIT, sizeof(char));
            strcpy(filename, strtok(args, Delim));
            i = atoi(strtok(NULL, Delim));

            if(getBinaryFile(filename, &fp, &header, "rb+") == FAILED)
                return 0;
            fseek(fp, 0L, SEEK_END);

            hashTable = createHashTable(fp, &header);
            Insert(fp, i, hashTable, &header, filename);   
            fclose(fp);

            binarioNaTela1(filename);
            break;

        case 7:
            filename = (char*)calloc(INPUT_LIMIT, sizeof(char));
            strcpy(filename, strtok(args, Delim));
            i = atoi(strtok(NULL, Delim));
            
            if(getBinaryFile(filename, &fp, &header, "rb+") == FAILED)
                return 0;
            fseek(fp, 0L, SEEK_SET);
            
            hashTable = createHashTable(fp, &header);
            updateRegister(fp, header, i, hashTable);
            fclose(fp);

            binarioNaTela1(filename);
            break;

        case 8:
            filename = (char*)calloc(INPUT_LIMIT, sizeof(char));
            strcpy(filename, strtok(args, Delim));

            outputFile = (char*)calloc(INPUT_LIMIT, sizeof(char));
            strcpy(outputFile, strtok(NULL, Delim));

            fp = fopen(filename, "rb");
            if(fp == NULL) {
                fprintf(stdout, "Falha no carregamento do arquivo.\n");
                return 0;
            }

            header = getHeader(fp);
            if(header.status == '0'){
                fprintf(stdout, "Falha no carregamento do arquivo.\n");
                return 0;
            }

            defragmenter(fp, outputFile, header);
            fclose(fp);

            binarioNaTela1(outputFile);
            break;
        
        case 9:
            filename = args;
            binarioNaTela1(filename);    
            fclose(fp);        
            break;


        default:
            break;
    }
    
    free(args);
    return 0;
}
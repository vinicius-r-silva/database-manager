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
#define SEARCH_FILES 1
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
    fwrite(&(header.dataUltimaCompactacao), sizeof(char), 10, fp);
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
        printf("Registro inexistente.\n");
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
    while (currRRN < header.numeroArestas){           //keeps reading the file while there is data available
        currReg = getRegister(fp, currRRN);    //get the next register in the file and print it
        if(!isRegRemoved(currReg)){
            printRegister(currReg);
            thereIsData = 1;
        }

        freeRegister(currReg);
        currRRN++;
    }
    if(!thereIsData)
        printf("Registro inexistente.\n");
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
        index % HASH_TABLE_SIZE;
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
        (header->numeroVertices)--;
    }
    else{
        hashTable[index].appearances--;
    }
}

//search a certain value of a field 
void searchByField(FILE *fp, DataHeader *header, char* value, char* field, int action, city *hashTable){
    
    printf("d-6\n");
    int rrn = 0;
    DataRegister reg;
    int fieldId = getFieldId(field);
    printf("d-7\n");
    printf("1 -field|value: |%s|%s|\n", field, value);

    if(value[0] == '\"')
        value = strtok(value, "\"");

    printf("2 -field|value: |%s|%s|\n", field, value);

    int registersRemoved = 0;
    for(rrn = 0; rrn < header->numeroArestas; rrn++){
        reg = getRegister(fp, rrn);
        if(isRegRemoved(reg))
            continue;
            
        if(compareFieldValue(reg, value, fieldId))
            if(action == SEARCH_FILES)
                printRegister(reg);
            else if(action == REMOVE_FILES){
                logicalDeleteReg(fp, reg);
                city Origem = {.name = reg.cidadeOrigem, .appearances = 1};
                city Destino = {.name = reg.cidadeDestino, .appearances = 1};
                hashRemove(hashTable, Origem, header);
                hashRemove(hashTable, Destino, header);

                (header->numeroArestas)--;
                registersRemoved = 1;
            }
    }

    if(registersRemoved)
        overwriteFileHeader(fp, *header);
}


void makingRegister(char* LINHA, FILE *newFile){
    printf("makingRegister abriu \n");

        char* temp;
        temp =  (char*)calloc(85, sizeof(char));

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

    //variable Size -- cidadeDestino    
    temp = strtok(NULL, delim);
        int lenCD;
        lenCD= 0;
        lenCD = strlen(temp);
        fwrite(temp, sizeof(char), lenCD, newFile);
        fputc('|', newFile );

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
        fwrite(completar,sizeof(char), espacoUsado, newFile);

} 

//converting csv
void DealingCSV (char* name ){

    char* linha;
    linha = (char*) calloc(85,sizeof(char));

    FILE *ptr = fopen(name, "r" );

     fseek(ptr, 77, SEEK_SET);

        
        FILE *newFile;
        newFile = fopen ("arquivoGerado.bin", "wb");

     while(fgets(linha, REGISTER_SIZE*sizeof(char), ptr) != NULL){
         

         printf("%s", linha);

         makingRegister(linha, newFile);
         memset(linha, '\0',85*sizeof(char) );
    }
        fclose (ptr);
        binarioNaTela1("arquivoGerado.bin");
    }





//Insert function   
void Insert (char *name, int num){
   
   //opening file
    FILE *ptr;
    ptr =  fopen(name, "ab+" );

    int espacoTotal;
        espacoTotal = 0;
    int tmp;
        tmp = 0;

    if(ptr == NULL){
        printf("Falha no processamento do arquivo.");
        return;
    }else{

        int i = 0;
         
        char* completar;
        completar = (char*)malloc (REGISTER_SIZE*sizeof(char));
        memset(completar, '#', REGISTER_SIZE*sizeof(char));

        while( i!= num ){
             i++;
          

            int espacoUsado;

            //fixed sixe
            char*estadoOrigem;
            estadoOrigem = (char*)calloc(3, sizeof(char));
            scanf("%s", estadoOrigem);
            printf("%s", estadoOrigem);
            fwrite(estadoOrigem ,sizeof(char), 2, ptr);
            free(estadoOrigem);

            char*estadoDestino;
            estadoDestino = (char*)calloc(3, sizeof(char));
            scanf("%s", estadoDestino);
            printf("%s", estadoDestino);
            fwrite(estadoDestino ,sizeof(char), 2, ptr);
            free(estadoDestino);
            

            int Distancia;
            scanf("%d", &Distancia);
            printf ("%d", Distancia);
            getchar();
            fwrite(&Distancia ,sizeof(int),1, ptr);
            
            //variable size 
            char*cidadeOrigem;
            int lenCO;
            lenCO= 0;
            cidadeOrigem = (char*)calloc(85, sizeof(char));
            scan_quote_string(cidadeOrigem);
            lenCO = strlen(cidadeOrigem);
            fwrite(cidadeOrigem ,sizeof(char), lenCO, ptr);
            fputc('|', ptr);
            free(cidadeOrigem);

            char*cidadeDestino;
            int lenCD;
            lenCD = 0;
            cidadeDestino = (char*)calloc(85, sizeof(char));
            scan_quote_string(cidadeDestino);
            lenCD =  strlen(cidadeDestino);
            fwrite(cidadeDestino ,sizeof(char), lenCD , ptr);
            fputc('|', ptr);
            free(cidadeDestino);

            char*tempoViagem;
            int tV;
            tV = 0;
            tempoViagem = (char*)calloc(85, sizeof(char));
            scan_quote_string(tempoViagem);
            tV = strlen(tempoViagem);
            fwrite(tempoViagem ,sizeof(char), tV, ptr);
            fputc('|', ptr);
            free(tempoViagem);

            espacoUsado = lenCD + lenCO + tV + 3;
            tmp = espacoUsado;

        }

        espacoTotal = espacoTotal + tmp;

        fwrite(completar,sizeof(char), espacoTotal, ptr);


        
        fclose (ptr);
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
    fwrite(&reg.distancia, sizeof(__int16_t), 1*sizeof(__int16_t), fp);
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

void hashInsert(city *hashTable, city item){
    int index = hashSearch(hashTable, item);

    if(hashTable[index].name == NULL){
        hashTable[index] = item;
        hashTable[index].appearances = item.appearances;
    }
    else{
        free(item.name);
        hashTable[index].appearances++;
    }
}


int recoverHashTable(city *hashTable, DataHeader header){
    FILE *hashFile;
    hashFile = fopen("aux.bin", "rb");
    if(hashFile == NULL){
        return FAILED;
    }

    int i;    
    city currCity;
    char delim[] = {'|', '\0'};
    char *variableSizeData = (char*)malloc(REGISTER_SIZE * sizeof(char));

    for(i = 0; i < header.numeroVertices; i++){
        memset(variableSizeData, '\0', REGISTER_SIZE * sizeof(char));
        fgets(variableSizeData, REGISTER_SIZE, hashFile);

        currCity.name = (char*)calloc(VARIABLE_FIELD_SIZE, sizeof(char));
        strcpy(currCity.name, strtok(variableSizeData, delim));
        currCity.appearances = atoi(strtok(NULL, delim));

        hashInsert(hashTable, currCity);
    }
    return SUCESS;

}

city* createHashTable(FILE* fp, DataHeader header){
    city *hashTable = (city*)calloc(HASH_TABLE_SIZE, sizeof(city));
    if(header.status == '1' && recoverHashTable(hashTable, header) == SUCESS)
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

    for(i = 0; i < header.numeroArestas; i++){
        fseek(fp, seek + aux, SEEK_SET);
        memset(variableSizeData, '\0', REGISTER_SIZE * sizeof(char));
        fread(variableSizeData, sizeof(char), VARIABLE_FIELD_SIZE, fp);

        cidadeOrigem.name  = (char*)calloc(VARIABLE_FIELD_SIZE, sizeof(char));
        cidadeDestino.name = (char*)calloc(VARIABLE_FIELD_SIZE, sizeof(char));
        strcpy(cidadeOrigem.name, strtok(variableSizeData, delim));
        strcpy(cidadeDestino.name, strtok(NULL, delim));
        
        hashInsert(hashTable, cidadeOrigem);
        //if(strcmp(cidadeOrigem.name, cidadeDestino.name) != 0)
        hashInsert(hashTable, cidadeDestino);

        seek += REGISTER_SIZE;
    }

    return hashTable;
}

void printHashTable(city *hashTable, FILE *outputStream){
    int i = 0;
    for(i = 0; i < HASH_TABLE_SIZE; i++){
        if(hashTable[i].name != NULL)
            fprintf(outputStream, "%s|%d\n", hashTable[i].name, hashTable[i].appearances);
    }
}

void saveHashTable(city *hashTable){
    FILE *output;
    output = fopen("aux.bin", "w+");
    printHashTable(hashTable, output);
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
    DataHeader header;
    header.status = '1';
    header.numeroArestas = arestas;
    header.numeroVertices = vertices;
    header.dataUltimaCompactacao = (char*)malloc(10 * sizeof(char));
    memset(header.dataUltimaCompactacao, '#', 10 * sizeof(char));

    return header;
}  

int getBinaryFile(char *filename, FILE **fp, DataHeader *header){
    FILE *tempFP = fopen(filename, "rb+");
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

int main(){
    int command = -1;
    char *args = (char*)malloc(INPUT_LIMIT * sizeof(char));
    memset(args, '\0', INPUT_LIMIT * sizeof(char));

    scanf(" %d ", &command);
    fgets(args, INPUT_LIMIT, stdin);
    args[strlen(args) - 1] = '\0';
    //printf("command: %d, args: %s!\n", command, args);

    int i = 0;
    FILE *fp = NULL;
    char Delim[] = {' ', '\0'};
    DataHeader header;
    header = getHeader(fp);
    char *filename;
    city *hashTable;

    // char *nomeCVS = "caso02.csv";
    // DealingCSV(nomeCVS);

    //Insert("caso02.bin", 2);


    //printHeader(header);

    switch (command){
        case 1:
            
            fclose(fp);
            break;

        case 2:
            filename = args;
            if(getBinaryFile(filename, &fp, &header) == FAILED)
                return 0;
            printDataFile(fp, header);    
            fclose(fp);        
            break;

        case 3:
            filename = strtok(args, Delim);
            if(getBinaryFile(filename, &fp, &header) == FAILED)
                return 0;
            searchByField(fp, &header, strtok(NULL, Delim), strtok(NULL, Delim), SEARCH_FILES, NULL);
            fclose(fp);
            break;

        case 4:
            filename = strtok(args, Delim);
            if(getBinaryFile(filename, &fp, &header) == FAILED)
                return 0;
            printRegister(getRegister(fp, atoi(strtok(NULL, Delim))));
            fclose(fp);
            break;

        case 5:
            filename = strtok(args, Delim);
            if(getBinaryFile(filename, &fp, &header) == FAILED)
                return 0;
            
            hashTable = createHashTable(fp, header);

            for(i = atoi(strtok(NULL, Delim)); i > 0; i--){
                fgets(args, INPUT_LIMIT, stdin);
                searchByField(fp, &header, strtok(args, Delim), strtok(NULL, Delim), REMOVE_FILES, hashTable);
            }      
            fclose(fp);
            binarioNaTela1(filename);
            break;

        case 6:
            fclose(fp);
            break;

        case 7:
            fclose(fp);
            break;

        case 8:
            fclose(fp);
            break;
        
        default:
            break;
    }

    //printDataFile(fp, header);

    //printf("\n\n");
    //searchByField(fp, header, "distancia", "150", SEARCH_FILES);
    //updateRegister(fp, header, 2);

    // city *hashTable = createHashTable(fp, header);
    // printHashTable(hashTable, stdout);
    // saveHashTable(hashTable);
    //free(args);
    return 0;
}
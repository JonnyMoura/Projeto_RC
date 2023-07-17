#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <pthread.h>
#include <time.h>
#include <strings.h>
#include <signal.h>

#define MAX_BUFFER 1024
#define TAM 254

int TCP_PORT;
int UDP_PORT;

FILE *fp;

int linhas;

typedef struct{
    char username[MAX_BUFFER];
    char password[MAX_BUFFER];
    char type[MAX_BUFFER];
}User;

typedef struct{
    char texto[MAX_BUFFER];
} Noticia;

typedef struct{
    char id[MAX_BUFFER];
    char titulo[MAX_BUFFER];
    Noticia noticias[TAM];
} Topico;

typedef struct{
    Topico topicos[TAM];
    User users[TAM];
}SHM;

#define SEM_SHM "/SHM_Semaphore"
sem_t *sem_SHM;
int shmid;

//sem_wait(sem_SHM);
//sem_post(sem_SHM);

void erro(char *s){
    perror(s);
    exit(1);
}

SHM *create_SHM(){
    SHM *shm;

    sem_unlink(SEM_SHM);

    sem_SHM = sem_open(SEM_SHM, O_CREAT | O_EXCL, 0666, 1);
    if (sem_SHM == SEM_FAILED){
        perror("Error em sem_open");
    }

    printf("Semafro da memoria partilhada criado\n");

    sem_wait(sem_SHM);

    if ((shmid = shmget(1235, sizeof(SHM), IPC_CREAT | 0766)) < 0){
        perror("Error in shmget.");
    }
    if ((shm = (SHM *)shmat(shmid, NULL, 0)) == (SHM *)-1){
        perror("Error in shmat.");
    }

    for(int i = 0; i < TAM; i++){
        strcpy(shm->users[i].username, "");
        strcpy(shm->users[i].password, "");
        strcpy(shm->users[i].type, "");
    }

    for(int i = 0; i < TAM; i++){
        strcpy(shm->topicos[i].id, "");
        strcpy(shm->topicos[i].titulo, "");
        for(int j = 0; j < TAM; j++){
            strcpy(shm->topicos[i].noticias[j].texto, "");
        }
    }
    
    // Liberta o aceco a shared memory
    sem_post(sem_SHM);

    printf("Memoria partilhada criada.\n");
    return shm;
}

// Abre uma shared memory ja criada
SHM *attach_SHM(){
    SHM *shm;
    if ((shm = shmat(shmid, NULL, 0)) == (SHM *)-1){
        perror("Erro ao associar a shared memory.");
    }
    return shm;
}

void detach_SHM(SHM *shm){
    if (shmdt(shm) == -1){
        perror("Erro in shmdt.");
    }
    perror("SHM desacociada do processo.");
}


void delete_SHM(){
    if (shmctl(shmid, IPC_RMID, NULL) == -1){
        perror("Erro in shmctl.");
    }
    perror("SHM eliminada.");
}

void sigint(int signum){
    printf("EXIT\n");
    exit(0);
}

void read_users_file(char *file_name){
    char linha[MAX_BUFFER];
    linhas = 0;

    SHM *shm = attach_SHM();

    FILE *fp;

    if ((fp = fopen("users.txt", "r")) == NULL){
        perror("Erro ao abrir o arquivo!\n");
        exit(0);
    }

    int i = 0;
    while (fgets(linha, sizeof(linha), fp) != NULL){
        linha[strcspn(linha, "\n")] = '\0';

        char *username = strtok(linha, ";");
        char *password = strtok(NULL, ";");
        char *type = strtok(NULL, ";");

        strcpy(shm->users[i].username, username);
        strcpy(shm->users[i].password, password);
        strcpy(shm->users[i].type, type);

        i++;
    }

    fclose(fp);
}

int write_users_file(char *user, char *password, char *type){
    FILE *fp = fopen("config_file", "a");
    if (fp == NULL){
        printf("Erro ao abrir o arquivo!\n");
        return 0;
    }

    fprintf(fp, "%s;%s;%s\n", user, password, type);

    fclose(fp);

    return 1;
}

int admin_server(int UDP_PORT){
    struct sockaddr_in si_minha, si_outra;

    int server_fd, recv_len, nread;
    socklen_t slen = sizeof(si_outra);
    char buf[MAX_BUFFER];
    char buf_send[MAX_BUFFER];
    char user_name[MAX_BUFFER], password_buf[MAX_BUFFER];

    SHM *shm = attach_SHM();

    // Cria um socket para recepção de pacotes UDP
    if ((server_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        erro("Erro na criação do socket");
    }

    // Preenchimento da socket address structure
    si_minha.sin_family = AF_INET;
    si_minha.sin_port = htons(UDP_PORT);
    si_minha.sin_addr.s_addr = htonl(INADDR_ANY);

    // Associa o socket à informação de endereço
    if (bind(server_fd, (struct sockaddr *)&si_minha, sizeof(si_minha)) == -1)
    {
        erro("Erro no bind");
    }

    while (1)
    {
        char user_aux[128];
        char pass_aux[128];
        int flag = 0;
        int flag_error = 0;
        int j = 0;

        if ((recv_len = recvfrom(server_fd, user_name, MAX_BUFFER, 0, (struct sockaddr *)&si_outra, &slen)) == -1)
        {
            erro("Erro no recvfrom");
        }

        while (recv_len != 1)
        {
            continue;
        }

        strcpy(buf_send, "Insira o username:");
        sendto(server_fd, buf_send, strlen(buf_send), 0, (struct sockaddr *)&si_outra, slen);

        if ((recv_len = recvfrom(server_fd, user_name, MAX_BUFFER, 0, (struct sockaddr *)&si_outra, &slen)) == -1)
        {
            erro("Erro no recvfrom");
        }

        fflush(stdout);
       

        user_name[recv_len] = '\0';
        j = 0;
        for (int i = 0; user_name[i] != '\0'; i++)
        {
            if (user_name[i] != ' ' && user_name[i] != '\n')
            {
                user_aux[j++] = user_name[i];
            }
        }
        user_aux[j] = '\0';
        fflush(stdin);
        fflush(stdout);
        while (recv_len <= 1)
        {
            if ((recv_len = recvfrom(server_fd, user_name, MAX_BUFFER, 0, (struct sockaddr *)&si_outra, &slen)) == -1){
                erro("Erro no recvfrom");
            }

            user_name[recv_len] = '\0';
            j = 0;

            for (int i = 0; user_name[i] != '\0'; i++){
                if (user_name[i] != ' ' && user_name[i] != '\n'){
                    user_aux[j++] = user_name[i];
                }
            }
            user_aux[j] = '\0';
        }
        fflush(stdin);
        fflush(stdout);

        strcpy(buf_send, "Insira a password:");
        sendto(server_fd, buf_send, strlen(buf_send), 0, (struct sockaddr *)&si_outra, slen);

        if ((recv_len = recvfrom(server_fd, password_buf, MAX_BUFFER, 0, (struct sockaddr *)&si_outra, (socklen_t *)&slen)) == -1){
            erro("Erro no recvfrom");
        }

        fflush(stdin);
        fflush(stdout);
        password_buf[recv_len] = '\0';
        j = 0;
        for (int i = 0; password_buf[i] != '\0'; i++){
            if (password_buf[i] != ' ' && password_buf[i] != '\n'){
                pass_aux[j++] = password_buf[i];
            }
        }
        pass_aux[j] = '\0';

        while (recv_len <= 1){
            if ((recv_len = recvfrom(server_fd, password_buf, MAX_BUFFER, 0, (struct sockaddr *)&si_outra, (socklen_t *)&slen)) == -1){
                erro("Erro no recvfrom");
            }

            fflush(stdin);
            fflush(stdout);

            password_buf[recv_len] = '\0';
            j = 0;

            for (int i = 0; password_buf[i] != '\0'; i++){
                if (password_buf[i] != ' ' && password_buf[i] != '\n'){
                    pass_aux[j++] = password_buf[i];
                }
            }
            pass_aux[j] = '\0';
        }
        
        sem_wait(sem_SHM);
        for (int i = 0; i < TAM; i++){
            if (strcmp(user_aux, shm->users[i].username) == 0 && strcmp(pass_aux, shm->users[i].password) == 0){
                if (strcmp(shm->users[i].type, "administrator") == 0)
                {
                    flag = 1;
                    flag_error = 1;
                }
                else if (strcmp(shm->users[i].type, "leitor") == 0)
                {
                    flag = 2;
                    flag_error = 1;
                }
                else if (strcmp(shm->users[i].type, "jornalista") == 0)
                {
                    flag = 3;
                    flag_error = 1;
                }
            }
        }
        sem_post(sem_SHM);
        fflush(stdout);
        fflush(stdin);

        

        if (flag_error == 0)
        {
            strcpy(buf_send, "Utlizador nao encontrado\n");
            sendto(server_fd, buf_send, strlen(buf_send), 0, (struct sockaddr *)&si_outra, slen);
            fflush(stdin);
            fflush(stdout);
        }
        else
        {
            char buf_aux[128];
            j = 0;

            if ((recv_len = recvfrom(server_fd, buf, MAX_BUFFER, 0, (struct sockaddr *)&si_outra, (socklen_t *)&slen)) == -1)
            {
                erro("Erro no recvfrom");
            }

            buf[recv_len] = '\0';
            fflush(stdin);
            fflush(stdout);

            for (int i = 0; buf[i] != '\0'; i++)
            {
                if (buf[i] != ' ' && buf[i] != '\n')
                {
                    buf_aux[j++] = buf[i];
                }
            }

            buf_aux[j] = '\0';

            if (flag == 1)
            {
                if (strcmp(buf_aux, "ADD_USER") == 0){
                    fflush(stdin);
                    fflush(stdout);
                    printf("Recebi uma mensagem do sistema com o endereço %s e o porto %d\n", inet_ntoa(si_outra.sin_addr), ntohs(si_outra.sin_port));
                    printf("Conteúdo da mensagem: %s\n", buf_aux);
                    strcpy(buf_send, "ADD_USER recebido");
                    sendto(server_fd, buf_send, strlen(buf_send), 0, (struct sockaddr *)&si_outra, slen);
                    fflush(stdin);
                    fflush(stdout);

                    strcpy(buf_send, "Insira o username:");
                    sendto(server_fd, buf_send, strlen(buf_send), 0, (struct sockaddr *)&si_outra, slen);
                    fflush(stdin);
                    fflush(stdout);

                    if ((recv_len = recvfrom(server_fd, user_aux, MAX_BUFFER, 0, (struct sockaddr *)&si_outra, (socklen_t *)&slen)) == -1){
                        erro("Erro no recvfrom");
                    }
                    fflush(stdin);
                    fflush(stdout);
                  

                    strcpy(buf_send, "Insira a password:");
                    sendto(server_fd, buf_send, strlen(buf_send), 0, (struct sockaddr *)&si_outra, slen);

                    if ((recv_len = recvfrom(server_fd, pass_aux, MAX_BUFFER, 0, (struct sockaddr *)&si_outra, (socklen_t *)&slen)) == -1){
                        erro("Erro no recvfrom");
                    }

                    fflush(stdin);
                    fflush(stdout);

              

                    

                    
                   while (1) {
                        strcpy(buf_send, "Insira o tipo:\n");
                        sendto(server_fd, buf_send, strlen(buf_send), 0, (struct sockaddr *)&si_outra, slen);
                        nread = read(server_fd, buf_aux, sizeof(buf_aux));
        
                        buf_aux[nread-1] = '\0';  // Adiciona o terminador de string

                        if (strcmp(buf_aux, "leitor") == 0 || strcmp(buf_aux, "jornalista") == 0 || strcmp(buf_aux, "administrator") == 0) {
                                break;  // Tipo de usuário válido
                        }
                        fflush(stdout);
                        strcpy(buf_send, "Tipo inválido\n");
                        sendto(server_fd, buf_send, strlen(buf_send), 0, (struct sockaddr *)&si_outra, slen);
      
                    }
            
                    fflush(stdin);
                    fflush(stdout);
                    sem_wait(sem_SHM);
                    write_users_file(user_aux, pass_aux, buf_aux);
                    sem_post(sem_SHM);
                    fflush(stdin);
                    fflush(stdout);

                    strcpy(buf_send, "Utlizador adicionado\n");
                    sendto(server_fd, buf_send, strlen(buf_send), 0, (struct sockaddr *)&si_outra, slen);
                    fflush(stdin);
                    fflush(stdout);
                }
                if (strcmp(buf_aux, "DEL") == 0){
                    fflush(stdin);
                    fflush(stdout);
                    printf("Recebi uma mensagem do sistema com o endereço %s e o porto %d\n", inet_ntoa(si_outra.sin_addr), ntohs(si_outra.sin_port));
                    printf("Conteúdo da mensagem: %s\n", buf_aux);
                    strcpy(buf_send, "DEL recebido");
                    sendto(server_fd, buf_send, strlen(buf_send), 0, (struct sockaddr *)&si_outra, slen);
                    sem_wait(sem_SHM);
                    for (int i = 0; i < TAM; i++){
                        if (strcmp(shm->users[i].username, user_aux) == 0){
                            strcpy(shm->users[i].username, "");
                            strcpy(shm->users[i].password, "");
                            strcpy(shm->users[i].type, "");
                        }
                    }
                    sem_post(sem_SHM);
                    fflush(stdin);
                    fflush(stdout);
                    
                    FILE *fp = fopen("config_file", "w");
                    if (fp == NULL){
                        printf("Erro ao abrir o arquivo!\n");
                        return 0;
                    }
                    sem_wait(sem_SHM);
                    for (int i = 0; i < TAM; i++){
                        if(strcmp(shm->users[i].username, "") != 0){
                            fprintf(fp, "%s;%s;%s\n", shm->users[i].username, shm->users[i].password, shm->users[i].type);
                        }
                    }
                    sem_post(sem_SHM);

                    fflush(stdout);
                    fflush(stdin);
                    
                    fclose(fp);

                    strcpy(buf_send, "Utlizador eliminado\n");
                    sendto(server_fd, buf_send, strlen(buf_send), 0, (struct sockaddr *)&si_outra, slen);
                    fflush(stdin);
                    fflush(stdout);
                }
                if (strcmp(buf_aux, "LIST") == 0){
                    fflush(stdin);
                    fflush(stdout);
                    printf("Recebi uma mensagem do sistema com o endereço %s e o porto %d\n", inet_ntoa(si_outra.sin_addr), ntohs(si_outra.sin_port));
                    printf("Conteúdo da mensagem: %s\n", buf_aux);
                    strcpy(buf_send, "LIST recebido");
                    sendto(server_fd, buf_send, strlen(buf_send), 0, (struct sockaddr *)&si_outra, slen);
                    sem_wait(sem_SHM);
                    for (int i = 0; i < TAM; i++){
                        if (strcmp(shm->users[i].username, "") != 0){
                            sprintf(buf_send,"%s\n",shm->users[i].username);

                            sendto(server_fd, buf_send, strlen(buf_send), 0, (struct sockaddr *)&si_outra, slen);
                        }
                    }
                    sem_post(sem_SHM);
                    fflush(stdout);
                    fflush(stdin);
                    
                }
                if (strcmp(buf_aux, "QUIT") == 0){
                    fflush(stdin);
                    fflush(stdout);
                    printf("Recebi uma mensagem do sistema com o endereço %s e o porto %d\n", inet_ntoa(si_outra.sin_addr), ntohs(si_outra.sin_port));
                    printf("Conteúdo da mensagem: %s\n", buf_aux);
                    strcpy(buf_send, "QUIT recebido");
                    sendto(server_fd, buf_send, strlen(buf_send), 0, (struct sockaddr *)&si_outra, slen);
                    flag = 0;
                    fflush(stdin);
                    fflush(stdout);
                }
                if (strcmp(buf_aux, "QUIT_SERVER") == 0){
                    fflush(stdin);
                    fflush(stdout);
                    printf("Recebi uma mensagem do sistema com o endereço %s e o porto %d\n", inet_ntoa(si_outra.sin_addr), ntohs(si_outra.sin_port));
                    printf("Conteúdo da mensagem: %s\n", buf_aux);
                    strcpy(buf_send, "QUIT_SERVER recebido");
                    sendto(server_fd, buf_send, strlen(buf_send), 0, (struct sockaddr *)&si_outra, slen);
                    flag = 0;
                    fflush(stdin);
                    fflush(stdout);
                    exit(0);
                }
            }
        }
    }
    exit(0);
}

int multicast_server(char id[MAX_BUFFER]){
    int addr_len, sock;
    struct sockaddr_in addr;
    struct ip_mreq mreq;

    SHM *shm = attach_SHM();
    char buffer[MAX_BUFFER] = ""; 

    if(fork() == 0){
        if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
            perror("Socket");
            exit(1);
        }

        int multicastTTL = 255;
        if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, (void *)&multicastTTL, sizeof(multicastTTL)) < 0){
            perror("socket opt");
            exit(1);
        }

        bzero((char *)&addr, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons(6000 + atoi(id));
        addr_len = sizeof(addr);
        while (1){
            int i = 0;
            sleep(10);
            sem_wait(sem_SHM);
            //error
            for(int i = 0; i < TAM; i++){
                if(strcmp(shm->topicos[i].id, id) == 0){
                    for(int j = 0; j < TAM; j++){
                        if(strcmp(shm->topicos[i].noticias[j].texto, "") != 0){
                            strcat(buffer, shm->topicos[i].noticias[j].texto);
                        }
                    }
                } 
            }
            sem_post(sem_SHM);
            int n = sendto(sock, buffer, strlen(buffer) + 1, 0, (struct sockaddr *)&addr, addr_len);
            fflush(stdout);
            fflush(stdin);

            if (n < 0){
                perror("sendto");
                exit(1);
            }
            strcpy(buffer, "");
        }
    }
}

int client_server(int client){
    bool find = false;
    int n_clients = 0;
    char type[MAX_BUFFER];
    char username[MAX_BUFFER];
    char password[MAX_BUFFER];
    char bufferEnvio[MAX_BUFFER];
    char buffer[MAX_BUFFER];
    int nread;

    strcpy(bufferEnvio, "Conectou-se ao servidor\n");
    write(client, bufferEnvio, 1 + strlen(bufferEnvio));
    fflush(stdout);

    SHM *shm = attach_SHM();

    while (!find){
        nread = read(client, username, sizeof(username));
        username[nread - 1] = '\0';

        nread = read(client, password, sizeof(password));
        password[nread - 1] = '\0';

        int i = 0;
        sem_wait(sem_SHM);
        while(strcmp(shm->users[i].username, "") != 0){
            if (strcmp(shm->users[i].username, username) == 0 && strcmp(shm->users[i].password, password) == 0){
                strcpy(type, shm->users[i].type);
                find = true;
                strcpy(bufferEnvio, type);
                write(client, bufferEnvio, 1 + strlen(bufferEnvio));
                fflush(stdout);
                n_clients++;
                break;
            }
            i++;
        }
        sem_post(sem_SHM);


        if (!find){
            write(client, "X", 1 + strlen("X"));
            fflush(stdout);
        }
    }

    while (1){
        if (strcmp(type, "leitor") == 0 ){
            nread = read(client, buffer, sizeof(buffer));
            buffer[nread-1] = '\0';
            if (strcmp(buffer, "LIST_TOPICS") == 0){
                strcpy(bufferEnvio, "Recebido");
                write(client, bufferEnvio, 1 + strlen(bufferEnvio));
                fflush(stdout);

                char lista_topicos[MAX_BUFFER] = "";
                sem_wait(sem_SHM);
                for(int i = 0; i < TAM; i++){
                    if(strcmp(shm->topicos[i].id, "") != 0){
                        strcat(lista_topicos, shm->topicos[i].titulo);
                        strcat(lista_topicos, " ");
                    }
                }
                sem_post(sem_SHM);
                write(client, lista_topicos, 1 + strlen(lista_topicos));\
            }

            else if (strcmp(buffer, "SUBSCRIBE_TOPIC") == 0){
                char recebido[MAX_BUFFER];
                char var[MAX_BUFFER];
                char result[10];

                nread = read(client, recebido, sizeof(recebido));
                recebido[nread-1] = '\0';
                fflush(stdout);
                fflush(stdin);

                char *token = strtok(recebido, " ");
                while (token != NULL) {
                    if (strcmp(token, "SUBSCRIBE_TOPIC") != 0) {
                        strcpy(result, token);
                        break;
                    }
                    token = strtok(NULL, " ");
                }
                for(int i = 0; i < TAM; i++){
                    if(strcmp(result, shm->topicos[i].id) == 0){
                        int id = atoi(shm->topicos[i].id) + 6000;
                        char aux[MAX_BUFFER] = "";

                        strcpy(bufferEnvio, "224.0.0.");
                        strcat(bufferEnvio, result);
                        strcat(bufferEnvio, " ");
                        sprintf(aux, "%d", id);
                        strcat(bufferEnvio, aux);
                        write(client, bufferEnvio, strlen(bufferEnvio) + 1);
                        fflush(stdout);
                        fflush(stdin);
                    }
                }
            }
        }
        else if (strcmp(type, "jornalista") == 0){
            nread = read(client, buffer, sizeof(buffer));
            buffer[nread-1] = '\0';
            if (strncmp(buffer, "CREATE_TOPIC", 12) == 0){

                char titulo[MAX_BUFFER];
                char id[MAX_BUFFER];
                char recebido[MAX_BUFFER];

                nread = read(client, recebido, sizeof(recebido));
                recebido[nread-1] = '\0';
                printf("%s\n",recebido);

                strcpy(bufferEnvio, "Recebido\n");
                write(client, bufferEnvio, strlen(bufferEnvio) + 1);
                fflush(stdout);

                char *token = strtok(recebido, " ");

                int count = 0; 
                while (token != NULL){
                    if (count == 1){
                        strcpy(id, token);
                    }
                    else if (count == 2){
                        strcpy(titulo, token);
                    }
                    token = strtok(NULL, " ");
                    count++;
                }
                int i = 0;
                sem_wait(sem_SHM);
                for(int i = 0; i < TAM; i++){
                    if(strcmp(shm->topicos[i].id, "") == 0){
                        strcpy(shm->topicos[i].id, id);
                        strcpy(shm->topicos[i].titulo, titulo);
                        break;
                    }
                }
                sem_post(sem_SHM);

                pid_t multicast = fork();

                if (multicast < 0){
                    printf("Erro a criar processo multicast.");
                    exit(1);
                }
                if (multicast == 0){
                    multicast_server(id);
                    exit(0);
                }
                else if (multicast < 0){
                    printf("Erro a criar processo multicast.");
                    exit(0);
                }
            }
            else if (strncmp(buffer, "SEND_NEWS", 9) == 0){
                char id[MAX_BUFFER];
                char noticia[MAX_BUFFER];
                char recebido[MAX_BUFFER];

                nread = read(client, recebido, sizeof(recebido));
                recebido[nread-1] = '\0';
                printf("%s\n",recebido);

                strcpy(bufferEnvio, "Recebido\n");
                write(client, bufferEnvio, strlen(bufferEnvio) + 1);
                fflush(stdout);

                char *token = strtok(buffer, " ");

                int count = 0;
                while (token != NULL)
                {
                    if (count == 1)
                    {
                        strcpy(id, token);
                    }
                    else if (count == 2)
                    {
                        strcpy(noticia, token);
                    }
                    token = strtok(NULL, " ");
                    count++;
                }
                int i = 0;
                sem_wait(sem_SHM);
                for(int i = 0;i < TAM; i++){
                    if(strcmp(shm->topicos[i].id, id) == 0){
                        for(int j = 0;j < TAM;j++){
                            if(strcmp(shm->topicos[i].noticias[j].texto, "") == 0){
                                strcpy(shm->topicos[i].noticias[j].texto, noticia);
                            }
                        }
                    }
                }
                sem_post(sem_SHM);
            }
        }
    }
}

void cliente() {
    int client, fd;
    struct sockaddr_in addr, client_addr;
    int client_addr_size;

    bzero((void *) &addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(TCP_PORT);

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        erro("na funcao socket");
    if (bind(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0)
        erro("na funcao bind");
    if (listen(fd, 5) < 0)
        erro("na funcao listen");
    client_addr_size = sizeof(client_addr);

    while(waitpid(-1, NULL, WNOHANG) > 0);
    while (1) {
        client = accept(fd, (struct sockaddr *) &client_addr, (socklen_t * ) & client_addr_size);
        if (client > 0){
            if (fork() == 0) {
                client_server(client);
                exit(0);
            }
        close(client);
        }
    }
}

int main(int argc, char *argv[]){

    if (argc != 4){
        printf("O comando deve ser news_server {PORTO_NOTICIAS} {PORTO_CONFIG} {ficheiro configuração}\n");
        exit(1);
    }

    TCP_PORT = atoi(argv[1]);
    UDP_PORT = atoi(argv[2]);
    char *config_file = argv[3];

    SHM *shm = create_SHM();

    read_users_file(config_file);

    signal(SIGINT, sigint);
    
    pid_t admin = fork();

    if(admin < 0){
        printf("Erro a criar processo admin.");
        exit(1);
    }
    if (admin == 0) {
        printf("Admin server criado\n");
        admin_server(UDP_PORT);
        exit(0);
    }
    else if(admin < 0){
        printf("Erro a criar processo admin.");
        exit(0);
    }
    
    pid_t client = fork();

    if(client < 0){
        printf("Erro a criar processo client.");
        exit(1);
    }
    if (client == 0) {
        printf("Client server criado\n");
        cliente();
        exit(0);
    }
    else if(client < 0){
        printf("Erro a criar processo client.");
        exit(0);
    }

    while(wait(0) > 0);

    return 0;
}
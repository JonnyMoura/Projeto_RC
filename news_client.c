#include <stdio.h>

#include <sys/types.h>

#include <sys/socket.h>

#include <netinet/in.h>

#include <stdlib.h>

#include <string.h>

#include <unistd.h>

#include <netdb.h>

#include <netinet/in.h>

#include <stdbool.h>

#include <signal.h>

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

#include <netinet/in.h>

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

#include <strings.h>

#include <time.h>

#include <sys/wait.h>

#include <arpa/inet.h>

#include <sys/shm.h>

#include <semaphore.h>

#include <fcntl.h>

#include <sys/ipc.h>

#include <pthread.h>

#define BUF_SIZE 2000

#define TAM_SIZE 250

typedef struct
{ // struct da shared memory

    bool sair;

    int socket;

    struct ip_mreq mreq;

    int processos[TAM_SIZE];

} shared_mem;

int shmid;

shared_mem *shared_var;

void delete_SHM()
{

    if (shmctl(shmid, IPC_RMID, NULL) == -1)
    {

        perror("Erro in shmctl.");
    }

    printf("SHM eliminada.");
}

void detach_SHM(shared_mem *shm)
{

    if (shmdt(shm) == -1)
    {

        perror("Erro in shmdt.");
    }

    printf("SHM desacociada do processo.");
}

void erro(char *msg);

void sigint(int signum);

int main(int argc, char *argv[])
{

    char endServer[100];

    struct sockaddr_in addr;

    struct hostent *hostPtr;

    int fd;

    signal(SIGINT, sigint);

    if (argc != 3)
    {

        printf("news_client <endereço> <porto_noticias>\n");

        exit(-1);
    }

    strcpy(endServer, argv[1]);

    if ((shmid = shmget(IPC_PRIVATE, sizeof(struct shared_mem *), IPC_CREAT | 0766)) < 0)
    {

        perror("Error in shmget with IPC_CREAT\n");

        exit(1);
    }

    if ((shared_var = (struct shared_mem *)shmat(shmid, NULL, 0)) == (struct shared_mem *)-1)
    {

        perror("Shmat error!");

        exit(1);
    }

    for (int i = 0; i < TAM_SIZE; i++)
    {

        shared_var->processos[i] = -1;
    }

    shared_var->sair = false;

    if ((hostPtr = gethostbyname(endServer)) == 0)

        erro("Não consegui obter endereço");

    bzero((void *)&addr, sizeof(addr));

    addr.sin_family = AF_INET;

    addr.sin_addr.s_addr = ((struct in_addr *)(hostPtr->h_addr))->s_addr;

    addr.sin_port = htons((short)atoi(argv[2]));

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)

        erro("socket");

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)

        erro("Connect");

    int nread = 0;

    char buffer[BUF_SIZE];

    /***************************************************** AUTENTICAÇÃO ***************************************************/

    char nome[50];

    char password[50];

    char ip[BUF_SIZE];

    char porto[BUF_SIZE];

    char recebido[BUF_SIZE];

    nread = read(fd, recebido, sizeof(recebido));

    recebido[nread - 1] = '\0';

    printf("%s", recebido);

    do
    {

        printf("Nome: ");

        scanf("%s", nome);

        write(fd, nome, strlen(nome) + 1);

        fflush(stdout);

        printf("Password: ");

        scanf("%s", password);

        write(fd, password, strlen(password) + 1);

        fflush(stdout);

        nread = read(fd, buffer, sizeof(buffer));

        buffer[nread] = '\0';

        fflush(stdout);

        fflush(stdin);

        if (strcmp(buffer, "X") == 0)
        {

            printf("username ou password inválidos...\n\n");
        }

    } while (strcmp(buffer, "X") == 0);

    char str[10];

    while (1)
    {

        printf("escolha uma operacao:\n"

               "1->listar tópicos de notícias\n"

               "2->Subscrever tópicos de notícias\n"

               "3->Criar tópico de notícias\n"

               "4->Enviar notícias\n"

               "5->Sair\n\n"

        );

        scanf("%s", str);

        if (strcmp(str, "1") == 0)
        {

            char *token;

            char comando[BUF_SIZE];

            sprintf(comando, "LIST_TOPICS");

            write(fd, comando, strlen(comando) + 1);

            fflush(stdout);

            nread = read(fd, recebido, sizeof(recebido));

            recebido[nread - 1] = '\0';

            printf("%s\n", recebido);

            nread = read(fd, buffer, sizeof(buffer));

            buffer[nread - 1] = '\0';

            printf("%s\n", buffer);

            fflush(stdout);

            fflush(stdin);
        }

        else if (strcmp(str, "2") == 0)
        {

            char id_topic[3];

            char comando[BUF_SIZE];

            char recebido[BUF_SIZE];

            char endereco[BUF_SIZE];

            char porto1[BUF_SIZE];

            bool valido = false;

            while (!valido)
            {

                printf("Insira o id do tópico a subscrever:\n");

                scanf("%s", id_topic);

                char *endptr;

                long num;

                int errno = 0;

                num = strtol(id_topic, &endptr, 10);

                if (errno != 0 || *endptr != '\0' || num < 0 || num > 255)
                {

                    printf("%s is not a valid integer\n", str);
                }

                else
                {

                    valido = true;
                }
            }

            sprintf(comando, "SUBSCRIBE_TOPIC", id_topic);
            write(fd, comando, strlen(comando) + 1);
            fflush(stdout);
            fflush(stdin);

            sprintf(comando, "SUBSCRIBE_TOPIC %s", id_topic);
            write(fd, comando, strlen(comando) + 1);
            fflush(stdout);
            fflush(stdin);

            nread = read(fd, endereco, sizeof(endereco));
            endereco[nread - 1] = '\0';
            fflush(stdout);
            fflush(stdin);

            char *token;
            char parts[2][100];

            int partCount = 0;

            token = strtok(endereco, " ");

            while (token != NULL && partCount < 2) {
                strcpy(parts[partCount], token); 
                partCount++;
                token = strtok(NULL, " "); 
            }

            strcpy(endereco, parts[0]);
            strcpy(porto1, parts[1]);

            pid_t coisa = fork();

            if (coisa < 0)
            {
                printf("Erro no fork.");
                exit(1);
            }

            if (coisa == 0)
            {
                printf("Fork com sucesso\n");
                for (int i = 0; i < TAM_SIZE; i++)
                {

                    if (shared_var->processos[i] == -1)
                    {

                        shared_var->processos[i] = getpid();
                    }
                }

                recebe_topicos(endereco, atoi(porto1));

                exit(0);
            }

            else if (coisa < 0)
            {

                printf("Erro a criar processo client.");

                exit(0);
            }

            printf("Tópico subscrito\n");

            fflush(stdout);
        }

        else if (strcmp(str, "3") == 0)
        {

            char recebido[BUF_SIZE];

            char id_topic[BUF_SIZE];

            char comando[BUF_SIZE];

            char titulo[BUF_SIZE];

            printf("Insira id do tópico a criar:\n");

            scanf("%s", id_topic);

            printf("Insira o título do tópico a criar:\n");

            scanf("%s", titulo);

            sprintf(comando, "CREATE_TOPIC", id_topic, titulo);

            write(fd, comando, strlen(comando) + 1);

            sprintf(comando, "CREATE_TOPIC %s %s", id_topic, titulo);

            write(fd, comando, strlen(comando) + 1);

            nread = read(fd, recebido, sizeof(recebido));

            recebido[nread - 1] = '\0';

            printf("%s", recebido);

            printf("Tópico adicionado\n");

            fflush(stdout);
        }

        else if (strcmp(str, "4") == 0)
        {

            char id_topic[BUF_SIZE];

            char noticia[BUF_SIZE];

            char comando[BUF_SIZE];

            char recebido[BUF_SIZE];

            printf("Insira id do tópico para onde enviar a noticia:\n");

            scanf("%s", id_topic);

            printf("Insira a notícia a enviar:\n");

            scanf("%s", noticia);

            sprintf(comando, "SEND_NEWS");

            write(fd, comando, strlen(comando) + 1);

            sprintf(comando, "SEND_NEWS %s %s", id_topic, noticia);

            write(fd, comando, strlen(comando) + 1);

            nread = read(fd, recebido, sizeof(recebido));

            recebido[nread - 1] = '\0';

            printf("%s", recebido);

            printf("Noticia enviada\n");

            fflush(stdout);

            fflush(stdin);
        }

        else if (strcmp(str, "5") == 0)
        {

            shared_var->sair = true;

            sprintf(buffer, "QUIT");

            write(fd, buffer, strlen(buffer) + 1);

            fflush(stdout);

            fflush(stdin);

            printf("a sair\n");

            delete_SHM();
        }

        else
        {

            printf("Comando inválido\n");
        }

        fflush(stdout);
    }

    close(fd);

    exit(0);
}

void erro(char *msg)
{

    printf("Erro: %s\n", msg);

    exit(-1);
}

void sigint(int signum)
{

    // acaba o programa quando clicar Control^C

    printf("EXITING ctrl-c...\n");

    delete_SHM();

    exit(0);
}

void recebe_topicos(char *grupo, int porto)
{
    struct sockaddr_in addr;
    int addrlen, cnt;

    char message[BUF_SIZE * 4];
    shared_var->socket = socket(AF_INET, SOCK_DGRAM, 0);

    if ((shared_var->socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        perror("socket");
        exit(1);
    }

    bzero((char *)&addr, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(porto);
    addrlen = sizeof(addr);
    
    if (bind(shared_var->socket, (struct sockaddr *)&addr, sizeof(addr)) < 0){
        perror("bind");
        exit(1);
    }
    shared_var->mreq.imr_multiaddr.s_addr = inet_addr(grupo);
    shared_var->mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    int multicastTTL = 255;
    if (setsockopt(shared_var->socket, IPPROTO_IP, IP_MULTICAST_TTL, (void *)&multicastTTL, sizeof(multicastTTL)) < 0){
        perror("socket opt");
        exit(1);
    }
    

    while (1){
        cnt = recvfrom(shared_var->socket, message, sizeof(message), 0, (struct sockaddr *)&addr, (socklen_t *)&addrlen);
        printf("%s", message);
        fflush(stdout);
        if (cnt < 0)
        {
            perror("recvfrom");
            exit(1);
        }
        else if (cnt == 0)
        {
            break;
        }

        if (strcmp(message, "QUIT") == 0 && shared_var->sair)
        {
            if (setsockopt(shared_var->socket, IPPROTO_IP, IP_DROP_MEMBERSHIP, &(shared_var->mreq), sizeof(shared_var->mreq)) < 0)
            {
                perror("setsockopt mreq3");
                exit(1);
            }
            printf("a encerrar\n");
            break;
        }
    }
}

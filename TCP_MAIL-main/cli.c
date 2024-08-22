#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/wait.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<errno.h>
#include<netdb.h>
#include<fcntl.h>
#include<time.h>

#define BUFFER_SIZE 1024
#define MAX_USERS 10
#define MAX_USERNAME_LENGTH 20
#define MAX_PASSWORD_LENGTH 20

void send_mail(char *server_ip, int smtp_port) {
    int client_fd;
    int connect_status,count_rec,count_send;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE],buffer1[BUFFER_SIZE];
    client_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if ( client_fd<0) {
        perror("Socket creation failed\n");
        exit(EXIT_FAILURE);
    }
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(smtp_port);
    inet_pton(AF_INET, server_ip, &server_addr.sin_addr);
    connect_status = connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (connect_status<0) {
        perror("Connection failed\n");
        exit(EXIT_FAILURE);
    }
    strcpy(buffer,"<client connects to SMTP port>");
    count_send=send(client_fd, buffer, BUFFER_SIZE, 0);
    if(count_send<=0){
        perror("Send Error\n");
        exit(EXIT_FAILURE);
    }

    // Read welcome message from server
    count_rec=recv(client_fd, buffer, BUFFER_SIZE,0);
    if (count_rec <= 0) {
        perror("Receive failed\n");
        exit(EXIT_FAILURE);
    }
    printf("S: %s\n", buffer); // Print server welcome message

    // Send HELO command
    strcpy(buffer,"HELO iitkgp.edu");
    count_send=send(client_fd, buffer, BUFFER_SIZE, 0);
    if(count_send<=0){
        perror("Send Error\n");
        exit(EXIT_FAILURE);
    }

    memset(buffer,0,BUFFER_SIZE);
    memset(buffer1,0,BUFFER_SIZE);
    count_rec = recv(client_fd, buffer, BUFFER_SIZE,0);
    if (count_rec <= 0) {
        perror("Receive failed\n");
        exit(EXIT_FAILURE);
    }
    printf("S: %s\n", buffer); // Print server response

    // Enter sender's email address
    printf("From: ");
    memset(buffer,0,BUFFER_SIZE);
    scanf("%s",buffer1);
    strcpy(buffer,"MAIL FROM: <");
    strcat(buffer,buffer1);
    strcat(buffer,">");

    count_send=send(client_fd, buffer, BUFFER_SIZE, 0);
    if(count_send<=0){
        perror("Send Error\n");
        exit(EXIT_FAILURE);
    }
    count_rec=recv(client_fd, buffer, BUFFER_SIZE,0);
    if (count_rec<= 0) {
        perror("Receive failed\n");
        exit(EXIT_FAILURE);
    }
    printf("S: %s\n", buffer); // Print server response

    // Enter recipient's email address
    memset(buffer1,0,BUFFER_SIZE);
    memset(buffer,0,BUFFER_SIZE);

    printf("To: ");
    memset(buffer,0,BUFFER_SIZE);
    scanf("%s",buffer1);
    strcpy(buffer,"RCPT TO: <");
    strcat(buffer,buffer1);
    strcat(buffer,">");
    count_send=send(client_fd, buffer, BUFFER_SIZE, 0);
    if(count_send <= 0){
        perror("Send Error1\n");
        exit(EXIT_FAILURE);
    }

    count_rec=recv(client_fd, buffer, BUFFER_SIZE,0);
    if (count_rec<= 0) {
        perror("Receive failed\n");
        exit(EXIT_FAILURE);
    }
    printf("S: %s\n", buffer); // Print server response

    // Enter subject
    memset(buffer,0,BUFFER_SIZE);
    memset(buffer1,0,BUFFER_SIZE);
    strcpy(buffer,"DATA");
    count_send=(client_fd, buffer, BUFFER_SIZE, 0);
    count_send=send(client_fd, buffer, BUFFER_SIZE, 0);
    if(count_send <= 0){
        perror("Send Errors\n");
        exit(EXIT_FAILURE);
    }
    memset(buffer,0,BUFFER_SIZE);
    count_rec=recv(client_fd, buffer, BUFFER_SIZE,0);
    if (count_rec<= 0) {
        perror("Receive failed\n");
        exit(EXIT_FAILURE);
    }
    printf("S: %s\n", buffer); // Print server response
    

    // Enter message body
    while (1) {
        memset(buffer,0,BUFFER_SIZE);
        fgets(buffer, BUFFER_SIZE, stdin);
        count_send=send(client_fd, buffer, BUFFER_SIZE, 0);
        if(count_send <= 0){
        perror("Send Error1\n");
        exit(EXIT_FAILURE);
        }
        if (strcmp(buffer, ".\n") == 0) // End message input if '.' is entered
            break;
    }
    memset(buffer,0,BUFFER_SIZE);
    count_rec=recv(client_fd, buffer, BUFFER_SIZE,0);
    if (count_rec<= 0) {
        perror("Receive failed\n");
        exit(EXIT_FAILURE);
    }
    printf("S: %s\n", buffer); // Print server response

    // Send QUIT command
    memset(buffer,0,BUFFER_SIZE);
    strcpy(buffer, "QUIT");
    count_send=send(client_fd, buffer,BUFFER_SIZE,0);
    if(count_send<=0){
        perror("Send Error\n");
        exit(EXIT_FAILURE);
    }
    memset(buffer,0,BUFFER_SIZE);
    count_rec=recv(client_fd, buffer, BUFFER_SIZE,0);
    if (count_rec<= 0) {
        perror("Receive failed\n");
        exit(EXIT_FAILURE);
    }
    printf("S: %s\n", buffer); // Print server response

    memset(buffer,0,BUFFER_SIZE);
    strcpy(buffer,"C: <client hangs up>");
    count_send=send(client_fd, buffer, BUFFER_SIZE, 0);
    if(count_send<=0){
        perror("Send Error\n");
        exit(EXIT_FAILURE);
    }
    close(client_fd);
}

typedef struct {
    char username[20];
    char password[20];
} User;

int authenticate_user(const char *username, const char *password, User *users, int num_users) {
    for (int i = 0; i < num_users; i++) {
        if (strcmp(username, users[i].username) == 0) {
            if (strcmp(password, users[i].password) == 0) {
                return 1; // Authentication successful
            } else {
                return 0; // Incorrect password
            }
        }
    }
    return -1; // User not found
}
int main(int argc, char* argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <Server_IP> <SMTP_port> <POP3_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Read server IP and ports
    char server_ip[16];
    strcpy(server_ip, argv[1]);
    int smtp_port = atoi(argv[2]);
    int pop3_port = atoi(argv[3]);

    // Read users and passwords from user.txt
    User users[MAX_USERS];
    FILE *file = fopen("user.txt", "r");
    if (file == NULL) {
        perror("Error opening user.txt");
        exit(EXIT_FAILURE);
    }

    int num_users = 0;
    char username[20];
    char password[20];
    while (fscanf(file, "%s %s", username, password) == 2 && num_users < MAX_USERS) {
        strcpy(users[num_users].username, username);
        strcpy(users[num_users].password, password);
        num_users++;
    }
    fclose(file);

    // Authenticate user
    char input_username[MAX_USERNAME_LENGTH];
    char input_password[MAX_PASSWORD_LENGTH];
    int authenticated = 0;
    do {
        printf("Enter username: ");
        scanf("%s", input_username);
        printf("Enter password: ");
        scanf("%s", input_password);
        authenticated = authenticate_user(input_username, input_password, users, num_users);
        if (!authenticated) {
            printf("Invalid username or password. Please try again.\n");
        }
    } while (!authenticated);

    // User authenticated, proceed with options
    int select;
    do {
        printf("\nSelect an option:\n");
        printf("1. Manage Mail\n");
        printf("2. Send Mail\n");
        printf("3. Quit\n\n");
        printf("Option: ");
        scanf("%d", &select);

        switch (select) {
            case 1:
                // Code to manage mail (retrieve and display mails)
                break;
            case 2:
                // Code to send mail
                printf("\n Send Mail Executing\n");
                 send_mail(server_ip, smtp_port);
                break;
            case 3:
                printf("Exiting program...\n");
                break;
            default:
                printf("Invalid option\n");
        }
    } while (select != 3);

    return 0;
}
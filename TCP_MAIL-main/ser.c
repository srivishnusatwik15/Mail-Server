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


void handle_client(int client_fd) {
    char buffer[BUFFER_SIZE],buffer1[BUFFER_SIZE];
    int count_rec,count_send;

    // Send welcome message
    memset(buffer,0,BUFFER_SIZE);
    count_rec=recv(client_fd, buffer, BUFFER_SIZE, 0);
    if ( count_rec <= 0 ) {
        perror("Receive failed\n");
        exit(EXIT_FAILURE);
    }
    printf("C: %s\n", buffer);

    memset(buffer,0,BUFFER_SIZE);
    strcpy(buffer,"220 <iitkgp.edu> Service ready");
    count_send=send(client_fd, buffer,BUFFER_SIZE,0);
    if(count_send<=0){
        perror("Send Error\n");
        exit(EXIT_FAILURE);
    }
    // Receive HELO command
    count_rec=recv(client_fd, buffer, BUFFER_SIZE, 0);
    if ( count_rec <= 0 ) {
        perror("Receive failed\n");
        exit(EXIT_FAILURE);
    }
    printf("C: %s\n", buffer); // Print client command


    memset(buffer,0,BUFFER_SIZE);
    strcpy(buffer,"250 OK Hello iitkgp.edu");
    count_send=send(client_fd, buffer,BUFFER_SIZE,0);
    if(count_send<=0){
        perror("Send Error\n");
        exit(EXIT_FAILURE);
    }

    // Receive MAIL FROM command
    count_rec=recv(client_fd, buffer, BUFFER_SIZE, 0);
    if ( count_rec <= 0 ) {
        perror("Receive failed\n");
        exit(EXIT_FAILURE);
    }
    printf("C: %s\n", buffer); // Print client command

    memset(buffer1,0,BUFFER_SIZE);
    strcpy(buffer1,"250 ");
    strcat(buffer1,buffer);
    strcat(buffer1,"... Sender ok");
    char username_send[BUFFER_SIZE];
    if (sscanf(buffer, "MAIL FROM: <%[^@]", username_send) != 1) {
    perror("Error extracting sender username\n");
    exit(EXIT_FAILURE);
    }

    memset(buffer,0,BUFFER_SIZE);
    count_send=send(client_fd, buffer1,BUFFER_SIZE,0);
    if(count_send<=0){
        perror("Send Error\n");
        exit(EXIT_FAILURE);
    }    

    // Receive RCPT TO command

    count_rec=recv(client_fd, buffer, BUFFER_SIZE, 0);
    if ( count_rec <= 0 ) {
        perror("Receive failed\n");
        exit(EXIT_FAILURE);
    }
    printf("C: %s\n", buffer); // Print client command
    char username_recv[BUFFER_SIZE];
    if (sscanf(buffer, "RCPT TO: <%[^@]", username_recv) != 1) {
    perror("Error extracting username\n");
    exit(EXIT_FAILURE);
    }
    memset(buffer,0,BUFFER_SIZE);
    strcpy(buffer,"250 root... Recipient ok");
    count_send=send(client_fd, buffer,BUFFER_SIZE,0);
    if(count_send<=0){
        perror("Send Error\n");
        exit(EXIT_FAILURE);
    }

    // Receive DATA command
    count_rec=recv(client_fd, buffer, BUFFER_SIZE, 0);
    if ( count_rec <= 0 ) {
        perror("Receive failed\n");
        exit(EXIT_FAILURE);
    }
    printf("C: %s\n", buffer); // Print client command

    memset(buffer,0,BUFFER_SIZE);
    strcpy(buffer,"354 Enter mail, end with '.' on a line by itself");
    count_send=send(client_fd, buffer,BUFFER_SIZE,0);
    if(count_send<=0){
        perror("Send Error\n");
        exit(EXIT_FAILURE);
    }


    char mailbox_path[BUFFER_SIZE];
    strcpy(mailbox_path,"");
    strcat(mailbox_path,username_recv);
    strcat(mailbox_path,"/mymailbox.txt");

    FILE *mymailbox_file = fopen(mailbox_path, "a");
    if (mymailbox_file == NULL) {
        perror("Error opening mailbox file");
        exit(EXIT_FAILURE);
    }
     time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    fprintf(mymailbox_file, "From: <%s>\n", username_send);
    fprintf(mymailbox_file, "To: <%s>\n", username_recv);
    fprintf(mymailbox_file, "Received: %s", asctime(timeinfo));


    int i=1;
    while (1) {
        count_rec=recv(client_fd, buffer, BUFFER_SIZE, 0);
        if ( count_rec <= 0 ) {
            perror("Receive failed\n");
            exit(EXIT_FAILURE);
        }
        if(i!=1){
        printf("C: %s", buffer); // Print client command
        }
        i++;
        if (strcmp(buffer, ".\n") == 0) {
            break;
        }
        // Write received message to mymailbox file
        fprintf(mymailbox_file, "%s", buffer);
    }
    fprintf(mymailbox_file, "%s", "\n\n");
    fclose(mymailbox_file);
    // Send message accepted response
    memset(buffer,0,BUFFER_SIZE);
    strcpy(buffer,"250 OK Message accepted for delivery");
    count_send=send(client_fd, buffer,BUFFER_SIZE,0);
    if(count_send<=0){
        perror("Send Error\n");
        exit(EXIT_FAILURE);
    }

    // Receive QUIT command
    count_rec=recv(client_fd, buffer, BUFFER_SIZE, 0);
    if ( count_rec <= 0 ) {
        perror("Receive failed\n");
        exit(EXIT_FAILURE);
    }
    printf("C: %s\n", buffer); // Print client command

    memset(buffer,0,BUFFER_SIZE);
    strcpy(buffer,"221 iitkgp.edu closing connection");
    count_send=send(client_fd, buffer,BUFFER_SIZE,0);
    if(count_send<=0){
        perror("Send Error\n");
        exit(EXIT_FAILURE);
    }

    count_rec=recv(client_fd, buffer, BUFFER_SIZE, 0);
    if ( count_rec <= 0 ) {
        perror("Receive failed\n");
        exit(EXIT_FAILURE);
    }
    printf("C: %s\n", buffer);
    printf("\n");

    close(client_fd);
}

int main(int argc,char* argv[]){
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <My_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    fprintf(stdout, "My_port = <%s>\n",argv[1]);
    int smtp_port = atoi(argv[1]);
    
    int server_fd, client_fd, port;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    int bind_status,listen_status;

    server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(smtp_port);

    bind_status=bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (bind_status < 0) {
        perror("Binding failed");
        exit(EXIT_FAILURE);
    }

    listen_status=listen(server_fd,5);
    if (listen_status < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("\nSMTP mail server running on port %d\n", smtp_port);

    while (1) {
        // Accept incoming connection
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }

        
        
        pid_t pid = fork();
        if (pid < 0) {
            perror("Fork failed");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Child process
            close(server_fd); // Close the listening socket in the child process
            handle_client(client_fd); // Handle the client connection
            exit(EXIT_SUCCESS);
        } else {
            // Parent process
            close(client_fd); // Close the client socket in the parent process
        }
    }

    return 0;
    
}
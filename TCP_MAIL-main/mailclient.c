#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<regex.h>
#include<ctype.h>
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

int check(char * buf){ //to check the format of from and to addresses
    const char *pattern = "\\s*[a-zA-Z0-9_.]+@[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+\\s*$";
    regex_t reg;
    if (regcomp(&reg, pattern, REG_EXTENDED) != 0) {
        fprintf(stderr, "Error compiling regular expression\n");
        return -1;
    }
    int m = regexec(&reg, buf, 0, NULL, 0);
    if (m == 0){
        return 1;
    } else{
        printf("Invalid email format: %s\n", buf);
        return 0;
    }
    regfree(&reg);
}


//Function to access mail using pop3 server
void receive_mail(char *server_ip, int pop3_port,const char* username,const char* password) {
    // Creating socket
    int client_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_fd < 0) {
        perror("Socket creation failed\n");
        exit(EXIT_FAILURE);
    }

    // Server address
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(pop3_port);
    inet_pton(AF_INET, server_ip, &server_addr.sin_addr);

    // Connect to server
    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed\n");
        exit(EXIT_FAILURE);
    }

    // Receive greeting message
    char buffer[BUFFER_SIZE];
    ssize_t count_rec = recv(client_fd, buffer, BUFFER_SIZE, 0);
    if (count_rec <= 0) {
        perror("Receive failed\n");
        exit(EXIT_FAILURE);
    }
    printf("S: %s\n", buffer);

    //Sending username to pop3 server as USER username command
    snprintf(buffer, BUFFER_SIZE, "%s\r\n", username);
    ssize_t count_send = send(client_fd, buffer, strlen(buffer), 0);
    if (count_send <= 0) {
        perror("Send Error\n");
        exit(EXIT_FAILURE);
    }
    //Recieving  "Recieved username" message from server
    count_rec = recv(client_fd, buffer, BUFFER_SIZE, 0);
    if (count_rec <= 0) {
        perror("Receive failed\n");
        exit(EXIT_FAILURE);
    }
    printf("S: %s\n", buffer);

    //Sending password as "PASS password" command
    snprintf(buffer, BUFFER_SIZE, "%s\r\n", password);
    count_send = send(client_fd, buffer, strlen(buffer), 0);
    if (count_send <= 0) {
        perror("Send Error\n");
        exit(EXIT_FAILURE);
    }
    //Recieving  "Recieved password" message from server
    count_rec = recv(client_fd, buffer, BUFFER_SIZE, 0);
    if (count_rec <= 0) {
        perror("Receive failed\n");
        exit(EXIT_FAILURE);
    }
    printf("S: %s\n", buffer);

    // Receiving authentication message from server
    ssize_t count_recv = recv(client_fd, buffer, BUFFER_SIZE, 0);
    if (count_recv <= 0) {
        perror("Receive error");
        exit(EXIT_FAILURE);
    }
    printf("S: %s\n", buffer);

    //passing STAT command to recieve number of mails
    const char *stat_command = "STAT\r\n";
    count_send = send(client_fd, stat_command, strlen(stat_command), 0);
    if (count_send <= 0) {
        perror("Send Error\n");
        exit(EXIT_FAILURE);
    }

    // Receive response for STAT command
    count_rec = recv(client_fd, buffer, BUFFER_SIZE, 0);
    if (count_rec <= 0) {
        perror("Receive failed\n");
        exit(EXIT_FAILURE);
    }
    printf("Server: %s\n", buffer);


    // Sending LIST command
    const char *list_command = "LIST\r\n";
     count_send = send(client_fd, list_command, strlen(list_command), 0);
    if (count_send <= 0) {
        perror("Send error");
        exit(EXIT_FAILURE);
    }

    // Receiving and print list of mails from server
     
    while ((count_recv = recv(client_fd, buffer, BUFFER_SIZE, 0)) > 0) {
        buffer[count_recv] = '\0';
        printf("%s", buffer);
    }
    if (count_recv < 0) {
        perror("Receive error");
        exit(EXIT_FAILURE);
    }


    // Prompting user to enter  an email number to view its content
    printf("Enter mail no. to see (-1 to exit): ");
    int mail_no;
    scanf("%d", &mail_no);
    if (mail_no == -1) {
        printf("Exiting mail view...\n");
        close(client_fd);
        return;
    }
    //recieving mail content by passing RETR n command
    snprintf(buffer, BUFFER_SIZE, "RETR %d\r\n", mail_no);
    count_send = send(client_fd, buffer, strlen(buffer), 0);
    if (count_send <= 0) {
        perror("Send Error\n");
        exit(EXIT_FAILURE);
    }
    count_rec = recv(client_fd, buffer, BUFFER_SIZE, 0);
    if (count_rec <= 0) {
        perror("Receive failed\n");
        exit(EXIT_FAILURE);
    }
    printf("S: %s\n", buffer);

    //PAssing DELE n command to delete nth mail
    snprintf(buffer, BUFFER_SIZE, "DELE %d\r\n", mail_no);
    count_send = send(client_fd, buffer, strlen(buffer), 0);
    if (count_send <= 0) {
        perror("Send Error\n");
        exit(EXIT_FAILURE);
    }
    count_rec = recv(client_fd, buffer, BUFFER_SIZE, 0);
    if (count_rec <= 0) {
        perror("Receive failed\n");
        exit(EXIT_FAILURE);
    }
    printf("S: %s\n", buffer);

    // Sending RSET command to retriev back deleted messages
    //char buffer[BUFFER_SIZE];
    snprintf(buffer, BUFFER_SIZE, "RSET\r\n");
    count_send = send(client_fd, buffer, strlen(buffer), 0);
    if (count_send <= 0) {
        perror("Send Error\n");
        exit(EXIT_FAILURE);
    }

    // Receiving response from server
    count_recv = recv(client_fd, buffer, BUFFER_SIZE, 0);
    if (count_recv <= 0) {
        perror("Receive failed\n");
        exit(EXIT_FAILURE);
    }
    buffer[count_recv] = '\0';
    printf("Server Response: %s\n", buffer);
    // Sending QUIT command
   // char buffer[BUFFER_SIZE];
    snprintf(buffer, BUFFER_SIZE, "QUIT\r\n");
    count_send = send(client_fd, buffer, strlen(buffer), 0);
    if (count_send <= 0) {
        perror("Send Error\n");
        exit(EXIT_FAILURE);
    }

    // Receiving response from server
   count_recv = recv(client_fd, buffer, BUFFER_SIZE, 0);
    if (count_recv <= 0) {
        perror("Receive failed\n");
        exit(EXIT_FAILURE);
    }
    buffer[count_recv] = '\0';
    printf("Server Response: %s\n", buffer);

    // Close connection
    close(client_fd);
}

//function to send mail using smtp server
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

    // Reading welcome message from server
    count_rec=recv(client_fd, buffer, BUFFER_SIZE,0);
    if (count_rec <= 0) {
        perror("Receive failed\n");
        exit(EXIT_FAILURE);
    }
    printf("S: %s\n", buffer); // Printing server welcome message

    // Sending HELO command
    strcpy(buffer,"HELO iitkgp.edu");
    count_send=send(client_fd, buffer, BUFFER_SIZE, 0);
    if(count_send<=0){
        perror("Send Error\n");
        exit(EXIT_FAILURE);
    }
    //recieving reply to hello command
    memset(buffer,0,BUFFER_SIZE);
    memset(buffer1,0,BUFFER_SIZE);
    count_rec = recv(client_fd, buffer, BUFFER_SIZE,0);
    if (count_rec <= 0) {
        perror("Receive failed\n");
        exit(EXIT_FAILURE);
    }
    printf("S: %s\n", buffer); 

    // Enter sender's email address

    int j=0;
    while(j==0){
        printf("From: ");
        memset(buffer,0,BUFFER_SIZE);
        memset(buffer1,0,BUFFER_SIZE);
        scanf("%s",buffer1);
        strcpy(buffer,"MAIL FROM: <");
        strcat(buffer,buffer1);
        strcat(buffer,">");
        j=check(buffer1); //check if mail is in correct format or not

    }
    j=0; //send mail from details
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
    printf("S: %s\n", buffer); 

    // Enter recipient's email address

    while(j==0){
        printf("To: ");
        memset(buffer,0,BUFFER_SIZE);
        memset(buffer1,0,BUFFER_SIZE);
        scanf("%s",buffer1);
        strcpy(buffer,"RCPT TO: <");
        strcat(buffer,buffer1);
        strcat(buffer,">");
        j=check(buffer1); //check if mail is in correct format or not

    }
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
    printf("S: %s\n", buffer); // Printing server response

    // Entering  subject
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
    printf("S: %s\n", buffer); // Printing server response
    

    // Entering  message body
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
    printf("S: %s\n", buffer); // Printing server response

    // Sending QUIT command
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
    printf("S: %s\n", buffer); // Printing server response

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
//function to authenticate user
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

//passing server ip adress , smtp port number, pop3 port number as command line arguments
int main(int argc, char* argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <Server_IP> <SMTP_port> <POP3_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Reading  server IP and ports from command line arguments
    char server_ip[16];
    strcpy(server_ip, argv[1]);
    int smtp_port = atoi(argv[2]);
    int pop3_port = atoi(argv[3]);

    // Reading  users and passwords from user.txt
    User users[MAX_USERS];
    FILE *file = fopen("user.txt", "r");
    if (file == NULL) {
        perror("Error opening user.txt");
        exit(EXIT_FAILURE);
    }
    //passing the usernames and passwords into array
    int num_users = 0;
    char username[20];
    char password[20];
    while (fscanf(file, "%s %s", username, password) == 2 && num_users < MAX_USERS) {
        strcpy(users[num_users].username, username);
        strcpy(users[num_users].password, password);
        num_users++;
    }
    fclose(file);

    // Authenticating user 
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

    // If User authenticated, proceeding  with options
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
                
                receive_mail(server_ip, pop3_port,input_username,input_password);
                break;
            case 2:
                
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
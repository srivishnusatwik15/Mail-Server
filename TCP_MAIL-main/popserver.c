#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#define MAX_MAIL_SIZE 1024
#define BUFFER_SIZE 1024
 // Change to your desired port number

typedef struct {
    char username[20];
    char password[20];
} User;
//Authenticating user
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
    return -1;
}
//Assigning serial numbers to each mail content
void assign_serial_numbers(FILE *mymailbox_file) {
    char buffer[BUFFER_SIZE];
    int serial_number = 1;
    int in_email = 0;

    // Rewind to the beginning of the file
    rewind(mymailbox_file);

    // Loop through each line in the file to find "From" statement
    while (fgets(buffer, BUFFER_SIZE, mymailbox_file) != NULL) {
        if (strncmp(buffer, "From", 4) == 0) {
            // Start of a new email
            if (in_email) {
                serial_number++;
            }
            in_email = 1;
            // Assigning serial number to this email
            fprintf(mymailbox_file, "Serial Number: %d\n", serial_number);
        } else if (in_email && strcmp(buffer, ".\n") == 0) {
            
            in_email = 0;
        }
    }
}
//Function to count number of mails (considering From as start of any mail)
int count_mails(const char *mailbox_path) {
    FILE *mailbox_file = fopen(mailbox_path, "r");
    if (mailbox_file == NULL) {
        perror("Error opening mailbox file");
        exit(EXIT_FAILURE);
    }

    int count = 0;
    char line[BUFFER_SIZE];
    while (fgets(line, BUFFER_SIZE, mailbox_file) != NULL) {
        if (strncmp(line, "From", 4) == 0) {
            count++;
        }
    }

    fclose(mailbox_file);
    return count;
}
// Function to parse and list mails
struct Mail {
    int serial_number;
    char sender_email[BUFFER_SIZE];
    char received_date[BUFFER_SIZE];
    char subject[BUFFER_SIZE];
};

// Function to parse and list mails
void list_mails(FILE *mymailbox_file, int client_fd) {
    char buffer[BUFFER_SIZE];
    int num_mails = 0;
    struct Mail mails[100];

    // Parse mailbox file, picking From, 
    int serial_number = 1;
    while (fgets(buffer, BUFFER_SIZE, mymailbox_file) != NULL) {
        if (strncmp(buffer, "From:", 5) == 0) {
            sscanf(buffer, "From: %[^\n]", mails[num_mails].sender_email);
        } else if (strncmp(buffer, "Date:", 5) == 0) {
            sscanf(buffer, "Date: %[^\n]", mails[num_mails].received_date);
        } else if (strncmp(buffer, "Subject:", 8) == 0) {
            sscanf(buffer, "Subject: %[^\n]", mails[num_mails].subject);
        } else if (strncmp(buffer, ".", 1) == 0) {
            // End of current email
            mails[num_mails].serial_number = serial_number++;
            num_mails++;
        }
    }

    // Send list of mails to client
    char response[BUFFER_SIZE];
    for (int i = 0; i < num_mails; i++) {
        snprintf(response, BUFFER_SIZE, "Sl. No. %d %s %s %s\n", 
                 mails[i].serial_number, mails[i].sender_email, 
                 mails[i].received_date, mails[i].subject);
        ssize_t count_send = send(client_fd, response, strlen(response), 0);
        if (count_send <= 0) {
            perror("Send error");
            exit(EXIT_FAILURE);
        }
    }
}
void handle_retrieve_mail(int client_fd, int mail_no) {
    char buffer[BUFFER_SIZE];

    // Open the mailbox file
    FILE *mailbox_file = fopen("mymailbox.txt", "r");
    if (mailbox_file == NULL) {
        perror("Error opening mailbox file");
        exit(EXIT_FAILURE);
    }

    // Retrieve the email with the given mail_no
    int current_mail_no = 0;
    int in_email = 0;
    while (fgets(buffer, BUFFER_SIZE, mailbox_file) != NULL) {
        if (strncmp(buffer, "From", 4) == 0) {
            // Start of a new email
            current_mail_no++;
            if (current_mail_no == mail_no) {
                // Found the requested email
                in_email = 1;
                snprintf(buffer, BUFFER_SIZE, "+OK Mail content follows:\n");
                ssize_t count_send = send(client_fd, buffer, strlen(buffer), 0);
                if (count_send <= 0) {
                    perror("Send Error\n");
                    exit(EXIT_FAILURE);
                }
            } else {
                in_email = 0;
            }
        }
        if (in_email) {
            ssize_t count_send = send(client_fd, buffer, strlen(buffer), 0);
            if (count_send <= 0) {
                perror("Send Error\n");
                exit(EXIT_FAILURE);
            }
        }
    }

    // Close the mailbox file
    fclose(mailbox_file);

    // If the mail_no was not found, inform the client
    if (current_mail_no < mail_no) {
        snprintf(buffer, BUFFER_SIZE, "-ERR Mail not found\n");
        ssize_t count_send = send(client_fd, buffer, strlen(buffer), 0);
        if (count_send <= 0) {
            perror("Send Error\n");
            exit(EXIT_FAILURE);
        }
    }
}

void handle_delete_mail(int client_fd, int mail_no) {
    char buffer[BUFFER_SIZE];

    // Perform deletion of mail with the given mail_no
    // Here, you can mark the mail as deleted in the mailbox file
    // For simplicity, let's assume the mail is deleted immediately
    snprintf(buffer, BUFFER_SIZE, "+OK Message %d deleted\n", mail_no);
    ssize_t count_send = send(client_fd, buffer, strlen(buffer), 0);
    if (count_send <= 0) {
        perror("Send Error\n");
        exit(EXIT_FAILURE);
    }
}
void handle_reset(int client_fd) {
    // Perform reset by unmarking any messages marked as deleted
    // For simplicity, let's assume there are no messages marked as deleted
    char response[] = "+OK maildrop reset\n";
    ssize_t count_send = send(client_fd, response, strlen(response), 0);
    if (count_send <= 0) {
        perror("Send Error\n");
        exit(EXIT_FAILURE);
    }
}
void handle_quit(int client_fd) {
    // Check if there are any messages marked for deletion
    // For simplicity, let's assume no messages are marked for deletion
    char response[] = "+OK Goodbye\n";
    ssize_t count_send = send(client_fd, response, strlen(response), 0);
    if (count_send <= 0) {
        perror("Send Error\n");
        exit(EXIT_FAILURE);
    }

    // Release any resources, close connections, etc.
    close(client_fd);
}

void handle_client(int client_fd) {
    char buffer[BUFFER_SIZE];
    ssize_t count_send, count_recv;

    // Send greeting message
    snprintf(buffer, BUFFER_SIZE, "+OK POP3 server ready\n");
    count_send = send(client_fd, buffer, strlen(buffer), 0);
    if (count_send <= 0) {
        perror("Send error");
        exit(EXIT_FAILURE);
    }

    // Receive username from client
    char username_send[BUFFER_SIZE];
    count_recv = recv(client_fd, username_send, BUFFER_SIZE, 0);
    if (count_recv <= 0) {
        perror("Receive error");
        exit(EXIT_FAILURE);
    }
    username_send[count_recv] = '\0';
    printf("Received username: %s\n", username_send);
    // Send response to client indicating username received
    

    const char *response_username = "+OK Received username";
    count_send = send(client_fd, response_username, strlen(response_username), 0);
    if (count_send <= 0) {
        perror("Send error");
        exit(EXIT_FAILURE);
    }

    // Receive password from client
    char password_send[BUFFER_SIZE];
    count_recv = recv(client_fd, password_send, BUFFER_SIZE, 0);
    if (count_recv <= 0) {
        perror("Receive error");
        exit(EXIT_FAILURE);
    }
    password_send[count_recv] = '\0';
    printf("Received password: %s\n", password_send);
    // Send response to client indicating password received
    const char *response_password = "+OK Received password";
    count_send = send(client_fd, response_password, strlen(response_password), 0);
    if (count_send <= 0) {
        perror("Send error");
        exit(EXIT_FAILURE);
    }

    
    User users[20];
    FILE *file = fopen("user.txt", "r");
    if (file == NULL) {
        perror("Error opening user.txt");
        exit(EXIT_FAILURE);
    }
    //passing the usernames and passwords into array
    int num_users = 0;
    char username[20];
    char password[20];
    while (fscanf(file, "%s %s", username, password) == 2 && num_users < 20) {
        strcpy(users[num_users].username, username);
        strcpy(users[num_users].password, password);
        num_users++;
    }
    fclose(file);

    // Authenticating user 
    
    int authenticated = 0;
    authenticated = authenticate_user(username_send, password_send, users, num_users);
    // Send authentication response
    if (authenticated) {
        snprintf(buffer, BUFFER_SIZE, "+OK User authenticated\n");
    } else {
        snprintf(buffer, BUFFER_SIZE, "-ERR Authentication failed\n");
    }
    count_send = send(client_fd, buffer, strlen(buffer), 0);
    if (count_send <= 0) {
        perror("Send error");
        exit(EXIT_FAILURE);
    }

    
    
    char mailbox_path[BUFFER_SIZE];
    snprintf(mailbox_path, BUFFER_SIZE, "%s/%s", username, "mymailbox.txt");

    FILE *mymailbox_file = fopen(mailbox_path, "a");
    if (mymailbox_file == NULL) {
        perror("Error opening mailbox file");
        exit(EXIT_FAILURE);
    }
    assign_serial_numbers(mymailbox_file);


 while(1){   
    // Receive  STAT command from client
        if (strncmp(buffer, "STAT", 4) == 0) {
        int num_mails = count_mails(mailbox_path);
        snprintf(buffer, BUFFER_SIZE, "+OK %d messages\r\n", num_mails);
        count_send = send(client_fd, buffer, strlen(buffer), 0);
        if (count_send <= 0) {
            perror("Send error");
            exit(EXIT_FAILURE);
        }
        } 
        else if(strncmp(buffer, "LIST", 4)==0){
              FILE *mymailbox_file = fopen(mailbox_path, "r");
            if (mymailbox_file == NULL) {
            perror("Error opening mailbox file");
                exit(EXIT_FAILURE);
            }

            // List all mails and send details to client
            list_mails(mymailbox_file, client_fd);

            // Close mailbox file
            fclose(mymailbox_file);  
        }
        else if(strncmp(buffer, "RETR", 4) == 0){
            int mail_no;
            sscanf(buffer, "RETR %d", &mail_no);
            handle_retrieve_mail(client_fd, mail_no);
        }
        else if (strncmp(buffer, "DELE", 4) == 0) {
            // Parse DELE command and mail number
            int mail_no;
            sscanf(buffer, "DELE %d", &mail_no);
            handle_delete_mail(client_fd, mail_no);
        }

        else if (strncmp(buffer, "RSET", 4) == 0) {
            // Handle RSET command
            handle_reset(client_fd);
        }
        else if (strncmp(buffer, "QUIT", 4) == 0) {
            // Handle QUIT command
            handle_quit(client_fd);
            break; // Exit loop and close connection
        }
        else {
        snprintf(buffer, BUFFER_SIZE, "-ERR Unknown command\r\n");
        count_send = send(client_fd, buffer, strlen(buffer), 0);
        if (count_send <= 0) {
            perror("Send error");
            exit(EXIT_FAILURE);
        }
        }
 }
    
// Close mailbox file
    fclose(mymailbox_file);

    // Close client socket
    close(client_fd);
}


int main(int argc, char* argv[]) {
    int pop3_port = atoi(argv[1]);
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Initialize server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(pop3_port);

    // Bind socket to address
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Binding failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 5) < 0) {
        perror("Listening failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", pop3_port);

    while (1) {
        // Accept connection from client
        client_len = sizeof(client_addr);
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("Acceptance failed");
            exit(EXIT_FAILURE);
        }

        printf("New client connected\n");

        // Handle client communication
        handle_client(client_fd);

        printf("Client disconnected\n");
    }

    // Close server socket
    close(server_fd);

    return 0;
}

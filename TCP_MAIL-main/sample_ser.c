void send_mail_details(int client_fd, FILE *mailbox_file) {
    char buffer[BUFFER_SIZE];
    int serial_number = 1;
    int in_email = 0;

    // Send mail details for each email
    while (fgets(buffer, BUFFER_SIZE, mailbox_file) != NULL) {
        if (strncmp(buffer, "From", 4) == 0) {
            // Start of a new email
            snprintf(buffer, BUFFER_SIZE, "Sl. No. %d: ", serial_number);
            send(client_fd, buffer, strlen(buffer), 0);

            // Extract sender's email id
            char *sender = strstr(buffer, "<");
            if (sender != NULL) {
                char *end_sender = strstr(sender, ">");
                if (end_sender != NULL) {
                    *end_sender = '\0';
                    send(client_fd, sender, strlen(sender) + 1, 0);
                }
            }

            // Extract date received (dummy value)
            snprintf(buffer, BUFFER_SIZE, " When received, in date : hour : minute");

            // Extract subject (dummy value)
            snprintf(buffer, BUFFER_SIZE, " Subject\n");

            serial_number++;
        }
    }
}
char* retrieve_mail_content(int mail_no) {
    // Open the mailbox file for reading
    FILE* mailbox_file = fopen("mymailbox.txt", "r");
    if (mailbox_file == NULL) {
        perror("Error opening mymailbox.txt");
        exit(EXIT_FAILURE);
    }

    char buffer[MAX_MAIL_SIZE];
    char* mail_content = NULL;
    int current_mail_no = 0;
    int in_mail = 0;

    // Read the mailbox file line by line
    while (fgets(buffer, MAX_MAIL_SIZE, mailbox_file) != NULL) {
        // Check if the line indicates the start of a new mail
        if (strncmp(buffer, "From", 4) == 0) {
            current_mail_no++;
            in_mail = (current_mail_no == mail_no); // Check if it's the mail we're looking for
        }

        // If we're in the mail content of the requested mail
        if (in_mail) {
            // Allocate memory for the mail content if it's not already allocated
            if (mail_content == NULL) {
                mail_content = (char*)malloc(MAX_MAIL_SIZE * sizeof(char));
                if (mail_content == NULL) {
                    perror("Memory allocation failed");
                    exit(EXIT_FAILURE);
                }
                mail_content[0] = '\0'; // Initialize as empty string
            }

            // Append the current line to the mail content
            strcat(mail_content, buffer);

            // Check if we've reached the end of the mail
            if (strcmp(buffer, ".\n") == 0) {
                break; // End of mail
            }
        }
    }

    // Close the mailbox file
    fclose(mailbox_file);

    return mail_content;
}
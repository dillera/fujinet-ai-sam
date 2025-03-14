#include "ai-sam.h"
#include "config.h"  // OpenAI API key and URL

// Initialize FujiNet device
bool init_fujinet()
{
    AdapterConfig config;

    if (!fuji_get_adapter_config(&config))
    {
        printf("Error: FujiNet not detected or failed to retrieve configuration!\n");
        return false;
    }

    // DEBUG: Print firmware version
    //printf("FujiNet Firmware: %s\n", config.fn_version);

    return true;
}

// Send request to OpenAI API
bool send_openai_request(char *user_input)
{
    uint8_t err;

    // Setup devicespec
    snprintf(devicespec, sizeof(devicespec), "N1:%s", OPENAI_API_URL);

    // DEBUG: Print devicespec
    //printf("%s\n", devicespec);

    // Clean up the user input
    escape_json_string(user_input, escaped_input, sizeof(escaped_input));

    // Get payload ready
    snprintf(json_payload, REQUEST_BUFFER_SIZE,
        "{"
        "\"model\":\"gpt-4o-2024-11-20\","
        "\"messages\":["
        "{\"role\":\"system\",\"content\":\""
        "You are a FujiNet SAM text-to-speech assistant. "
        "Your response must return 2 fields: text_display and text_sam. "
        "Rules for BOTH text_display and text_sam:"
        "do NOT use any special formatting, characters, quotation marks, forward or back slashes, "
        "special symbols, or escape sequences. Do NOT respond with Unicode characters. "
        "Rules only applying to text_display: numbers must be printed as digits, use ASCII new"
        "lines when needed, limit the text_display response to 960 characters or less."
        "Rules only applying to text_sam: numbers must be written as words\"},"
        "{\"role\":\"user\",\"content\":\"%s\"}"
        "],"
        "\"functions\":["
        "{\"name\":\"vintage_computers_response\","
        "\"description\":\"Provides two text fields for the user\","
        "\"parameters\":{"
        "\"type\":\"object\","
        "\"properties\":{"
        "\"text_display\":{\"type\":\"string\",\"description\":\"Human-readable output limited to 960 characters\"},"
        "\"text_sam\":{\"type\":\"string\",\"description\":\"Phonetic version of the text_display string for SAM\"}"
        "},"
        "\"required\":[\"text_display\",\"text_sam\"]"
        "}}],"
        "\"function_call\":{\"name\":\"vintage_computers_response\"}"
        "}",
        escaped_input
    );

    printf("Thinking...\n");

    err = network_open(devicespec, OPEN_MODE_HTTP_POST, OPEN_TRANS_NONE);
    if (err != 0)
    {
        printf("Error: Unable to open network channel.\n");
        return false;
    }

    err = network_http_start_add_headers(devicespec);
    if (err != 0)
    {
        printf("Error: Unable to add headers.\n");
        return false;
    }

    network_http_add_header(devicespec, "Authorization: Bearer " OPENAI_API_KEY);
    network_http_add_header(devicespec, "Content-Type: application/json");
    network_http_end_add_headers(devicespec);

    err = network_http_post(devicespec, json_payload);
    if (err != 0)
    {
        printf("Error: Failed to send data.\n");
        network_close(devicespec);
        return false;
    }

    err = network_json_parse(devicespec);
    if (err != 0)
    {
        printf("Error: Failed to parse JSON response.\n");
        network_close(devicespec);
        return false;
    }

    err = network_json_query(devicespec, "/choices/0/message/function_call/arguments", response_buffer);
    if (err == 0)
    {
        printf("Error: Query failed.\n");
        network_close(devicespec);
        return false;
    }

    network_close(devicespec);
    process_response(response_buffer);
    return true;
}

// Process the JSON Response
void process_response(const char *json_response)
{
    char *start, *end;

    // DEBUG: Print the full JSON response
    //printf("DEBUG: JSON Response:\n%s\n", json_response);
    //getchar();  // Wait for key press before continuing

    // Extract text_display
    start = strstr(json_response, "\"text_display\":\"");
    if (start)
    {
        start += strlen("\"text_display\":\"");  // Move past key name
        end = strchr(start, '\"');  // Find closing quote
        if (end && (end - start) < sizeof(text_display))
        {
            strncpy(text_display, start, end - start);
            text_display[end - start] = '\0';  // Null-terminate
        }
    }

    // DEBUG: Print extracted text_display
    //printf("DEBUG: Extracted text_display:\n%s\n", text_display);
    //getchar();  // Wait for key press before continuing

    // Extract text_sam
    start = strstr(json_response, "\"text_sam\":\"");
    if (start)
    {
        start += strlen("\"text_sam\":\"");  // Move past key name
        end = strchr(start, '\"');  // Find closing quote
        if (end && (end - start) < sizeof(text_sam))
        {
            strncpy(text_sam, start, end - start);
            text_sam[end - start] = '\0';  // Null-terminate
        }
    }

    // DEBUG: Print extracted text_sam
    //printf("DEBUG: Extracted text_sam:\n%s\n", text_sam);
    //getchar();  // Wait for key press before continuing

    // Display extracted text
    if (strlen(text_display) > 0)
    {
        display_text(text_display);
    }
    else
    {
        printf("Error: No text to display\n");
    }

    // DEBUG: Pause after output
    //printf("\nPress any key to continue...\n");
    //getchar();  // Wait for key press before continuing

    if (speak == true)
    {
        if (strlen(text_sam) > 0)
        {
            speak_text(text_sam);
        }
        else
        {
            printf("Error: No text to speak\n");
        }
    }

    // DEBUG: Pause after output
    //printf("\nPress any key to continue...\n");
    //getchar();  // Wait for key press before continuing
}

// Convert UTF-8 characters to ASCII equivalents and replace them in text
void process_text(char *text)
{
    char *src = text, *dst = text;
    unsigned int unicode_char;

    while (*src) {
        if ((unsigned char)*src == '\\' && *(src + 1) == 'n')
        {
            *dst++ = 0x9B; // Convert "\n" to ATASCII newline
            src += 2;
        }
        else if ((unsigned char)*src >= 0xC0 && (unsigned char)*src <= 0xDF && *(src + 1))
        {
            unicode_char = (((unsigned char)*(src) & 0x1F) << 6) | ((unsigned char)*(src + 1) & 0x3F);
            switch (unicode_char) {
                // Polish characters
                case 0x0105: *dst++ = 'a'; break; // ą
                case 0x0107: *dst++ = 'c'; break; //  ć
                case 0x0119: *dst++ = 'e'; break; //  ę
                case 0x0142: *dst++ = 'l'; break; //  ł
                case 0x0144: *dst++ = 'n'; break; //  ń
                case 0x00F3: *dst++ = 'o'; break; //  ó
                case 0x015B: *dst++ = 's'; break; //  ś
                case 0x017A: *dst++ = 'z'; break; //  ź
                case 0x017C: *dst++ = 'z'; break; //  ż
        
                // German characters
                case 0x00E4: *dst++ = 'a'; break; // ä
                case 0x00F6: *dst++ = 'o'; break; // ö
                case 0x00FC: *dst++ = 'u'; break; // ü
                case 0x00DF: *dst++ = 's'; break; // ß
        
                // French characters
                case 0x00E0: *dst++ = 'a'; break; // à
                case 0x00E2: *dst++ = 'a'; break; // â
                case 0x00E7: *dst++ = 'c'; break; // ç
                case 0x00E9: *dst++ = 'e'; break; // é
                case 0x00E8: *dst++ = 'e'; break; // è
                case 0x00EA: *dst++ = 'e'; break; // ê
                case 0x00EB: *dst++ = 'e'; break; // ë
                case 0x00EE: *dst++ = 'i'; break; // î
                case 0x00EF: *dst++ = 'i'; break; // ï
                case 0x00F4: *dst++ = 'o'; break; // ô
                case 0x00F9: *dst++ = 'u'; break; // ù
                case 0x00FB: *dst++ = 'u'; break; // û
        
                // Spanish characters
                case 0x00E1: *dst++ = 'a'; break; // á
                case 0x00ED: *dst++ = 'i'; break; // í
                case 0x00F1: *dst++ = 'n'; break; // ñ
                case 0x00FA: *dst++ = 'u'; break; // ú
        
                // Italian characters
                case 0x00EC: *dst++ = 'i'; break; // ì
                case 0x00F2: *dst++ = 'o'; break; // ò
        
                default: *dst++ = '_'; break; // Replace unsupported characters
            }
            src += 2; // Skip UTF-8 second byte
        }
        else
        {
            *dst++ = *src++;
        }
    }
    *dst = '\0'; // Null terminate
}

// Display text on Atari screen with word wrap
void display_text(char *text)
{
    int line_length = 0, word_length = 0, i = 0, w = 0, lines = 0;
    char *word_start;
    char *pos;

    process_text(text); // Convert UTF-8 and prepare text
    pos = text;

    while (*text)
    {
        word_start = text;
        while (*text && !isspace(*text))
        {
            text++; // Move to end of word
        }
        word_length = text - word_start;

        if (line_length + word_length >= SCREEN_WIDTH - 1)
        {
            putchar(0x9B); // Move to next line if word doesn't fit
            line_length = 0;
            lines++;
        }

        // Print each character from the processed text
        for (i = 0; i < word_length; i++)
        {
            putchar(word_start[i]);
        }
        line_length += word_length;

        while (*text && isspace(*text))
        {
            putchar(*text == '\n' ? 0x9B : *text); // Convert '\n' to ATASCII newline
            if (*text == '\n')
            {
                line_length = 0;
                lines++;
            }
            else
            {
                line_length++;
            }
            text++;
        }

        // Pause for reading if screen full
        if (lines >= SCREEN_HEIGHT)
        {
            printf("\nPress any key to continue...\n");
            getchar();  // Wait for key press before continuing
            lines = 0;
        }
    }
    putchar(0x9B);
}

// Speak text using FujiNet SAM
void speak_text(const char *sam_text)
{
    int len = strlen(sam_text);
    int start = 0;
    char chunk[SAM_CHUNK_SIZE + 1];

    FILE *printer = fopen("P4:", "w");
    if (!printer)
    {
        printf("Unable to access FujiNet SAM printer device (P4)\nTurning off speech.");
        speak = false;
        return;
    }

    while (start < len)
    {
        int end = start + SAM_CHUNK_SIZE;
        if (end > len)
        {
            end = len;
        }
        else
        {
            while (end > start && !isspace(sam_text[end]))
            {
                end--;  // Move back to a space to avoid breaking words
            }
            if (end == start)
            {
                end = start + SAM_CHUNK_SIZE; // If no space found, force break
            }
        }

        strncpy(chunk, &sam_text[start], end - start);
        chunk[end - start] = '\0';

        fprintf(printer, "%s\n", chunk);
        start = end + 1;  // Move past the space
    }

    fclose(printer);
}

// Escape special characters in user input for JSON compatibility
void escape_json_string(const char *input, char *output, int output_size)
{
    int i = 0, j = 0;
    while (input[i] != '\0' && j < output_size - 2) // Leave space for null terminator
    {
        if (input[i] == '"' || input[i] == '\\' || input[i] == 0x39)
        {
            if (j < output_size - 3) // Ensure enough space for escape character
            {
                output[j++] = '\\';
            }
            else
            {
                break; // Avoid buffer overflow
            }
        }
        output[j++] = input[i++];
    }
    output[j] = '\0'; // Null-terminate the output string
}

void get_user_input(char *buffer, int max_length)
{
    int index = 0;
    char ch;

    while (1)
    {
        ch = cgetc(); // Read keypress

        if (ch == '\r' || ch == '\n') // Enter key
        {
            buffer[index] = '\0'; // Null-terminate string
            printf("\n"); // Move to new line
            break;
        }
        else if ((ch == 0x08 || ch == 0x7F) && index > 0) // Handle Backspace
        {
            index--;
            printf("\b \b"); // Erase last character on screen
        }
        else if (index < max_length - 1 && ch >= 32 && ch <= 126) // Normal character
        {
            buffer[index++] = ch;
            putchar(ch); // Echo character
        }
    }
}

void print_help()
{
    printf(" HELP       Prints this message\n");
    printf(" EXIT       Exit the program\n");
    printf(" SPEAKOFF   Turn OFF SAM audio\n");
    printf(" SPEAKON    Turn ON SAM audio\n");
    printf(" CLS        Clear the screen\n");
}

int main()
{
    bool res = false;

    if (!init_fujinet())
    {
        return 1;  // Exit if FujiNet is not available
    }

    printf("         Welcome to AI SAM!\n");
    printf("   Type HELP for a list of commands\n");
    printf("           Ask me anything...");
    speak_text("I AEM SAEM, YOR FOO-JEE-NET UH-SIS-TUHNT");

    while (1)
    {
        user_input[0] = '\0';
        text_display[0] = '\0';
        text_sam[0] = '\0';
        json_payload[0] = '\0';
        response_buffer[0] = '\0';

        printf("\n> ");
        get_user_input(user_input, sizeof(user_input)-1);
    
        if (strcmp(user_input, "help") == 0 || strcmp(user_input, "HELP") == 0)
        {
            print_help();
        }
        else if (strcmp(user_input, "exit") == 0 || strcmp(user_input, "EXIT") == 0)
        {
            printf("Goodbye!\n");
            break;
        }
        else if (strcmp(user_input, "speakon") == 0 || strcmp(user_input, "SPEAKON") == 0)
        {
            speak = true;
            printf("Turned ON SAM audio output\n");
        }
        else if (strcmp(user_input, "speakoff") == 0 || strcmp(user_input, "SPEAKOFF") == 0)
        {
            speak = false;
            printf("Turned OFF SAM audio output\n");
        }
        else if(strcmp(user_input, "cls") == 0 || strcmp(user_input, "CLS") == 0)
        {
            clrscr();
        }
        else
        {
            res = send_openai_request(user_input);
            if (res)
            {
                // Do something if request failed
            }
        }
    }

    return 0;
}
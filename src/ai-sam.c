#include "ai-sam.h"
#include "config.h"  // OpenAI API key and URL

char response_buffer[RESPONSE_BUFFER_SIZE];
char devicespec[256];
char json_payload[REQUEST_BUFFER_SIZE];
char user_input[256];
char text_display[960] = "";
char text_sam[960] = "";

// Initialize FujiNet device
bool init_fujinet() {
    AdapterConfig config;

    if (!fuji_get_adapter_config(&config)) {
        printf("Error: FujiNet not detected or failed to retrieve configuration!\n");
        return false;
    }

    // Debug: Print firmware version
    //printf("FujiNet Firmware: %s\n", config.fn_version);

    return true;
}

// Send request to OpenAI API
bool send_openai_request(const char *user_input) {
    uint8_t err;

    // Setup devicespec
    snprintf(devicespec, sizeof(devicespec), "N1:%s", OPENAI_API_URL);

    // Get payload ready
    snprintf(json_payload, REQUEST_BUFFER_SIZE,
        "{"
        "\"model\":\"gpt-4o-mini\","
        "\"messages\":["
        "{\"role\":\"system\",\"content\":\"You are a FujiNet and vintage computers expert. Respond with text_display and text_sam. Rules for text_display and text_sam: do NOT use any formatting, special characters, quotation marks, backslashes, special symbols or escape sequences, numbers must be printed as numbers. Rules only applying to text_sam: Numbers must be written as words.\"},"
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
        user_input
    );

    printf("Thinking...\n");

    err = network_open(devicespec, OPEN_MODE_HTTP_POST, OPEN_TRANS_NONE);
    if (err != 0) {
        printf("Error: Unable to open network channel.\n");
        return false;
    }

    err = network_http_start_add_headers(devicespec);
    if (err != 0)
    {
        printf("Error: Unable to start adding headers.\n");
        return false;
    }

    network_http_add_header(devicespec, "Authorization: Bearer " OPENAI_API_KEY);
    network_http_add_header(devicespec, "Content-Type: application/json");
    network_http_end_add_headers(devicespec);

    err = network_http_post(devicespec, json_payload);
    if (err != 0) {
        printf("Error: Failed to send data.\n");
        network_close(devicespec);
        return false;
    }

    err = network_json_parse(devicespec);
    if (err != 0) {
        printf("Error: Failed to parse JSON response.\n");
        network_close(devicespec);
        return false;
    }

    err = network_json_query(devicespec, "/choices/0/message/function_call/arguments", response_buffer);
    if (err == 0) {
        printf("Error: Query failed.\n");
        network_close(devicespec);
        return false;
    }

    network_close(devicespec);
    process_response(response_buffer);
    return true;
}

// Process the JSON Response
void process_response(const char *json_response) {
    char *start, *end;

    // Debug: Print the full JSON response
    //printf("DEBUG: JSON Response:\n%s\n", json_response);
    //getchar();  // Wait for key press before continuing

    // Extract text_display
    start = strstr(json_response, "\"text_display\":\"");
    if (start) {
        start += strlen("\"text_display\":\"");  // Move past key name
        end = strchr(start, '\"');  // Find closing quote
        if (end && (end - start) < sizeof(text_display)) {
            strncpy(text_display, start, end - start);
            text_display[end - start] = '\0';  // Null-terminate
        }
    }

    // Debug: Print extracted text_display
    //printf("DEBUG: Extracted text_display:\n%s\n", text_display);
    //getchar();  // Wait for key press before continuing

    // Extract text_sam
    start = strstr(json_response, "\"text_sam\":\"");
    if (start) {
        start += strlen("\"text_sam\":\"");  // Move past key name
        end = strchr(start, '\"');  // Find closing quote
        if (end && (end - start) < sizeof(text_sam)) {
            strncpy(text_sam, start, end - start);
            text_sam[end - start] = '\0';  // Null-terminate
        }
    }

    // Debug: Print extracted text_sam
    //printf("DEBUG: Extracted text_sam:\n%s\n", text_sam);
    //getchar();  // Wait for key press before continuing

    // Display extracted text
    if (strlen(text_display) > 0)
    {
        display_text(text_display);
    }
    else
    {
        printf("DEBUG: text_display was empty!\n");
    }

    // Debug: Pause after output
    //printf("\nPress any key to continue...\n");
    //getchar();  // Wait for key press before continuing

    if (strlen(text_sam) > 0)
    {
        speak_text(text_sam);
    }
    else
    {
        printf("DEBUG: text_sam was empty!\n");
    }

    // Debug: Pause after output
    //printf("\nPress any key to continue...\n");
    //getchar();  // Wait for key press before continuing
}


// Display text on Atari screen with word wrap
void display_text(const char *text) {
    int line_length = 0, word_length = 0;
    const char *word_start;

    while (*text) {
        word_start = text;
        while (*text && !isspace(*text)) {
            text++; // Move to end of word
        }
        word_length = text - word_start;

        if (line_length + word_length >= SCREEN_WIDTH) {
            printf("\n"); // Move to next line if word doesn't fit
            line_length = 0;
        }

        fwrite(word_start, 1, word_length, stdout); // Print word
        line_length += word_length;

        while (*text && isspace(*text)) {
            putchar(*text); // Print spaces/newlines
            if (*text == '\n') {
                line_length = 0; // Reset for new line
            } else {
                line_length++;
            }
            text++;
        }
    }
    printf("\n");
}

// Speak text using FujiNet SAM
void speak_text(const char *sam_text) {
    int len = strlen(sam_text);
    int start = 0;
    char chunk[SAM_CHUNK_SIZE + 1];

    FILE *printer = fopen("P4:", "w");
    if (!printer) {
        printf("Error: Unable to access FujiNet SAM printer device (P4)\n");
        return;
    }

    while (start < len) {
        int end = start + SAM_CHUNK_SIZE;
        if (end > len) {
            end = len;
        } else {
            while (end > start && !isspace(sam_text[end])) {
                end--;  // Move back to a space to avoid breaking words
            }
            if (end == start) {
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

int main() {
    bool res = false;

    if (!init_fujinet())
    {
        return 1;  // Exit if FujiNet is not available
    }

    printf("Welcome to AI-SAM!\n");
    speak_text("I AEM SAEM, YOR FOO-JEE-NET UH-SIS-TUHNT");

    while (1) {
        printf("\nYou: ");
        fgets(user_input, sizeof(user_input), stdin);

        // Remove newline character from input
        user_input[strcspn(user_input, "\n")] = 0;

        if (strcmp(user_input, "exit") == 0) {
            printf("Goodbye!\n");
            break;
        }

        res = send_openai_request(user_input);
        if (res)
        {
            // Do something if request failed
            //process_response(response_buffer);
        }
    }

    return 0;
}
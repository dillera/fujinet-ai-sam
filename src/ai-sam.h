#ifndef AI_SAM_H
#define AI_SAM_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <conio.h>

// FujiNet library includes
#include "../fujinet-lib/fujinet-network.h"
#include "../fujinet-lib/fujinet-fuji.h"
#include "../fujinet-lib/fujinet-clock.h"

// Buffer Sizes
#define RESPONSE_BUFFER_SIZE 3072
#define REQUEST_BUFFER_SIZE 2048
#define SAM_CHUNK_SIZE 100

#define SCREEN_WIDTH 40
#define SCREEN_HEIGHT 20

// Variables
char response_buffer[RESPONSE_BUFFER_SIZE];
char devicespec[256];
char json_payload[REQUEST_BUFFER_SIZE];
char user_input[1024];
char escaped_input[1200];
char text_display[960] = "";
char text_sam[960] = "";
bool speak = true;

// Functions
bool init_fujinet();
bool send_openai_request(char *user_input);
void process_response(const char *json_response);
void display_text(char *text);
void speak_text(const char *sam_text);
void escape_json_string(const char *input, char *output, int output_size);
void get_user_input(char *buffer, int max_length);
void print_help();
void process_text(char *text);

#endif // AI_SAM_H

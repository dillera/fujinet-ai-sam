#ifndef AI_SAM_H
#define AI_SAM_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>

// FujiNet library includes
#include "../fujinet-lib/fujinet-network.h"
#include "../fujinet-lib/fujinet-fuji.h"
#include "../fujinet-lib/fujinet-clock.h"

// Buffer Sizes
#define RESPONSE_BUFFER_SIZE 1024
#define REQUEST_BUFFER_SIZE 2048
#define SAM_CHUNK_SIZE 100

#define SCREEN_WIDTH 40

// Functions
bool init_fujinet();
bool send_openai_request(const char *user_input);
void process_response(const char *json_response);
void display_text(const char *text);
void speak_text(const char *sam_text);

#endif // AI_SAM_H

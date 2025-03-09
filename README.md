# FujiNet SAM AI Chatbot

This is a simple interface using the OpenAI API to create a chatbot that runs on Atari 8-Bit computers using FujiNet. If using real FujiNet hardware the SAM emulation in FujiNet will also speak the response.

# Build

1. Download latest [fujinet-lib](https://github.com/FujiNetWIFI/fujinet-lib/releases) for Atari and place the contents in `fujinet-ai-sam/fujinet-lib`
2. Make you have cc65 installed
3. Modify `src/config.h` with your OpenAI API
4. run `make` to compile the program

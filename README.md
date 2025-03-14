# FujiNet SAM AI Chatbot

This is a simple interface using the OpenAI API to create a chatbot that runs on Atari 8-Bit computers using FujiNet. If using real FujiNet hardware the SAM emulation in FujiNet will also speak the response.

# Build

1. Download latest [fujinet-lib](https://github.com/FujiNetWIFI/fujinet-lib/releases) for Atari and place the contents in `fujinet-ai-sam/fujinet-lib`
2. Make you have cc65 installed
3. Modify `src/config.h` with your OpenAI API
4. run `make` to compile the program

# Notes

 * If you make the program publicly available, it is recommended to use a proxy for the API calls as it is trivial to extract the API key from the binary. Example proxy server written in PHP is included in this repo. Set `OPENAI_API_URL` in `config.h` to point to the PHP script on your server and set `$API_KEY` in the PHP script to your actual API key.
 * Neither the API nor Atari program keep track of the conversation. Each request is new new question and the AI has no idea what you asked it previously. This could be done and I may implement something server side with the proxy and using the FujiNet Appkeys.
 * The JSON payload in `send_openai_request()` sent to the API contains a system role message which defines the "purpose" of the AI and can be used to set limits. It currently has a function call defined to output text for display on screen and a SAM formatted text for output to the FujiNet SAM emulation. More information can be found in the [OpenAI API documentation](https://platform.openai.com/docs/overview).
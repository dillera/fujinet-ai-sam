<?php
// OpenAI API key
$API_KEY = "YOUR OPENAI API KEY";

// Only accept POST requests
if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    http_response_code(405);
    echo json_encode(["error" => "Method Not Allowed"]);
    exit;
}

// Read JSON input from the Atari program
$inputJSON = file_get_contents("php://input");
$decodedInput = json_decode($inputJSON, true);

// Ensure request contains valid JSON
if (!$decodedInput) {
    http_response_code(400);

    // Log the invalid JSON to "invalid.log"
    //file_put_contents("invalid.log", date("[Y-m-d H:i:s]") . " Invalid JSON received:\n" . $inputJSON . "\n\n", FILE_APPEND);

    echo json_encode(["error" => "Invalid JSON input"]);
    exit;
}

// Setup OpenAI API request
$url = "https://api.openai.com/v1/chat/completions";
$options = [
    "http" => [
        "header"  => "Authorization: Bearer $API_KEY\r\n" .
                     "Content-Type: application/json\r\n",
        "method"  => "POST",
        "content" => json_encode($decodedInput),
    ]
];

// Send request to OpenAI
$context  = stream_context_create($options);
$response = file_get_contents($url, false, $context);

// Return OpenAI's response
if ($response === FALSE) {
    http_response_code(500);
    echo json_encode(["error" => "Failed to connect to OpenAI"]);
} else {
    echo $response;
}
?>

<?php
header("Content-Type: application/json");
// PHP code
//Create a UDP socket
if(!($sock = socket_create(AF_INET, SOCK_DGRAM, 0)))
{
    $errorcode = socket_last_error();
    $errormsg = socket_strerror($errorcode);
    
    die("Couldn't create socket: [$errorcode] $errormsg \n");
}

// Bind the source address
if( !socket_bind($sock, "127.0.0.1" , 65000) )
{
    $errorcode = socket_last_error();
    $errormsg = socket_strerror($errorcode);
    
    die("Could not bind socket : [$errorcode] $errormsg \n");
}

$temperature = 0;
$navg = 10;
for ($x = 0; $x <= $navg; $x++) {
    //Receive some data
    $r = socket_recvfrom($sock, $buf, 512, 0, $remote_ip, $remote_port);
    $temperature = $temperature + (float)$buf / 65536 * 2.5 * 0.6 * 100;
}

$temperature = $temperature / $navg;
socket_close($sock);

$response = array(
    'status' => true,
    'data' => $temperature,
    'epoch' => microtime(true)
);

echo json_encode($response);

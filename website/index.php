<!DOCTYPE html>
<html>
<head>
<title>Welcome to the Realtime Embedded Test server</title>
<style>
    body {
        width: 35em;
        margin: 0 auto;
        font-family: Tahoma, Verdana, Arial, sans-serif;
    }
</style>
</head>
<body>
  <h1>Welcome to Realtime Embedded</h1>

  <?php
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

    echo "<h2>LM35 emperature</h2>";
    for ($x = 0; $x <= 10; $x++) {
        //Receive some data
        $r = socket_recvfrom($sock, $buf, 512, 0, $remote_ip, $remote_port);
        $temperature = (float)$buf / 65536 * 2.5 * 0.6 * 100;
	echo "<p>".$temperature."C</p>";
    }

    // this data could now be fed into a java script plotting program		   

    socket_close($sock);

?>
  
</body>
</html>

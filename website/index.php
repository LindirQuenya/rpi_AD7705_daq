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
	<script src="//cdnjs.cloudflare.com/ajax/libs/dygraph/2.1.0/dygraph.min.js"></script>
	<link rel="stylesheet" href="//cdnjs.cloudflare.com/ajax/libs/dygraph/2.1.0/dygraph.min.css" />
    </head>
    <body>
	<h1>Welcome to Realtime Embedded</h1>

	<?php
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
	echo "<h2>LM35 temperature sensor connected to the PI</h2>";
	for ($x = 0; $x <= $navg; $x++) {
            //Receive some data
            $r = socket_recvfrom($sock, $buf, 512, 0, $remote_ip, $remote_port);
            $temperature = $temperature + (float)$buf / 65536 * 2.5 * 0.6 * 100;
	}
	$temperature = $temperature / $navg;
	socket_close($sock);

	echo "<p>Current temperature in Bernd's house: ".$temperature." &#8451;</p>";

	$output = fopen("data.txt",'a') or die("Can't save");
	$p = array(round(microtime(true)*1000),$temperature);
	fputcsv($output, $p);
	fclose($output) or die("Can't close");

	?>

	<div id="graphdiv"></div>
	<script type="text/javascript">
	 g = new Dygraph(
	     document.getElementById("graphdiv"),
	     "data.txt",
             {
                 legend:'always',
                 ticker: Dygraph.dateTicker
             }
	 );
	</script>

<br />
<br />
<br />
<br />
<br />
<br />
<br />
<br />
<br />
<br />

<h2>References</h2>

<p><a href="http://dygraphs.com/">dygraphs</p>

<p><a href="https://github.com/berndporr/rpi_AD7705_daq">github repo</a></p>

<br />
<br />
<br />
<br />
<br />
<br />
<br />
<br />
<br />
<br />

<p><a href="textonly.php">Text only version</a></p>
	
    </body>
</html>

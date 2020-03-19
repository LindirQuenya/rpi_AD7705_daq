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
	<script src="https://ajax.googleapis.com/ajax/libs/jquery/3.4.1/jquery.min.js"></script>
	<script src="//cdnjs.cloudflare.com/ajax/libs/dygraph/2.1.0/dygraph.min.js"></script>
	<link rel="stylesheet" href="//cdnjs.cloudflare.com/ajax/libs/dygraph/2.1.0/dygraph.min.css" />
    </head>
    <body>
	<h2>Realtime data plot with JSON data transfer</h2>

	<p>This is a realtime demo where the java script requests the data
	  as JSON from the server and then appends it to the plot every second.</p>
	
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



	<div id="div_g" style="width:600px; height:300px;"></div>
	
	<script type="text/javascript">
	  var maxSamples = 60;
	  // callback when the Web page has been loaded
	  $(document).ready(function() {
	      var data = [];
	      var g = new Dygraph(document.getElementById("div_g"), data,
				  {
				      drawPoints: true,
				      valueRange: [0.0, 100],
				      labels: ['Time', 'Temperature'],
				  });
	      window.intervalId = setInterval(function() {
		  // callback for interval timer for every second
		  $.getJSON("json.php",function(result){
		      // callback after the php script has been called
		      var utcSeconds = result.epoch;
		      var d = new Date(0);
		      d.setUTCSeconds(utcSeconds);
		      var x = d;
		      var y = result.data;
		      if (data.length > maxSamples) {
			  data.shift();
		      }
		      data.push([x, y]);
		      g.updateOptions( { 'file': data } );
		  });
	      }, 1000);
	  });
	</script>
	
	
	<br />
	<br />
<br />
<br />
<br />
<br />
<p><a href="/">Main page</a></p>
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

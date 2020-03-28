<!DOCTYPE html>
<html>
    <head>
	<meta name="viewport" content="width=device-width, initial-scale=1.0">
	<meta name="author" content="Bernd Porr">
	<meta http-equiv="refresh" content="60">
	<title>Welcome to the Realtime Embedded Test server</title>
	<script src="https://code.jquery.com/jquery-3.4.1.slim.min.js" integrity="sha384-J6qa4849blE2+poT4WnyKhv5vZF5SrPo0iEjwBvKU7imGFAV0wwj1yYfoRSJoZ+n" crossorigin="anonymous"></script>
	<script src="https://cdn.jsdelivr.net/npm/popper.js@1.16.0/dist/umd/popper.min.js" integrity="sha384-Q6E9RHvbIyZFJoft+2mJbHaEWldlvI9IOYy5n3zV9zzTtmI3UksdQRVvoxMfooAo" crossorigin="anonymous"></script>
	<link rel="stylesheet" href="https://stackpath.bootstrapcdn.com/bootstrap/4.4.1/css/bootstrap.min.css" integrity="sha384-Vkoo8x4CGsO3+Hhxv8T/Q5PaXtkKtu6ug5TOeNV6gBiFeWPGFN9MuhOf23Q9Ifjh" crossorigin="anonymous">
	<script src="https://stackpath.bootstrapcdn.com/bootstrap/4.4.1/js/bootstrap.min.js" integrity="sha384-wfSDF2E50Y2D1uUdj0O3uMBJnjuUD4Ih7YwaYd1iqfktj0Uod8GCExl3Og8ifwB6" crossorigin="anonymous"></script>
	<script src="//cdnjs.cloudflare.com/ajax/libs/dygraph/2.1.0/dygraph.min.js"></script>
	<link rel="stylesheet" href="//cdnjs.cloudflare.com/ajax/libs/dygraph/2.1.0/dygraph.min.css" />
    </head>
    <body>
	<div class="container">
	    <div class="row">
		<div class="col-md-2"></div>
		<div class="col-md-8">
		    <h2>Welcome to Realtime Embedded Coding 5</h2>
		    <p>LM35 temperature sensor connected to the PI</p>

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
		    if( socket_bind($sock, "127.0.0.1" , 65000) )
		    {
			// success!
			$temperature = 0;
			$navg = 10;
			for ($x = 0; $x <= $navg; $x++) {
			    //Receive some data
			    $r = socket_recvfrom($sock, $buf, 512, 0, $remote_ip, $remote_port);
			    $temperature = $temperature + (float)$buf / 65536 * 2.5 * 0.6 * 100;
			}
			$temperature = $temperature / $navg;
			$temperature = round($temperature*100)/100;
			socket_close($sock);
			
			echo "<p>Current temperature in Bernd's house: ".$temperature." &#8451;</p>";
			
			$output = fopen("data.txt",'a') or die("Can't save");
			$p = array(round(microtime(true)*1000),$temperature);
			fputcsv($output, $p);
			fclose($output) or die("Can't close");
		    }
		    
		    ?>
		    <div class="embed-responsive">
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
		    </div>
		    
		    <br />
		    <p>Try it out on your mobile. You can zoom in/out and pan across the data!</p>
		    <p><a href="animate.php">Realtime animation demo (not responsive yet).</a></p>
		    <br />
		    <br />
		    <br />
		    <br />
		    <br />
		    <br />
		    <br />
		    <hr />
		    <br />
		    <br />
		    <br />
		    <br />
		    <br />
		    <br />
		    
		    <h3>References</h3>
		    <ul>
			<li><a href="http://dygraphs.com/">dygraphs</a></li>
			<li><a href="https://getbootstrap.com/">Bootstrap CSS/JS for responsive web design</a></li>
			<li><a href="https://github.com/berndporr/rpi_AD7705_daq">github repo</a></li>
		    </ul>
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
		</div>
		<div class="col-md-2"></div>
	    </div>
	</div>
    </body>
</html>

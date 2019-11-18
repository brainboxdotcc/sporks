<?php
$settings = json_decode(file_get_contents("config.json"));
$conn = mysqli_connect($settings->dbhost, $settings->dbuser, $settings->dbpass);

if (!$conn) {
	die("Can't connect to database, check config.json\n");
}

mysqli_select_db($conn, $settings->dbname);

while (true) {

	$q = mysqli_query($conn, "SELECT * FROM infobot_web_requests WHERE statuscode = '000'");

	while ($rs = mysqli_fetch_object($q)) {
		if (preg_match('/^\{.+?\}$/im', $rs->postdata) && $rs->type == 'POST') {
			$mt = "application/json";
		} else {
			$mt = "application/x-www-form-urlencoded";
		}
		$response = @file_get_contents($rs->url, false, stream_context_create([
		'http' => [
				'method' => $rs->type,
				'ignore_errors' => true,
				'header'  => "Content-Type: $mt",
				'content' => $rs->postdata,
			]
		]));
		if ($response === FALSE) {
			mysqli_query($conn, "UPDATE infobot_web_requests SET statuscode = '999', returndata = '' WHERE channel_id = ".$rs->channel_id);
			echo $rs->url . " => Connection failure\n";
		} else {
			list($proto, $status, $msg) = explode(' ', $http_response_header[0]);
			mysqli_query($conn, "UPDATE infobot_web_requests SET statuscode = '".mysqli_real_escape_string($conn, $status)."', returndata = '".mysqli_real_escape_string($conn, $response)."' WHERE channel_id = ".$rs->channel_id);
			echo $rs->url . " => " . $http_response_header[0] . "\n";
		}
	}

	sleep(1);
}


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
		if (!preg_match("/^http:\/\/i/", $rs->url) && !preg_match("/^https:\/\/i/", $rs->url)) {
			/* Prevent access to local files, stream context may be ignored. FU PHP. */
			$response = FALSE;
		}
		/* Maximum of 1 meg actually retrieved, truncated after this */
		$headers = ['User-Agent: Sporks/1.2', 'Content-Type: ' . $mt];

		$ch = curl_init($rs->url);
		curl_setopt_array($ch, [
			CURLOPT_HTTP_VERSION    =>      CURL_HTTP_VERSION_1_1,
			CURLOPT_FOLLOWLOCATION  =>      1,
			CURLOPT_HEADER	 	=>      0,
			CURLOPT_RETURNTRANSFER  =>      1,
			CURLOPT_CUSTOMREQUEST   =>      $rs->type,
			CURLOPT_HTTPHEADER      =>      $headers,
		]);
		if ($rs->type == "POST") {
			curl_setopt_array($ch, [
				CURLOPT_POST		=> 1,
				CURLOPT_POSTFIELDS	=> $rs->postdata,
			]);
		}
		curl_setopt($ch, CURLOPT_BUFFERSIZE, 128); // more progress info
		curl_setopt($ch, CURLOPT_NOPROGRESS, false);
		curl_setopt($ch, CURLOPT_PROGRESSFUNCTION, function($DownloadSize, $Downloaded, $UploadSize, $Uploaded) {
			return ($Downloaded > (1024 * 1024)) ? 1 : 0;
		});

		$response = curl_exec($ch);
		$code = curl_getinfo($ch, CURLINFO_RESPONSE_CODE);

		if ($response === FALSE) {
			mysqli_query($conn, "UPDATE infobot_web_requests SET statuscode = '999', returndata = '' WHERE channel_id = ".$rs->channel_id);
			echo $rs->url . " => Connection failure\n";
		} else {
			list($proto, $status, $msg) = explode(' ', $http_response_header[0]);
			mysqli_query($conn, "UPDATE infobot_web_requests SET statuscode = '$code', returndata = '".mysqli_real_escape_string($conn, $response)."' WHERE channel_id = ".$rs->channel_id);
			echo $rs->url . " => $code\n";
		}
	}

	sleep(1);
}


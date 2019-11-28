<?php

/***********************************************************************************************************
 *
 * Bot list updater for Botnix
 *
 * There are a ton of discord bot listing sites out there, they all seem to have similar-ish APIs which let
 * you update your guild count and user count via a http POST with an Authorisation header. To allow this to
 * be done on a more or less generic basis, this script accepts details of these sites from a mysql table
 * (infobot_discord_list sites) and submits totals to them from all shards. Each shard has individual
 * responsibility for updating its row in the infobot_discord_counts table into the mysql cluster.
 *
 * Don't crontab this script more often than an hour, as you don't weant to annoy these sites by spamming
 * their API endpoints with updates!
 *
 ***********************************************************************************************************/

$settings = json_decode(file_get_contents("config.json"));
$conn = mysqli_connect($settings->dbhost, $settings->dbuser, $settings->dbpass);

if (!$conn) {
	die("Can't connect to database, check config.json\n");
}

mysqli_select_db($conn, $settings->dbname);

$payload = [
	'api_key' => $settings->utr_readonly_key,
	'format' => 'json',
	'logs' => '0',
	'types' => '5',
	'search' => 'Discord Bot Shard ',
];
$context = [
	'http' => [
			'method' => 'POST',
			'ignore_errors' => true,
			'header'  => "Content-Type: application/x-www-form-urlencoded\r\nCache-Control: no-cache",
			'content' => http_build_query($payload),
		]
	];
$response = @file_get_contents("https://api.uptimerobot.com/v2/getMonitors", false, stream_context_create($context));
$monitors = json_decode($response);
foreach($monitors->monitors as $monitor) {
	if (preg_match("/^Discord Bot Shard (\d+)$/", $monitor->friendly_name, $m)) {
		$shard_id = $m[1];
		$url = $monitor->url;
		$shardinfo = mysqli_fetch_object(mysqli_query($conn, "SELECT * FROM infobot_shard_status WHERE id = " . mysqli_real_escape_string($conn, $m[1])));
		if ($shardinfo) {
			mysqli_query($conn, "UPDATE infobot_shard_status SET uptimerobot_heartbeat = '" . mysqli_real_escape_string($conn, $monitor->url) . "' WHERE id = " . mysqli_real_escape_string($conn, $m[1]));
			if ($shardinfo->connected && $shardinfo->online) {
				echo "Shard " . $m[1] . " => " . file_get_contents($monitor->url) . "\n";
			}
		}
	}
}

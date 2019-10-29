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
/* Fetch statistics from all shards for non-development instances */
$totals = mysqli_fetch_object(mysqli_query($conn, "SELECT SUM(user_count) AS users, SUM(server_count) AS servers FROM infobot_discord_counts WHERE dev = 0"));

/* Get a list of all bot listing sites */
$q = mysqli_query($conn, "SELECT * FROM infobot_discord_list_sites");
while ($site = mysqli_fetch_object($q)) {
	$payload = new stdClass;
	$payload->{$site->server_count_field} = $totals->servers;
	if (!empty($site->user_count_field)) {
		$payload->{$site->user_count_field} = $totals->users;
	}
	$json_payload = json_encode($payload);
	@file_get_contents($site->url, false, stream_context_create([
	'http' => [
			'method' => 'POST',
			'header'  => "Content-Type: application/json\r\nAuthorization: " . $site->authorization,
			'content' => $json_payload,
		]
	]));
	echo "\n";
}


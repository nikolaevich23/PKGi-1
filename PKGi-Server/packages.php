<?php
header("Content-Type:application/json");

// Database settings

// Create a table "packages" with inside :
// - id       | int          | primary keys
// - name     | varchar(255) | N/A
// - ico_link | varchar(255) | N/A
// - pkg_link | varchar(255) | N/A
// Note : N/A = Nothing to do

// Connect to database
$pdo = new PDO('mysql:host=localhost;dbname=pkgi', 'root', 'root');


$line = 5;
$page = 0;

if ($_GET['line']) {
	$line = intval($_GET['line']);
}

if ($_GET['page']) {
	$page = intval($_GET['page']);
	$startfrom = ($_GET['page'] - 1) * $line;
} else {
	$page = 0;
	$startfrom = 0;
}

$total_nbr = 0;

if (!$_GET['search']) {
	$req = $pdo->prepare("SELECT COUNT(*) FROM packages");
	$req->execute();
	$total_nbr = $req->fetchColumn();

	$req = $pdo->prepare("SELECT id, name, download FROM packages ORDER BY id ASC LIMIT ?, ?");
	$req->bindParam(1, intval(trim($startfrom)), PDO::PARAM_INT);
	$req->bindParam(2, intval(trim($line)), PDO::PARAM_INT);
} else {
	$search = "%".$_GET['search']."%";
	$req = $pdo->prepare("SELECT COUNT(*) FROM packages WHERE name LIKE ?");
	$req->bindParam(1, $search);
	$req->execute();

	$total_nbr = $req->fetchColumn();

	$req = $pdo->prepare("SELECT id, name, download FROM packages WHERE name LIKE ? ORDER BY id ASC LIMIT ?, ?");
	$req->bindParam(1, $search);
	$req->bindParam(2, intval(trim($startfrom)), PDO::PARAM_INT);
	$req->bindParam(3, intval(trim($line)), PDO::PARAM_INT);
}

$req->execute();
$data = $req->fetchAll(PDO::FETCH_OBJ);

$data = array(
	"total" => $total_nbr,
	"page" => $page,
	"line" => $line,
	"packages" => $data
);

$json_data = json_encode($data, JSON_NUMERIC_CHECK);
header("Content-length: " . strlen($json_data));
echo $json_data;

?>
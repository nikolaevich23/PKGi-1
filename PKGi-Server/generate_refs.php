<?php
$baseurl = "http://localhost/pkgi/";

$id = intval($_GET['id']);
$path = "pkgs/".$id.".pkg";
$ref_path = "refs/".$id.".json";

$handle = @fopen($path, "r");
fseek($handle, 0xFE0);
$digest = bin2hex(fread ($handle , 0x20 ));
fclose($handle);
$filesize = filesize($path);
$fake_piece_hash = "0000000000000000000000000000000000000000";

$pieces = [ 
	array(
		"url" => $baseurl.$path,
		"fileOffset" => 0,
		"fileSize" => $filesize,
		"hashValue" => $fake_piece_hash
	)
];

$data = array(
	"originalFileSize" => $filesize,
	"packageDigest" => strtoupper($digest),
	"numberOfSplitFiles" => 1,
	"pieces" => $pieces
);

$json_data = json_encode($data);
file_put_contents($ref_path, $json_data);

echo "Generated.";
?>
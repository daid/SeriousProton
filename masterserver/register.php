<?
require("config.inc.php");

if (!isset($_POST['port']))
{
    http_response_code(400);
    die("'port' not set");
}
if (!isset($_POST['name']))
{
    http_response_code(400);
    die("'name' not set");
}
if (!isset($_POST['version']))
{
    http_response_code(400);
    die("'version' not set");
}

$port = (int)($_POST['port']);
$name = $_POST['name'];
$version = (int)($_POST['version']);

$db = new PDO($database['dsn'], $database['user'], $database['password']);
$db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);

$db->exec("CREATE TABLE IF NOT EXISTS `servers` (
  `ip` varchar(32) NOT NULL,
  `port` int(11) NOT NULL,
  `version` int(11) NOT NULL,
  `last_update` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `name` varchar(255) NOT NULL,
  PRIMARY KEY (`ip`,`port`)
);");

$q = $db->prepare("INSERT INTO `servers`(`ip`, `port`, `version`, `name`) VALUES (:ip, :port, :version, :name) ON DUPLICATE KEY UPDATE `name` = :name, `version` = :version, `last_update` = NOW()");
$q->execute(array("ip" => $_SERVER['REMOTE_ADDR'], "port" => $port, "version" => $version, "name" => $name));

?>
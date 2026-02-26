<?php
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

// Optional IP override: used when the game server tunnels through a reverse
// proxy and wants to advertise the proxy's address instead of its own NAT'd IP.
$ip = $_SERVER['REMOTE_ADDR'];
if (isset($_POST['ip']))
{
    $candidate = trim($_POST['ip']);
    if (strlen($candidate) > 0 && strlen($candidate) <= 253 && preg_match('/^[0-9a-zA-Z:.\-]+$/', $candidate))
        $ip = $candidate;
    else
    {
        http_response_code(400);
        die("Invalid 'ip'");
    }
}

$db = new PDO($database['dsn'], $database['user'], $database['password']);
$db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);

$db->exec("CREATE TABLE IF NOT EXISTS `servers` (
  `ip` varchar(253) NOT NULL,
  `port` int(11) NOT NULL,
  `version` int(11) NOT NULL,
  `last_update` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `name` varchar(255) NOT NULL,
  PRIMARY KEY (`ip`,`port`)
);");

$q = $db->prepare("SELECT * FROM `servers` WHERE `ip` = :ip AND `port` = :port AND `version` = :version");
$result = $q->execute(array("ip" => $ip, "port" => $port, "version" => $version));
if($q->fetch() === false)
{
    //First time server is added. Try if we can connect to the server. If not, then port forwarding is most likely not setup.
    $socket = socket_create(AF_INET, SOCK_STREAM, SOL_TCP);
    $result = @socket_connect($socket, $ip, $port);
    if ($result === false)
    {
        die("CONNECT FAILED: " . $ip . ":" . $port);
    }else{
        socket_close($socket);
    }
}

$q = $db->prepare("INSERT INTO `servers`(`ip`, `port`, `version`, `name`) VALUES (:ip, :port, :version, :name) ON DUPLICATE KEY UPDATE `name` = :name, `version` = :version, `last_update` = NOW()");
$q->execute(array("ip" => $ip, "port" => $port, "version" => $version, "name" => $name));

echo "OK";
?>

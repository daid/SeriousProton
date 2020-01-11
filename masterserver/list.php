<?php
require("config.inc.php");

$db = new PDO($database['dsn'], $database['user'], $database['password']);
$db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);

$db->exec("DELETE FROM `servers` WHERE `last_update` < DATE_SUB(NOW(), INTERVAL 5 MINUTE)");

$q = $db->query("SELECT * FROM `servers`");
$q->setFetchMode(PDO::FETCH_OBJ);
while($row = $q->fetch())
{
    printf("%s:%d:%d:%s\n", $row->ip, $row->port, $row->version, $row->name);
}

?>

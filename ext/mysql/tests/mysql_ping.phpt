--TEST--
mysql_ping()
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
    include_once "connect.inc";
    
    $tmp    = NULL;   
    $link   = NULL;    
    
    require('table.inc');
    
    if (!is_null($tmp = @mysql_ping($link, $link)))
        printf("[001] Expecting NULL, got %s/%s\n", gettype($tmp), $tmp);
        
    var_dump(mysql_ping($link));    
    
    // provoke an error to check if mysql_ping resets it
    $res = mysql_query('SELECT * FROM unknown_table', $link);
    if (!($errno = mysql_errno($link)))
        printf("[002] Statement should have caused an error\n");
        
    var_dump(mysql_ping($link));    
    if ($errno === mysql_errno($link))
        printf("[003] Error codes should have been reset\n");    
       
    mysql_close($link);
    
    if (false !== ($tmp = mysql_ping($link)))
        printf("[004] Expecting boolean/false, got %s/%s\n", gettype($tmp), $tmp);
    
    print "done!";
?>
--EXPECTF--
bool(true)
bool(true)

Warning: mysql_ping(): %d is not a valid MySQL-Link resource in %s on line %d
done!
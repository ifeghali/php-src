--TEST--
highlight_string() buffering
--POST--
--INI--
highlight.string=#DD0000
highlight.comment=#FF9900
highlight.keyword=#007700
highlight.bg=#FFFFFF
highlight.default=#0000BB
highlight.html=#000000
--GET--
--FILE--
<?php 
$var = highlight_string("<br /><?php echo \"foo\"; ?><br />");
$var = highlight_string("<br /><?php echo \"bar\"; ?><br />", TRUE);
echo "\n[$var]\n";
?>
--EXPECT--
<code><font color="#000000">
&lt;br /&gt;<font color="#0000BB">&lt;?php </font><font color="#007700">echo </font><font color="#DD0000">"foo"</font><font color="#007700">; </font><font color="#0000BB">?&gt;</font>&lt;br /&gt;</font>
</code>
[<code><font color="#000000">
&lt;br /&gt;<font color="#0000BB">&lt;?php </font><font color="#007700">echo </font><font color="#DD0000">"bar"</font><font color="#007700">; </font><font color="#0000BB">?&gt;</font>&lt;br /&gt;</font>
</code>]

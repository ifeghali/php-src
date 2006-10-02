<?php
/* $Id: utils.php,v 1.1 2006/07/24 16:58:58 pajoye Exp $ */
function dump_entries_name($z) {
	for($i=0; $i<$z->numFiles; $i++) {
	    $sb = $z->statIndex($i);
	    echo $i . ' ' . $sb['name'] . "\n";
	}
}

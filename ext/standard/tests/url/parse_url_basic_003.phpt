--TEST--
Test parse_url() function: Parse a load of URLs without specifying PHP_URL_HOST as the URL component 
--FILE--
<?php
/* Prototype  : proto mixed parse_url(string url, [int url_component])
 * Description: Parse a URL and return its components 
 * Source code: ext/standard/url.c
 * Alias to functions: 
 */

/*
 * Parse a load of URLs without specifying PHP_URL_HOST as the URL component
 */
include_once(dirname(__FILE__) . '/urls.inc');

foreach ($urls as $url) {
	echo "--> $url   : ";
	var_dump(parse_url($url, PHP_URL_HOST));
}

echo "Done";
?>
--EXPECTF--
--> 64.246.30.37   : NULL
--> http://64.246.30.37   : string(12) "64.246.30.37"
--> http://64.246.30.37/   : string(12) "64.246.30.37"
--> 64.246.30.37/   : NULL
--> 64.246.30.37:80/   : string(12) "64.246.30.37"
--> php.net   : NULL
--> php.net/   : NULL
--> http://php.net   : string(7) "php.net"
--> http://php.net/   : string(7) "php.net"
--> www.php.net   : NULL
--> www.php.net/   : NULL
--> http://www.php.net   : string(11) "www.php.net"
--> http://www.php.net/   : string(11) "www.php.net"
--> www.php.net:80   : string(11) "www.php.net"
--> http://www.php.net:80   : string(11) "www.php.net"
--> http://www.php.net:80/   : string(11) "www.php.net"
--> http://www.php.net/index.php   : string(11) "www.php.net"
--> www.php.net/?   : NULL
--> www.php.net:80/?   : string(11) "www.php.net"
--> http://www.php.net/?   : string(11) "www.php.net"
--> http://www.php.net:80/?   : string(11) "www.php.net"
--> http://www.php.net:80/index.php   : string(11) "www.php.net"
--> http://www.php.net:80/foo/bar/index.php   : string(11) "www.php.net"
--> http://www.php.net:80/this/is/a/very/deep/directory/structure/and/file.php   : string(11) "www.php.net"
--> http://www.php.net:80/this/is/a/very/deep/directory/structure/and/file.php?lots=1&of=2&parameters=3&too=4&here=5   : string(11) "www.php.net"
--> http://www.php.net:80/this/is/a/very/deep/directory/structure/and/   : string(11) "www.php.net"
--> http://www.php.net:80/this/is/a/very/deep/directory/structure/and/file.php   : string(11) "www.php.net"
--> http://www.php.net:80/this/../a/../deep/directory   : string(11) "www.php.net"
--> http://www.php.net:80/this/../a/../deep/directory/   : string(11) "www.php.net"
--> http://www.php.net:80/this/is/a/very/deep/directory/../file.php   : string(11) "www.php.net"
--> http://www.php.net:80/index.php   : string(11) "www.php.net"
--> http://www.php.net:80/index.php?   : string(11) "www.php.net"
--> http://www.php.net:80/#foo   : string(11) "www.php.net"
--> http://www.php.net:80/?#   : string(11) "www.php.net"
--> http://www.php.net:80/?test=1   : string(11) "www.php.net"
--> http://www.php.net/?test=1&   : string(11) "www.php.net"
--> http://www.php.net:80/?&   : string(11) "www.php.net"
--> http://www.php.net:80/index.php?test=1&   : string(11) "www.php.net"
--> http://www.php.net/index.php?&   : string(11) "www.php.net"
--> http://www.php.net:80/index.php?foo&   : string(11) "www.php.net"
--> http://www.php.net/index.php?&foo   : string(11) "www.php.net"
--> http://www.php.net:80/index.php?test=1&test2=char&test3=mixesCI   : string(11) "www.php.net"
--> www.php.net:80/index.php?test=1&test2=char&test3=mixesCI#some_page_ref123   : string(11) "www.php.net"
--> http://secret@www.php.net:80/index.php?test=1&test2=char&test3=mixesCI#some_page_ref123   : string(11) "www.php.net"
--> http://secret:@www.php.net/index.php?test=1&test2=char&test3=mixesCI#some_page_ref123   : string(11) "www.php.net"
--> http://:hideout@www.php.net:80/index.php?test=1&test2=char&test3=mixesCI#some_page_ref123   : string(11) "www.php.net"
--> http://secret:hideout@www.php.net/index.php?test=1&test2=char&test3=mixesCI#some_page_ref123   : string(11) "www.php.net"
--> http://secret@hideout@www.php.net:80/index.php?test=1&test2=char&test3=mixesCI#some_page_ref123   : string(11) "www.php.net"
--> http://secret:hid:out@www.php.net:80/index.php?test=1&test2=char&test3=mixesCI#some_page_ref123   : string(11) "www.php.net"
--> nntp://news.php.net   : string(12) "news.php.net"
--> ftp://ftp.gnu.org/gnu/glic/glibc.tar.gz   : string(11) "ftp.gnu.org"
--> zlib:http://foo@bar   : NULL
--> zlib:filename.txt   : NULL
--> zlib:/path/to/my/file/file.txt   : NULL
--> foo://foo@bar   : string(3) "bar"
--> mailto:me@mydomain.com   : NULL
--> /foo.php?a=b&c=d   : NULL
--> foo.php?a=b&c=d   : NULL
--> http://user:passwd@www.example.com:8080?bar=1&boom=0   : string(15) "www.example.com"
--> file:///path/to/file   : NULL
--> file://path/to/file   : string(4) "path"
--> file:/path/to/file   : NULL
--> http://1.2.3.4:/abc.asp?a=1&b=2   : string(7) "1.2.3.4"
--> http://foo.com#bar   : string(7) "foo.com"
--> scheme:   : NULL
--> foo+bar://baz@bang/bla   : string(4) "bang"
--> gg:9130731   : NULL
--> http://user:@pass@host/path?argument?value#etc   : string(4) "host"
--> http://10.10.10.10/:80   : string(11) "10.10.10.10"
--> http://x:?   : string(1) "x"
--> x:blah.com   : NULL
--> x:/blah.com   : NULL
--> x://::abc/?   : string(1) ":"
--> http://::?   : string(1) ":"
--> x://::6.5   : string(1) ":"
--> http://?:/   : string(1) "?"
--> http://@?:/   : string(1) "?"
--> file:///:   : NULL
--> file:///a:/   : NULL
--> file:///ab:/   : NULL
--> file:///a:/   : NULL
--> file:///@:/   : NULL
--> file:///:80/   : NULL
--> []   : NULL
--> http://[x:80]/   : string(6) "[x:80]"
-->    : NULL
--> /   : NULL
--> http:///blah.com   : 
Warning: parse_url(http:///blah.com): Unable to parse URL in %s on line 15
bool(false)
--> http://:80   : 
Warning: parse_url(http://:80): Unable to parse URL in %s on line 15
bool(false)
--> http://user@:80   : 
Warning: parse_url(http://user@:80): Unable to parse URL in %s on line 15
bool(false)
--> http://user:pass@:80   : 
Warning: parse_url(http://user:pass@:80): Unable to parse URL in %s on line 15
bool(false)
--> http://:   : 
Warning: parse_url(http://:): Unable to parse URL in %s on line 15
bool(false)
--> http://@/   : 
Warning: parse_url(http://@/): Unable to parse URL in %s on line 15
bool(false)
--> http://@:/   : 
Warning: parse_url(http://@:/): Unable to parse URL in %s on line 15
bool(false)
--> http://:/   : 
Warning: parse_url(http://:/): Unable to parse URL in %s on line 15
bool(false)
--> http://?   : 
Warning: parse_url(http://?): Unable to parse URL in %s on line 15
bool(false)
--> http://?:   : 
Warning: parse_url(http://?:): Unable to parse URL in %s on line 15
bool(false)
--> http://:?   : 
Warning: parse_url(http://:?): Unable to parse URL in %s on line 15
bool(false)
--> http://blah.com:123456   : 
Warning: parse_url(http://blah.com:123456): Unable to parse URL in %s on line 15
bool(false)
--> http://blah.com:abcdef   : 
Warning: parse_url(http://blah.com:abcdef): Unable to parse URL in %s on line 15
bool(false)
Done
--UEXPECTF--
--> 64.246.30.37   : NULL
--> http://64.246.30.37   : unicode(12) "64.246.30.37"
--> http://64.246.30.37/   : unicode(12) "64.246.30.37"
--> 64.246.30.37/   : NULL
--> 64.246.30.37:80/   : unicode(12) "64.246.30.37"
--> php.net   : NULL
--> php.net/   : NULL
--> http://php.net   : unicode(7) "php.net"
--> http://php.net/   : unicode(7) "php.net"
--> www.php.net   : NULL
--> www.php.net/   : NULL
--> http://www.php.net   : unicode(11) "www.php.net"
--> http://www.php.net/   : unicode(11) "www.php.net"
--> www.php.net:80   : unicode(11) "www.php.net"
--> http://www.php.net:80   : unicode(11) "www.php.net"
--> http://www.php.net:80/   : unicode(11) "www.php.net"
--> http://www.php.net/index.php   : unicode(11) "www.php.net"
--> www.php.net/?   : NULL
--> www.php.net:80/?   : unicode(11) "www.php.net"
--> http://www.php.net/?   : unicode(11) "www.php.net"
--> http://www.php.net:80/?   : unicode(11) "www.php.net"
--> http://www.php.net:80/index.php   : unicode(11) "www.php.net"
--> http://www.php.net:80/foo/bar/index.php   : unicode(11) "www.php.net"
--> http://www.php.net:80/this/is/a/very/deep/directory/structure/and/file.php   : unicode(11) "www.php.net"
--> http://www.php.net:80/this/is/a/very/deep/directory/structure/and/file.php?lots=1&of=2&parameters=3&too=4&here=5   : unicode(11) "www.php.net"
--> http://www.php.net:80/this/is/a/very/deep/directory/structure/and/   : unicode(11) "www.php.net"
--> http://www.php.net:80/this/is/a/very/deep/directory/structure/and/file.php   : unicode(11) "www.php.net"
--> http://www.php.net:80/this/../a/../deep/directory   : unicode(11) "www.php.net"
--> http://www.php.net:80/this/../a/../deep/directory/   : unicode(11) "www.php.net"
--> http://www.php.net:80/this/is/a/very/deep/directory/../file.php   : unicode(11) "www.php.net"
--> http://www.php.net:80/index.php   : unicode(11) "www.php.net"
--> http://www.php.net:80/index.php?   : unicode(11) "www.php.net"
--> http://www.php.net:80/#foo   : unicode(11) "www.php.net"
--> http://www.php.net:80/?#   : unicode(11) "www.php.net"
--> http://www.php.net:80/?test=1   : unicode(11) "www.php.net"
--> http://www.php.net/?test=1&   : unicode(11) "www.php.net"
--> http://www.php.net:80/?&   : unicode(11) "www.php.net"
--> http://www.php.net:80/index.php?test=1&   : unicode(11) "www.php.net"
--> http://www.php.net/index.php?&   : unicode(11) "www.php.net"
--> http://www.php.net:80/index.php?foo&   : unicode(11) "www.php.net"
--> http://www.php.net/index.php?&foo   : unicode(11) "www.php.net"
--> http://www.php.net:80/index.php?test=1&test2=char&test3=mixesCI   : unicode(11) "www.php.net"
--> www.php.net:80/index.php?test=1&test2=char&test3=mixesCI#some_page_ref123   : unicode(11) "www.php.net"
--> http://secret@www.php.net:80/index.php?test=1&test2=char&test3=mixesCI#some_page_ref123   : unicode(11) "www.php.net"
--> http://secret:@www.php.net/index.php?test=1&test2=char&test3=mixesCI#some_page_ref123   : unicode(11) "www.php.net"
--> http://:hideout@www.php.net:80/index.php?test=1&test2=char&test3=mixesCI#some_page_ref123   : unicode(11) "www.php.net"
--> http://secret:hideout@www.php.net/index.php?test=1&test2=char&test3=mixesCI#some_page_ref123   : unicode(11) "www.php.net"
--> http://secret@hideout@www.php.net:80/index.php?test=1&test2=char&test3=mixesCI#some_page_ref123   : unicode(11) "www.php.net"
--> http://secret:hid:out@www.php.net:80/index.php?test=1&test2=char&test3=mixesCI#some_page_ref123   : unicode(11) "www.php.net"
--> nntp://news.php.net   : unicode(12) "news.php.net"
--> ftp://ftp.gnu.org/gnu/glic/glibc.tar.gz   : unicode(11) "ftp.gnu.org"
--> zlib:http://foo@bar   : NULL
--> zlib:filename.txt   : NULL
--> zlib:/path/to/my/file/file.txt   : NULL
--> foo://foo@bar   : unicode(3) "bar"
--> mailto:me@mydomain.com   : NULL
--> /foo.php?a=b&c=d   : NULL
--> foo.php?a=b&c=d   : NULL
--> http://user:passwd@www.example.com:8080?bar=1&boom=0   : unicode(15) "www.example.com"
--> file:///path/to/file   : NULL
--> file://path/to/file   : unicode(4) "path"
--> file:/path/to/file   : NULL
--> http://1.2.3.4:/abc.asp?a=1&b=2   : unicode(7) "1.2.3.4"
--> http://foo.com#bar   : unicode(7) "foo.com"
--> scheme:   : NULL
--> foo+bar://baz@bang/bla   : unicode(4) "bang"
--> gg:9130731   : NULL
--> http://user:@pass@host/path?argument?value#etc   : unicode(4) "host"
--> http://10.10.10.10/:80   : unicode(11) "10.10.10.10"
--> http://x:?   : unicode(1) "x"
--> x:blah.com   : NULL
--> x:/blah.com   : NULL
--> x://::abc/?   : unicode(1) ":"
--> http://::?   : unicode(1) ":"
--> x://::6.5   : unicode(1) ":"
--> http://?:/   : unicode(1) "?"
--> http://@?:/   : unicode(1) "?"
--> file:///:   : NULL
--> file:///a:/   : NULL
--> file:///ab:/   : NULL
--> file:///a:/   : NULL
--> file:///@:/   : NULL
--> file:///:80/   : NULL
--> []   : NULL
--> http://[x:80]/   : unicode(6) "[x:80]"
-->    : NULL
--> /   : NULL
--> http:///blah.com   : 
Warning: parse_url(http:///blah.com): Unable to parse URL in %s on line 15
bool(false)
--> http://:80   : 
Warning: parse_url(http://:80): Unable to parse URL in %s on line 15
bool(false)
--> http://user@:80   : 
Warning: parse_url(http://user@:80): Unable to parse URL in %s on line 15
bool(false)
--> http://user:pass@:80   : 
Warning: parse_url(http://user:pass@:80): Unable to parse URL in %s on line 15
bool(false)
--> http://:   : 
Warning: parse_url(http://:): Unable to parse URL in %s on line 15
bool(false)
--> http://@/   : 
Warning: parse_url(http://@/): Unable to parse URL in %s on line 15
bool(false)
--> http://@:/   : 
Warning: parse_url(http://@:/): Unable to parse URL in %s on line 15
bool(false)
--> http://:/   : 
Warning: parse_url(http://:/): Unable to parse URL in %s on line 15
bool(false)
--> http://?   : 
Warning: parse_url(http://?): Unable to parse URL in %s on line 15
bool(false)
--> http://?:   : 
Warning: parse_url(http://?:): Unable to parse URL in %s on line 15
bool(false)
--> http://:?   : 
Warning: parse_url(http://:?): Unable to parse URL in %s on line 15
bool(false)
--> http://blah.com:123456   : 
Warning: parse_url(http://blah.com:123456): Unable to parse URL in %s on line 15
bool(false)
--> http://blah.com:abcdef   : 
Warning: parse_url(http://blah.com:abcdef): Unable to parse URL in %s on line 15
bool(false)
Done
<?php
//
// +----------------------------------------------------------------------+
// | PHP Version 4                                                        |
// +----------------------------------------------------------------------+
// | Copyright (c) 1997-2002 The PHP Group                                |
// +----------------------------------------------------------------------+
// | This source file is subject to version 2.02 of the PHP license,      |
// | that is bundled with this package in the file LICENSE, and is        |
// | available at through the world-wide-web at                           |
// | http://www.php.net/license/2_02.txt.                                 |
// | If you did not receive a copy of the PHP license and are unable to   |
// | obtain it through the world-wide-web, please send a note to          |
// | license@php.net so we can mail you a copy immediately.               |
// +----------------------------------------------------------------------+
// | Author: Stig Bakken <ssb@fast.no>                                    |
// +----------------------------------------------------------------------+
//
// $Id$

require_once 'PEAR.php';
require_once 'PEAR/Config.php';

/**
 * This is a class for doing remote operations against the central
 * PEAR database.
 */
class PEAR_Remote extends PEAR
{
    // {{{ properties

    var $config = null;

    // }}}

    // {{{ PEAR_Remote(config_object)

    function PEAR_Remote(&$config)
    {
        $this->PEAR();
        $this->config = &$config;
    }

    // }}}

    // {{{ call(method, [args...])

    function call($method)
    {
        if (extension_loaded("xmlrpc")) {
            $args = func_get_args();
            return call_user_func_array(array(&$this, 'call_epi'), $args);
        }
        if (!@include_once("XML/RPC.php")) {
            return $this->raiseError("For this remote PEAR operation you need to install the XML_RPC package");
        }
        $args = func_get_args();
        array_shift($args);
        $server_host = $this->config->get('master_server');
        $username = $this->config->get('username');
        $password = $this->config->get('password');
        $f = new XML_RPC_Message($method, $this->_encode($args));
        $c = new XML_RPC_Client('/xmlrpc.php', $server_host, 80);
        if ($username && $password) {
            $c->setCredentials($username, $password);
        }
        if ($this->config->get('verbose') >= 3) {
            $c->setDebug(1);
        }
        $r = $c->send($f);
        if (!$r) {
            return $this->raiseError("XML_RPC send failed");
        }
        $v = $r->value();
        if ($e = $r->faultCode()) {
            return $this->raiseError($r->faultString(), $e);
        }

        return XML_RPC_decode($v);
    }

    // }}}

    // {{{ call_epi(method, [args...])

    function call_epi($method)
    {
        do {
            if (extension_loaded("xmlrpc")) {
                break;
            }
            if (OS_WINDOWS) {
                $ext = 'dll';
            } elseif (PHP_OS == 'HP-UX') {
                $ext = 'sl';
            } elseif (PHP_OS == 'AIX') {
                $ext = 'a';
            } else {
                $ext = 'so';
            }
            $ext = OS_WINDOWS ? 'dll' : 'so';
            @dl("xmlrpc-epi.$ext");
            if (extension_loaded("xmlrpc")) {
                break;
            }
            @dl("xmlrpc.$ext");
            if (extension_loaded("xmlrpc")) {
                break;
            }
            return $this->raiseError("unable to load xmlrpc extension");
        } while (false);
        $params = func_get_args();
        array_shift($params);
        $method = str_replace("_", ".", $method);
        $request = xmlrpc_encode_request($method, $params);
        $server_host = $this->config->get("master_server");
        if (empty($server_host)) {
            return $this->raiseError("PEAR_Remote::call: no master_server configured");
        }
        $server_port = 80;
        $fp = @fsockopen($server_host, $server_port);
        if (!$fp) {
            return $this->raiseError("PEAR_Remote::call: fsockopen(`$server_host', $server_port) failed");
        }
        $len = strlen($request);
        $req_headers = "Host: $server_host:$server_port\r\n" .
             "Content-type: text/xml\r\n" .
             "Content-length: $len\r\n";
        $username = $this->config->get('username');
        $password = $this->config->get('password');
        if ($username && $password) {
            $req_headers .= "Cookie: PEAR_USER=$username; PEAR_PW=$password\r\n";
            $tmp = base64_encode("$username:$password");
            $req_headers .= "Authorization: Basic $tmp\r\n";
        }
        fwrite($fp, ("POST /xmlrpc.php HTTP/1.0\r\n$req_headers\r\n$request"));
        $response = '';
        $line1 = fgets($fp, 2048);
        if (!preg_match('!^HTTP/[0-9\.]+ (\d+) (.*)!', $line1, $matches)) {
            return $this->raiseError("PEAR_Remote: invalid HTTP response from XML-RPC server");
        }
        switch ($matches[1]) {
            case "200":
                break;
            case "401":
                if ($username && $password) {
                    return $this->raiseError("PEAR_Remote: authorization failed", 401);
                } else {
                    return $this->raiseError("PEAR_Remote: authorization required, please log in first", 401);
                }
            default:
                return $this->raiseError("PEAR_Remote: unexpected HTTP response", (int)$matches[1], null, null, "$matches[1] $matches[2]");
        }
        while (trim(fgets($fp, 2048)) != ''); // skip rest of headers
        while ($chunk = fread($fp, 10240)) {
            $response .= $chunk;
        }
        fclose($fp);
        $ret = xmlrpc_decode($response);
        if (is_array($ret) && isset($ret['__PEAR_TYPE__'])) {
            if ($ret['__PEAR_TYPE__'] == 'error') {
                if (isset($ret['__PEAR_CLASS__'])) {
                    $class = $ret['__PEAR_CLASS__'];
                } else {
                    $class = "PEAR_Error";
                }
                if ($ret['code']     === '') $ret['code']     = null;
                if ($ret['message']  === '') $ret['message']  = null;
                if ($ret['userinfo'] === '') $ret['userinfo'] = null;
                if (strtolower($class) == 'db_error') {
                    $ret = $this->raiseError(DB::errorMessage($ret['code']),
                                             $ret['code'], null, null,
                                             $ret['userinfo']);
                } else {
                    $ret = $this->raiseError($ret['message'], $ret['code'],
                                             null, null, $ret['userinfo']);
                }
            }
        } elseif (is_array($ret) && sizeof($ret) == 1 &&
                  isset($ret[0]['faultString']) &&
                  isset($ret[0]['faultCode'])) {
            extract($ret[0]);
            $faultString = "XML-RPC Server Fault: " .
                 str_replace("\n", " ", $faultString);
            return $this->raiseError($faultString, $faultCode);
        }
        return $ret;
    }

    // }}}

    // {{{ _encode

    // a slightly extended version of XML_RPC_encode
    function _encode($php_val)
    {
        global $XML_RPC_Boolean, $XML_RPC_Int, $XML_RPC_Double;
        global $XML_RPC_String, $XML_RPC_Array, $XML_RPC_Struct;

        $type = gettype($php_val);
        $xmlrpcval = new XML_RPC_value;

        switch($type) {
            case "array":
                reset($php_val);
                $firstkey = key($php_val);
                end($php_val);
                $lastkey = key($php_val);
                if ($firstkey === 0 && is_int($lastkey) &&
                    ($lastkey + 1) == count($php_val)) {
                    $is_continous = true;
                    reset($php_val);
                    for ($expect = 0; $expect < count($php_val); $expect++) {
                        if (key($php_val) !== $expect) {
                            $is_continous = false;
                            break;
                        }
                    }
                    if ($is_continous) {
                        reset($php_val);
                        $arr = array();
                        while (list($k, $v) = each($php_val)) {
                            $arr[$k] = $this->_encode($v);
                        }
                        $xmlrpcval->addArray($arr);
                        break;
                    }
                }
                // fall though if not numerical and continous
            case "object":
                $arr = array();
                while (list($k, $v) = each($php_val)) {
                    $arr[$k] = $this->_encode($v);
                }
                $xmlrpcval->addStruct($arr);
                break;
            case "integer":
                $xmlrpcval->addScalar($php_val, $XML_RPC_Int);
                break;
            case "double":
                $xmlrpcval->addScalar($php_val, $XML_RPC_Double);
                break;
            case "string":
            case "NULL":
                $xmlrpcval->addScalar($php_val, $XML_RPC_String);
                break;
            case "boolean":
                $xmlrpcval->addScalar($php_val, $XML_RPC_Boolean);
                break;
            case "unknown type":
            default:
                return null;
        }
        return $xmlrpcval;
    }

    // }}}

}

?>
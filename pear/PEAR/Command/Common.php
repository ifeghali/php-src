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

require_once "PEAR.php";

class PEAR_Command_Common extends PEAR
{
    /**
     * PEAR_Config object used to pass user system and configuration
     * on when executing commands
     *
     * @var object
     */
    var $config;

    /**
     * User Interface object, for all interaction with the user.
     * @var object
     */
    var $ui;

    /**
     * PEAR_Command_Common constructor.
     *
     * @access public
     */
    function PEAR_Command_Common(&$ui, &$config)
    {
        parent::PEAR();
        $this->config = &$config;
        $this->ui = &$ui;
    }

    function getOptions()
    {
        return array();
    }

    function getHelp($command)
    {
        return array(null, 'No help avaible yet');
    }
}

?>
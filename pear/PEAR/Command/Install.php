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
// | Author: Stig S�ther Bakken <ssb@fast.no>                             |
// +----------------------------------------------------------------------+
//
// $Id$

require_once "PEAR/Command/Common.php";
require_once "PEAR/Installer.php";
require_once "Console/Getopt.php";

/**
 * PEAR commands for installation or deinstallation/upgrading of
 * packages.
 *
 */
class PEAR_Command_Install extends PEAR_Command_Common
{
    // {{{ command definitions

    var $commands = array(
        'install' => array(
            'summary' => 'Install Package',
            'function' => 'doInstall',
            'options' => array(
                'force' => array(
                    'shortopt' => 'f',
                    'doc' => 'will overwrite newer installed packages',
                    ),
                'nodeps' => array(
                    'shortopt' => 'n',
                    'doc' => 'ignore dependencies, install anyway',
                    ),
                'register-only' => array(
                    'shortopt' => 'r',
                    'doc' => 'do not install files, only register the package as installed',
                    ),
                'soft' => array(
                    'shortopt' => 's',
                    'doc' => 'soft install, fail silently, or upgrade if already installed',
                    ),
                'nocompress' => array(
                    'shortopt' => 'Z',
                    'doc' => 'request uncompressed files when downloading',
                    ),
                ),
            'doc' => 'Installs one or more PEAR packages.  You can specify a package to
install in four ways:

"Package-1.0.tgz" : installs from a local file

"http://example.com/Package-1.0.tgz" : installs from
anywhere on the net.

"package.xml" : installs the package described in
package.xml.  Useful for testing, or for wrapping a PEAR package in
another package manager such as RPM.

"Package" : queries your configured server
({config master_server}) and downloads the newest package with
the preferred quality/state ({config preferred_state}).

More than one package may be specified at once.  It is ok to mix these
four ways of specifying packages.
'),
        'upgrade' => array(
            'summary' => 'Upgrade Package',
            'function' => 'doInstall',
            'options' => array(
                'force' => array(
                    'shortopt' => 'f',
                    'doc' => 'overwrite newer installed packages',
                    ),
                'nodeps' => array(
                    'shortopt' => 'n',
                    'doc' => 'ignore dependencies, upgrade anyway',
                    ),
                'register-only' => array(
                    'shortopt' => 'r',
                    'doc' => 'do not install files, only register the package as upgraded',
                    ),
                'nocompress' => array(
                    'shortopt' => 'Z',
                    'doc' => 'request uncompressed files when downloading',
                    ),
                ),
            'doc' => 'Upgrades one or more PEAR packages.  See documentation for the
"install" command for ways to specify a package.

When upgrading, your package will be updated if the provided new
package has a higher version number (use the -f option if you need to
upgrade anyway).

More than one package may be specified at once.
'),
        'uninstall' => array(
            'summary' => 'Un-install Package',
            'function' => 'doUninstall',
            'options' => array(
                'nodeps' => array(
                    'shortopt' => 'n',
                    'doc' => 'ignore dependencies, uninstall anyway',
                    ),
                'register-only' => array(
                    'shortopt' => 'r',
                    'doc' => 'do not remove files, only register the packages as not installed',
                    ),
                ),
            'doc' => 'Upgrades one or more PEAR packages.  See documentation for the
"install" command for ways to specify a package.

When upgrading, your package will be updated if the provided new
package has a higher version number (use the -f option if you need to
upgrade anyway).

More than one package may be specified at once.
'),

    );

    // }}}
    // {{{ constructor

    /**
     * PEAR_Command_Install constructor.
     *
     * @access public
     */
    function PEAR_Command_Install(&$ui, &$config)
    {
        parent::PEAR_Command_Common($ui, $config);
    }

    // }}}
    // {{{ run()

    function run($command, $options, $params)
    {
        $this->installer = &new PEAR_Installer($ui);
//        return parent::run($command, $options, $params);
        $failmsg = '';
        switch ($command) {
            case 'upgrade':
                $options['upgrade'] = true;
                // fall through
            case 'install':
                foreach ($params as $pkg) {
                    $bn = basename($pkg);
                    $info = $this->installer->install($pkg, $options, $this->config);
                    if (is_array($info)) {
                        $label = "$info[package] $info[version]";
                        $this->ui->displayLine("$command ok: $label");
                    } else {
                        $failmsg = "$command failed";
                    }
                }
                break;
            case 'uninstall':
                foreach ($params as $pkg) {
                    if ($this->installer->uninstall($pkg, $options)) {
                        $this->ui->displayLine("uninstall ok");
                    } else {
                        $failmsg = "uninstall failed";
                    }
                }
                break;
            default:
                return false;
        }
        if ($failmsg) {
            return $this->raiseError($failmsg);
        }
        return true;
    }

    // }}}
}

?>
<?php

require_once 'PEAR/Command/Common.php';
require_once 'PEAR/Registry.php';
require_once 'PEAR/Config.php';

class PEAR_Command_Registry extends PEAR_Command_Common
{
    // {{{ constructor

    /**
     * PEAR_Command_Registry constructor.
     *
     * @access public
     */
    function PEAR_Command_Registry(&$ui, &$config)
    {
        parent::PEAR_Command_Common($ui, $config);
    }

    // }}}

    // {{{ getCommands()

    /**
     * Return a list of all the commands defined by this class.
     * @return array list of commands
     * @access public
     */
    function getCommands()
    {
        return array('list-installed', 'shell-test');
    }

    function getHelp($command)
    {
        switch ($command) {
            case 'list-installed':
                return array(null, 'List the installed PEAR packages in the system');
        }
    }

    // }}}
    // {{{ run()

    /**
     * Execute the command.
     *
     * @param string command name
     *
     * @param array option_name => value
     *
     * @param array list of additional parameters
     *
     * @return bool TRUE on success, FALSE if the command was unknown,
     *              or a PEAR error on failure
     *
     * @access public
     */
    function run($command, $options, $params)
    {
        $failmsg = '';
        $cf = &PEAR_Config::singleton();
        switch ($command) {
            // {{{ list-installed

            case 'list-installed': {
                $reg = new PEAR_Registry($cf->get('php_dir'));
                $installed = $reg->packageInfo();
                $i = $j = 0;
                $this->ui->startTable(
                    array('caption' => 'Installed packages:',
                          'border' => true));
                foreach ($installed as $package) {
                    if ($i++ % 20 == 0) {
                        $this->ui->tableRow(
                            array('Package', 'Version', 'State'),
                            array('bold' => true));
                    }
                    $this->ui->tableRow(array($package['package'],
                                              $package['version'],
                                              @$package['release_state']));
                }
                $this->ui->endTable();
                break;
            }

            // }}}
            case 'shell-test': {
                // silence error messages for the rest of the execution
                PEAR::pushErrorHandling(PEAR_ERROR_RETURN);
                $reg = &new PEAR_Registry($this->config->get('php_dir'));
                // "pear shell-test Foo"
                if (sizeof($params) == 1) {
                    if (!$reg->packageExists($params[0])) {
                        exit(1);
                    }
                // "pear shell-test Foo 1.0"
                } elseif (sizeof($params) == 2) {
                    $v = $reg->packageInfo($params[0], 'version');
                    if (!$v || !version_compare($v, $params[1], "ge")) {
                        exit(1);
                    }
                // "pear shell-test Foo ge 1.0"
                } elseif (sizeof($params) == 3) {
                    $v = $reg->packageInfo($params[0], 'version');
                    if (!$v || !version_compare($v, $params[2], $params[1])) {
                        exit(1);
                    }
                } else {
                    PEAR::popErrorHandling();
                    PEAR::raiseError("$command: expects 1 to 3 parameters");
                    exit(1);
                }
                break;
            }
            default: {
                return false;
            }
        }
        if ($failmsg) {
            return $this->raiseError($failmsg);
        }
        return true;
    }

    // }}}


}

?>
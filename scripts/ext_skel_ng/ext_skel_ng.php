<?php
  require_once "extension_parser.php";
	require_once "System.php";

  $filename = isset($_SERVER["argv"][1]) ? $_SERVER["argv"][1] : "extension.xml";

  $ext = &new extension_parser(fopen($filename, "r"));

  System::rm("-rf {$ext->name}");
  mkdir($ext->name);

  // generate code
  $ext->write_header_file();
  $ext->write_code_file();
  if(isset($ext->logo)) {
    $fp = fopen("{$ext->name}/{$ext->name}_logo.h", "w");
    fwrite($fp, $ext->logo->h_code());
    fclose($fp);
    $ext->logo->h_code();
  }

  // generate project files for configure and ms dev studio
  $ext->write_config_m4();
  $ext->write_ms_devstudio_dsp();

  $ext->generate_documentation();
?>
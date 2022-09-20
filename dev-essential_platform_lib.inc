<?PHP
global $__debug_handled, $__module, $db, $redis, $redis_write, $session, $fs_root, $url_root, $log_level, $debug;

function __handle_debugging() {
	global $debug, $__module;
//print_r($debug);
	if (isset($__debug_handled) && $__debug_handled) return;

	if ($debug['master_disable']) {
		header("HTTP/1.0 500 $__module: $debug[master_disable]");
		echo "500 / $__module: $debug[master_disable]\n";
		exit;
	}

	if ($debug['devel_prefix']) {
		$f=dirname($__module).'/'.$debug['devel_prefix'].basename($__module);
		if (!file_exists($f)) die("essential_platform_lib: debug code version requested, but file $f not exists");
		include($f);
		exit;
	}

	if ($debug['total_debug']) {
		error_reporting(E_ALL);
		ini_set('display_errors', TRUE);
	} // Assume that PHP has reporting right for production
}


?>
<?PHP
global $__debug_handled, $__module, $db, $redis, $redis_write, $session, $fs_root, $url_root, $log_level, $debug;
//print_r($debug);

if (!isset($__debug_handled) || !$__debug_handled) {
	$__debug_handled=TRUE;
	if ($debug['master_disable']) {
		header("HTTP/1.0 500 $__module: $debug[master_disable]");
		echo "500 / $__module: $debug[master_disable]\n";
		exit;
	}

	setlocale(LC_MONETARY, 'cs_CZ.UTF-8');
	require_once($fs_root."/fwlib/util.inc");
	require_once($fs_root."/fwlib/tsredis.inc");

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

	if ($debug['enable_redis_debug']) {
		connect_redis();
		$dbg=$redis->smembers('debug_session.'.$session);

		if ($dbg) {
			if (in_array('master_disable', $dbg)) {
				header("HTTP/1.0 500 $__module: $debug[master_disable]");
				echo "500 / $__module: $debug[master_disable]\n";
				exit;
			}
			if (in_array('total_debug', $dbg)) {
				error_reporting(E_ALL);
				ini_set('display_errors', TRUE);
			} // Assume that PHP has reporting right for production
			if (in_array('devel', $dbg)) {
				$f=dirname($__module).'/'.$debug['devel_prefix'].basename($__module);
				if (!file_exists($f)) die("essential_platform_lib: debug code version requested, but file $f not exists");
				include($f);
				exit;
			}
			if (in_array('lib-devel', $dbg)) $debug['lib_devel_prefix']='dev-';
			if (in_array('brutlog_requests', $dbg)) $debug['brutlog_requests']=TRUE;
		}
	}
}

?>
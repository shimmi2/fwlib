<?PHP
global $__module, $db, $redis, $redis_write, $session, $fs_root, $url_root, $log_level, $debug;

if (isset($debug)) {
	if ($debug['master_disable']) {
		header("HTTP/1.0 500 $__module: $debug[master_disable]");
		echo "500 / $__module: $debug[master_disable]\n";
		exit;
	}
	
	if ($debug['total_debug']) {
		error_reporting(E_ALL);
		ini_set('display_errors', TRUE);
		ini_set('display_startup_errors', TRUE);
	} // Assume that PHP has reporting right for production

	if ($debug['enable_redis_debug']) {
		require_once($fs_root."/fwlib/tsredis.inc");
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
				ini_set('display_startup_errors', TRUE);
			} // Assume that PHP has reporting right for production
			if (in_array('devel', $dbg)) $debug['devel_prefix']='dev-';
			if (in_array('lib-devel', $dbg)) $debug['lib_devel_prefix']='dev-';
			if (in_array('brutlog_requests', $dbg)) $debug['brutlog_requests']=TRUE;	
		}
	}
	
	if ($debug['brutlog_requests']) {
		$brutlog=var_export($_REQUEST, true);
		$brutlog=str_replace("\n", " ", $brutlog);
		file_put_contents( "/tmp/brutlog-$__module.log", $_SERVER['REMOTE_ADDR'].": $brutlog\n", FILE_APPEND);
}

	
}

?>
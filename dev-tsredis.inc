<?PHP
/* 
	This implementation uses phpredis library.
	For speed, no wrapper of Redis work function needed.
	This lib is used for proper HA startup, not for check connection failure
	ICE connection failures ETC, daemon should change servers in 5 seconds.
	*REMEMBER*, REDIS should not be useed for transaction safe functions and data
	This lib requires $redis_host, $redis_host_write vars at minimal,
	but STRONGLY recomends $redis_accessfile to be set and maintained by tsredis-accessfile daemon
*/
function connect_redis() {
	global $redis, $redis_host, $redis_pass, $redis_accessfile;
	if ($redis) return;
	// override with HA version of basic setting
	if (file_exists($redis_accessfile)) include_once($redis_accessfile);
	if (!$redis_host) tolog("FATAL: define \$redis_accessfile or \$redis_host in module_settings.inc", L_ERR | L_DIE);
	$r=explode(':', $redis_host);
	$r[1]= $r[1] ?? '6379';
	$redis=new Redis();
	$redis->pconnect($r[0], $r[1], 1, NULL, 100); // 1s timeout, 100ms between attempts
	if (!$redis) {
		$redis=NULL;
		return(FALSE); 
	}
	if ($redis_pass) $redis->auth($redis_pass); // ignore result to prevent case with auth to free server
}

function connect_redis_write() {
	global $redis, $redis_write, $redis_host_write, $redis_pass_write, $redis_accessfile;
	if (isset($redis_write) && $redis_write) return;
	// override with HA version of basic setting
	if (file_exists($redis_accessfile)) include_once($redis_accessfile);	
	if (isset($redis_host_write)) {
		$redis_write=new Redis();
		$redis_write->pconnect($redis_host_write);
		if ($redis_pass_write) $redis_write->auth($redis_pass_write);
	} else $redis_write=$redis;
	
	if ($redis_write) return;
	
	connect_redis();
	$redis_write=$redis;		
	if (!$redis_write) tolog("FATAL: invalid REDIS write setting in module_settings.inc", L_ERR | L_DIE);
}

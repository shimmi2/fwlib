<?PHP

function _cache_k($which, $id) {
	switch ($which) {
		case 'session':
			return("sver$id");
		case 'user':
			return("uver$id");
		case 'system':
			return("sys");
		default:
			tolog("unsupported which param: $which", L_ERR | L_DIE);
	}
}	

// REDIS cloud versions for cache content dirtying
// this will call admin when sometning is changed
function cache_setdirty($which, $kind, $id) {
	global $redis_write, $debug;
	
	if (isset($debug['bypas_data_caches']) && $debug['bypas_data_caches']) return;
	
	connect_redis_write();
	$redis_write->hIncrBy(_cache_k($which, $id), $kind, 1);
}

// this is called by worker to check if local cache is valid
// returns: 0=fresh, >0=dirty, store this as new version
function usercache_freshver($which, $kind, $id, $cached_ver) {
	global $redis;
	if (isset($debug['data_caches_force_clean']) && $debug['data_caches_force_clean']) return (0);
	if (isset($debug['bypas_data_caches']) && $debug['bypas_data_caches']) return(1);
	
	connect_redis();
	$act_ver=$redis->hget(_cache_k($which, $id), $kind);

	if (!$act_ver) return(0); // no change recorded, all cached must be valid
	if ($act_ver>$cached_ver) return($act_ver);
	return(0);
}

function inteligent_shorten(&$str, $pos) {
	$mb_l=mb_strlen($str);							// Multibyte len
	if ($mb_l<=$pos) return($str);					
	$ret=mb_substr($str, 0, $pos+5);				// Shorten, but add + 5 chars
	$l=strlen($ret);								//
	$i=-1;								
	do {		// We'll try search for end of word, ignoring mb chars
		$i++;
		if ($i>=$l) return ('');
		if ($ret[$l-$i-1]>0x7f) continue;			// Unicode part we must move before mb char (this handles all mb up to 4 byte char)
		if ($ret[$l-$i-1]==' ') return(substr($ret, 0, $l-$i)); // We found, truncate
	} while ($i<20);								// gave up, truncate in the middle of word
	return(mb_substr($ret, 0, $pos));				
}		


function get_old_session_taco(&$sid) {
	return(db_trivialquery("SELECT CONCAT(us_tarif, '.', IFNULL(GROUP_CONCAT(up_tarif), '-')) AS tacombo FROM users LEFT JOIN user_packages ON up_user=us_id WHERE us_id=$sid->ses_uid", 'get use tacombo'));
}

function json_out(&$data) {
	global $debug;
	header('Content-Type: application/json');
	if (isset($debug['json_params']))
		echo json_encode($data, $debug['json_params']);
	else 
		echo json_encode($data, JSON_UNESCAPED_UNICODE);
}

function throw_error($http_code, $err, $body='') {
	$err=trim($err);
	header("HTTP/1.0 $http_code $err");
	if (!$body) $body=$err;
	echo "$body\n";
	exit;
}

function cipher_output($out) {
	return($out);
}

// just for SQL query
function norm_taco($taco){
	$taco=str_replace('.', ',', str_replace('-', '', $taco));
	$taco=preg_replace('/\,$/', '', $taco);
	return($taco);
}

function right_get_contents($uri) {
	$page=file_get_contents($uri);
	if (in_array('content-encoding: gzip', $http_response_header) || 
		in_array('Content-Encoding: gzip', $http_response_header)) {
		tolog("content-encoding: gzip", L_DEBUG1);
		return(gzdecode($page));
	}
	return($page);
}


function secure_conn() {
    if ((!empty($_SERVER['HTTPS']) && $_SERVER['HTTPS'] !== 'off') || 
		(!empty($_SERVER['HTTP_X_FORWARDED_PROTO']) && $_SERVER['HTTP_X_FORWARDED_PROTO'] == 'https') || 
		(!empty($_SERVER['HTTP_X_FORWARDED_SSL']) && $_SERVER['HTTP_X_FORWARDED_SSL'] == 'on') || 
		(isset($_SERVER['SERVER_PORT']) && $_SERVER['SERVER_PORT'] == 443) || 
		(isset($_SERVER['HTTP_X_FORWARDED_PORT']) && $_SERVER['HTTP_X_FORWARDED_PORT'] == 443) || 
		(isset($_SERVER['REQUEST_SCHEME']) && $_SERVER['REQUEST_SCHEME'] == 'https')) 
		return (TRUE); 
	else 
		return (FALSE);
}
if (!function_exists('connect_redis')) {
function connect_redis() {
	global $redis, $redis_host, $redis_pass;
	$redis=new Redis();
	$redis->pconnect($redis_host);
	$redis->auth($redis_pass);
}
}

if (!function_exists('connect_redis_write')) {
function connect_redis_write() {
	global $redis, $redis_write, $redis_host_write, $redis_pass_write;
	if (isset($redis_write) && $redis_write) return;
	if (isset($redis_host_write)) {
		$redis_write=new Redis();
		$redis_write->pconnect($redis_host_write);
		if ($redis_pass_write) $redis_write->auth($redis_pass_write);
	} else $redis_write=$redis;
}
}

define('V_INT', 1);
define('V_ID', 6);
define('V_FLOAT', 2);
define('V_BOOL', 3);
define('V_STR', 4);
define('V_ISSET', 5);
define('V_BLOB', 7);

define('V_MUST', 256);

function import_var($varname, $type, $default=NULL) {
	if ($type & V_MUST && !isset($_REQUEST[$varname])) throw_error(400, "required variable $varname not sent");
	if (!isset($_REQUEST[$varname])) return($default);
	$t=$type & 255;
	switch ($t) {
		case V_INT:
			return(intval($_REQUEST[$varname]));
		case V_ID:
			$id=intval($_REQUEST[$varname]);
			if (!$id) throw_error(400, "required variable $varname is NULL");
			return($id);
		case V_FLOAT:
			return(floatval($_REQUEST[$varname]));
		case V_BOOL:
			return(boolval($_REQUEST[$varname]));
		case V_STR:
			return(strval($_REQUEST[$varname]));
		case V_ISSET:
			return(isset($_REQUEST[$varname]) && boolval($_REQUEST[$varname]));
		case V_BLOB:
			return($_REQUEST[$varname]);
		default:
			throw_error(500, "import_var: invalid parameter type requested");
	}
}

function deploy_data($path, $data, $check_same=TRUE, $log_param=0) {
echo "deploy_data($path, data, $check_same, $log_param)\n";
	if ($check_same && file_exists($path) && file_get_contents($path)==$data) {
		tolog("not writing $path, file unchanged", L_DEBUG);
		return(FALSE);
	}
	$dir=dirname($path);
	if (!file_exists($dir)) {
		tolog("creating directory $dir", L_DEBUG);
		if (!mkdir ($dir , 0755, TRUE)) tolog ("failed mkdir '$dir'", L_ERR);
	}
	
	if (!file_put_contents("$path.new", $data))
    tolog("failed to deploy file $path", L_ERR | $ignore);
	
	if (!copy("$path", "$path.1"))
    tolog("failed to backup file as $path.new.1", L_WARN);

	if (!rename("$path.new", "$path"))
    tolog("failed to rename $path.1 as $path", L_WARN);
	return(TRUE);
}

/*
	// local deploy for this time
	if ($compress) {
		$path.='.gz';
		$json=gzencode(cipher_output(json_encode($data, JSON_PRETTY_PRINT)));
//		$json=gzencode(cipher_output(json_encode($data)));
	} else 
		$json=json_encode(cipher_output($data),  JSON_PRETTY_PRINT);
//		$json=json_encode(cipher_output($data));
	
*/	

function deploy_jsondata($path, $data, $check_same=1, $compress=FALSE, $ignore=FALSE) {
	global $debug;
	
	if (isset($debug['json_params']))
		$d=json_encode($data, $debug['json_params']);
	else 
		$d=json_encode($data, JSON_UNESCAPED_UNICODE);

	$d=cipher_output($d);
	
	if ($compress) {
		$path.='.gz';
		$d=gzencode($d);
	}
	
	$ignore ? $ignore=L_DIE : $ignore=0;
	deploy_data($path, $d, $check_same, $ignore);
}

// more important, mnore checking :)
function _get_manifest($resource, $generator_key, $out_root) {
	global $db;
	if (!$out_root) tolog("deploy_clound_jsondata: out_root us unset", L_ERR | L_DIE);	
	if (!$resource) tolog("deploy_clound_jsondata: resource IS NULL", L_ERR | L_DIE);
	if (!$generator_key) tolog("deploy_clound_jsondata: generator_key IS NULL", L_ERR | L_DIE);
	$res=$db->tsquery("SELECT * FROM gen_manifest WHERE gm_resource='$resource' AND gm_generator_key='$generator_key'", 'get manifest');
	if ($res->rowCount()>1) tolog("deploy_clound_jsondata: multipple matching resources occured in gen_manifest, solve generator_key", L_ERR | L_DIE);
	if (!$m=fetch_o($res)) tolog("manifest id $manifest_id not exists in gen_manifest table", L_ERR | L_DIE);
	return($m);
}


function deploy_cloud_data($resource, $out_root, $data, $local_path='__use_call_template__', $generator_key_override=NULL, $log_param=L_DIE) {
	global $debug, $db, $generator_key;
	$generator_key_override ? $gk=$generator_key_override : $gk=$generator_key;
	$m=_get_manifest($resource, $gk, $out_root);
	switch ($m->gm_format) {
		case 'json':
			if (isset($debug['json_params']))
				$data=json_encode($data, $debug['json_params']);
			else 
				$data=json_encode($data, JSON_UNESCAPED_UNICODE);
			$suffix='.json';
			break;								
		case 'txt':
			$suffix='.txt';
			break;					
		case 'html':
			$suffix='.html';
			break;
		case 'status':
		case 'bin':
		default:
			$suffix='';
	}
	switch ($m->gm_transport) {
		case 'gz':
			$suffix.='.gz';
			$data=gzencode($data);
			break;
	}
//echo "$local_path\n";

	if ($local_path=='__use_call_template__') $path=$out_root.'/'.$m->gm_local_path.$m->gm_call_template;
	else $path=$out_root.'/'.$m->gm_local_path.$local_path.$suffix;
	if (deploy_data($path, $data, TRUE, L_DIE)) {
		tolog("new version of $resource ($local_path), writing change to cloud_deployment table", L_DEBUG);
		if ($m->gm_cloud_versions) {
			db_trivialquery("INSERT INTO cloud_deployment SET
				cld_manifest_id=$m->gm_id, cld_localpart='$local_path$suffix', cld_sequence=1
				ON DUPLICATE KEY update cld_sequence=cld_sequence+1", 'update cloud deploy data');
		}
	}
}

// old, deprecated
function deploy_clound_jsondata($resource, $out_root, $local_path, $data, $compress=FALSE, $generator_key_override=NULL, $ignore=FALSE) {
	global $db, $generator_key;
	$generator_key_override ? $gk=$generator_key_override : $gk=$generator_key;
	$m=_get_manifest($resource, $gk, $out_root);
	if ($local_path=='__use_call_template__') $local_path=$m->gm_call_template;
	$path=$out_root.'/'.$m->gm_local_path.$local_path;
	if (deploy_jsondata($path, $data, TRUE, $compress, $ignore)) {
		tolog("new version of $resource ($local_path), writing change to cloud_deployment table", L_DEBUG);
		db_trivialquery("INSERT INTO cloud_deployment SET
			cld_manifest_id=$m->gm_id, cld_localpart='$local_path', cld_sequence=1
			ON DUPLICATE KEY update cld_sequence=cld_sequence+1", 'update cloud deploy data');
	}
}

function remove_cloud_old_content($resource, $out_root, $older_than_days=NULL) {
	global $db, $generator_key;
	$m=_get_manifest($resource, $generator_key, $out_root);
	if (!$older_than_days) 
		$older_than_days=db_trivialquery("SELECT DATEDIFF(DATE_ADD(CURDATE(), INTERVAL $m->gm_cloud_remove_after), CURDATE())", 'get gm_cloud_remove_after');
	if (!$older_than_days) tolog("resource $resource older_than_days=0? thats error, stop now", L_ERR | L_DIE);
	
	$path="$out_root/$m->gm_local_path";
	tolog("removing old content in $path...", L_INFO);
	tolog("exec: find $path -mtime +$older_than_days -exec rm -r {} \;", L_DEBUG);
	exec ("find $path -mtime -mindepth 1 +$older_than_days -exec rm -r {} \;");
}
	
function ack_daem_lock($max_run=10800) {
	global $__locked_by_me;
	$daem=basename(__FILE__, '.php'); 
	$fn="/var/lock/startv_$daem.lock";
	if (file_exists($fn)) {
		$rt=time()-filemtime($fn);
		if ($rt<$max_run)
			tolog("lock file $fn exist still running (for $rt seconds), abort this run", L_ERR | L_DIE);
		else tolog("WARNING: lock file $fn exist, but for $rt seconds (threshold=$max_run), ignoring and continuing", L_ERR);
	}
	$pid=getmypid();
	file_put_contents($fn, $pid);
	$__locked_by_me=TRUE;
	tolog("claimed lock $fn", L_DEBUG1);
	return(TRUE);
}

function free_daem_lock() {
	global $__locked_by_me;
	tolog("free_lock: shutdown registered", L_DEBUG);
	$daem=basename(__FILE__, '.php'); 

	if (!$__locked_by_me) 
		tolog("free_lock without locked", L_DEBUG);
	else {
		$fn="/var/lock/startv_$daem.lock";
		$rt=time()-filemtime($fn);
		tolog ("freelock is removing $fn, running time was $rt seconds", L_DEBUG1);
		if (file_exists($fn)) unlink($fn);
	}
	return(TRUE);
}

function free_lock_quit() {
	free_lock();
	exit;
}

function v2_store_cache($key, &$data, $module='V2', $json=TRUE) {
	global $debug, $cache_ramdisk;
	if (!$cache_ramdisk) tolog("FATAL: $cache_ramdisk not defined if modules_sessting.inc", L_ERR | L_DIE);
	// Hardwired to file ramdisk cache as best way, but we can do another way in future
	if (!file_exists($cache_ramdisk.'/'.$module)) mkdir($cache_ramdisk.'/'.$module, 0700, true);
	$cache_file=$cache_ramdisk.'/'.$module.'/'.$key;
	if ($json) {
		if (isset($debug['json_params']))
			file_put_contents($cache_file, json_encode($data, $debug['json_params']));
		else 
			file_put_contents($cache_file, json_encode($data, JSON_UNESCAPED_UNICODE));
	} else file_put_contents($cache_file, $data);
//	$json ? file_put_contents($cache_file, json_encode($data)) : file_put_contents($cache_file, $data);
}

function v2_try_cache($key, $module='V2', $json=TRUE, &$created=0) {
	global $cache_ramdisk;
	if (!$cache_ramdisk) tolog("FATAL: $cache_ramdisk not defined if modules_sessting.inc", L_ERR | L_DIE);
	// Hardwired to file ramdisk cache as best way, but we can do another way in future
	$cache_file=$cache_ramdisk.'/'.$module.'/'.$key;
	if (!file_exists($cache_file)) return(FALSE);
	$created=filemtime($cache_file);
	if ($json) return(json_decode(file_get_contents($cache_file), TRUE)); else return(file_get_contents($cache_file));
}


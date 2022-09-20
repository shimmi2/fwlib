<?PHP




function old_to_new_session($session, $lazytouch=0) {
	global $db, $redis;
	if (!$redis) return;
	$time=time();
	$key='sid'.$session;
	$sid=$redis->hGetAll($key);
	if ($sid && $time-$sid['ses_touch']<600) return;
//	if ($time-$lazytouch<600) return;
	$res=$db->tsquery("SELECT session.*, us_admin_access, us_sesflags FROM session INNER JOIN users ON ses_uid=us_id WHERE ses_session='$session' AND ses_fromv2=0", 'fetch sess');
	if (!$sid=fetch_o($res)) return;
//	$acl=db_trivialquery("SELECT us_admin_access FROM users WHERE us_id=$sid->ses_uid", 'f acl');

//print_r($sid);
	$nsid=array(
		'ses_session' 		=> $session,
		'ses_uid' 			=> $sid->ses_uid,
		'ses_begin' 		=> $sid->ses_begin,
		'ses_touch' 		=> $time,
		'ses_from' 			=> $sid->ses_from,
		'ses_bandwitchclass' => $sid->ses_bandwitchclass,
		'ses_family'		=> $sid->ses_family,
		'ses_os'			=> $sid->ses_os,
		'ses_pk'			=> $sid->ses_pk,
		'ses_os_ver_major'	=> $sid->ses_os_ver_major, 
		'ses_os_ver'		=> $sid->ses_os_ver,
		'ses_country'		=> $sid->ses_country,
		'ses_place'			=> $sid->ses_place,
		'ses_subscribed_by'	=> $sid->ses_subscribed_by,
		'ses_seen_alertmsgs' => 0,
		'ses_stbserial'		=> $sid->ses_stbserial,
		'ses_client_connection' =>$sid->ses_client_connection,
		'ses_login_log'		=> $sid->ses_login_log,
		'ses_taco'			=> $sid->ses_taco,
		'ses_flags'			=> $sid->us_sesflags,
		'ses_fromv1'		=> 1,
		'ses_usacl'			=> $sid->us_admin_access,
		'ses_usv'			=> usercache_freshver('user', 'access', $sid->ses_uid, 0)
	);


	$redis->hmset('sid'.$session, $nsid);
	$redis->expire('sid'.$session, 3600);
	$redis->zAdd('sescount'.$sid->ses_uid, $time, $session);
	// cleanup list of active sessions
	$redis->zRemRangeByScore('sescount'.$sid->ses_uid, 0, $time-3600);
	$redis->expire('sescount'.$sid->ses_uid, 3600);
	//$acls=explode(',', $sid->us_admin_access);	
}

function new_to_old_session($session, $lazytouch=0) {
	global $db, $redis, $redis_write;
	$time=time();
	if ($time-$lazytouch<600) return;
	$key='sid'.$session;
	$sid=$redis->hGetAll($key);
	if (!isset($sid['ses_session'])) return;
//print_r($sid);
	if (isset($sid['ses_fromv1']) && $sid['ses_fromv1']) return;
	
	
		$sql="
	ses_session='$session', 
	ses_uid=$sid[ses_uid],
	ses_begin=$sid[ses_begin],
	ses_touch=$time,
	ses_from='$sid[ses_from]',
	ses_status='active',
	ses_bandwitchclass='$sid[ses_bandwitchclass]',
	ses_family='$sid[ses_family]',
	ses_os='$sid[ses_os]',
	ses_pk=$sid[ses_pk],
	ses_os_ver_major='$sid[ses_os_ver_major]',
	ses_os_ver='$sid[ses_os_ver]',
	ses_country='$sid[ses_country]',
	ses_place='$sid[ses_place]',
	ses_subscribed_by=$sid[ses_subscribed_by],
	ses_seen_alertmsgs=$sid[ses_seen_alertmsgs],
	ses_stbserial='$sid[ses_stbserial]',
	ses_client_connection='$sid[ses_client_connection]',
	ses_login_log=$sid[ses_login_log],
	ses_taco='$sid[ses_taco]',
	ses_fromv2=1";
//echo $sql;
	db_trivialupdate("INSERT INTO session SET $sql ON DUPLICATE KEY UPDATE $sql", 'convert session');
}

$call_platform_url="http://soap.tv.starnet.cz";

function v2tov1_callbacks() {
	switch ($_REQUEST['function']) {
		case 'favorites':
			$_REQUEST['function']='epg/favorites';
			return(TRUE);
		case 'reorder_channel':
			$_REQUEST['function']='epg/reorder_channel';
			return(TRUE);
		case 'clear_user_prefs':
			$_REQUEST['function']='epg/clear_user_prefs';
			return(TRUE);
			break;
	}
	return(FALSE);
}

function v1tov2_callbacks() {
	switch ($_REQUEST['function']) {
		case 'epg/favorites':
			$_REQUEST['__url']='http://soap-tv.starnet.cz/V2/epg/user';
			$_REQUEST['function']='favorites';
			return(TRUE);
		case 'epg/reorder_channel':
			$_REQUEST['__url']='http://soap-tv.starnet.cz/V2/epg/user';
			$_REQUEST['function']='reorder_channel';
			return(TRUE);
		case 'epg/clear_user_prefs':
			$_REQUEST['__url']='http://soap-tv.starnet.cz/V2/epg/user';
			$_REQUEST['function']='clear_user_prefs';
			return(TRUE);

		case 'partner/delete_user':
		case 'partner/store_user':
		case 'partner/set_user_props':
		case 'partner/delete_subscriptions':
		case 'partner/store_subscriptions':
		case 'partner/delete_iplist':
		case 'partner/delete_ip':
		case 'partner/store_ip':
		case 'partner/register_stb':
			global $us_id;
			cache_setdirty('user', 'access', $us_id);
			return(FALSE);
	}
	return(FALSE);
}

function call_platform($callback=NULL) {
//return;
    global $call_platform_url, $soap_error, $soap_errdesc;
	if (isset($_REQUEST['__im_callback'])) return; // Recursion protect
	$_REQUEST['__im_callback']=TRUE;
	
	if ($callback && !$callback()) return; // We can't handle, do nothing
	if (isset($_REQUEST['__url'])) $purl=$_REQUEST['__url']; else $purl=$call_platform_url;

    $de = http_build_query($_REQUEST);

    $defaults = array(
		CURLOPT_URL => $purl.'?'.$de,
		CURLOPT_POST => FALSE,
		CURLOPT_TIMEOUT => 4, 
		CURLOPT_FORBID_REUSE => 0,
		CURLOPT_RETURNTRANSFER => 1,
		CURLOPT_FOLLOWLOCATION => TRUE,
//		CURLOPT_POSTFIELDS => $de
    );
	//print_r($defaults);
    $ch = curl_init();
    curl_setopt_array($ch, $defaults);

    $ret = curl_exec($ch);
    $soap_error = curl_getinfo($ch, CURLINFO_HTTP_CODE);
    if ($soap_error<200 || $soap_error>=300) {
		$soap_errdesc=$ret;
		return(NULL);
    }
    $soap_errdesc='OK';
    return($ret);
}

	
	
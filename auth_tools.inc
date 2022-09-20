<?PHP

define('BUY_PIN', 1);
define('PARENT_PIN', 2);

function acl_check($acl){
	global $sid;
	if (!isset($sid['ses_usacl'])) return(FALSE);
	$pos=strpos($sid['ses_usacl'], $acl);
	if ($pos!==false) return(TRUE); else return(FALSE);
}


function acl_check_or($acl){
	global $sid;
	$acl=explode(',', $acl);
	foreach ($acl as $a) 
		if (acl_check($a)) return (TRUE);
	return (FALSE);
}

function get_matched_iplist_items($ip, $listwhere) {
        global $db;
        $ip_pos_ip=compute_ip_posibility_sql($ip);

        $plist=db_trivialquery("SELECT GROUP_CONCAT(ip_id SEPARATOR ', ') FROM iplists
                        WHERE ip_type='allow' $listwhere AND ((ip_ver=4 AND ip_ip IN ($ip_pos_ip)))", 'users_by_ip: get_plist_ids');
        if (!$plist) $plist=0;
        $nlistids=db_trivialquery("SELECT GROUP_CONCAT(ip_listid SEPARATOR ', ') FROM iplists
                        WHERE ip_type='deny' AND ip_listid IN ($plist) $listwhere AND ((ip_ver=4 AND ip_ip IN ($ip_pos_ip)))", 'users_by_ip: get_nlist_ids');

        if (!$nlistids) $nlistids=0;

        $items=db_trivialquery("SELECT GROUP_CONCAT(ip_id ORDER BY IF (SUBSTRING_INDEX(ip_ip, '/', -1)>0,SUBSTRING_INDEX(ip_ip, '/', -1),1000) DESC SEPARATOR ',')
        FROM iplists WHERE ip_id IN ($plist) AND
                ip_listid NOT IN ($nlistids)", 'compare plist and nlist');
        return ($items);
}


function post_user_info($us, $sid) {
	global $debug;

	$sid['us_name']					=$us['us_name'];
	$sid['us_suggested_upgrade'] 	=$us['us_suggested_upgrade'];
	$sid['ta_name']					=$us['ta_name'];
	$sid['ta_description']			=$us['ta_description'];
	$sid['us_town']					=$us['us_town'];
	$sid['us_partner_id']			=$us['us_partner_id'];
	$sid['us_partner_code']			=$us['us_partner_code'];
	json_out($sid);
}

// not so IQ hash but perfectly simple and powerfull
function compute_session_hashpath($s) {
	return(substr($s, 0, 2).'/'.substr($s, 2, 2).'/'.substr($s, 4, 2).'/'.substr($s, 4, 2));
}

function new_sessiondata($us, $from, $ses_bandwitchclass='standard', $pk, 
	$country='cz', $place='unspecified', $stbserial='[none]', $client_connection='unknown', $login_log=NULL) {
	
	$session=bin2hex(random_bytes(16));
	$time=time();

	$pins=0;
	if ($us['us_buy_pin'] && $us['us_buy_pin']!='1111') $pins|=BUY_PIN;
	if ($us['us_parent_pin'] && $us['us_parent_pin']!='1111') $pins|=PARENT_PIN;

	return(
		array(
			'ses_session' 		=> $session,
			'ses_uid' 			=> $us['us_id'],
			'ses_begin' 		=> $time,
			'ses_touch' 		=> $time,
			'ses_from' 			=> $from,
			'ses_bandwitchclass' => $ses_bandwitchclass,
			'ses_family'		=> $pk->pk_family,
			'ses_os'			=> $pk->pk_os,
			'ses_pk'			=> $pk->pk_id,
			'ses_flags'			=> [],
			'ses_os_ver_major'	=> $pk->pk_major_ver, 
			'ses_os_ver'		=> $pk->pk_full_ver,
			'ses_country'		=> $country,
			'ses_place'			=> $place,
			'ses_subscribed_by'	=> NULL,
			'ses_seen_alertmsgs' => NULL,
			'ses_stbserial'		=> $stbserial,
			'ses_client_connection' =>$client_connection,
			'ses_login_log'		=> $login_log,
			'ses_taco'			=> $us['taco_online'],
			'ses_usacl'			=> $us['us_admin_access'],
			'pins'				=> $pins,
			'ses_usv' 			=> 0
		)
	);
}

// call and maintain on user change
function update_sessiondata($session, $us_id) {
	global $session_life, $redis_write, $db;
	if (!isset($db) || !$db) return;
	$pin_buy=db_trivialquery("SELECT us_buy_pin FROM users WHERE us_id=$us_id LIMIT 1", 'get bp');
	$pin_parent=db_trivialquery("SELECT us_parent_pin FROM users WHERE us_id=$us_id LIMIT 1", 'get bp');
	$ses_flags=db_trivialquery("SELECT us_sesflags FROM users WHERE us_id=$us_id LIMIT 1", 'get bp');
	$us_admin_access=db_trivialquery("SELECT us_admin_access FROM users WHERE us_id=$us_id LIMIT 1", 'get bp');

	$pins=0;
	if ($pin_buy && $pin_buy!='1111') $pins|=BUY_PIN;
	if ($pin_parent && $pin_parent!='1111') $pins|=PARENT_PIN;
	connect_redis_write();
	
	$taco=db_trivialquery("SELECT CONCAT(us_tarif, '.', IFNULL(GROUP_CONCAT(up_tarif), '-')) FROM users 
		LEFT JOIN user_packages ON up_user=us_id WHERE us_id=$us_id", 'get taco');
	
	$redis_write->hset('sid'.$session, 'pins', $pins);
	$redis_write->hset('sid'.$session, 'ses_taco', $taco);
	$redis_write->hset('sid'.$session, 'ses_flags', $ses_flags);
	$redis_write->hset('sid'.$session, 'ses_usacl', $us_admin_access);
	
	//$redis_write->expire('sid'.$session, $session_life);
	$redis_write->hset('sid'.$session, 'ses_usv', usercache_freshver('user', 'access', $us_id, 0));
}

function post_session_info($sid) {
	global $dbg;
//	if (isset($dbg->ebgs_flags)) $dbgf=$dbg->ebgs_flags; else $dbgf=NULL;
//print_r($sid);
	if (isset($sid['pins']) && $sid['pins'] & BUY_PIN) $pins['parent']=TRUE; else $pins['parent']=FALSE;
	if (isset($sid['pins']) && $sid['pins'] & PARENT_PIN) $pins['buy']=TRUE; else $pins['buy']=FALSE;
	if (!isset($sid['ses_usacl'])) $sid['ses_usacl']='';

				$data=array(
					'session' => $sid['ses_session'],
					'ses_begin' => $sid['ses_begin'],
					'ses_touch' => $sid['ses_touch'],
					'ses_bandwitchclass' => $sid['ses_bandwitchclass'],
					'ses_family' => $sid['ses_family'],
					'ses_place' => $sid['ses_place'],
					'ses_country' => $sid['ses_country'],
					'ses_client_connection' => $sid['client_connection'] ?? 'unknown',
					'ses_taco' => $sid['ses_taco'],
					'ses_uid' => $sid['ses_uid'],
					'ses_family' => $sid['ses_family'],
					'ses_os' => $sid['ses_os'],
					'ses_pk' => $sid['ses_pk'],
					'enabled_pins' => $pins,
					'us_enabled_pins' => $pins,
					'timestamp' => time(),
					'time' => date('Y-m-d H:i:s'), 
					'ses_flags' => explode(',', $sid['ses_flags']),
					'ses_usv' => $sid['ses_usv'],
					'ses_usacl'	=> $sid['ses_usacl']
				);
			json_out($data);
}

function session_check($session, $post=TRUE, $noexit=FALSE) {
	global $redis, $redis_write, $session_life, $session_update, $db;
	if (!$session && $noexit) return(FALSE);
	if (!$session) throw_error(401, 'no session sent in request');
	$time=time();
	$key='sid'.$session;
	$sid=$redis->hGetAll($key);
	if (empty($sid) && $noexit) return(FALSE);
	if (empty($sid)) throw_error(401, 'session not found or is invalid (V2)');
	if ($time-$sid['ses_touch']>$session_update) {
		connect_redis_write();
		$sid['ses_touch']=$time;
		$redis_write->expire($key, 3600);
		$redis_write->hset($key, 'ses_touch', $time);
		$redis_write->zAdd('sescount'.$sid['ses_uid'], $time, $session);
		if (isset($db) && $db) db_trivialupdate("UPDATE session SET ses_status='active', ses_touch=UNIX_TIMESTAMP() WHERE ses_session='$session' LIMIT 1", 'updating V1 session status');
	}
	if (!isset($sid['pins']) || $sid['pins']=='') update_sessiondata($session, $sid['ses_uid']); // should be call by store_user, store_user_props etc through V2V1 bridge
	if ($post) post_session_info($sid);
	return($sid);
}

function get_taco($uid) {
	return(db_trivialquery("
		SELECT 
			CONCAT(us_tarif, '.', IFNULL(GROUP_CONCAT(up_tarif), '-'))
		FROM users
		LEFT JOIN user_packages ON up_user=us_id
		WHERE us_id=$uid
	", 'copute taco'));
}

function session_end($session) {
	global $redis, $redis_write;
	connect_redis_write();
	$key='sid'.$session;
	$uid=$redis->hGet($key, 'ses_uid');
	if (!$uid) return(FALSE);
	$redis_write->zRem('sescount'.$uid, $session);
	$redis_write->del($key);
	return(TRUE);
}


function read_user_info($uid) {
	global $db;
	$res=$db->tsquery("
			SELECT 
				users.*,
				ta_name, ta_description,
				pa_name, pa_productname, pa_info_email, pa_support_email, pa_support_phone,
			CONCAT(us_tarif, '.', IFNULL(GROUP_CONCAT(up_tarif), '-')) AS taco_online
			FROM users 
			INNER JOIN partners ON users.us_partner_id=partners.pa_id
			INNER JOIN tarifs ON users.us_tarif=ta_id
			LEFT JOIN user_packages ON up_user=us_id AND up_flag='valid'
			WHERE us_id=$uid", 'checking userdata');
	$sid=fetch_a($res);
	if (!$sid || !$sid['us_id']) throw_error(417, "login successfull, but inconsistent user data like tarif, packages etc");
	return($sid);
}


function v2_auth_iptv($client_ip, $ip_cookie, $gateway_ip, $stbserial, $login, $password, $p_key, $client_info='', $client_connection='unknown') {
	global $fs_root, $db, $sid, $redis, $redis_write, $session_life, $cache_ramdisk, $module_name;
	
	$client_ip=preg_replace("/\s|\\\\r|\\\\n|\\\\/i", '', $client_ip);
	if ($client_ip && !$ip_cookie) throw_error(401, "IP auth without IP cookie forbidden in V2");

	if ($client_ip) {
		$sd="$cache_ramdisk/$module_name/$client_ip=$ip_cookie";
		if (!file_exists($sd)) {
//			throw_error(401, "IP address certification failed") // strict way
//			$client_ip='';  // lets do friendly way
		}
	}
	
	$time=time();
	dblog("autorizace V2: client_ip=$client_ip, gateway_ip=$gateway_ip, stbserial=$stbserial, login=$login", 'auth', 'login'); 
	
	if (!$gateway_ip) $gateway_ip=$_SERVER['REMOTE_ADDR'];
	
//	echo "auth_iptv($client_ip, $gateway_ip, $stbserial, $login, $password, $check_only, $p_key)";

//	if ($stbserial && $stbserial!='undefined' && $stbserial!='null') {
	if ($stbserial && $stbserial!='undefined' && $client_ip!='null' && $client_ip!='2.2.2.2' && $client_ip!='1.1.1.1' && stripos($client_ip, '10.')===0) {
		$session=$redis->get("seshook.$stbserial.$client_ip");
		if ($session) {
			session_check($session);
			dblog("login hook V2: returning session $flow_hook_ses instead login, tdif=$off (client_ip=$client_ip, stbserial=$stbserial, login=$login)", 'auth', 'loginok', 'warning');
			return;
		}
	}

	$pk_override=FALSE;
	$pk_override=db_trivialquery("SELECT sbb_arg FROM subscription_blacklist WHERE sbb_type='login' AND sbb_action='change_pk' AND sbb_data='$login' LIMIT 1", 'check blacklist login change');
	if (!$pk_override) $pk_override=db_trivialquery("SELECT sbb_arg FROM subscription_blacklist WHERE sbb_type='stb_serial' AND sbb_action='change_pk' AND '$stbserial' LIKE sbb_data LIMIT 1", 'check blacklist pk change');
	if ($pk_override) $res=$db->tsquery("SELECT * FROM platform_keys WHERE pk_id=$pk_override", 'check platform key');
	else $res=$db->tsquery("SELECT * FROM platform_keys WHERE pk_value='$p_key'", 'check platform key');
	if(!$pk=fetch_o($res)) {
		dblog("login fail V2: platform key $p_key invalid: client_ip=$client_ip, gateway_ip=$gateway_ip, stbserial=$stbserial, login=$login", 'auth', 'loginfail', 'warning');
		throw_error(400, "platform key wrong");
	}

//	print_r($pk);
	$ip_pos_ip=compute_ip_posibility_sql($client_ip);
	$ip_pos_gate=compute_ip_posibility_sql($gateway_ip);

	$ip_pos_ip_lists=db_trivialquery("SELECT GROUP_CONCAT(DISTINCT ip_listid) FROM iplists WHERE NOT ip_customer IS NULL AND ip_type='allow' AND ip_ver=4 AND ip_ip IN ($ip_pos_ip)", 'get p ip l');
	if (!$ip_pos_ip_lists) $ip_pos_ip_lists="-1";
	$ip_neg_ip_lists=db_trivialquery("SELECT GROUP_CONCAT(DISTINCT ip_listid) FROM iplists WHERE NOT ip_customer IS NULL AND ip_type='deny' AND ip_ver=4 AND ip_ip IN ($ip_pos_ip)", 'get n ip l');
	if (!$ip_neg_ip_lists) $ip_neg_ip_lists="-1";

	$ip_pos_gate_lists=db_trivialquery("SELECT GROUP_CONCAT(DISTINCT ip_listid) FROM iplists WHERE NOT ip_customer IS NULL AND ip_type='allow' AND ip_ver=4 AND ip_ip IN ($ip_pos_gate)", 'get p gate l');
	if (!$ip_pos_gate_lists) $ip_pos_gate_lists="-1";
	$ip_neg_gate_lists=db_trivialquery("SELECT GROUP_CONCAT(DISTINCT ip_listid) FROM iplists WHERE NOT ip_customer IS NULL AND ip_type='deny' AND ip_ver=4 AND ip_ip IN ($ip_pos_gate)", 'get n gate l');
	if (!$ip_neg_gate_lists) $ip_neg_gate_lists="-1";
//print_r($ip_pos_ip);
	
	$subs_login=db_trivialquery("SELECT GROUP_CONCAT(DISTINCT sb_id) FROM subscriptions WHERE sb_check_type like '%login%' AND sb_user='$login' AND sb_user<>''", 'q l s');
	$subs_login=explode(',', $subs_login);

	$subs_serial=db_trivialquery("SELECT GROUP_CONCAT(DISTINCT sb_id) FROM subscriptions WHERE sb_check_type like '%serial%' AND sb_stbserial='$stbserial' AND sb_stbserial<>''", 'q s s');
	$subs_serial=explode(',', $subs_serial);

	$subs_ip=db_trivialquery("SELECT GROUP_CONCAT(DISTINCT sb_id) FROM subscriptions WHERE sb_check_type like '%ip%' AND sb_ip IN ($ip_pos_ip_lists) AND NOT sb_ip IN ($ip_neg_ip_lists)", 'q i s');
	$subs_ip=explode(',', $subs_ip);

	$subs_gateway=db_trivialquery("SELECT GROUP_CONCAT(DISTINCT sb_id) FROM subscriptions WHERE sb_check_type like '%gateway%' AND sb_ip IN ($ip_pos_gate_lists) AND NOT sb_ip IN ($ip_neg_gate_lists)", 'q g s');
	$subs_gateway=explode(',', $subs_gateway);

	$s_array=array();
	
	foreach ($subs_login as $s) $s_array[$s]['login']=1;
	foreach ($subs_serial as $s) $s_array[$s]['serial']=1;
	foreach ($subs_ip as $s) $s_array[$s]['ip']=1;
	foreach ($subs_gateway as $s) $s_array[$s]['gateway']=1;

	
	$s_valid='';
	foreach ($s_array as $s => $ch) {
		if (!$s) continue;
		if (!isset($ch['login']) && db_trivialquery("SELECT sb_id FROM subscriptions WHERE sb_id=$s AND sb_check_type like '%login%'", 'qfl')) continue;
		if (!isset($ch['serial']) && db_trivialquery("SELECT sb_id FROM subscriptions WHERE sb_id=$s AND sb_check_type like '%serial%'", 'qfs')) continue;
		if (!isset($ch['ip']) && db_trivialquery("SELECT sb_id FROM subscriptions WHERE sb_id=$s AND sb_check_type like '%ip%'", 'qqi')) continue;
		if (!isset($ch['gateway']) && db_trivialquery("SELECT sb_id FROM subscriptions WHERE sb_id=$s AND sb_check_type like '%gateway%'", 'qqg')) continue;
		if ($s_valid) $s_valid.=',';
		$s_valid.=$s;
	}
	if (!$s_valid) $s_valid='-1';

	$res=$db->tsquery("SELECT * FROM subscriptions WHERE sb_id IN ($s_valid) AND sb_userid<>0 ORDER BY sb_check_priority", 'overit auth');
	$lp=0;
	$subs_pos=$res->fetchAll(PDO::FETCH_ASSOC);
//print_r($subs_pos);
	foreach($subs_pos as $usr) {

		if (!strstr($usr['sb_check_type'], 'login') || $check_only) {
			$lp=1;
			break; // pokud neni login nebo checkujeme, pak vyhovi jakykoliv radek
		}
		if ($password==$usr['sb_password'] && $usr['sb_password'][0]!='$' && $usr['sb_password']<>'') {
			$lp=1;
			break;
		}
		if ($usr['sb_password'][0]=='$' && password_verify($password, $usr['sb_password'])) {
			$lp=1;
			break;
		}
	}
	if (!$lp) {
		dblog("login fail: no suitable subscription: client_ip=$client_ip, gateway_ip=$gateway_ip, stbserial=$stbserial, login=$login", 'auth', 'loginfail', 'warning'); 
		throw_error(401, "unable to login, credentials invalid");
	}

	$us=read_user_info($usr['sb_userid']);

	$active_sessions=$redis->zRangeByScore('sescount'.$usr['sb_userid'], $time-$session_life, $time+100,  ['withscores' => TRUE]);
	if (count($active_sessions)>$us['us_max_logins']) {
		dblog("login fail V2: max_concurent_users: client_ip=$client_ip, gateway_ip=$gateway_ip, stbserial=$stbserial, login=$login", 'auth', 'loginfail', 'warning', '', $usr['sb_userid']); 
		throw_error(429, "maximum concurent connection reached for user $us[us_name]");
	}
	

	$bandwitchclass=$us['us_force_linespeed'];
	if ($gateway_ip && $gateway_ip!='null' && $gateway_ip!='1.1.1.1' && $gateway_ip!='undefined') {
		if (!$bandwitchclass && db_trivialquery("SELECT COUNT(*) FROM iplists WHERE ip_listid=(SELECT pa_wideband_ip FROM partners WHERE pa_id=(SELECT us_partner_id from users WHERE us_id=$usr[sb_userid])) AND ip_type='allow' AND ((ip_ver=4 AND ip_ip IN ($ip_pos_gate)))", 'ceck_wide_bandwitch')) $bandwitchclass='wideband';
		if (!$bandwitchclass && db_trivialquery("SELECT COUNT(*) FROM iplists WHERE ip_listid=(SELECT pa_stdband_ip FROM partners WHERE pa_id=(SELECT us_partner_id from users WHERE us_id=$usr[sb_userid])) AND ip_type='allow' AND ((ip_ver=4 AND ip_ip IN ($ip_pos_gate)))", 'ceck_std_bandwitch')) $bandwitchclass='standard';
		if (!$bandwitchclass && db_trivialquery("SELECT COUNT(*) FROM iplists WHERE ip_listid=(SELECT pa_cellband_ip FROM partners WHERE pa_id=(SELECT us_partner_id from users WHERE us_id=$usr[sb_userid])) AND ip_type='allow' AND ((ip_ver=4 AND ip_ip IN ($ip_pos_gate)))", 'ceck_std_bandwitch')) $bandwitchclass='cell';
	}
	if ($client_ip && $client_ip!='null' && $client_ip!='1.1.1.1' && $client_ip!='undefined') {
		if (!$bandwitchclass && db_trivialquery("SELECT COUNT(*) FROM iplists WHERE ip_listid=(SELECT pa_cellband_ip FROM partners WHERE pa_id=(SELECT us_partner_id from users WHERE us_id=$usr[sb_userid])) AND ip_type='allow' AND ((ip_ver=4 AND ip_ip IN ($ip_pos_ip)))", 'ceck_std_bandwitch')) $bandwitchclass='cell';
	}

	if (!$bandwitchclass) $bandwitchclass=db_trivialquery("SELECT pa_default_bandwith FROM partners WHERE pa_id=
		(SELECT us_partner_id FROM users WHERE us_id=$usr[sb_userid])", 'get dfl bandwith');
		
	$ff=db_trivialquery("SELECT us_force_family FROM users WHERE us_id=$usr[sb_userid]", 'check force_family');
	if ($ff) $pk->pk_family=$ff;
		
	if (db_trivialquery("SELECT us_force_home FROM users WHERE us_id=$usr[sb_userid]", 'check force_home')) $place='override';
	else 
		if	(get_matched_iplist_items($client_ip, "AND FIND_IN_SET('home', ip_flag) AND ip_customer=$usr[sb_userid]"))
		$place='home'; 
		else 
			if (get_matched_iplist_items($client_ip, "AND FIND_IN_SET('partner', ip_flag)"))
			$place='network';
			else {
				$place=geoip_country_code_by_name($client_ip);
				$country=$place;
				if (!$place) $place='unspecified';
				
			}
	if ($place=='home' && db_trivialquery("SELECT COUNT(*) FROM tarifs WHERE ta_overrides like '%home_wideband%' AND (ta_id=$us[us_tarif] OR ta_id IN (SELECT up_tarif FROM user_packages WHERE up_user=$usr[sb_userid]))", 'check home_wideband override'))
		$bandwitchclass='wideband';
	if (db_trivialquery("SELECT FIND_IN_SET('$pk->pk_family', ta_disabled_family) FROM tarifs WHERE ta_id=$us[us_tarif]", 'check disabled family'))
		throw_error(415, "use this device forbidden by tarif");

	$iret=NULL;
//	include ($fs_root.'/usrscripts/login_hook_login_ok_before_session.inc');
	if ($iret) return($iret);

	
	if (!@$country) $country='CZ';
	dblog("login OK: client_ip=$client_ip, gateway_ip=$gateway_ip, stbserial=$stbserial, login=$login, pk=$pk->pk_id, conn=$client_connection, hw=$client_info", 'auth', 'loginok', 'info', '', $usr['sb_userid']);
	
	$db->tsquery("INSERT INTO log_logins SET ll_user='$usr[sb_userid]', 
		ll_sid=NULL, ll_client_ip='$client_ip', 
		ll_client_gateway='$gateway_ip', ll_stbserial='$stbserial', ll_username='$login', ll_pk='$pk->pk_id', 
		ll_hw_revision='$client_info', ll_sb_id=$usr[sb_id], ll_client_connection='$client_connection',
		ll_place='$place'", 'insert loginlog');
	
	$login_log=$db->lastInsertId();
	
	
	
	
	$sid=new_sessiondata($us, $client_ip, $bandwitchclass, $pk, $country, $place, $stbserial, $client_connection, $login_log);
	connect_redis_write();
	$redis_write->hmset('sid'.$sid['ses_session'], $sid);
	$redis_write->expire('sid'.$sid['ses_session'], $session_life);
	$redis_write->zAdd('sescount'.$usr['sb_userid'], $time, $sid['ses_session']);
	// cleanup list of active sessions
	$redis_write->zRemRangeByScore('sescount'.$usr['sb_userid'], 0, $time-$session_life);
	$redis_write->expire('sescount'.$usr['sb_userid'], $session_life);
	update_sessiondata($sid['ses_session'], $usr['sb_userid']);
	post_user_info($us, $sid);
}
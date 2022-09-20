<?PHP
global $db;
// very carefull about persist=1 wher user not logged see below
function pref_get($pref, $type='user') {
   global $sid, $db;
   switch ($type) {
	case 'user':
		$key="pref_uid='$sid->us_id' AND pref_partner='$sid->us_partner_id'";
		break;
	case 'session':
		$key="pref_sid='$sid->ses_id'";
		break;
	case 'partner':
		$key="pref_partner='$sid->us_partner_id'";
		break;
	default:
		die("pref_get: invalid type $user");
	}
    return(db_trivialquery("SELECT pref_val FROM prefs WHERE $key AND `pref_key`='$pref'", "reading prefs ($pref)"));
}

// careful on persist=1, it allows change user prefs wo login
// you can use something as is_logged as persist (when !0, pref is assigned 
// to user (even no logged), otherwise to session 
function pref_set($pref, $val, $type='user') {
  global $sid, $db;
   switch ($type) {
	case 'user':
		$key="pref_uid='$sid->us_id', pref_partner='$sid->us_partner_id'";
		break;
	case 'session':
		$key="pref_sid='$sid->ses_id', pref_partner='$sid->us_partner_id'";
		break;
	case 'partner':
		$key="pref_partner='$sid->us_partner_id'";
		break;
	default:
		die("pref_set: invalid type $user");
	}
   $val=addslashes($val);
   $res=$db->tsquery("INSERT INTO prefs SET $key, pref_key='$pref', pref_val='$val' ON DUPLICATE KEY UPDATE pref_val='$val'",
		"setting prefs ($pref=$val)");
}

function sys_pref_get($pref) {
   global $db;
   return(db_trivialquery("SELECT pref_val FROM prefs WHERE `pref_key`='$pref'", "reading prefs ($pref)"));
}

function sys_pref_set($pref, $val) {
   global $db;
   $val=addslashes($val);
   $res=$db->tsquery("REPLACE INTO prefs SET pref_partner=-1, `pref_key`='$pref', pref_val='$val'", "setting prefs ($pref=$val)");
}


?>
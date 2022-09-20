<?PHP
function get_access_cond($us_id) {
	global $db;
	$res=$db->tsquery("SELECT ada_grant, ada_revoke FROM api_doc_access WHERE 
		ada_name=(SELECT adp_access FROM ap_doc_passwords WHERE adp_id=$us_id)", 'get access');
	$c_grant='';
	$c_revoke='';
	while ($c=fetch_o($res)) {
//print_r($c);
		if ($c->ada_grant) {
			if ($c_grant) $c_grant.=' OR ';
			$c_grant.="ad_resource LIKE '$c->ada_grant'";
		}
		if ($c->ada_revoke) {
			if ($c_revoke) $c_revoke.=' AND NOT ';
			$c_revoke.="ad_resource LIKE '$c->ada_revoke'";
		}
	}
	$c='';
	if ($c_grant) $c="($c_grant)";
	if ($c_revoke) {
		if ($c) $c.=' AND ';
		$c.=$c_revoke;
	}

	if (!$c) return(FALSE);
	return($c);
}
	
?>
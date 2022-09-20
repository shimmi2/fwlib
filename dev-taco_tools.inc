<?PHP

function gen_all_taco_rights() {
	global $db;
	$res=$db->tsquery("SELECT taco_taco FROM tarif_combos where 1");
	$ret=[];
	while ($r=fetch_o($res))
		$ret[$r->taco_taco]=gen_taco_rights($r->taco_taco);
		
	return($ret);
}

function gen_taco_rights($taco) {
	global $db, $tarif_rights, $ch_classes;

	if (!$tarif_rights) $tarif_rights=get_set_string('tarif_rules', 'tr_type');
	if (!$ch_classes) $ch_classes=get_set_string('channels', 'ch_type');

	tolog ("computing rights taco $taco", L_DEBUG1);
	$taco=norm_taco($taco);
	$rest=$db->tsquery("SELECT ch_id, ch_name, ch_admin_state, IFNULL(sg_default_timeline, 'none') AS tl_name,
			BIT_OR(ta_display_channels) & (1 << (FIND_IN_SET(ch_type, '$ch_classes')-1)) || BIT_OR(tr_type) & (1 << (FIND_IN_SET('display', '$tarif_rights')-1)) AS ta_display_channels, 
			BIT_OR(ta_rights_live) & (1 << (FIND_IN_SET(ch_type, '$ch_classes')-1)) || BIT_OR(tr_type) & (1 << (FIND_IN_SET('live', '$tarif_rights')-1)) AS ta_rights_live,
			BIT_OR(ta_rights_history) & (1 << (FIND_IN_SET(ch_type, '$ch_classes')-1)) || BIT_OR(tr_type) & (1 << (FIND_IN_SET('history', '$tarif_rights')-1)) AS ta_rights_history,
			BIT_OR(ta_rights_offset) & (1 << (FIND_IN_SET(ch_type, '$ch_classes')-1)) || BIT_OR(tr_type) & (1 << (FIND_IN_SET('offset', '$tarif_rights')-1)) AS ta_rights_offset,
			BIT_OR(ta_rights_recording) & (1 << (FIND_IN_SET(ch_type, '$ch_classes')-1)) || BIT_OR(tr_type) & (1 << (FIND_IN_SET('recording', '$tarif_rights')-1)) AS ta_rights_recording
		FROM channels
		LEFT JOIN tarifs ON ta_id IN ($taco)
		LEFT JOIN tarif_rules ON ta_id=tr_tarif AND tr_channel=ch_id
		LEFT JOIN statgroups ON ch_statgroup1=sg_id
		GROUP BY ch_id", 'fetch rights');
	$ret=[];
	while ($r=fetch_a($rest))
		$ret[$r['ch_id']]=$r;
	return($ret);
}

function update_channel_allowed_users() {
	global $db;
	
	$res=$db->tsquery("SELECT ch_id, ch_name, GROUP_CONCAT(DISTINCT ta_id ORDER BY ta_id) AS tarifs FROM channels
LEFT JOIN tarif_rules ON tr_channel=ch_id AND FIND_IN_SET('live', tr_type)
LEFT JOIN tarifs ON (FIND_IN_SET(ch_type, ta_rights_live) OR tr_tarif=ta_id) AND ta_anounce_asactive
GROUP BY ch_id", 'get tarifs per ch');
	$tachache=[];
	while ($chi=fetch_a($res)) {
		if (isset($tachache[$chi['tarifs']])) {
			$cnt=$tachache[$chi['tarifs']];
			tolog ("cached count ch_id=$chi[ch_id], tarifs allowed $chi[tarifs]: $cnt", L_DEBUG1);
		} else {			
			$cnt=db_trivialquery("SELECT COUNT(*) FROM users WHERE us_tarif IN ($chi[tarifs]) OR us_id IN 
				(SELECT up_user FROM user_packages WHERE up_tarif IN ($chi[tarifs]) AND up_flag IN ('valid','no_invoice'))",
				'get count');
			$tachache[$chi['tarifs']]=$cnt;
			tolog ("computed count ch_id=$chi[ch_id], tarifs allowed $chi[tarifs]: $cnt", L_DEBUG1);			
		}
		db_trivialupdate("UPDATE channels SET ch_allowed_active_users=$cnt WHERE ch_id=$chi[ch_id]", 'sto ch_users');
	}
}





	

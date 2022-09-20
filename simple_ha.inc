<?PHP

/* This is pre implementation of HA lib
	Its stupid, withou HA, detections etc
	Just for preliminary call
	
*/

global $ha_ep_selections;

function handle_V2_ret(&$sel, &$data, $status, $tolog) {
	switch ($sel['transport']) {
		case 'none':
			break;
		case 'gz':
			$data=gzdecode($data);
			break:
		default:
			if ($tolog) tolog("V2call: invalid transport format '$sel[transport]'", $tolog);
			return(FALSE);
	}
	switch ($sel['format']) {
		case 'bin':
			return($data);
		case 'json':
			return(json_decode($data, TRUE));
		case 'status':
			return($status); // errors already returned before
		default:
			if ($tolog) tolog("V2call: decode format '$sel[format]'", $tolog);
			return(FALSE);
	}
}

function V2call($ep, $param, &$soap_status, &$soap_error, $tolog=L_ERR | L_DIE) {
	global $ha_ep_selections, $ha_ep_cache;
	if (!isset($ha_ep_selections) || !$ha_ep_selections) require_once($ha_ep_cache);
	if (!isset($ha_ep_selections) || !$ha_ep_selections) {
		 $soap_status=-1;
		 &$soap_error="SYSTEM BROKEN, no API call rules in $ha_ep_cache";
		 if ($tolog) $tolog("V2call status $soap_status/$soap_error", $tolog);
		 return(FALSE);
	}
	if (!isset($ha_ep_selections[$ep] {
		 $soap_status=-1;
		 &$soap_error="SYSTEM BROKEN, no API call rules in $ha_ep_cache";
		 if ($tolog) $tolog("V2call status $soap_status/$soap_error", $tolog);
		 return(FALSE);		 
	}
	$sel=&$ha_ep_selections[$ep];
	
	$cp=$sel['call_template'];
	foreach ($param as $k=> $v)
		$cp=str_replace($k, $v, $cp);
		
	if ($sel['local']) {
		tolog("V2call local: $uri", L_DEBUG1);
		if ($ret=file_get_contents($sel['url'].$cp;)) {
			$soap_status='200';
			$soap_error='local OK';
			return(handle_V2_ret($sel, $ret, $soap_status, $tolog));
		} else {
			$soap_status='404';
			$soap_error='local file not exists or unreadable';
			if ($tolog) $tolog("V2call local status $soap_status/$soap_error", $tolog);
			return(FALSE);
		}
	} else {
		// force get at this time do this at manifest
		tolog("V2call HTTP: $sel[url]$cp", L_DEBUG1);
		$ret=do_curl($sel['url'], $cp, $http_code, NULL, $timeout=15) {
		if ($http_code<200 || $http_code>=300) {
			// handle error here, escalation etc for HA
			$soap_error=$ret;
			if ($tolog) tolog("V2call HTTP $http_code/$soap_error", $tolog);
			return(FALSE);
		}
		$soap_error='OK';
		return(handle_V2_ret($sel, $ret, $soap_status, $tolog));
}

function gen_ep_selections($can_local=TRUE, $can_http=TRUE) {
	global $ha_ep_selections, $ha_ep_cache, $ha_ep_resourcefile;
	if (!file_exists($ha_ep_resourcefile)) tolog ("FATAL: generate endpoins cache: HA resource file $ha_ep_resourcefile not exists, API calls will be broken", L_ERR | L_DIE);
	$rf=file_get_contents($ha_ep_resourcefile);
	if (!$rf) tolog ("FATAL: generate endpoins cache: HA resource file $ha_ep_resourcefileempty or unreadable, API calls will be broken", L_ERR | L_DIE);
	$res=json_decode(gzdecode($rf), TRUE);
	if (!$res) tolog ("FATAL: generate endpoins cache: HA resource file $ha_ep_resourcefile failed do decompress and decode API calls will be broken", L_ERR | L_DIE);

	$new=[];
	
	//print_r($res);
	
	foreach  ($res as $resource => $res_data) {
		if ($resource=='__config') continue;
		foreach  ($res_data as $escalation => $escal_data)
			foreach  ($escal_data as $cloud_vesrion => $ver_data)
				if ($ver_data['generator_key']=='master') {
					if ($can_local && $ver_data['target_local']) {
						$url=$ver_data['target_local'];
						$loc=TRUE;
					} else {
						$loc=FALSE;
						if ($can_http && $ver_data['prefer']=='http' && $ver_data['target_http']) $url=$ver_data['target_http']; 
						else if ($ver_data['target_https']) $url=$ver_data['target_https'];
							else {
								$url=NULL;
								tolog("failed to select URL for resource $resource@$escalation:$cloud_vesrion", L_ERR);
							}
					}
					$new[$resource]=$ver_data;
					$new[$resource]['url']=$url;
					$new[$resource]['local']=$url;
				}
	}
	
	if (!$new) tolog ("FATAL: $ha_ep_resourcefileempty didn't generated ANY endpoings, stopping for sanity here or API calls will be broken", L_ERR | L_DIE);
	
	// this part is stricly for PHP fast like hell, but not JSON compatible with others
	$out="<?PHP
global \$ha_ep_selections;
\$ha_ep_selections=[";
	foreach ($new as $ep => $d)
		$out.="\n\t'$ep' => [ 'url' => '$d[url]', 'call_template' => '$d[call_template]', 'transport' => '$d[transport]', 'format' => '$d[format]', 'local' => '$d[local]' ],";
		
	$out.="\n];
";
	
	//echo $out;
	if (file_exists($ha_ep_cache) && file_get_contents($ha_ep_cache)==$out)
		tolog ("NOT writting $ha_ep_cache, file unchanged", L_DEBUG);
	else {
		if (!file_put_contents($ha_ep_cache, $out)) tolog ("FATAL: unable to write endpoinsts cache file, $ha_ep_cache, API calls will be broken", L_ERR | L_DIE);
		tolog ("file $ha_ep_cache generated", L_INFO);
	}
}
		
		
		
	
	
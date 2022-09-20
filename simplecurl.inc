<?PHP

function do_curl($url, $uri, &$soap_error, $postfields, $timeout=15) {
   if ($postfields) {
	   $de = http_build_query($postfields);
	   $post=TRUE;
   } else $post=FALSE;
 
 if ($uri) $uri=http_build_query($uri);
   $defaults = array(
        CURLOPT_URL				=> $url.$uri,
        CURLOPT_TIMEOUT			=> $timeout,
		CURLOPT_POST			=> $post,
        CURLOPT_FORBID_REUSE	=> 0,
		CURLOPT_FOLLOWLOCATION	=> TRUE,
        CURLOPT_RETURNTRANSFER	=> TRUE,
    );
	if ($post) $defaults[CURLOPT_POSTFIELDS] = $de;

    $ch = curl_init();
    curl_setopt_array($ch, $defaults);

    $ret = curl_exec($ch);
    $soap_error = curl_getinfo($ch, CURLINFO_HTTP_CODE);
    curl_close($ch);
    return($ret);
}

function do_curl_checked($url, $uri, &$soap_error, &$soap_errdesc, $postfields, $logparam=L_DIE, $timeout=15) {
	$ret=do_curl($url, $uri, $soap_error, $postfields, $timeout);
    if ($soap_error<200 || $soap_error>=300) {
		$soap_errdesc=$ret;
		tolog("do_curl failed: HTTP $soap_error/$ret", L_WARN | $logparam);
		return(NULL);
	}
    $soap_errdesc='OK';
	return($ret);
}

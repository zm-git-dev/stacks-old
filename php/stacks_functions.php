<?php
//
// Copyright 2010, Julian Catchen <jcatchen@uoregon.edu>
//
// This file is part of Stacks.
//
// Stacks is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Stacks is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Stacks.  If not, see <http://www.gnu.org/licenses/>.
//

function write_header($page_title) {
    global $version, $site_title; 
    global $root_path, $img_path;

    echo <<< EOQ
<?xml version="1.0" encoding="iso-8859-1"?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"
    "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">

<head>
  <title>$site_title: $page_title</title>
  <link rel="stylesheet" type="text/css" href="$root_path/stacks.css" />
  <script type="text/javascript" src="$root_path/stacks.js"></script>
  <script type="text/javascript" src="$root_path/annotate.js"></script>
  <script type="text/javascript" src="$root_path/export.js"></script>
</head>

<body>
<div class="main" style="width: 95%;">
<div id="header">
<h1><a href="$root_path/"><img src="$img_path/stacks_logo_rev_small.png" /></a></h1>
<p>
  <a href="http://creskolab.uoregon.edu/stacks/">version 

EOQ;

    include("version.php");
    echo <<< EOQ
  </a>
</p>
</div>

EOQ;
}

function write_compact_header($page_title) {
    global $root_path, $css_path, $site_title, $js_path;

    echo <<< EOQ
<?xml version="1.0" encoding="iso-8859-1"?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"
    "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">

<head>
  <title>$site_title: $page_title</title>
  <link rel="stylesheet" type="text/css" href="$root_path/stacks.css" />
  <script type="text/javascript" src="$root_path/stacks.js"></script>
</head>

<body>

EOQ;
}

function write_footer() {

    echo <<< EOQ
<div class="footer">
  <div style="float: left;">
    last updated: 
EOQ;

    include("last_modified.php");

    echo <<< EOQ
  </div>
  <div style="float: right;">
    <a href="http://validator.w3.org/check/referer">
      <img src="/graphics/valid-xhtml10.png" alt="Valid XHTML 1.0!" height="31" width="88" />
    </a>
  </div>
</div>

</body>
</html>

EOQ;
}

function write_compact_footer() {

    echo <<< EOQ

</body>
</html>

EOQ;
}

function print_scale($max_len) {
    $class = array("light_scale", "dark_scale");
    $str   = "";
    $c     = 0;

    $str .= "<span class=\"" . $class[$c % 2] . "\">";
    for ($i = 0; $i < $max_len; $i++) {
      
        $str .= $i % 10;

	if (($i + 1) % 10 == 0) {
	  $c++;
	  $str .= "</span><span class=\"". $class[$c % 2] . "\">";
	}
    }
    $str .= "</span>";

    return $str;
}

function print_snps($consensus, $seq, $snps) {
    if (count($snps) == 0) 
	return $seq;

    $str   = "";
    $start = 0;

    while (count($snps)) {
	$snp = array_shift($snps);
	$con = substr($consensus, $snp['col'], 1);
	$end = $snp['col'];

	$s    = substr($seq, $start, $end - $start);
	$str .= $s;
	$s    = substr($seq, $end, 1);

	if ($con == $s)
	    $str .= "<span class=\"rank_1\">$s</span>";
	else
	    $str .= "<span class=\"rank_2\">$s</span>";

	$start = $end + 1;
    }

    $s    = substr($seq, $start);
    $str .= $s;

    return $str;
}

function print_snps_errs($consensus, $sequence, $snps) {
    $str = "";
    $con = str_split($consensus);
    $seq = str_split($sequence);

    for ($i = 0; $i < count($con); $i++) {
	// Is a SNP defined in this column?
	if (isset($snps[$i])) {
	    if ($con[$i] == $seq[$i])
		$str .= "<span class=\"rank_1\">$seq[$i]</span>";
	    else
		$str .= "<span class=\"rank_2\">$seq[$i]</span>";

	} else {
	    // Does this nucleotide equal the consensus nucleotide at position $i?
	    if ($con[$i] == $seq[$i])
		$str .= $seq[$i];
	    else
		$str .= "<span class=\"err\">$seq[$i]</span>";
	}
    }

    return $str;
}

function generate_key_element_select($name, $elements, $selected_key, $javascript) {
    $script_code = "";

    if (strlen($javascript) > 0) {
        $script_code = " onchange=\"$javascript\"";
    }

    $ctl = "  <select id=\"$name\" name=\"$name\"" . $script_code . ">\n";

    foreach ($elements as $key => $element) {

        if ($key == $selected_key) 
            $ctl .= "  <option selected=\"selected\" value=\"$key\">$element</option>\n";
        else
            $ctl .= "  <option value=\"$key\">$element</option>\n";
    }

    $ctl .= "  </select>\n";

    return $ctl;
}

function generate_element_select($name, $elements, $selected_ele, $javascript) {
    $script_code = "";

    if (strlen($javascript) > 0) {
        $script_code = " onchange=\"$javascript\"";
    }

    $ctl = "  <select id=\"$name\" name=\"$name\"" . $script_code . ">\n";

    foreach ($elements as $element) {

        if ($element == $selected_ele) 
            $ctl .= "  <option selected=\"selected\">$element</option>\n";
        else
            $ctl .= "  <option>$element</option>\n";
    }

    $ctl .= "  </select>\n";

    return $ctl;
}

?>

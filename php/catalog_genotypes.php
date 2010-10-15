<?php
require_once("header.php");

if (isset($_GET['db']))
    $database  = $_GET['db'];
else if (isset($_POST['db']))
    $database = $_POST['db'];
else 
    $database = "radtags";

if (isset($_GET['tag_id']))
    $tag_id  = $_GET['tag_id'];
else if (isset($_POST['tag_id']))
    $tag_id = $_POST['tag_id'];
else 
    $tag_id = 0;

if (isset($_GET['batch_id']))
    $batch_id  = $_GET['batch_id'];
else if (isset($_POST['batch_id']))
    $batch_id = $_POST['batch_id'];
else 
    $batch_id = 0;

// Connect to the database
$db = db_connect($database);

// Save these variables for automatic URL formation later on.
$display = array();
$display['db']       = $database;
$display['tag_id']   = $tag_id;
$display['batch_id'] = $batch_id;

//
// Prepare the possible select lists we will want to construct
//
$marker_types = array('lmxll' => array('ll', 'lm', '--'),
                      'nnxnp' => array('nn', 'np', '--'),
                      'hkxhk' => array('hh', 'hk', 'kk', '--'),
                      'efxeg' => array('ee', 'ef', 'eg', 'fg', '--'),
                      'abxcd' => array('ac', 'ad', 'bc', 'bd', '--'));
$elements     = array('ll' => $marker_types['lmxll'],
                      'lm' => $marker_types['lmxll'],
                      'nn' => $marker_types['nnxnp'],
                      'np' => $marker_types['nnxnp'],
                      'hh' => $marker_types['hkxhk'],
                      'hk' => $marker_types['hkxhk'],
                      'kk' => $marker_types['hkxhk'],
                      'ee' => $marker_types['efxeg'],
                      'ef' => $marker_types['efxeg'],
                      'eg' => $marker_types['efxeg'],
                      'fg' => $marker_types['efxeg'],
                      'ac' => $marker_types['abxcd'],
                      'ad' => $marker_types['abxcd'],
                      'bc' => $marker_types['abxcd'],
                      'bd' => $marker_types['abxcd'],
                      '--' => array('--', 'll', 'lm', 'nn', 'np', 'hh', 'hk', 'kk', 'ee', 'ef', 'eg', 'fg', 'ab', 'ad', 'bc', 'bd'));

//
// Prepare some SQL queries
//
$query = 
    "SELECT count(samples.id) as count FROM samples WHERE batch_id=?";
$db['samp_sth'] = $db['dbh']->prepare($query);
check_db_error($db['samp_sth'], __FILE__, __LINE__);

$query = 
    "SELECT catalog_genotypes.sample_id, file, " . 
    "catalog_genotypes.genotype, genotype_corrections.genotype as corrected " . 
    "FROM catalog_genotypes " . 
    "LEFT JOIN genotype_corrections ON " . 
    "(genotype_corrections.catalog_id=catalog_genotypes.catalog_id AND " .
    "genotype_corrections.sample_id=catalog_genotypes.sample_id AND " .
    "genotype_corrections.batch_id=catalog_genotypes.batch_id) " .
    "JOIN samples ON (catalog_genotypes.sample_id=samples.id) " . 
    "WHERE catalog_genotypes.batch_id=? and catalog_genotypes.catalog_id=? " . 
    "ORDER BY catalog_genotypes.sample_id";
$db['geno_sth'] = $db['dbh']->prepare($query);
check_db_error($db['geno_sth'], __FILE__, __LINE__);

$page_title = "Catalog Genotype Viewer";
write_compact_header($page_title, $batch);

// 
// Get number of samples so we can determine how many rows to display 
// in the genotype table.
//
$result = $db['samp_sth']->execute($batch_id);
check_db_error($result, __FILE__, __LINE__);
$row = $result->fetchRow();
$num_samples = $row['count'];
$num_cols    = 10;
$num_rows    = ceil($num_samples / $num_cols);
$gtypes      = array();

$result = $db['geno_sth']->execute(array($batch_id, $tag_id));
check_db_error($result, __FILE__, __LINE__);

if ($result->numRows() == 0) {
    print 
        "<h4 style=\"margin-left: auto; margin-right: auto; text-align: center;\">" . 
        "This marker has no genotypes, probably because this tag does not have enough mappable progeny.</h4>\n";
    write_compact_footer();
    return;
}

while ($row = $result->fetchRow()) {
    $gtypes[$row['sample_id']] = array('file'      => $row['file'], 
                                       'genotype'  => $row['genotype'], 
                                       'corrected' => $row['corrected']);
}

print 
    "<form id=\"genotypes\" name=\"genotypes\" method=\"post\" action=\"$root_path/correct_genotypes.php\">\n" .
    "<input type=\"hidden\" name=\"op\" id=\"op\" value=\"\" />\n" .
    "<input type=\"hidden\" name=\"batch_id\" value=\"$batch_id\" />\n" .
    "<input type=\"hidden\" name=\"tag_id\" value=\"$tag_id\" />\n" .
    "<input type=\"hidden\" name=\"db\" value=\"$database\" />\n" .
    "<table class=\"genotypes\">\n" .
    "<tr>\n";
$i = 0;
foreach ($gtypes as $sample_id => $sample) {
    $i++;

    $id  = "gtype_" . $batch_id . "_" . $tag_id . "_" . $sample_id;

    if (strlen($sample['corrected']) > 0) {
        $a = isset($elements[strtolower($sample['corrected'])]) ? 
            $elements[strtolower($sample['corrected'])] : $elements["--"];
        $sel = generate_element_select($id, $a, strtolower($sample['corrected']), "");
        $genotype = "<span class=\"corrected\">$sample[corrected]</span>";
    } else {
        $a = isset($elements[strtolower($sample['genotype'])]) ? 
            $elements[strtolower($sample['genotype'])] : $elements["--"];
        $sel = generate_element_select($id, $elements[strtolower($sample['genotype'])], strtolower($sample['genotype']), "");
        $genotype = $sample['genotype'];
    }

    print 
        "  <td>" .
        "<span class=\"title\">" . ucfirst(str_replace("_", " ", $sample['file'])) . "</span><br />" .
        "<a onclick=\"toggle_sel('{$id}_div')\">" . $genotype . "</a>\n" . 
        "  <div id=\"{$id}_div\" style=\"display: none;\">\n" . 
        $sel . 
        "  </div>" .
        "</td>\n";

    if ($i % $num_cols == 0)
        print 
            "</tr>\n" .
            "<tr>\n";
}

while ($i % $num_cols != 0) {
    print "  <td></td>\n";
    $i++;
}

echo <<< EOQ
</tr>
<tr>
  <td colspan="$num_cols" style="text-align: right;">
    <input type="button" value="Reset" onclick="set_operation('genotypes', 'reset')" />
    <input type="button" value="Submit" onclick="set_operation('genotypes', 'correct')" />
  </td>
</tr>
</table>
</form>

EOQ;

write_compact_footer();

?>

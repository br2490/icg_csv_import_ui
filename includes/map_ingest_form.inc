<?php

// I am a terrible global variable user. - BR
$_icg_csv_import_ui_glob = array();

/**
 * CSV Import Form Callback
 *
 * @param $form
 * @param $form_state
 * @param $collection
 * @return mixed
 */
function icg_csv_import_ui_form($form, &$form_state, $collection) {
  // If this is the first time the user hits the form page
  if (!isset($form_state['values'])) {
    $form_state['step'] = 0;
    $form_state['storage'] = array();
  }

  // Build the form steps
  try {
    $step = $form_state['step'];
    $form = icg_csv_import_ui_get_form($step, $collection, $form_state);
  } catch (Exception $e) {
    drupal_set_message(t($e->getMessage()), 'error');
    return FALSE;
  }

  // Return the current page for the step
  return $form;
}

/**
 * Implements hook_form_submit().
 *
 * @param $form
 * @param $form_state
 */
function icg_csv_import_ui_form_submit($form, &$form_state) {
  module_load_include('inc', 'icg_csv_import_ui', 'includes/batch');

  // Save our current values to storage.
  $form_state['storage'] += $form_state['values'];

  switch ($form_state['step']):
    case 0:
      // Save our current values to storage.
      $form_state['storage'] += $form_state['values'];
      // Do not submit the form
      $form_state['rebuild'] = TRUE;
      // Instead go to the next page.
      $form_state['step']++;
      break;

    case 1:
      // This is our XML_FORM (objective form). Treat it nicely.
      dpm($form);
      // XML_FORM final form declaration. i.e. a defined mods form ready to be processed by XML_Forms.
      $form_state['storage']['icg_xml_form'] = $form;
      // XML_FORM final form values from the user - our pathway.
      $form_state['storage']['icg_xml_values'] = $form_state['values'];
      // Do not submit the form
      $form_state['rebuild'] = TRUE;
      // Instead go to the next page.
      $form_state['step']++;
      break;

    case 2:

//      // Fetch defaults
//      $defaults = _fetch_CSV_defaults();
//
//      // Open the constants file, if any
//      if (isset($form_state['storage']['constants_csv'])) {
//        $constants = $form_state['storage']['constants_csv']->uri;
//        $delimiter = ($form_state['storage']['csv_delimiter'] ? $form_state['storage']['csv_delimiter'] : "\t");
//        if ($file_handle = fopen($constants, 'r')) {
//          $headers = fgetcsv($file_handle, 0, $delimiter);  // Read first line to get xpath/header info
//          $data = fgetcsv($file_handle, 0, $delimiter);     // Read the next line to get data
//          for ($i = 0; $i < count($headers); $i++) {  // Save the data in $form_state['storage']['constants']
//            $form_state['storage']['constants'][$headers[$i]] = $data[$i];
//          }
//          fclose($file_handle);  // Close file connection
//        }
//      }
//
//      // Do something with storage and redirect away from the form!
//      icg_csv_import_ui_build_batch($form_state);
//      if (module_exists('devel')) {
//        dpm($form_state);
//      }
      break;

  endswitch;
}

/**
 * @param $form
 * @param $form_state
 * @return array
 */
function icg_csv_import_ui_form_ajax_callback($form, &$form_state) {
  $commands = array();
  $commands[] = ajax_command_replace('#xml-dropdown-update',
    drupal_render($form['xml_form']));
  $commands[] = ajax_command_replace('#namespace-dropdown-update',
    drupal_render($form['namespace']));
  return array('#type' => 'ajax', '#commands' => $commands);
}

/**
 * @param $step
 * @param $collection
 * @param $form_state
 *
 * @return array
 * @throws \Exception
 */
function icg_csv_import_ui_get_form($step, $collection, &$form_state) {
  $form = array();

  // Since this is used frequently in here, we include it universally for each
  // form part.
  module_load_include('inc', 'icg_csv_import_ui', 'includes/utilities');
  module_load_include('inc', 'xml_forms', 'builder/includes/associations.form');

  if (!isset($form_state['collection_policy'])) {
    // This will throw an error if no CPOLICY.
    $collection_info =
      _icg_csv_import_ui_get_available_ingest_models($collection);
    // Keep a copy in the form_state, we may need it later, else remove.
    $form_state['collection_policy'] = $collection_info;
    // Save parent PIDs.
    $form_state['storage']['parent_pid'] = $collection->id;
  }

  // Switch through the form $steps.
  switch ($step):

    case 0:    // The first form page... collect all control parameters.

      // This is unnecessary but it feels right.
      $content_models = $form_state['collection_policy']['models'];
      $namespaces = $form_state['collection_policy']['namespaces'];
      $xml_forms = $form_state['collection_policy']['forms'];

      $selected_cmodel = isset($form_state['values']['content_model']) ?
        $form_state['values']['content_model'] : key($content_models);

      $form['#validate'][] = 'icg_csv_import_ui_validate';

      $form['csv_file'] = array(
        '#name' => 'files[csv_file]',
        '#type' => 'file',
        '#title' => t('Select your CSV File.'),
        '#description' => t('Allowed file types are .csv, .tsv and .txt'),
        '#size' => 200,
        '#required' => TRUE,
        '#default_value' => TRUE,
      );

      $form['constants_csv'] = array(
        '#name' => 'files[constants_csv]',
        '#type' => 'file',
        '#title' => t('Select your constants CSV File.'),
        '#description' => t('Allowed file types are .csv, .tsv and .txt'),
        '#size' => 200,
        '#required' => FALSE,
        // '#default_value' => TRUE,
      );

      $form['options_fieldset'] = array(
        '#type' => 'fieldset',
        '#title' => t('Map CSV Fields to XPaths'),
        '#description' => t('If row 1 of the selected CSV file already contains a mapped list of MODS XPaths you can skip the other parameters in this section.'),
      );

      $form['options_fieldset']['skip_field_mapping'] = array(
        '#type' => 'checkbox',
        '#title' => t('CSV First Row Already Contains the Field Mapping.'),
        '#description' => t('Check this box if the first row of your CSV file already contains mapped XPaths.  Note that checking this option also carries the assumption that your object LABELs will be determined based on the /mods/titleInfo/title field.'),
        '#default_value' => FALSE,
        '#prefix' => "<br/><hr/>",
        '#suffix' => "<hr/>",
      );

      $form['options_fieldset']['content_model'] = array(
        '#type' => 'select',
        '#title' => t('Destination Content Model'),
        '#description' => t('Select an available content model to ingest into.'),
        '#options' => $content_models,
        '#default_value' => $selected_cmodel,
        '#required' => FALSE,
        '#ajax' => array(
          'callback' => 'icg_csv_import_ui_form_ajax_callback',
        ),
      );

      $form['options_fieldset']['xml_form'] = array(
        '#type' => 'select',
        '#title' => t('Select XML form to use...'),
        '#prefix' => '<div id="xml-dropdown-update">',
        '#suffix' => '</div>',
        '#description' => t('Lorem ipsum.'),
        '#required' => FALSE,
        '#options' => _icg_csv_ui_ajax_callback_xml_form($selected_cmodel, $xml_forms),
      );

      $form['namespace'] = array(
        '#type' => 'select',
        '#title' => t('Destination Namespace'),
        '#prefix' => '<div id="namespace-dropdown-update">',
        '#suffix' => '</div>',
        '#description' => t('Select an available destination namespace'),
        '#required' => TRUE,
        '#options' => _icg_csv_ui_ajax_callback_ns_form($selected_cmodel, $namespaces),
      );

      $form['csv_delimiter'] = array(
        '#type' => 'textfield',
        '#title' => t('CSV Field Delimiter'),
        '#description' => t('Enter the character that separates fields in your CSV file or leave blank to accept the default tab (\t) character.'),
        '#size' => 10,
        '#required' => FALSE,
        '#default_value' => '',
      );

      $form['ingest_as_inactive'] = array(
        '#type' => 'checkbox',
        '#title' => t('Ingest items as inactive?'),
        '#default_value' => TRUE,
      );

      $form['credentials_fieldset'] = array(
        '#type' => 'fieldset',
        '#title' => t('Provide Credentials for Content Download'),
        '#description' => t('These parameters are only necessary if your CSV file includes an OBJ column indicating where each content objet is stored. By default the download process is dictated by your hook_fetch_OBJ( ) function.  See icg_csv_import_ui.api.php for details.'),
      );

      $form['credentials_fieldset']['credentials_username'] = array(
        '#type' => 'textfield',
        '#title' => t('File Transfer Username'),
        '#description' => t('Enter the username required to execute content file transfers.'),
        '#size' => 20,
        '#required' => FALSE,
        '#default_value' => 'None',
      );

      $form['credentials_fieldset']['credentials_password'] = array(
        '#type' => 'password',
        '#title' => t('File Transfer Password'),
        '#description' => t('Enter the password corresponding to username (above) to execute content file transfers.'),
        '#size' => 20,
        '#required' => FALSE,
        '#default_value' => 'None',
      );

      $form['submit'] = array(
        '#type' => 'submit',
        '#value' => t('Next'),
      );

      break;

    case 1:     // Form page 2... Map CSV columns to metadata form elements.

      // No mapping necessary...let the user know to proceed.
      if ($form_state['storage']['skip_field_mapping']) {
        $form['message'] = array(
          '#markup' => "<p>You have indicated that metadata XPaths are already mapped in the first row of the CSV file.  Click 'Next' to proceed.</p>",
        );
        $form_state['step']++;   // skip the mapping AND review forms
        $form_state['storage'] += _fetch_CSV_defaults(); // THIS COMES FROM BATCH
        // Just leave the code early if this is true. Reducing nesting.
        break;
      }
      // Otherwise map CSV columns to metadata form elements.

      global $_icg_csv_import_ui_glob; // Shame, shame, shame. :D.

      //@TODO: RETHINK ALL OF THESE THINGS. SERIOUSLY. ALL.

      // A metadata form has been selected.  Fetch it's title and add it's data array to
      // $form_state['collection_policy']['selected']
      $selected_form = $form_state['storage']['xml_form'];
      $cmodel = $form_state['storage']['content_model'];
      $forms = $form_state['collection_policy']['forms'][$cmodel];

      // What is this doing? Why? All of this data is available by default.
      // @TODO: Is mfate using this data?
      foreach ($forms as $fdata) {
        if ($fdata['form_name'] === $selected_form) {
          $form_state['collection_policy']['selected'] = $fdata;
          $path = drupal_get_path('module', 'xml_form_builder');
          $form_state['storage']['transform'] = $path . '/transforms/' . $fdata['transform'];
          $label_field = '/mods'; // Why is this assumption being made?
          foreach ($fdata['title_field'] as $tfield) {
            $label_field .= '/' . $tfield;
          }
          $form_state['storage']['label_field'] = $label_field;
          break;
        }
      }

      // Load our CSV parser.
      module_load_include('inc', 'icg_csv_import_ui', 'includes/csv');

      // Set some variables we know we'll end up using.
      $file = $form_state['storage']['csv_file'];
      $delimiter = ($form_state['storage']['csv_delimiter'] ? $form_state['storage']['csv_delimiter'] : "\t");
      $form_name = $form_state['storage']['xml_form'];

      // Parse our CSV file.
      // @TODO: ask JDP if this is correct CSV.inc use.
      $csv_prepared = icg_csv_load_csv_data($file->uri, $delimiter);
      $csv_parsed = icg_csv_parse_csv_data($csv_prepared);

      // HOOK TO ADD MAPPABLE OPTIONS. <--- this.
      // Create an array of options for our form. Include an 'ignore' option.
      $mappable_options = array(
        NULL => 'Ignored Field',
        "{#title}_CONSTANT" => 'Use a CONSTANT value across all records.',
      );
      foreach ($csv_parsed[0] as $column_heading) {
        $mappable_options[$column_heading] = $column_heading;
        $form_state['storage']['csvColumns'][] = $column_heading;
      }
      $form_state['storage']['mappable_options'] = $mappable_options;

      // If this is our first run we must set the mappable options to our global.
      if (empty($_icg_csv_import_ui_glob['mappable_options'])) {
        $_icg_csv_import_ui_glob['mappable_options'] = $form_state['storage']['mappable_options'];
        $_icg_csv_import_ui_glob['manipulable_fields'] = array(
          'textfield',
          'textarea',
          'tags',
        );
      }

      // Just using what's already there.
      $form = xml_form_builder_get_form($form, $form_state, $form_name);

      $form['submit'] = array(
        '#type' => 'submit',
        '#value' => t('Next'),
      );

      break;

    case 2:   // Step 2 is complete...build the $step3 form from the step 2 input!

      $form = array();

      dpm($form_state);
      $icg_xml_form = $form_state['storage']['icg_xml_form'];

      // Wait, what?

      try {
        $xml_form = new XMLForm($form_state);
        $document = $xml_form->submit($icg_xml_form, $form_state);
        dpm($document->saveXML());
      } catch (Exception $e) {
        $message = 'File: ' . $e->getFile() . '</br>';
        $message .= 'Line: ' . $e->getLine() . '</br>';
        $message .= 'Error: ' . $e->getMessage() . '</br>';
        drupal_set_message(filter_xss($message), 'error');
        $form_state['rebuild'] = TRUE;
      }

      $form['submit'] = array(
        '#type' => 'submit',
        '#value' => t('Continue'),
      );

      return;

      // OLD CODE THAT IS NOT OFFICIALLY USED OR PART OF UI

      $values = $form_state['values'];
      _icg_csv_import_ui_convert_array_keys($values, '0', 'x');
      _icg_csv_import_ui_remove_numeric_keys($values);
      $merged = array_merge_recursive($form_state['storage']['xpaths'], $values);
      _icg_csv_import_ui_convert_array_keys($merged, '0', 'csv');
      _icg_csv_import_ui_remove_numeric_keys($merged);
      if (module_exists('devel')) {
        dpm($merged);
      }

      // Make a review form from $merged.
      $step3 = array();
      _icg_csv_import_ui_make_review_form($merged, $step3, $form_state);

      $step3['submit'] = array(
        '#type' => 'submit',
        '#value' => t('Submit')
      );

      $form = $step3;
      if (module_exists('devel')) {
        dpm($form);
      }

      // END OLD CODE

      break;

  endswitch;

  return $form;
}

// EVERYTHING BELOW THIS LINE IS INHERITED AND NOT YET USED IN UI, POSSIBLY DEPRECATED.

/**
 * @param $merged
 * @param $step3
 * @param $form_state
 * @param bool $parent
 */
function _icg_csv_import_ui_make_review_form($merged, &$step3, $form_state, $fieldset = FALSE) {
  static $fieldsets = array();
  static $csvColumns = array();

  // If $fieldset is FALSE, loop through all the first-level elements of $merged.
  // Build a fieldset form element for each.

  if (!$fieldset) {
    foreach ($merged as $key => $value) {
      if (isset($value['#hash'])) {
        $hash = $value['#hash'];
        $fieldsets[] = $hash;
        $step3[$hash] = array(
          '#title' => $key,
          '#type' => 'fieldset',
        );
      }
    }

    // Get the list of CSV column names from $form_state['storage']['csvColumns']
    $csvColumns = $form_state['storage']['csvColumns'];
  }

  // Loop through each element of $merged.  If the element has a '#hash' $value
  // which is in $fieldsets, use it.
  foreach ($merged as $key => $value) {
    if (isset($value['#hash']) && in_array($value['#hash'], $fieldsets)) {
      $fieldset = $value['#hash'];
    }

    if (isset($value['#originalType'])) {
      $original = $value['#originalType'];
    }

    // If the current element has both 'csv' and 'xpath' $value,
    // then it must represent an imported or constant field; display it.
    if (!empty($value['csv']) && isset($value['xpath'])) {
      $useConstant = ($value['csv'] == "USE_CONSTANT");
      $container = $key . '-container';

      $step3[$fieldset][$container] = array(
        '#prefix' => '',
        '#suffix' => '<br/>',
        '#type' => 'container',
        '#attributes' => array(
          'class' => array('container-inline'),
        ),
      );

      $csv = $value['csv'];
      $fromCSV = in_array($csv, $csvColumns);

      $step3[$fieldset][$container][$key] = array(
        '#type' => 'textfield',
        '#title' => "'$key' XPath for CSV column '$csv': ",
        '#prefix' => "&nbsp;&nbsp;&nbsp;&nbsp;",
        '#suffix' => "&nbsp;&nbsp;",
        '#size' => 40,
        '#required' => FALSE,
        '#default_value' => $value['xpath'],
      );

      if (!$fromCSV) {
        $step3[$fieldset][$container][$key]['#title'] = "'$key' XPath is: ";
      }

      // If this field uses a constant or if it was originally a select box...present a field box for that.
      if ($useConstant) {
        $step3[$fieldset][$container][$key . '-constant'] = array(
          '#type' => 'textfield',
          '#size' => 40,
          '#required' => FALSE,
          '#default_value' => 'Enter CONSTANT here.',
        );
      }
      else {
        if ($original === 'select') {  // An XML form select box.  Present it as though it were a constant.
          $step3[$fieldset][$container][$key . '-constant'] = array(
            '#type' => 'textfield',
            '#disabled' => TRUE,
            '#size' => 40,
            '#required' => FALSE,
            '#value' => $value['csv'],
          );
        }
      }
    }

    // If $value is an array, traverse it recursively to look for more 'csv' elements.
    if (is_array($value)) {
      _icg_csv_import_ui_make_review_form($value, $step3, $form_state, $fieldset);
    }
  }
}


/**
 * Form validation handler.
 *
 * @param $form
 * @param $form_state
 */
function icg_csv_import_ui_validate($form, &$form_state) {
  $validators = array('file_validate_extensions' => array('csv tsv txt'));

  // Check for a new uploaded CSV.
  $file = file_save_upload('csv_file', $validators);
  if (isset($file)) {
    // File upload was attempted.
    if ($file) {
      $form_state['values']['csv_file'] = $file;  // Put the temporary file in $form_values so we can save it on submit.
    }
    else {
      form_set_error('icg_csv_import_ui', t('The CSV data file could not be uploaded.'));  // File upload failed.
    }
  }

  // Check for a new uploaded constants CSV.
  $file = file_save_upload('constants_csv', $validators);
  if (isset($file)) {
    // File upload was attempted.
    if ($file) {
      $form_state['values']['constants_csv'] = $file;  // Put the temporary file in $form_values so we can save it on submit.
    }
    else {
      form_set_error('icg_csv_import_ui', t('The constants CSV file could not be uploaded.'));  // File upload failed.
    }
  }
}


<?php

/**
 * @file
 * Defines a Workflow field, widget and formatter. (copied from list field).
 */

/**
 * Implements hook_field_info().
 */
function workflowfield_field_info() {
  return array(
    'workflow' => array(
      'label' => t('Workflow'),
      'description' => t("This field stores Workflow values for a certain Workflow type from a list of allowed 'value => label' pairs, i.e. 'Publishing': 1 => unpublished, 2 => draft, 3 => published."),
      'settings' => array(
//          'allowed_values' => array(),
//          'allowed_values_function' => '',
          'allowed_values_function' => 'workflowfield_allowed_values',
          'wid' => '',
          'history' => TRUE,
          'schedule' => FALSE,
          'comment' => FALSE,
        ),
      'default_widget' => 'options_select',
      'default_formatter' => 'list_default',
    ),
  );
}

/**
 * Implements hook_form_FORM_ID_alter().
 *
 * Changes the hook_field_settings_form.
 * Fixes some Field settings and Field Instance settings, and makes sure users cannot change it.
 * @todo: perhaps this is core functionality, but these values are only saved 
 *        when the site builder explicitely save the instance settings. :-(
 */
function workflowfield_form_field_ui_field_edit_form_alter(&$form, &$form_state, $form_id) {
  if ($form['#field']['type'] == 'workflow') {
    // The Workflow field must have a value, so set to required.
    $form['instance']['required']['#type'] = 'hidden';
    $form['instance']['required']['#default_value'] = 1;
    $form['instance']['required']['#disabled'] = TRUE;

    // User may not set a default value, this is done by the Workflow module, @see workflow_field_choices().
    $form['instance']['default_value_widget']['#type'] = 'hidden';
    $form['instance']['default_value_widget']['#disabled'] = TRUE;

    // Make sure only 1 value can be entered in the Workflow field.
    $form['field']['cardinality']['#type'] = 'hidden';
    $form['field']['cardinality']['#default_value'] = 1;
    $form['field']['cardinality']['#disabled'] = TRUE;
  }
}

/**
 * Implements hook_field_settings_form().
 */
function workflowfield_field_settings_form($field, $instance, $has_data) {
  $settings = $field['settings'];
  $wid = $field['settings']['wid'];

  // Create list of all Workflow types. Include an initial empty value.
  $workflows = array();
  $workflows[''] = t('- Select a value -');
  foreach (workflow_get_workflows() as $workflow) {
    $workflows[$workflow->wid] = $workflow->name;
  }

  $form['allowed_values_function'] = array(
    '#type' => 'value',
    '#value' => $settings['allowed_values_function'], // = 'workflowfield_allowed_values',
  );

//  $form['allowed_values_function_display'] = array(
//    '#type' => 'item',
//    '#title' => t('Allowed values list'),
//    '#markup' => t('The value of this field is being determined by the %function function and may not be changed.', array('%function' => $settings['allowed_values_function'])),
//    '#access' => !empty($settings['allowed_values_function']),
//  );

  // Let the user choose between the available workflow types.
  $form['wid'] = array(
    '#type' => 'select',
    '#title' => t('Workflow type'),
    '#options' => $workflows,
    '#default_value' => $wid,
    '#required' => TRUE,
    '#disabled' => $has_data,
    '#description' => t('Choose the Workflow type.'),
  );

  // Inform the user of possible states. 
  // If no Workflow type is selected yet, do not show anything.
  if ($wid) {
    // Get a string representation to show all options.
    $allowed_values_string = _workflowfield_allowed_values_string($wid);

    $form['allowed_values_string'] = array(
      '#type' => 'textarea',
      '#title' => t('Allowed values for the selected Workflow type'),
      '#default_value' => $allowed_values_string,
      '#rows' => 10,
      '#field_has_data' => $has_data,
      '#field' => $field,
      '#field_type' => $field['type'],
//      '#access' => empty($settings['allowed_values_function']),
      '#access' => TRUE, // user can see the data, 
      '#disabled' => TRUE, //.. but cannot change them.
    );
  }

  $form['history'] = array(
    '#type' => 'checkbox',
    '#title' => t('Use the workflow history.'),
    '#required' => FALSE,
    '#default_value' => $settings['history'],
    '#description' => t('If checked, the state change is recorded in table {workflow_node_history}, ' .
      "and a tab 'Worklow' is shown on the node page, which gives access to the History of the workflow."),
  );
  $form['comment'] = array(
    '#type' => 'checkbox',
    '#title' => t('Allow adding a comment to workflow transitions.'),
    '#required' => FALSE,
    '#default_value' => $settings['comment'],
    '#description' => t('On the Workflow form, a Comment form can be included so that the person making the state change can record ' .
      'reasons for doing so. The comment is then included in the node\'s workflow history. ' . 
      'This may be hidden by settings in widgets, formatters or permissions.'),
  );
  $form['schedule'] = array(
    '#type' => 'checkbox',
    '#title' => t('Allow scheduling of workflow transitions.'),
    '#required' => FALSE,
    '#default_value' => $settings['schedule'],
    '#description' => t('Workflow transitions may be scheduled to a moment in the future. ' . 
      'Soon after the desired moment, the transition is executed by Cron. ' .
      'This may be hidden by settings in widgets, formatters or permissions.'),
  );

  return $form;
}

/**
 * We will be using some default formatters and widgets from the List and Options modules.
 */

/**
 * Implements hook_field_formatter_info_alter().
 *
 * The module reuses the formatters defined in list.module.
 */
function workflowfield_field_formatter_info_alter(&$info) {
  $info['list_default']['field types'][] = 'workflow';
}

/**
 * Implements hook_field_widget_info_alter().
 *
 * The module does not implement widgets of its own, but reuses the
 * widgets defined in options.module.
 *
 * @see workflowfield_options_list()
 */
function workflowfield_field_widget_info_alter(&$info) {
  $info['options_select']['field types'][] = 'workflow';
  $info['options_buttons']['field types'][] = 'workflow';
}

/**
 * Implements hook_field_widget_form_alter().
 *
 * Set the default value in the Options Widget, 
 * since it cannot be set in the Field Instnce settings, 
 * because this can be different per user.
 */
function workflowfield_field_widget_form_alter(&$element, &$form_state, $context) {
  if ($context['field']['type'] == 'workflow' && $element['#default_value'] == NULL) {
    $wid = $context['field']['settings']['wid'];
    $workflow = workflow_get_workflows_by_wid($wid);
    $sid = workflow_get_creation_state_by_wid($wid);
    $state = workflow_get_workflow_states_by_sid($sid);
    $node = (object) array('uid' => isset($context['form']['uid']['#value']) ? $context['form']['uid']['#value'] : '0');
    $choices = workflow_field_choices($node, $force = FALSE, $state);
    $element['#default_value'] = $sid = workflow_get_first_state($workflow, $choices);
  }
}

/**
 * Implements hook_form_BASE_FORM_ID_alter().
 *
 * Shows the form only on Comment forms.
 * Thanks to jrowny's sandbox: "Comment Node Edit" : https://drupal.org/sandbox/jrowny/1281972
 *
 */
function workflowfield_form_comment_form_alter(&$form, &$form_state, $form_id) {
  // Set the default value for the field that is referenced from the node.
  foreach ($form_state['field'] as $field_name =>$field_info) {
    if ($field_info['und']['field']['type'] == 'workflow' && isset($form[$field_name])) {
      if ($form['cid']['#value']) {
        // The comment is being changed. You may not change the Workflow field here.
        // @todo: change this form into a formatter.
        //dpm($form[$field_name]);
        drupal_set_message(" The Workflow field may not be changed anymore via comment", 'warning');
      }
      else {
        // Get the value from the same field of the referenced Entity.
        $entity_type = 'node'; // Comments only exist on nodes in D7.
        $entity = $form['#node'];
        if (isset($entity->{$field_name})) {
          $current_sid = _workflowfield_get_sid_by_items($entity->{$field_name}['und']);

          $form[$field_name]['und']['#default_value'] = $current_sid;

          // Add the submit and handlers, which updates the node when saving the comment.
          array_push($form['#submit'],"_workflowfield_form_comment_form_submit");
        }
      }
    }
  }
}

/**
 * Submits and saves added fields.
 */
function _workflowfield_form_comment_form_submit($form, &$form_state) {
  field_attach_submit('node', $form['#node'], $form, $form_state);
  node_save($node = $form['#node']);
}

/**
 * Do not implement hook_field_presave(), 
 * since $nid is needed, but not yet known at this moment.
 */
//function workflowfield_field_presave($entity_type, $entity, $field, $instance, $langcode, &$items) {
//}

/**
 * Implements hook_field_insert().
 */
function workflowfield_field_insert($entity_type, $entity, $field, $instance, $langcode, &$items) {
  return workflowfield_field_update($entity_type, $entity, $field, $instance, $langcode, $items);
}

/**
 * Implements hook_field_update().
 * It is called also from hook_field_insert, since we need $nid to store workflow_node_history.
 * We cannot use hook_field_presave, since $nid is not yet known at that moment.
 */
function workflowfield_field_update($entity_type, $entity, $field, $instance, $langcode, &$items) {
  $wid = $field['settings']['wid'];
  $workflow = workflow_get_workflows_by_wid($wid);
  $new_sid = _workflowfield_get_sid_by_items($items);

  $nid = isset($entity->nid) ? $entity->nid : 0;
  if (!$nid) {
    // on comment form: not possible: a comment on a non-existent node.
    // on node insert: should not happen, since we only do inserts when $nid is known.
    // on field settings page: cannot save default value (hidden from user) - no extra action needed.
  }
  else {
    if ($entity_type == 'comment') {
      // This happens when we are on an entity's comment.
      // todo: for now, if nid is set, then it is a node. What happens with other entities?
      $referenced_entity_type = 'node';
      $referenced_entities = entity_load($referenced_entity_type, array($nid));
      $referenced_entity = $referenced_entities[$nid];

      // Update the entity, who's state has changed.
      // The update of the node is done via _workflowfield_form_comment_form_submit().
//      // todo: support scheduling
//      _workflowfield_save_new_state($referenced_entity_type, $referenced_entity, $field, $instance, $langcode, $items);

//      // Execute the transition on the node.
//      workflow_transition($referenced_entity, $new_sid, FALSE, $entity_type, $field, $instance, $langcode, $items);
    }
    else {
//      // This happens on a 'normal' node edit page.
//      workflow_transition($referenced_entity, $new_sid, FALSE, $entity_type, $field, $instance, $langcode, $items);
    }
  }
}

/**
 * Implements hook_field_delete().
 *
 * @todo: implement
 */
//function workflowfield_field_delete($entity_type, $entity, $field, $instance, $langcode, &$items) {
//}

/**
 * Implements hook_field_delete_field().
 *
 * @todo: implement functionality from workflow_node_delete().
 */
//function workflowfield_field_delete_field($entity_type, $entity, $field, $instance, $langcode, &$items) {
//}

/**
 * Implements hook_field_is_empty().
 */
function workflowfield_field_is_empty($item, $field) {
  if (empty($item['value']) && (string) $item['value'] !== '0') {
    return TRUE;
  }
  return FALSE;
}

/**
 * Implements hook_options_list().
 * This is called from the default Options widgets.
 */
function workflowfield_options_list($field, $instance, $entity_type, $entity) {
  $field_name = $field['field_name'];
  $wid = $field['settings']['wid'];
  $workflow = workflow_get_workflows_by_wid($wid);
  $current_sid = FALSE;
  $options = array();

  $nid = isset($entity->nid) ? $entity->nid : 0;
  if ($nid && $entity_type == 'comment') {
    // This happens when we are on an entity's comment.
    // We need to fetch the field value of the original node, and show it on the comment. 

    // Parse the 'bundle' into 'comment'_<entity_type>_<entity_bundle>
    $entity_type = 'node'; // Comments only exist on nodes.
    $referenced_entities = entity_load($entity_type, array($nid));
    $entity = $referenced_entities[$nid];
    $items = field_get_items($entity_type, $entity, $field_name, $langcode = NULL);
    $current_sid = _workflowfield_get_sid_by_items($items);
    if (!$current_sid) {
      // E.g., the node was created before the field was added: do the same as 'Node Add' page.
      $current_sid = workflow_get_creation_state_by_wid($wid);
    }
  }
  elseif ($nid && $entity_type != 'comment') {
    // A 'normal' node edit page.
    $items = field_get_items($entity_type, $entity, $field_name, $langcode = NULL);
    $current_sid = _workflowfield_get_sid_by_items($items);
    if (!$current_sid) {
      // E.g., the node was created before the field was added: do the same as 'Node Add' page.
      $current_sid = workflow_get_creation_state_by_wid($wid);
    }
  }
  elseif (!$nid && $entity_type == 'comment') {
    // not possible: a comment on a non-existent node.
  }
  elseif (!$nid && $entity_type != 'comment') {
    if ($entity) {
      // A 'normal' node add page.
      $current_sid = workflow_get_creation_state_by_wid($wid);
    }
    else {
      // No entity available, we are on the field Settings page - 'default value' field.
      // This is hidden from the admin, because the default value can be different for every user.
    }
  }

  if ($current_sid) {
    $state = workflow_get_workflow_states_by_sid($current_sid);
    $options = workflow_field_choices($entity, FALSE, $state);
  }

  return $options;
}

/**
 * Returns the array of allowed values for a list field.
 * Used as a callback function in the list module.
 * @see list_allowed_values() : 
 * "The strings are not safe for output. Keys and values of the array should be
 * "sanitized through field_filter_xss() before being displayed.
 *
 * @param $field
 *   The field definition.
 * @param $instance
 *   (optional) A field instance array. Defaults to NULL.
 * @param $entity_type
 *   (optional) The type of entity; e.g. 'node' or 'user'. Defaults to NULL.
 * @param $entity
 *   (optional) The entity object. Defaults to NULL.
 *
 * @return
 *   The array of allowed values. Keys of the array are the raw stored values
 *   (number or text), values of the array are the display labels.
 *
 */
function workflowfield_allowed_values($field, $instance, $entity_type, $entity) {

  $options = workflowfield_options_list($field, $instance, $entity_type, $entity);
  return $options;

//  ? Is this function only used in the formatter, to determine the option's text? 
// @todo: decide if above function call is OK, oth that we should use below code: 
//  Pro: no double code
//  Contra: _options_list is more expensive, and shows only valid options for this state, 
//          where below code, just lists ALL values.

/*
  $allowed_values = &drupal_static(__FUNCTION__, array());
  $wid = $field['settings']['wid'];

  // todo: remove this cache, if already cached by workflow module.
  // or: move cache methode to workflow api.
  if (isset($allowed_values[$field['id']][$wid])) {
    $values = $allowed_values[$field['id']][$wid];
  }
  else {
    // If $cacheable is FALSE, then the allowed values are not statically
    // cached. See list_test_dynamic_values_callback() for an example of
    // generating dynamic and uncached values.
    $cacheable = FALSE;

    $values = array();
    $states = workflow_get_workflow_states_by_wid($wid);
    foreach ($states as $data) {
      if ($wid == $data->wid) { 
        $values[$data->sid] = check_plain(t($data->state));
      }
    }
    if ($cacheable) {
      $allowed_values[$field['id']][$wid] = $values;
    }
  }
*/

// @todo: remove: These are the testdata from list_test.
//        This is listed here because it hints to the use of 'Phases', 
//        just like Commerce module uses State ('Phase') vs. Status (Workflow State).
//  $values = array(
//    'Group 1' => array(
//      0 => 'Zero',
//    ),
//    2 => 'One',
//    'Group 2' => array(
//      2 => 'Some <script>dangerous</script> & unescaped <strong>markup</strong>',
//    ),
//  );

  return $values;
}

/**
 * Generates a string representation of an array of 'allowed values'.
 *
 * This string format is suitable for edition in a textarea.
 *
 * @param $values
 *   An array of values, where array keys are values and array values are
 *   labels.
 *
 * @return
 *   The string representation of the $values array:
 *    - Values are separated by a carriage return.
 *    - Each value is in the format "value|label" or "value".
 */
function _workflowfield_allowed_values_string($wid = 0) {
  $lines = array();
  $states = $wid ? workflow_get_workflow_states_by_wid($wid) : workflow_get_workflow_states();
  $last_wid = -1;

  foreach ($states as $state) {
    // Only show enabled states.
    if ($state->status) {
      if (($wid == 0) && ($last_wid <> $state->wid)) {
        // Show a Workflow name between Workflows, if more then 1 in the list.
        $last_wid = $state->wid;
        $lines[] = $state->name . "'s states: ";
      }
      $states[$state->sid] = check_plain(t($state->state));
      $lines[] = $state->sid . ' | ' . check_plain(t($state->state));
    }
  }
  return implode("\n", $lines);
}

/*
 * Helper functions
 */

/**
 * Get a single value from an $items array.
 *   D7: This is a wrapper for field_get_items(). 
 *   D8: as of Drupal 8.0. Use $entity->getTranslation($langcode)->{$field_name}
 * But since only 1 item exist, we use our own method.
 *
 * @param $items
 *   Array with values, as passed in the hook_field_<op> functions.
 *   Although we are parsing an array, 
 *   the Workflow Field settings ensure that the cardinality is set to 1.
 * 
 * @return $sid
 *   A State ID.
 */
function _workflowfield_get_sid_by_items($items) {
  $sid = isset($items[0]['value']) ? $items[0]['value'] : 0;
  return $sid;
}

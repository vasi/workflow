<?php
/**
 * @file
 * Provide user interface for changing workflow state.
 */

define('MARK_STATE_IS_DELETED', '*');

/**
 * Menu callback. Display workflow summary of a node.
 */
function workflow_tab_page($entity_type, $entity = NULL) {
  drupal_set_title(entity_label($entity_type, $entity));

  // $figure out the $entity's bundle and id.
  list($entity_id, , $entity_bundle) = entity_extract_ids($entity_type, $entity);
  $workflow = workflow_get_workflows_by_type($entity_bundle, $entity_type);

  // Show the current state and the Workflow form to allow state changing.
  // (But only show form if we are not at the terminal state.)
  // Choose a different form, depending if this is a Workflow Node or a Workflow Field.
  // Probably this can be done in hook_forms(), but this is better to maintain.
  if ($workflow_item = $workflow->getWorkflowItem()) {
    // This is a Workflow Field workflow.
    $field = $workflow_item->getField();
    $instance = $workflow_item->getInstance();
    $langcode = NULL;
    
    $field_name = $field['field_name'];
    $current_sid = workflow_node_current_state($entity, $entity_type, $field_name);
    $items[0]['value'] = $current_sid;
    $display = array();

    $form = workflowfield_field_formatter_view($entity_type, $entity, $field, $instance, $langcode, $items, $display);
    $output = drupal_render($form);
  }
  else {
    // This is a Workflow Node workflow.
    $field_name = '';
    $current_sid = workflow_node_current_state($entity, $entity_type, $field_name);
    $current_state = WorkflowState::load($current_sid);

    $output = theme('workflow_current_state', array('state' => $current_state->label(),
                                                    'state_system_name' => $current_state->getName(),
                                                    'sid' => $current_state->sid));

    // Emulate a Field API interface.
    $field = array();
    $field['field_name'] = '';
    $field['settings']['wid'] = $workflow->wid;
    $field['settings']['widget'] = $workflow->options;
    $field['settings']['widget']['options'] = 'radios';
    $field['settings']['widget']['comment'] = $workflow->options['comment_log_tab']; // vs. ['comment_log_node']; 
    $field['settings']['widget']['current_status'] = TRUE;

    $instance = array();
    $instance['widget']['setting'] = $workflow->options;
//    $form = drupal_get_form('workflow_tab_form', $entity, $workflow, $states, $current_sid);
    $form = drupal_get_form('workflow_transition_form', $field, $instance, $entity_type, $entity);
    $output .= drupal_render($form);
  }

  // Show the history table.
  $rows = array();
  $current_themed = FALSE;
  $field_name = isset($field['field_name']) ? $field['field_name'] : '';
  $current_sid = workflow_node_current_state($entity, $entity_type, $field_name);
  $limit = variable_get('workflow_states_per_page', 20);
  foreach (WorkflowTransition::load($entity_type, $entity_id, $field_name, $limit) as $history) {
    $old_state_name = $new_state_name = '';

    $label = $name = '';
    $state = WorkflowState::load($history->new_sid);
    if ($state) {
      $name = $state->getName();
      $label = $state->label();
    }

    if (!$state) {
      // This is an invalid/deleted state.
      $old_state_name = check_plain($label);
    }
    elseif ($history->new_sid == $current_sid && $state->isActive() && !$current_themed) {
      // Theme the current state differently so it stands out.
      $new_state_name = theme('workflow_history_current_state', array('state_name' => $label,
                                                                      'state_system_name' => $name,
                                                                      'sid' => $history->new_sid));
      // Make a note that we have themed the current state; other times in the history
      // of this node where the node was in this state do not need to be specially themed.
      $current_themed = TRUE;
    }
    elseif (!$state->isActive()) {
      // The state has been deleted, but we include it in the history.
      $new_state_name = theme('workflow_deleted_state', array('state_name' => $label,
                                                              'state_system_name' => $name,
                                                              'sid' => $history->new_sid));
      $footer_needed = TRUE;
    }
    else {
      // Regular state.
      $new_state_name = check_plain($label);
    }

    $label = $name = MARK_STATE_IS_DELETED;
    $state = WorkflowState::load($history->old_sid);
    if ($state) {
      $name = $state->getName();
      $label = $state->label();
    }

    if (!$state) {
      // This is an invalid/deleted state.
      $old_state_name = check_plain($label);
    }
    elseif (!$state->isActive()) {
      $old_state_name = theme('workflow_deleted_state', array('state_name' => $label,
                                                              'state_system_name' => $name,
                                                              'sid' => $history->old_sid));
      $footer_needed = TRUE;
    }
    else {
      // Regular state.
      $old_state_name = check_plain($label);
    }

    $variables = array(
      'transition' => $history, // @todo D8: pass this WorkflowTransition as only variable. It contains everything.
      'extra' => '',

      'history' => $history,  // @todo D8: remove, as this is the same as 'transition'.
      'old_sid' => $history->old_sid, // @todo D8: remove this redundant property.
      'sid' => $history->new_sid, // @todo D8: remove this redundant property.
      'uid' => $history->uid, // @todo D8: remove this redundant property.
      'old_state_name' => $old_state_name,
      'state_name' => $new_state_name,
    );

    // Allow other modules to modify the row.
    // $todo D8: pass only a $transition object.
    drupal_alter('workflow_history', $variables);

    $rows[] = theme('workflow_history_table_row', $variables);
  }

  // Mark the first and last rows.
  $rows[0]['class'][] = 'first';
  $last = count($rows) - 1;
  $rows[$last]['class'][] = 'last';

  $header = array(t('Date'), t('Old State'), t('New State'), t('By'), t('Comment'));
  $header[] = array('data' => t('Operations'));

  // Only display the table if there's anything in it.
  if ($rows) {
    $output .= theme('workflow_history_table', array('header' => $header, 'rows' => $rows, 'footer' => !empty($footer_needed)));
    $output .= theme('pager', array('tags' => $limit));
  }
  return $output;
}

/*
 * Theme one WorkflowTansition in a workflow history table row.
 *
 * $old_state_name and $state_name must be run through check_plain(t()) prior
 * to calling this theme function.
 */
function theme_workflow_history_table_row($variables) {
  $row = array();
  $old_state_name = $variables['old_state_name'];
  $state_name = $variables['state_name'];
  $transition = $variables['transition'];
  $account = user_load($transition->uid);
  $row = array(
    'data' => array(
      array('data' => format_date($transition->stamp), 'class' => array('timestamp')),
      array('data' => $old_state_name, 'class' => array('previous-state-name')),
      array('data' => $state_name, 'class' => array('state-name')),
      array('data' => theme('username', array('account' => $account)), 'class' => array('user-name')),
      array('data' => filter_xss($transition->comment), 'class' => array('log-comment')),
      $variables['extra'],
    ),
  'class' => array('workflow_history_row'),
  );

  return $row;
}

/*
 * Theme entire workflow history table.
 */
function theme_workflow_history_table($variables) {
  $header = $variables['header'];
  $rows = $variables['rows'];
  $footer = $variables['footer'];
  $extra_column = 5;

  // Remove the Operations column if none are added.
  $empty = TRUE;
  foreach ($rows as $row) {
    $empty &= empty($row['data'][$extra_column]);
  }
  if ($empty) {
    foreach ($rows as &$row) {
      unset($row['data'][$extra_column]);
      unset($header[$extra_column]);
    }
  }

  $output = theme('table', array('header' => $header, 'rows' => $rows, 'caption' => t('Workflow History')));
  if ($footer) {
    $output .= MARK_STATE_IS_DELETED . ' ' . t('State is no longer available.');
  }
  return $output;
}

/**
 * Theme the current state in the workflow history table.
 */
function theme_workflow_history_current_state($variables) {
  return check_plain(t($variables['state_name']));
}

/**
 * Theme a deleted state in the workflow history table.
 */
function theme_workflow_deleted_state($variables) {
  return check_plain(t($variables['state_name'])) . MARK_STATE_IS_DELETED;
}

/**
 * Form builder. Allow workflow state change and scheduling from workflow tab.
 * N.B. This function is only used for Node API, not Field API.
 *
 * @param $node
 *   Node for which workflow information will be displayed.
 * @param $workflow
 *   Workflow object to display.
 * @param $states
 *   Array of states for the workflow.
 * @param $current
 *   Current workflow state of this node.
 * @return
 *   Form definition array.
 */
function workflow_tab_form($form, $form_state, $node, $workflow, $states, $current_sid) {
  // N.B. This function is only used for Node API, not Field API.
  // Let's make sure we should be here.
  if (workflow_tab_access('node', $node) === FALSE) {
    return;
  }

  // @todo: remove this in a future version.
  // This is to support workflow-extensions 7.x-1.0, which still passes a $wid.
  if (is_numeric($workflow)) {
    $workflow = Workflow::load($wid = $workflow);
  }

  // This function is only used for Node API, not Field API, hence only 'node'.
  $current_state = WorkflowState::load($current_sid);
  $options = $current_state->getOptions($entity_type = 'node', $node, $force = FALSE);

  // Only build form if user has possible target state(s).
  if ($current_state->showWidget($options)) {
    // Tell FAPI where this form is.
    form_load_include($form_state, 'inc', 'workflow', 'workflow.pages');

    $form['#tab'] = TRUE;
    $label = $workflow->label();
    $form['#wf'] = $workflow;
    $form['#choices'] = $options;
    $form['#node'] = $node; // Added for integration with Field widget.

    $timestamp = NULL;
    $comment = NULL;
    // See if scheduling information is present.
    if (!empty($node->workflow_scheduled_timestamp) && !empty($node->workflow_scheduled_sid)) {
      global $user;
      if (variable_get('configurable_timezones', 1) && $user->uid && drupal_strlen($user->timezone)) {
        $timezone = $user->timezone;
      }
      else {
        $timezone = variable_get('date_default_timezone', 0);
      }
      // The default value should be the upcoming sid.
      $current_sid = $node->workflow_scheduled_sid;
      $timestamp = $node->workflow_scheduled_timestamp;
      $comment = $node->workflow_scheduled_comment;
    }

    // Include the same form elements here that are included on a
    // regular node editing page. $form is modified by reference.
    workflow_node_form($form, $form_state, t('Change !name state', array('!name' => $label)), $label, $current_sid, $options, $timestamp, $comment);
    $form['node'] = array(
      '#type' => 'value',
      '#value' => $node,
      );

    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Update workflow'),
      );
  }
  return $form;
}

/**
 * Submit handler for the form on the workflow tab.
 *
 * @see workflow_tab_form()
 */
function workflow_tab_form_submit($form, &$form_state) {
  // The entire node object was stashed in the form.
  $node = $form_state['values']['node'];

  if (isset($form_state['values']['workflow'])) {
    $node->workflow = $form_state['values']['workflow'];
    $node->workflow_comment = isset($form_state['values']['workflow_comment']) ?
      $form_state['values']['workflow_comment'] : '';

    if (!empty($form_state['values']['workflow_scheduled'])) {
      $node->workflow_scheduled = $form_state['values']['workflow_scheduled'];
    }
    if (!empty($form_state['values']['workflow_scheduled_date'])) {
      $node->workflow_scheduled_date = $form_state['values']['workflow_scheduled_date'];
    }
    if (!empty($form_state['values']['workflow_scheduled_hour'])) {
      $node->workflow_scheduled_hour = $form_state['values']['workflow_scheduled_hour'];
    }
    if (!empty($form_state['values']['workflow_scheduled_timezone'])) {
      $node->workflow_scheduled_timezone = $form_state['values']['workflow_scheduled_timezone'];
    }
  }
  // ALERT: Rules that use node_save to check the node transition are going to be missed if
  // the tab form is used to check for the change. It is *always* better practice to use
  // the transition change itself as your value to check for changes with Rules and other
  // behaviors. Do NOT rely on node_save() to drive transition changes.
  workflow_transition($node, $node->workflow);
}

<?php
/**
 * @file
 * Provide user interface for changing workflow state.
 */

define('MARK_STATE_IS_DELETED', '*');

/**
 * Menu callback. Display workflow summary of a node.
 */
function workflow_tab_page($node = NULL) {
  drupal_set_title($node->title);

  $entity = $node;
  $entity_type = 'node'; // @todo: add support for other entity types.
  $entity_bundle = $node->type;
  $workflow = workflow_get_workflows_by_type($entity_bundle, $entity_type);
  $states = $workflow->getStates();

  // Show the current state and the Workflow form to allow state changing.
  // (But only show form if we are not at the terminal state.)
  // Choose a different form, depending if this is a Workflow Node or a Workflow Field.
  // Probably this can be done in hook_forms(), but this is better to maintain.
  if ($workflow_item = $workflow->getWorkflowItem()) {
    // This is a Workflow Field workflow.
    $current_sid = workflow_node_current_state($entity, $entity_type, $workflow_item->getField());
    $field = $workflow_item->getField();
    $instance = $workflow_item->getInstance();
    $langcode = NULL;
    $items[0]['value'] = $current_sid;
    $display = array();
    $form = workflowfield_field_formatter_view($entity_type, $entity, $field, $instance, $langcode, $items, $display);
    $output = drupal_render($form);
  }
  else {
    // This is a Workflow Node workflow.
    $current_sid = workflow_node_current_state($entity, $entity_type, $field = array());
    $name = $states[$current_sid]->getName();
    $label = $states[$current_sid]->label();
    $output = theme('workflow_current_state', array('state' => $label,
                                                    'state_system_name' => $name,
                                                    'sid' => $current_sid));
    $form = drupal_get_form('workflow_tab_form', $node, $workflow, $states, $current_sid);
    $output .= drupal_render($form);
  }

  // Show the history table.
  $rows = array();
  $current_themed = FALSE;
  foreach (workflow_get_workflow_node_history_by_nid($node->nid) as $history) {
    $old_state_name = $new_state_name = '';
    $label = $name = '';
    $history_state = isset($states[$history->sid]) ? $states[$history->sid] : 0;
    if ($history_state != NULL) {
      $name = $history_state->getName();
      $label = $history_state->label();
    }

    if (!$history_state) {
      // This is an invalid/deleted state.
      $old_state_name = check_plain($label);
    }
    elseif ($history->sid == $current_sid && $history_state->isActive() && !$current_themed) {
      // Theme the current state differently so it stands out.
      $new_state_name = theme('workflow_history_current_state',  array('state_name' => $label,
                                                                   'state_system_name' => $name,
                                                                   'sid' => $history->sid));
      // Make a note that we have themed the current state; other times in the history
      // of this node where the node was in this state do not need to be specially themed.
      $current_themed = TRUE;
    }
    elseif (!$history_state->isActive()) {
      // The state has been deleted, but we include it in the history.
      $new_state_name = theme('workflow_deleted_state', array('state_name' => $label,
                                                          'state_system_name' => $name,
                                                          'sid' => $history->sid));
      $footer_needed = TRUE;
    }
    else {
      // Regular state.
      $new_state_name = check_plain($label);
    }

    $label = $name = MARK_STATE_IS_DELETED;
    $history_state = $states[$history->old_sid];
    if ($history_state != NULL) {
      $name = $history_state->getName();
      $label = $history_state->label();
    }

    if (!$history_state) {
      // This is an invalid/deleted state.
      $old_state_name = check_plain($label);
    }
    elseif (!$history_state->isActive()) {
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
        'history' => $history,  // @todo D8: remove, as this is the same as 'transition'.
        'old_sid' => $history->old_sid, // @todo D8: remove this redundant property.
        'sid' => $history->sid, // @todo D8: remove this redundant property.
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

  // Only display the table if there's anything in it.
  if ($rows) {
    $output .= theme('workflow_history_table', array('rows' => $rows, 'footer' => !empty($footer_needed)));
    $output .= theme('pager', array('tags' => variable_get('workflow_states_per_page', 20)));
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
      ),
    'class' => array('workflow_history_row'),
    );

  if (!empty($variables['extra'])) {
    $row['data'][] = $variables['extra'];
  }

  return $row;
}

/*
 * Theme entire workflow history table.
 */
function theme_workflow_history_table($variables) {
  $rows = $variables['rows'];
  $footer = $variables['footer'];
  $headers = array(t('Date'), t('Old State'), t('New State'), t('By'), t('Comment'));
  $output = theme('table', array('header' => $headers, 'rows' => $rows, 'caption' => t('Workflow History')));
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
  // Let's make sure we should be here.
  if (workflow_tab_access($node) === FALSE) {
    return;
  }

  // @todo: remove this in a future version.
  // This is to support workflow-extensions 7.x-1.0, which still passes a $wid.
  if (is_numeric($workflow)) {
    $workflow = Workflow::load($wid = $workflow);
  }

  // This function is only used for Node API, not Field API, hence only 'node'.
  $state = WorkflowState::load($current_sid);
  $options = $state->getOptions($entity_type = 'node', $node, $force = FALSE);

  // Only build form if user has possible target state(s).
  if (workflow_show_form($current_sid, $workflow, $options)) {
    // Tell FAPI where this form is.
    form_load_include($form_state, 'inc', 'workflow', 'workflow.pages');

    $form['#tab'] = TRUE;
    $name = t($workflow->name);
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
    workflow_node_form($form, $form_state, t('Change !name state', array('!name' => $name)), $name, $current_sid, $options, $timestamp, $comment);

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

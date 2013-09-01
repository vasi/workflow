<?php
/**
 * @file
 * Node specific functions, remnants of nodeapi.
 */

/**
 * Theme the current workflow state.
 */
function theme_workflow_current_state($variables) {
  $state = $variables['state'];
  return '<div class="workflow-current-state '
    . 'workflow-current-sid-' . intval($variables['sid']) . ' '
    . drupal_html_class($state)
    . '">'
    . t('Current state: <span class="state">@state</span>', array('@state' => $state))
    . '</div>';
}

/**
 * Implements hook_node_load().
 * @TODO: Consider replacing with hook_entity_load().
 */
function workflow_node_load($nodes, $types) {
  // Get which types have workflows associated with them.
  $workflow_types = array_filter(workflow_get_workflow_type_map());

  foreach ($nodes as $node) {
    // If it's not a workflow type, quit immediately.
    if (!array_key_exists($node->type, $workflow_types)) {
      continue;
    }

    // ALERT: With the upgrade to Drupal 7, values stored on the node as _workflow_x
    // have been standardized to workflow_x, dropping the initial underscore.
    // Module maintainers integrating with workflow should keep that in mind.
    $last_history = workflow_get_recent_node_history($node->nid);

    if ($workflow = workflow_get_workflows_by_type($node->type)) {
      $node->workflow_wid = $workflow->wid;
      $node->workflow = $workflow->creation_state;
    }

    // Nodes that existed before the workflow don't have any history.
    if ($last_history === FALSE) {
      $node->workflow_stamp = $node->created;
      continue;
    }
    else {
      $node->workflow = $last_history->sid;
      $node->workflow_stamp = $last_history->stamp;
      $node->workflow_state_name = $last_history->state_name;
    }

    // Add scheduling information.
    // Technically you could have more than one scheduled, but this will only add the soonest one.
    foreach (workflow_get_workflow_scheduled_transition_by_nid($node->nid) as $row) {
      $node->workflow_scheduled_sid = $row->sid;
      $node->workflow_scheduled_timestamp = $row->scheduled;
      $node->workflow_scheduled_comment = $row->comment;
      break;
    }
  }
}

/**
 * Implements hook_node_insert().
 * 
 * This is executed after saving data to the database.
 * We cannot use hook_node_presave, because workflow_execute_transition() needs the nid.  
 */
function workflow_node_insert($node) {
  // Skip if there are no workflows.
  if ($workflow = workflow_get_workflows_by_type($node->type)) {
    // Normally, the new state is already set.
    // If the state is not specified, use first valid state.
    // For example, a new node must move from (creation) to some
    // initial state.
    if (empty($node->workflow)) {
      $choices = workflow_field_choices($node);
      $sid = workflow_get_first_state($workflow, $choices);
    }
    else {
      $sid = $node->workflow;
    }
    if ($sid) {
      workflow_transition($node, $sid);
    }
  }
}

/**
 * Implements hook_node_update().
 */
function workflow_node_update($node) {
  // Skip if there are no workflows.
  if ($workflow = workflow_get_workflows_by_type($node->type)) {
    // Get new state from workflow form, stored in $node->workflow.
    if (!empty($node->workflow)) {
      $sid = $node->workflow;
    }
    if ($sid) {
      workflow_transition($node, $sid);
    }
  }
}

/**
 * Implements hook_node_delete().
 */
function workflow_node_delete($node) {
  $node->workflow_stamp = REQUEST_TIME;
  // Delete the association of node to state.
  workflow_delete_workflow_node_by_nid($node->nid);
  if (!empty($node->workflow)) {
    global $user;
    $data = array(
      'nid' => $node->nid,
      'old_sid' => $node->workflow,
      'sid' => WORKFLOW_DELETION,
      'uid' => $user->uid,
      'stamp' => $node->workflow_stamp,
      'comment' => t('Node deleted'),
    );
    workflow_insert_workflow_node_history($data);
  }
  // Delete any scheduled transitions for this node.
  workflow_delete_workflow_scheduled_transition_by_nid($node->nid);
}

/**
 * Implements hook_comment_insert().
 */
function workflow_comment_insert($comment) {
  workflow_comment_update($comment);
}

/**
 * Implements hook_comment_update().
 */
function workflow_comment_update($comment) {
  if (isset($comment->workflow)) {
    $node = node_load($comment->nid);
    $sid = $comment->workflow;
    $node->workflow_comment = $comment->workflow_comment;
    if (isset($comment->workflow_scheduled)) {
      $node->workflow_scheduled = $comment->workflow_scheduled;
    }
    if (isset($comment->workflow_scheduled_date)) {
      $node->workflow_scheduled_date = $comment->workflow_scheduled_date;
    }
    if (isset($comment->workflow_scheduled_hour)) {
      $node->workflow_scheduled_hour = $comment->workflow_scheduled_hour;
    }
    workflow_transition($node, $sid);
  }
}

/**
 * Implements hook_node_view().
 *
 * @param object $node.
 * @param string $view_mode.
 * @param string $langcode.
 *
 * @return renderable content in $node->content array.
 */
function workflow_node_view($node, $view_mode, $langcode) {
  if (!user_access('show workflow state form') || $node->status == 0) {
    return;
  }

  if (!$workflow = workflow_get_workflows_by_type($node->type)) {
    return;
  }
  $current = workflow_node_current_state($node);
  $choices = workflow_field_choices($node);

  $states = array();
  $state_system_names = array();
  foreach (workflow_get_workflow_states() as $data) {
    $states[$data->sid] = check_plain(t($data->state));
    $state_system_names[$data->sid] = check_plain($data->state);
  }

  // Show current state at the top of the node display.
  $markup = theme('workflow_current_state', array('state' => $states[$current], 'state_system_name' => $state_system_names[$current], 'sid' => $current));

  $node->content['workflow_current_state'] = array(
    '#markup' => $markup,
    '#weight' => -99,
    );

  if (workflow_show_form($current, $workflow, $choices)) {
    // Show state change form at the bottom of the node display.
    module_load_include('inc', 'workflow', 'workflow.pages');

    // By including the nid in the form id.
    $form = drupal_get_form("workflow_tab_form_$node->nid", $node, $workflow, $states, $current);
    $form['#weight'] = 99;

    $node->content['workflow'] = $form;
  }
}

/**
 * Implements hook_field_extra_fields().
 */
function workflow_field_extra_fields() {
  $extra = array();

  // Get all workflows by content types.
  $types = array_filter(workflow_get_workflow_type_map());

  // Add the extra fields to each content type that has a workflow.
  foreach ($types as $type => $wid) {
    $extra['node'][$type] = array(
      'form' => array(
        'workflow' => array(
          'label' => t('Workflow'),
          'description' => t('Workflow module form'),
          'weight' => 99,    // Default to bottom.
          ),
        ),
      'display' => array(
        'workflow_current_state' => array(
          'label' => t('Workflow: Current State'),
          'description' => t('Current workflow state'),
          'weight' => -99,   // Default to top.
          ),
        'workflow' => array(
          'label' => t('Workflow: State Change Form'),
          'description' => t('The form for controlling workflow state changes.'),
          'weight' => 99,    // Default to bottom.
          ),
        ),
      );
  }

  return $extra;
}

/**
 * Implements hook_form_BASE_FORM_ID_alter().
 *
 * Shows the form only on Node Edit forms.
 *
 */
function workflow_form_node_form_alter(&$form, &$form_state, $form_id) {
  return _workflow_form_alter($form, $form_state, $form_id);
}

/**
 * Implements hook_form_BASE_FORM_ID_alter().
 *
 * Shows the form only on Comment forms.
 *
 */
function workflow_form_comment_form_alter(&$form, &$form_state, $form_id) {
  return _workflow_form_alter($form, $form_state, $form_id);
}

/**
 * Used to Implement hook_form_alter().
 * Is now a subfunction of workflow_form_BASE_FORM_ID_alter().
 * This is more performant, since it is called only on form with correct BASE_FORM_ID.
 *
 * @param object &$node
 * @return array
 *
 * @see http://bryanbraun.com/2013/08/17/using-hook-form-base-form-id-alter
 */
function _workflow_form_alter(&$form, &$form_state, $form_id) {
  // Ignore all forms except comment forms and node editing forms.
  if ((isset($form['#node']) && $form_id == 'comment_node_' . $form['#node']->type . '_form')
    || (isset($form['#node']->type) && isset($form['#node']) && $form['#node']->type . '_node_form' == $form_id)) {
    // Set node to #node if available or load from nid value.
    $node = isset($form['#node']) ? $form['#node'] : node_load($form['nid']['#value']);
    $type = $node->type;

    if ($workflow = workflow_get_workflows_by_type($node->type)) {
      $workflow_entities = variable_get('workflow_' . $type, array());
      // Abort if the entity type of the form is not in the list that the user
      // wants to display the workflow form on.
      if (!in_array($form['#entity_type'], $workflow_entities)) {
        return;
      }

      $choices = workflow_field_choices($node);
      // If this is a preview, the current state should come from
      // the form values, not the node, as the user may have changed
      // the state.
      $current = isset($form_state['values']['workflow']) ?
        $form_state['values']['workflow'] : workflow_node_current_state($node);

      // If the current node state is not one of the choices, pick first valid choice.
      // We know all states in $choices are states that user has permission to
      // go to because workflow_field_choices() has already checked that.
      if (!isset($choices[$current])) {
        $current = workflow_get_first_state($workflow, $choices);
      }

      // Stop if user has no new target state(s) to choose.
      if (!workflow_show_form($current, $workflow, $choices)) {
        return;
      }

      $name = t($workflow->name);

      $form['#wf'] = $workflow;
      $form['workflow'] = array(
        '#type' => 'fieldset',
        '#title' => $name,
        '#collapsible' => TRUE,
        '#collapsed' => FALSE,
        '#weight' => 10,
      );

      $timestamp = NULL;
      $comment = NULL;
      // See if scheduling information is present.
      if (isset($node->workflow_scheduled_timestamp) && isset($node->workflow_scheduled_sid)) {
        // The default value should be the upcoming sid.
        $current = $node->workflow_scheduled_sid;
        $timestamp = $node->workflow_scheduled_timestamp;
        $comment = $node->workflow_scheduled_comment;
      }
      if (isset($form_state['values']['workflow_comment'])) {
        $comment = $form_state['values']['workflow_comment'];
      }

     // Include the 'workflow form'. $form is modified by reference.
      workflow_node_form($form, $form_state, t('Change !name state', array('!name' => $name)), $name, $current, $choices, $timestamp, $comment);
    }
  }
}

/**
 * Wrapper function for workflow_tab_access after 
 * renaming workflow_node_tab_access() to workflow_tab_access(),
 * since *_tab_access() is a Workflow functionality, not Node API.
 * 
 * @todo: deprecated. Remove if it is not called anymore.
 */
function workflow_node_tab_access($node = NULL) {
  return workflow_tab_access($node);
}

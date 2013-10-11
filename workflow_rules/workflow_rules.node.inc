<?php
/**
 * @file
 * Rules integration for the Workflow module with Node API
 */

/**
 * Implements subfunction of hook_rules_condition_info().
 */
function _workflow_rules_rules_node_condition_info() {
  return array(
    'workflow_check_transition' => array(
      'group' => t('Workflow'),
      'label' => t('Content makes a specific transition'),
      'parameter' => array(
        'node' => array(
          'type' => 'node',
          'label' => t('Node'),
          'description' => t('The node whose workflow state is being checked.'),
        ),
        'old_state' => array(
          'type' => 'list<integer>',
          'label' => t('Old workflow state'),
          'options list' => '_workflow_rules_workflow_get_options',
          'description' => t('The workflow state moved from.'),
        ),
        'new_state' => array(
          'type' => 'list<integer>',
          'label' => t('New workflow state'),
          'options list' => '_workflow_rules_workflow_get_options',
          'description' => t('The workflow state moved to.'),
        ),
      ),
      'base' => '_workflow_rules_workflow_check_transition',
      'callbacks' => array(
        'execute' => '_workflow_rules_workflow_check_transition',
      ),
    ),
    'workflow_check_state' => array(
      'group' => t('Workflow'),
      'label' => t('Content has a workflow state'),
      'parameter' => array(
        'node' => array(
          'type' => 'node',
          'label' => t('Node'),
          'description' => t('The node to compare the current workflow state of.'),
        ),
        'workflow_state' => array(
          'type' => 'list<integer>',
          'label' => t('Compare workflow state'),
          'options list' => '_workflow_rules_workflow_get_options',
          'description' => t('The possible workflow states to compare against.'),
        ),
      ),
      'base' => '_workflow_rules_workflow_check_state',
      'callbacks' => array(
        'execute' => '_workflow_rules_workflow_check_state',
      ),
    ),
    'workflow_check_previous_state' => array(
      'group' => t('Workflow'),
      'label' => t('Content has a previous workflow state'),
      'parameter' => array(
        'node' => array(
          'type' => 'node',
          'label' => t('Node'),
          'description' => t('The node to compare the previous workflow state of.'),
        ),
        'workflow_state' => array(
          'type' => 'list<integer>',
          'label' => t('Compare workflow state'),
          'options list' => '_workflow_rules_workflow_get_options',
          'description' => t('The possible workflow states to compare against.'),
        ),
      ),
      'base' => '_workflow_rules_workflow_check_previous_state',
      'callbacks' => array(
        'execute' => '_workflow_rules_workflow_check_previous_state',
      ),
    ),
  );
}

/**
 * Implements subfunction of hook_rules_action_info().
 */
function _workflow_rules_rules_node_action_info() {
  return array(
    'workflow_rules_set_state' => array(
      'group' => t('Workflow'),
      'label' => t('Set workflow state for content'),
      'parameter' => array(
        'node' => array(
          'type' => 'node',
          'label' => t('Node'),
          'description' => t('The node to set the current workflow state of.'),
//          'save' => TRUE,
        ),
        'workflow_state' => array(
          'type' => 'list<integer>',
          'label' => t('New workflow state'),
          'options list' => '_workflow_rules_workflow_get_options',
          'description' => t('The workflow state to set (select only one).'),
        ),
        'workflow_comment' => array(
          'type' => 'text',
          'label' => t('Workflow Comment'),
          'description' => t('The workflow comment to set.'),
          'optional' => TRUE,
        ),
      ),
    ),
  );
}

/**
 * Condition implementation: check state transition.
 * 
 * @param $node : the node with the new values. Other entity types are not supported.
 * @param : array $old_sids : an array of old sids to meet the condition.
 * @param : array $new_sids : an array of new sids to meet the condition.
 * @param : array $condition: a RulesCondition->settings array.
 */
function _workflow_rules_workflow_check_transition($node, array $old_sids, array $new_sids, array $condition) {
  $old_sid = workflow_node_previous_state($node);
  $new_sid = workflow_node_current_state($node);

  return _workflow_rules_workflow_check_given_state($old_sid, $old_sids) &&
         _workflow_rules_workflow_check_given_state($new_sid, $new_sids);
}

/**
 * Condition implementation: check current state.
 */
function _workflow_rules_workflow_check_state($node, $sids) {
  $sid = workflow_node_current_state($node);
  return _workflow_rules_workflow_check_given_state($sid, $sids);
}

/**
 * Condition implementation: check previous state.
 */
function _workflow_rules_workflow_check_previous_state($node, $sids) {
  $sid = workflow_node_previous_state($node);
  return _workflow_rules_workflow_check_given_state($sid, $sids);
}

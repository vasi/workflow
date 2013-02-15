<?php

/**
 * @file
 * Hooks provided by the workflow module.
 */

/**
 * Implements hook_workflow().
 *
 * @param $op
 *   The current workflow operation: 'transition pre' or 'transition post'.
 * @param $old_state
 *   The state ID of the current state.
 * @param $new_state
 *   The state ID of the new state.
 * @param $node
 *   The node whose workflow state is changing.
 */
function hook_workflow($op, $old_state, $new_state, $node) {
  switch ($op) {
    case 'transition pre':
      // The workflow module does nothing during this operation.
      // But your module's Implements the workflow hook could
      // return FALSE here and veto the transition.
      break;

    case 'transition post':
      break;

    case 'transition delete':
      break;
  }
}

/**
 * Implements hook_workflow_operations().
 *
 * @param $op
 *   'operations': Allow modules to insert their own workflow operations.
 *   'state':  Allow modules to insert state operations.
 * @param $workflow
 *   The current workflow object.
 * @param $state
 *   The current state object.
 */
function hook_workflow($op, object $workflow, object $state) {
  switch ($op) {
    case 'operations':
      $actions = array();
      // The workflow_admin_ui module creates links to add a new state,
      // edit the workflow, and delete the workflow.
      // Your module may add to these actions.
      return $actions;

    case 'state':
      $ops = array();
      // The workflow_admin_ui module creates links to edit a state
      // and delete the state.
      // Your module may add to these operations.
      return $ops;
  }
}

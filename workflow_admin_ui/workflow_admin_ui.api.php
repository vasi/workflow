<?php
/**
 * @file
 * Hooks provided by the workflow_admin_ui module.
 */

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

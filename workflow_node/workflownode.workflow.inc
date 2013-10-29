<?php

/**
 * @file
 * Hooks provided by the workflow module.
 */

/**
 * Implements hook_workflow().
 */
function workflownode_workflow($op, $id, $new_sid, $entity, $force, $entity_type = '', $field_name = '') {
  switch ($op) {
    case 'transition permitted':
      // The workflow module does nothing during this operation.
      // This operation occurs when the list of available transitions
      // is built. Your module's implementation could return FALSE
      // here and disallow the presentation of the choice.
      break;

    case 'transition pre':
      // The workflow module does nothing during this operation.
      // But your module's implementation of the workflow hook could
      // return FALSE here and veto the transition.
      break;

    case 'transition post':
      break;

    case 'transition delete':
      // A transition is deleted Only the first parameter is used.
      $tid = $id;
      break;

    case 'state delete':
      $current_sid = $id;
      // Re-parent any nodes that we don't want to orphan, whilst deactivating a State.
      // This is called in WorkflowState::deactivate().
      if ($new_sid) {
        global $user;
        // A candidate for the batch API.
        // @TODO: Future updates should seriously consider setting this with batch.
        $node = new stdClass();
        $node->workflow_stamp = REQUEST_TIME;
        foreach (workflow_get_workflow_node_by_sid($current_sid) as $data) {
          $node->nid = $data->nid;
          $node->workflow = $current_sid;
          $data = array(
            'nid' => $node->nid,
            'sid' => $new_sid,
            'uid' => $user->uid,
            'stamp' => $node->workflow_stamp,
          );
          workflow_update_workflow_node($data, $current_sid, t('Previous state deleted'));
        }
      }

      // Delete any lingering node to state values.
      workflow_delete_workflow_node_by_sid($current_sid);

      break;

    case 'workflow delete':
      // Only the first parameter is used.
      $wid = $id;
      workflow_delete_workflow_type_map_by_wid($wid);
      break;
  }
}

/**
 * Implements hook_workflow_history_alter().
 */
// function workflownode_workflow_history_alter(array &$variables) {
  // The Workflow module does nothing with this hook.
  // For an example implementation, see the Workflow Revert add-on.
// }

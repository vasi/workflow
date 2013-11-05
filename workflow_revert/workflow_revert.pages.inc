<?php
/**
 * @file
 * Adds an 'Revert' link to the first workflow history row.
 */

/**
 * Menu callback to do the revert function.
 */
function workflow_revert_form($form, $form_state, $nid = NULL, $sid = NULL) {

  if (drupal_valid_token($_GET['token'], 'workflow_revert ' . $sid)) {
    $state = WorkflowState::load($sid);

    $entity_type = 'node';
    $node = node_load($nid);

    $args['#sid'] = $sid;
    $args['#nid'] = $nid;
    $args['#node'] = $node;
    $question = t('Are you sure you want to revert %title to the "@state" state?', array(
      '@state' => $state->label(),
      '%title' => $node->title,
      )
    );
    return confirm_form($args,
      $question,
      'node/$nid',
      t('The workflow state will be changed.')
    );
  }
  else {
    watchdog('workflow_revert', 'Invalid token', array(), WATCHDOG_ERROR);
    drupal_set_message(t('Invalid token. Your information has been recorded.'), 'error');
    drupal_goto('node/$nid');
  }
}

function workflow_revert_form_submit($form, $form_state) {
  $sid = $form['#sid'];
  $nid = $form['#nid'];
  $node = $form['#node'];
  $comment = t('State reverted.');
  
  // If Rules is available, signal the reversion.
  if (module_exists('rules')) {
    rules_invoke_event('workflow_state_reverted', $node);
  }

  // Force the transition because it's probably not valid.
  //@todo: use WorkflowTransition->execute() in workflow_revert.
  workflow_execute_transition($node, $sid, $comment, TRUE);
  drupal_set_message($comment);

  drupal_goto('node/' . $form['#nid'] . '/workflow');
}

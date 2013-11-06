<?php
/**
 * @file
 * Contains 'workflow_revert' page.
 */

/**
 * Menu callback to do the revert function.
 *
 * @todo: you can revert to an inactive state and to creation state.
 * @todo: add support for Field in workflow_revert.
 */
function workflow_revert_form($form, $form_state, $nid = NULL, $sid = NULL) {

  if (drupal_valid_token($_GET['token'], 'workflow_revert ' . $sid)) {
    $state = WorkflowState::load($sid);

    $entity_type = 'node'; //@todo: adapt workflow_revert_form() for Entity.
    $node = entity_load_single($entity_type, $nid);

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
  global $user;

  $new_sid = $form['#sid'];
  $nid = $form['#nid'];
  $entity = $form['#node'];
  $entity_type = 'node';
  $field_name = '';
  $comment = t('State reverted.');
  
  // If Rules is available, signal the reversion.
  if (module_exists('rules')) {
    rules_invoke_event('workflow_state_reverted', $entity);
  }

  $current_sid = workflow_node_current_state($entity, $entity_type, $field_name);
  $transition = new WorkflowTransition($entity_type, $entity, $field_name, $current_sid, $new_sid, $user->uid, REQUEST_TIME, $comment);

  // Force the transition because it's probably not valid.
  workflow_execute_transition($entity_type, $entity, $field_name, $transition, $force = TRUE);

  drupal_set_message($comment);

  drupal_goto('node/' . $form['#nid'] . '/workflow');
}

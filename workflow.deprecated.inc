<?php
/**
 * @file
 * Contains contains per-class functions, that are deprecated.
 * Usage: The new code can be tested, by removing this file-include from workflow.module. 
 */

/**
 * Deprecated functions related to table workflow_scheduled_transition.
 * These are replaced by methods of class WorkflowScheduledTransition.
 */

/**
 * Given a node, get all scheduled transitions for it.
 * @deprecated: workflow_get_workflow_scheduled_transition_by_nid() --> WorkflowScheduledTransition::load()
 */
function workflow_get_workflow_scheduled_transition_by_nid($nid) {
  return WorkflowScheduledTransition::load($nid);
}

/**
 * Given a timeframe, get all scheduled transitions.
 * @deprecated: workflow_get_workflow_scheduled_transition_by_between() --> WorkflowScheduledTransition::loadBetween()
 */
function workflow_get_workflow_scheduled_transition_by_between($start = 0, $end = REQUEST_TIME) {
  $results = db_query('SELECT nid, old_sid, sid, uid, scheduled, comment FROM {workflow_scheduled_transition} WHERE scheduled > :start AND scheduled < :end ORDER BY scheduled ASC', array(':start' => $start, ':end' => $end));
  return $results->fetchAll();
}

/**
 * Insert a new scheduled transition.
 * Only one transition at a time (for now).
 * @deprecated: workflow_insert_workflow_scheduled_transition() --> WorkflowScheduledTransition::save()
 */
function workflow_insert_workflow_scheduled_transition($data) {
  $data = (object) $data;
  workflow_delete_workflow_scheduled_transition_by_nid($data->nid);
  drupal_write_record('workflow_scheduled_transition', $data);
}

/**
 * Given a node, delete transitions for it.
 * @deprecated: workflow_delete_workflow_scheduled_transition_by_nid() --> WorkflowScheduledTransition::delete()
 *        It is still used in workflow_execute_transition().
 */
function workflow_delete_workflow_scheduled_transition_by_nid($nid) {
  return WorkflowScheduledTransition::deleteByNid($nid);
}

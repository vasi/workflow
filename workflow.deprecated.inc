<?php
/**
 * @file
 * Contains contains per-class functions, that are deprecated.
 * Usage: The new code can be tested, by removing this file-include from workflow.module. 
 */

/**
 * Deprecated functions related to table {workflows}.
 * These are replaced by methods of class Workflow.
 */

/**
 * Menu wild card loader: %workflow.
 * May be used by add on modules, such as workflow_admin_ui.
 * @deprecated: workflow_load($wid) --> Workflow::load($wid)
 */
function workflow_load($wid) {
  return Workflow::getWorkflow($wid);
}

/**
 * Get all workflows.
 * @deprecated: workflow_get_workflows() --> Workflow::getWorkflows($wid)
 */
function workflow_get_workflows() {
  return Workflow::getWorkflows();
}

/**
 * Get a specific workflow, wid is a unique ID.
 * @deprecated: workflow_get_workflows_by_wid() --> Workflow::getWorkflow($wid)
 */
function workflow_get_workflows_by_wid($wid, $reset = FALSE) {
  return Workflow::getWorkflow($wid, $reset);
}

/**
 * Get a specific workflow, name is a unique ID.
 * @deprecated: workflow_get_workflows_by_name() --> Workflow::getWorkflowByName($name)
 */
function workflow_get_workflows_by_name($name, $unserialize_options = FALSE) {
  return Workflow::getWorkflowByName($name, $unserialize_options);
}

/**
 * Return the ID of the creation state for this workflow.
 * @deprecated: workflow_get_creation_state_by_wid($wid) --> $workflow->getCreationSid().
 *
 * @param $wid
 *   The ID of the workflow.
 */
function workflow_get_creation_state_by_wid($wid) {
  $sid = 0;
  if ($workflow = Workflow::load($wid)) {
    $sid = $workflow->getCreationSid();
  }
  return $sid;
}

/**
 * Return the ID of the creation state given a content type.
 *
 * @param $type
 *   The type of the content.
 */
function workflow_get_creation_state_by_type($type) {
  $sid = FALSE;
  if ($workflow = workflow_get_workflows_by_type($type)) {
    $sid = $workflow->getCreationSid();
  }
  return $sid;
}

/**
 * Given a wid, delete the workflow and its data.
 *
 * @deprecated: workflow_delete_workflows_by_wid() --> Workflow::delete().
 */
function workflow_delete_workflows_by_wid($wid) {
  $workflow = Workflow::getWorkflow($wid);
  $workflow->delete();
}

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

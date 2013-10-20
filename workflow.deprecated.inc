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
 * Get all workflows.
 * @deprecated: workflow_get_workflows() --> Workflow::getWorkflows($wid)
 */
function workflow_get_workflows() {
  return Workflow::getWorkflows();
}

/**
 * Get a specific workflow, wid is a unique ID.
 * @deprecated: workflow_get_workflows_by_wid() --> Workflow::load($wid)
 */
function workflow_get_workflows_by_wid($wid, $reset = FALSE) {
  return Workflow::load($wid, $reset);
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
  $workflow = Workflow::load($wid);
  $workflow->delete();
}

/**
 * Deprecated functions related to table {workflow_states}.
 * These are replaced by methods of class WorkflowState.
 */

/**
 * Get all active states in the system.
 *
 * @return
 *  An array $id => $name, of all active states.
 * @deprecated: workflow_get_workflow_states_all() --> Workflow::getOptions()
 */
function workflow_get_workflow_states_all() {
  $options = array();

  // Get all states, only where active.
  foreach (Workflow::getWorkflows() as $workflow) {
    $options += $workflow->getOptions($grouped = FALSE);
  }

  return $options;
}

/**
 * Get all states in the system by content type.
 */
function workflow_get_workflow_states_by_type($type) {
  $query = "SELECT ws.sid, ws.wid, ws.state, ws.weight, ws.sysid "
    . "FROM {workflow_type_map} wtm "
    . "INNER JOIN {workflow_states} ws ON ws.wid = wtm.wid "
    . "WHERE wtm.type = :type AND ws.status = 1 "
    . "ORDER BY ws.weight, ws.sid "
  ;
  $query_array = array(':type' => $type);
  $results = db_query($query, $query_array);
  return $results->fetchAll();
}

/**
 * Get all states in the system, with options to filter, only where a workflow exists.
 *
 * @deprecated: workflow_get_workflow_states() --> WorkflowState::getStates()
 * @deprecated: workflow_get_workflow_states_by_wid() --> WorkflowState::getStates()
 */
function workflow_get_workflow_states($options = array()) {
  // Build the basic query.
  $query = db_select('workflow_states', 'ws');
  $query->leftJoin('workflows', 'w', 'w.wid = ws.wid');
  $query->fields('ws');
  $query->addField('w', 'wid');
  $query->addField('w', 'name');

  // Spin through the options and add conditions.
  foreach ($options as $column => $value) {
    $query->condition('ws.' . $column, $value);
  }

  // Set the sorting order.
  $query->orderBy('ws.wid');
  $query->orderBy('ws.weight');

  // Just for grins, add a tag that might result in modifications.
  $query->addTag('workflow_states');

  // Give them the answer.
  return $query->execute()->fetchAllAssoc('sid');
}

/**
 * Get all states in the system, with options to filter, only where a workflow exists.
 *
 * @deprecated: workflow_get_workflow_states_by_wid() --> Workflow->getOptions()
 */
function workflow_get_workflow_states_by_wid($wid, $options = array()) {
  $options['wid'] = $wid;
  return workflow_get_workflow_states($options);
}

/*
 * @deprecated: workflow_get_workflow_by_sid --> WorkflowState::load()
 */
function workflow_get_workflow_by_sid($sid) {
  return db_query("SELECT w.wid, w.name, w.tab_roles, w.options FROM {workflow_states} s
    INNER JOIN {workflows} w ON w.wid=s.wid WHERE sid = :sid ",
    array(':sid' => $sid))->fetchObject();
}

/**
 * Given a sid, return a state. Sids are a unique id.
 *
 * @deprecated: workflow_get_workflow_states_by_sid($sid) --> WorkflowState::load($sid)
 */
function workflow_get_workflow_states_by_sid($sid, $options = array()) {
  static $sids = array();
  if (!isset($sids[$sid])) {
    $states = workflow_get_workflow_states(array('sid' => $sid));
    $sids[$sid] = reset($states);
  }
  return $sids[$sid];
}

/**
 * Given a sid, return all other states in that workflow.
 *
 * @deprecated: replaced by WorkflowState::getStates($sid)
 */
function workflow_get_other_states_by_sid($sid) {
  $query = "SELECT sid, state "
    . "FROM {workflow_states} "
    . "WHERE wid = (SELECT wid FROM {workflow_states} WHERE sid = :sid AND status = 1 AND sysid = 0) "
    ;
  return db_query($query, array(':sid' => $sid))->fetchAllKeyed();
}

/**
 * Given a wid and state, return a state. Wids / states are a unique id.
 */
function workflow_get_workflow_states_by_wid_state($wid, $state) {
  $options = array(
    'state' => $state,
    'wid' => $wid,
    );
  return workflow_get_workflow_states($options);
}

/**
 * Given a sid, delete the state and all associated data.
 * @deprecated: workflow_delete_workflow_states_by_sid($sid, $new_sid, $true_delete) --> WorkflowState->delete()
 */
function workflow_delete_workflow_states_by_sid($sid, $new_sid = FALSE, $true_delete = FALSE) {
  if ($state = WorkflowState::load($sid)) {
    $state->delete($new_sid, $true_delete);
  }
}

/**
 * Save (update/insert) a Workflow State into table {workflow_states}.
 * @deprecated: workflow_update_workflow_states() --> WorkflowState->save()
 */
function workflow_update_workflow_states(&$data) {
  $data = (object) $data;
  if (!isset($data->sysid)) {
    $data->sysid = 0;
  }
  if (!isset($data->status)) {
    $data->status = 1;
  }
  if (isset($data->sid) && WorkflowState::load($data->sid)) {
    drupal_write_record('workflow_states', $data, 'sid');
  }
  else {
    drupal_write_record('workflow_states', $data);
  }
}

/**
 * Deprecated functions related to table {workflow_scheduled_transition}.
 * These are replaced by methods of class WorkflowScheduledTransition.
 */

/**
 * Given a node, get all scheduled transitions for it.
 * @deprecated: workflow_get_workflow_scheduled_transition_by_nid() --> WorkflowScheduledTransition::load()
 */
function workflow_get_workflow_scheduled_transition_by_nid($nid) {
  return WorkflowScheduledTransition::load('node', $nid);
}

/**
 * Given a timeframe, get all scheduled transitions.
 * @deprecated: workflow_get_workflow_scheduled_transition_by_between() --> WorkflowScheduledTransition::loadBetween()
 */
function workflow_get_workflow_scheduled_transition_by_between($start = 0, $end = REQUEST_TIME) {
  return WorkflowScheduledTransition::loadBetween($start, $end);
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

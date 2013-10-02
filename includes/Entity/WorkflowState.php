<?php

/**
 * @file
 * Contains workflow\includes\Entity\WorkflowState.
 */

class WorkflowState {
  // Since workflows do not change, it is implemented as a singleton.
  private static $states = array();

  public $sid = 0;
  public $wid = 0;
  private $weight = 0; 
  private $sysid = 0; 
  private $state; // @todo: rename to 'label'.
  public $status;
  private $workflow = NULL; 

  public function __construct($sid = 0, $wid = 0) {
    if (!$sid) {
      // automatic constructor when casting an array or object.
      if (!isset(self::$states[$this->sid])) {
        self::$states[$this->sid] = $this;
      }
    }
    else {
      if (!isset(self::$states[$sid])) {
        self::$states[$sid] = WorkflowState::getState($sid, $wid);
      }
      // @todo: this copy-thing should not be necessary.
      $this->sid = self::$states[$sid]->sid;
      $this->wid = self::$states[$sid]->wid;
      $this->weight = self::$states[$sid]->weight; 
      $this->sysid = self::$states[$sid]->sysid;
      $this->state = self::$states[$sid]->state;
      $this->status = self::$states[$sid]->status;
      $this->workflow = self::$states[$sid]->workflow;
    }
  }

  /**
   * Alternative constructor, via a static function.
   */
  public static function getState($sid, $wid = 0) {
    $states = self::getStates($sid, $wid);
    return $states[$sid];
  }

  /**
   * Get all states in the system, with options to filter, only where a workflow exists.
   * @deprecated workflow_get_workflow_states() --> WorkflowState->getStates()
   * @deprecated workflow_get_workflow_states_all() --> WorkflowState->getStates()
   * @deprecated workflow_get_other_states_by_sid($sid) --> WorkflowState->getStates($sid)
   */
  public static function getStates($sid = 0, $wid = 0, $reset = FALSE) {
    if ($reset) {
      self::$states = array();
    }

    if ($sid && isset(self::$states[$sid])) {
      // Only 1 is requested and cached: return this one.
      return array($sid => self::$states[$sid]);
    }

    // Build the query.
    $query = db_select('workflow_states', 'ws');
    $query->fields('ws');

    if ($wid) {
      $query->condition('ws.wid', $wid);
    }

    // Set the sorting order.
    $query->orderBy('ws.wid');
    $query->orderBy('ws.weight');

    // Just for grins, add a tag that might result in modifications.
    $query->addTag('workflow_states');

    // return array of objects, even if only 1 is requested.
    // note: self::states[] is populated in respective constructors.
    if ($sid) {
      // return 1 object.
      $query->condition('ws.sid', $sid);
      $query->execute()->fetchAll(PDO::FETCH_CLASS, 'WorkflowState');
      return array($sid => self::$states[$sid]);
    }
    else {
      $query->execute()->fetchAll(PDO::FETCH_CLASS, 'WorkflowState');
      return self::$states;
    }
  }

  /*
   * Returns the Workflow object of this State.
   */
  function getWorkflow() {
    if (!isset($this->workflow)) {
      $this->workflow = new Workflow($this->wid);
    }
    return $this->workflow;
  }

  function isActive() {
    return (bool) $this->status;
  }

  /*
   * Returns the allowed values for the current state.
   */
  function getOptions($node, $force = FALSE) {
    return workflow_field_choices($node, $force, $this);
  }

  /*
   * Mimics Entity API functions.
   *
   */
  function label($langcode = NULL) {
    return t($this->state, $args = array(), $options = array('langcode' => $langcode));
  }
  function getName() {
    return $this->state;
  }
  function value() {
    return $this->sid;
  }

  /**
   * Given a sid, delete the state and all associated data.
   * @deprecated: workflow_delete_workflow_states_by_sid($sid, $new_sid, $true_delete) --> WorkflowState->delete()
   */
  function delete($new_sid = FALSE, $true_delete = FALSE) {
    $sid = $this->sid;
    // Notify interested modules. We notify first to allow access to data before we zap it.
    module_invoke_all('workflow', 'state delete', $sid, NULL, NULL, FALSE);

    // Re-parent any nodes that we don't want to orphan.
    if ($new_sid) {
      global $user;
      // A candidate for the batch API.
      // @TODO: Future updates should seriously consider setting this with batch.
      $node = new stdClass();
      $node->workflow_stamp = REQUEST_TIME;
      foreach (workflow_get_workflow_node_by_sid($sid) as $data) {
        $node->nid = $data->nid;
        $node->workflow = $sid;
        $data = array(
          'nid' => $node->nid,
          'sid' => $new_sid,
          'uid' => $user->uid,
          'stamp' => $node->workflow_stamp,
        );
        workflow_update_workflow_node($data, $sid, t('Previous state deleted'));
      }
    }

    // Find out which transitions this state is involved in.
    $preexisting = array();
    foreach (workflow_get_workflow_transitions_by_sid_involved($sid) as $data) {
      $preexisting[$data->sid][$data->target_sid] = TRUE;
    }

    // Delete the transitions.
    foreach ($preexisting as $from => $array) {
      foreach (array_keys($array) as $target_id) {
        if ($transition = workflow_get_workflow_transitions_by_sid_target_sid($from, $target_id)) {
          workflow_delete_workflow_transitions_by_tid($transition->tid);
        }
      }
    }

    // Delete any lingering node to state values.
    workflow_delete_workflow_node_by_sid($sid);

    // Delete the state. -- We don't actually delete, just deactivate.
    // This is a matter up for some debate, to delete or not to delete, since this
    // causes name conflicts for states. In the meantime, we just stick with what we know.
    if ($true_delete) {
      db_delete('workflow_states')->condition('sid', $sid)->execute();
    }
    else {
      db_update('workflow_states')->fields(array('status' => 0))->condition('sid', $sid, '=')->execute();
    }

    // Clear the cache.
    self::$states = array();
  }

  /**
   * Given data, update or insert into workflow_states.
   * @deprecate: workflow_update_workflow_states() --> WorkflowState->delete()
   * @todo: implement WorkflowState->save()
   */
  function save() {
  }
}

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
  public static function getState($sid, $wid) {
    $states = self::getStates($sid, $wid);
    return $states[$sid];
  }

  /**
   * Get all states in the system, with options to filter, only where a workflow exists.
   * @deprecated workflow_get_workflow_states() --> WorkflowState->getStates()
   * @deprecated workflow_get_workflow_states_all() --> WorkflowState->getStates()
   * @deprecated workflow_get_other_states_by_sid($sid) --> WorkflowState->getStates($sid)
   */
  public static function getStates($sid = 0, $wid = 0) {
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
}

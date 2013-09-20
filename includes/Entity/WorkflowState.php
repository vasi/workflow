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
  private $name = '';
  private $weight = 0; 
  private $sysid = 0; 
  private $status;
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
      $this->name = self::$states[$sid]->state; 
      $this->weight = self::$states[$sid]->weight; 
      $this->sysid = self::$states[$sid]->sysid;
      $this->status = self::$states[$sid]->status; 
      $this->workflow = self::$states[$sid]->workflow;
    }
  }

  /**
   * Get all states in the system, with options to filter, only where a workflow exists.
   * @todo: deprecate workflow_get_workflow_states()
   * @todo: deprecate workflow_get_workflow_states_all()
   * @todo: rename table column {workflow_states}-state to {workflow_states}-name
   */
  public static function getState($sid, $wid) {
    $states = self::getStates($sid, $wid);
    return $states[$sid];
  }

  public static function getStates($sid = 0, $wid = 0, $options = array()) {
    if ($sid && isset(self::$states[$sid])) {
      // Only 1 is requested and cached: return this one.
      return array($sid => self::$states[$sid]);
    }

    // Build the query.
    $query = db_select('workflow_states', 'ws');
    $query->leftJoin('workflows', 'w', 'w.wid = ws.wid');
    $query->fields('ws');
    $query->addField('w', 'wid');
    $query->addField('w', 'name');
//    @todo: add "WHERE status = 1 " in some calls 
//    $query->condition('ws.' . 'status', '1');

    // Spin through the options and add conditions.
    foreach ($options as $column => $value) {
      $query->condition('ws.' . $column, $value);
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

  /*
   * Returns the allowed values for the current state.
   */
  function getOptions($node) {
    return workflow_field_choices($node, $force = FALSE, $this);
  }

  /*
   * Mimics Entity::getName().
   *
   * @see Entity::getName
   */
  function getName() {
    return $this->name;
  }
}

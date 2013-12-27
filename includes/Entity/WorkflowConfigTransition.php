<?php

/**
 * @file
 * Contains workflow\includes\Entity\WorkflowConfigTransition.
 * Contains workflow\includes\Entity\WorkflowConfigTransitionController.
 */

class WorkflowConfigTransitionController extends EntityAPIController {
}


/**
 * Implements an configurated Transition.
 *
 */
class WorkflowConfigTransition extends Entity {

  // Transition data.
  public $tid = 0;
  // public $old_sid = 0;
  // public $new_sid = 0;
  public $sid = 0; // @todo D8: remove $sid, use $new_sid. (requires conversion of Views displays.)
  public $target_sid = 0;
  public $roles = array();
  protected $wid = 0;
  protected $workflow = NULL;
  // protected $is_scheduled = FALSE;
  // protected $is_executed = FALSE;
  // protected $force = NULL;

  /**
   * Entity class functions.
   */

  /**
   * Creates a new entity.
   *
   * @see entity_create()
   */
  public function __construct(array $values = array(), $entityType = NULL) {
    $entityType = 'WorkflowConfigTransition';
    return parent::__construct($values, $entityType); 
  }

  protected function defaultLabel() {
    return ''; // $this->title;
  }
  protected function defaultUri() {
    return array('path' => 'admin/config/workflow/workflow/transitions/' . $this->wid);
  }

  /**
   * Property functions.
   */

  /**
   * Returns the Workflow object of this State.
   *
   * @return Workflow
   *  Workflow object.
   */
  public function getWorkflow() {
    return isset($this->workflow) ? $this->workflow : Workflow::load($this->wid);
  }

}

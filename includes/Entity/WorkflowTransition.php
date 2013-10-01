<?php

/**
 * @file
 * Contains workflow\includes\Entity\WorkflowTransition.
 */

/*
 * Implements an actual Transition.
 */
class WorkflowTransition {
  public $nid; // @todo: make private. Use getEntity() instead.
  private $entity;
  private $field_name = ''; // @todo: add support for Fields in WorkflowTransition.

  public $old_sid;
  public $new_sid;
  public $sid; // @todo: remove $sid in D8: replaced by $new_sid.

  public $uid;
  public $stamp;
  public $comment;

  /**
   * Constructor
   * No arguments passed, when loading from DB.
   * All arguments must be passed, when creating an object programmatically.
   * One argument $entity may be passed, only to directly call delete() afterwards.
   */
  public function __construct($entity = NULL, $old_sid = 0, $new_sid = 0, $uid = 0, $stamp = 0, $comment = '') {
    $this->entity = $entity;
    $this->nid = isset($entity->nid) ? $entity->nid : $this->nid; // @todo: support other entity types.

    $this->old_sid = $old_sid;
    $this->new_sid = $new_sid;
    $this->sid = $new_sid; //@todo: deprecate $sid, replaced by $new_sid.

    $this->uid = $uid;
    $this->stamp = $stamp;
    $this->comment = $comment;
  }

  /*
   * Verifies if the given transition is allowed.
   * - in settings
   * - in permissions
   * - by permission hooks, implemented by other modules.
   *
   * @return string message: empty if OK, else a message for drupal_set_message.
   */
  public function isAllowed($force) {
    $old_sid = $this->old_sid;
    $new_sid = $this->new_sid;
    $new_state = new WorkflowState($new_sid);

    // Get all states from the Workflow, or only the valid transitions for this state.
    // WorkflowState::getOptions() will consider all permissions, etc.
    $options = $force ? $new_state->getWorkflow()->getOptions()
                      : $new_state->getOptions($this->entity, $force);

    if (!array_key_exists($new_sid, $options)) {
      $t_args = array('%old_sid' => $old_sid, '%new_sid' => $new_sid, );
      return $error_message = t('The transition from %old_sid to %new_sid is not allowed.', $t_args);
    }
  }

  /*
   * Get the Transitions $entity.
   * @todo: support other Entity types then only 'node'.
   */
  public function getEntity() {
    return node_load($this->nid);
  }

  /*
   * Functions, common to the WorkflowTransitions.
   */

  /*
   * Returns if this is a Scheduled Transition.
   */
  public function isScheduled() {
    return FALSE;
  } 
  public function isExecuted() {
    return NULL;
  } 

}

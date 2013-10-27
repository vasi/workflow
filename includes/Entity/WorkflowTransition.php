<?php

/**
 * @file
 * Contains workflow\includes\Entity\WorkflowTransition.
 */

/**
 * Implements an actual Transition.
 */
class WorkflowTransition {
  // Field data.
  public $entity_type;
  public $field_name = ''; // @todo: add support for Fields in WorkflowTransition.
  public $language = 'und';
  public $delta = 0;
  // Entity data.
  public $entity_id;
  public $nid; // @todo D8: remove $nid, use $entity_id. (requires conversion of Views displays.)
  private $entity; // This is dynamically loaded. Use WorkflowTransition->getEntity() to fetch this.
  // Transition data.
  public $old_sid = 0;
  public $new_sid = 0;
  public $sid; // @todo: remove $sid in D8: replaced by $new_sid. (requires conversion of Views displays.)
  public $uid = 0;
  public $stamp;
  public $comment = '';

  /**
   * CRUD functions.
   */

  /**
   * Constructor.
   *
   * No arguments passed, when loading from DB.
   * All arguments must be passed, when creating an object programmatically.
   * One argument $entity may be passed, only to directly call delete() afterwards.
   */
  public function __construct($entity_type = 'node', $entity = NULL, $field_name = '', $old_sid = 0, $new_sid = 0, $uid = 0, $stamp = 0, $comment = '') {
    $this->entity_type = ($this->entity_type) ? $this->entity_type : $entity_type;
    $this->field_name = (!$field_name) ? $this->field_name : $field_name;
    $this->language = ($this->language) ? $this->language : 'und';
    $this->entity = $entity;
    $this->nid = entity_id($entity_type, $entity);

    $this->old_sid = $old_sid;
    $this->sid = $new_sid;

    $this->uid = $uid;
    $this->stamp = $stamp;
    $this->comment = $comment;

    // Fill the 'new' fields correctly. @todo: rename these fields in db table.
    $this->entity_id = $this->nid;
    $this->new_sid = $this->sid;
  }

  /**
   * Property functions.
   */

  /**
   * Verifies if the given transition is allowed.
   * 
   * - in settings
   * - in permissions
   * - by permission hooks, implemented by other modules.
   *
   * @return string message
   *  empty if OK, else a message for drupal_set_message.
   */
  public function isAllowed($force) {
    $old_sid = $this->old_sid;
    $new_sid = $this->new_sid;
    $entity_type = $this->entity_type;
    $entity = $this->getEntity(); // Entity may not be loaded, yet.
    $old_state = WorkflowState::load($old_sid);

    // Get all states from the Workflow, or only the valid transitions for this state.
    // WorkflowState::getOptions() will consider all permissions, etc.
    $options = $force ? $old_state->getWorkflow()->getOptions() : $old_state->getOptions($entity_type, $entity, $force);
    if (!array_key_exists($new_sid, $options)) {
      $t_args = array(
        '%old_sid' => $old_sid,
        '%new_sid' => $new_sid,
      );
      return $error_message = t('The transition from %old_sid to %new_sid is not allowed.', $t_args);
    }
  }

/**
 * Execute a transition (change state of a node).
 * @deprecated: workflow_execute_transition() --> WorkflowTransition::execute().
 *
 * @param $force
 *   If set to TRUE, workflow permissions will be ignored.
 *
 * @return
 *  new state ID. If execution failed, old state ID is returned,
 */
  public function execute($force = FALSE) {
    global $user;

    $old_sid = $this->old_sid;
    $new_sid = $this->new_sid;
    $entity_type = $this->entity_type;
    $entity_id = $this->entity_id;
    $entity = $this->getEntity(); // Entity may not be loaded, yet.
    $field_name = $this->field_name;

    if (!$force) {
      // Make sure this transition is allowed.
      $result = module_invoke_all('workflow', 'transition permitted', $new_sid, $old_sid, $entity, $force, $entity_type, $field_name);
      // Did anybody veto this choice?
      if (in_array(FALSE, $result)) {
        // If vetoed, quit.
        return $old_sid;
      }
    }

    // Let other modules modify the comment.
    // @todo D8: remove a but last items from $context.
    $context = array(
      'node' => $entity,
      'sid' => $new_sid,
      'old_sid' => $old_sid,
      'uid' => $this->uid,
      'transition' => $this,
    );
    drupal_alter('workflow_comment', $this->comment, $context);

    if ($old_sid == $new_sid) {
      // Stop if not going to a different state.
      // Write comment into history though.
      if ($this->comment) {
        $this->stamp = REQUEST_TIME;

        // @todo D8: remove; this is only for Node API.
        $entity->workflow_stamp = REQUEST_TIME;
        workflow_update_workflow_node_stamp($entity_id, $this->stamp); //@todo: only for Node API

        $result = module_invoke_all('workflow', 'transition pre', $old_sid, $new_sid, $entity, $force, $entity_type, $field_name);
        $data = array(
          'nid' => $entity_id,
          'sid' => $new_sid,
          'old_sid' => $old_sid,
          'uid' => $this->uid,
          'stamp' => $this->stamp,
          'comment' => $this->comment,
        );
        workflow_insert_workflow_node_history($data);
        unset($entity->workflow_comment);  // @todo D8: remove; this line is only for Node API.
        $result = module_invoke_all('workflow', 'transition post', $old_sid, $new_sid, $entity, $force, $entity_type, $field_name);
      }

      // Clear any references in the scheduled listing.
      foreach (WorkflowScheduledTransition::load($entity_type, $entity_id, $field_name) as $scheduled_transition) {
        $scheduled_transition->delete();
      }
      return $old_sid;
    }

    $transition = workflow_get_workflow_transitions_by_sid_target_sid($old_sid, $new_sid);
    if (!$transition && !$force) {
      watchdog('workflow',
               'Attempt to go to nonexistent transition (from %old to %new)',
               array(
                 '%user' => $user->name,
                 '%old' => $old_sid,
                 '%new' => $new_sid,
               ),
               WATCHDOG_ERROR
      );
      return $old_sid;
    }

    // Make sure this transition is valid and allowed for the current user.
    // Check allow-ability of state change if user is not superuser (might be cron).
    if (($user->uid != 1) && !$force) {
      if (!workflow_transition_allowed($transition->tid, array_merge(array_keys($user->roles), array('author')))) {
        watchdog('workflow',
                 'User %user not allowed to go from state %old to %new',
                 array(
                   '%user' => $user->name,
                   '%old' => $old_sid,
                   '%new' => $new_sid,
                 ),
                 WATCHDOG_NOTICE
        );
        return $old_sid;
      }
    }

    // Invoke a callback indicating a transition is about to occur.
    // Modules may veto the transition by returning FALSE.
    $result = module_invoke_all('workflow', 'transition pre', $old_sid, $new_sid, $entity, $force, $entity_type, $field_name);

    // Stop if a module says so.
    if (in_array(FALSE, $result)) {
      watchdog('workflow', 'Transition vetoed by module.');
      return $old_sid;
    }

    // If the node does not have an existing 'workflow' property, save the $old_sid there so it can be logged.
    // This is only valid for Node API.
    // @todo: why is this set here? It is set again 16 lines down.
    if (!$field_name && !isset($entity->workflow)) {
      $entity->workflow = $old_sid;
    }

    // Change the state.
    $data = array(
      'nid' => $entity_id,
      'sid' => $new_sid,
      'uid' => (isset($entity->workflow_uid) ? $entity->workflow_uid : $user->uid),
      'stamp' => REQUEST_TIME,
    );

    // Workflow_update_workflow_node places a history comment as well.
    workflow_update_workflow_node($data, $old_sid, $this->comment);

    if (!$field_name) {
      // Only for Node API.
      $entity->workflow = $new_sid;
    }

    // Register state change with watchdog.
    if ($state = WorkflowState::load($new_sid)) {
      $workflow = $state->getWorkflow();
      if ($workflow->options['watchdog_log']) {
        $entity_type_info = entity_get_info($entity_type);
        $message = ($this->isScheduled()) ? 'Scheduled state change of @type %label to %state_name executed' : 'State of @type %label set to %state_name';
        $args = array(
          '@type' => $entity_type_info['label'],
          '%label' => entity_label($entity_type, $entity),
          '%state_name' => $state->label(),
        );
        $uri = entity_uri($entity_type, $entity);
        watchdog('workflow', $message, $args, WATCHDOG_NOTICE, l('view', $uri['path']));
      }
    }

    // Notify modules that transition has occurred.
    // Action triggers should take place in response to this callback, not the previous one.
    module_invoke_all('workflow', 'transition post', $old_sid, $new_sid, $entity, $force, $entity_type, $field_name);

    // Clear any references in the scheduled listing.
    foreach (WorkflowScheduledTransition::load($entity_type, $entity_id, $field_name) as $scheduled_transition) {
      $scheduled_transition->delete();
    }

    return $new_sid;
  }

  /**
   * Get the Transitions $entity.
   *
   * @return object $entity
   *   The entity, that is added to the Transition.
   */
  public function getEntity() {
    // A correct call, return the $entity.
    if (empty($this->entity)) {
      $entity_type = $this->entity_type;
      $entity_id = $this->entity_id;
      $this->entity = entity_load_single($entity_type, $entity_id);
    }
    return $this->entity;
  }

  /**
   * Set the Transitions $entity.
   *
   * @param $entity_type
   * @param $entity
   *  The Entity ID or the Entity object, to add to the Transition.
   *
   * @return object $entity
   *   The entity, that is added to the Transition.
   */
  public function setEntity($entity_type, $entity) {
    if (!is_object($entity)) {
      $entity_id = $entity;
      // Use node API or Entity API to load the object first.
      $entity = entity_load_single($entity_type, $entity_id);
    }
    $this->entity = $entity;
    $this->entity_type = $entity_type;
    $this->entity_id = entity_id($entity_type, $entity);
    $this->nid = $this->entity_id;

    return $this->entity;
  }

  /**
   * Functions, common to the WorkflowTransitions.
   */

  /**
   * Returns if this is a Scheduled Transition.
   */
  public function isScheduled() {
    return FALSE;
  }
  public function isExecuted() {
    return NULL;
  }

}

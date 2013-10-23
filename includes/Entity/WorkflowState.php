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
  public $weight = 0;
  private $sysid = 0;
  private $state = ''; // @todo: rename to 'label'.
  public $status = 1;
  private $workflow = NULL;

  /**
   * CRUD functions.
   */

  public function __construct($sid = 0, $wid = 0) {
    if (empty($sid) && empty($wid)) {
      // automatic constructor when casting an array or object.
      if (!isset(self::$states[$this->sid])) {
        self::$states[$this->sid] = $this;
      }
    }
    elseif (empty($sid)) {
      // Creating an dummy/new state for a workflow.
      // Do not add to 'cache' self::$tates.
      	$this->wid = $wid;
    }
    else {
      // Fetching an existing state for a workflow.
      if (!isset(self::$states[$sid])) {
        self::$states[$sid] = WorkflowState::load($sid, $wid);
      }
      // State may not exist.
      if (self::$states[$sid]) {
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
  }

  /**
   * Creates and returns a new WorkflowState object.
   *
   * $return WorkflowState $state 
   *  A new WorkflowState object
   *
   * "New considered harmful".
   */
  public static function create($sid, $wid) {
    $state = new WorkflowState($sid, $wid);
    return $state;
  }

  /**
   * Alternative constructor, via a static function, loading objects from table {workflow_states}.
   */
  public static function load($sid) {
    $states = self::getStates($sid);
    $state = isset($states[$sid]) ? $states[$sid] : NULL;
    return $state;
  }

  /**
   * Get all states in the system, with options to filter, only where a workflow exists.
   * @param $sid   : the requested State ID
   * @param $wid   : the requested Workflow ID
   * @param $reset : an option to refresh all caches.
   * @return       : an array of states.
   *
   * @deprecated workflow_get_workflow_states() --> WorkflowState::getStates()
   * @deprecated workflow_get_workflow_states_all() --> WorkflowState::getStates()
   * @deprecated workflow_get_other_states_by_sid($sid) --> WorkflowState::getStates()
   */
  public static function getStates($sid = 0, $wid = 0, $reset = FALSE) {
    if ($reset) {
      self::$states = array();
    }

    if (empty(self::$states)) {
      // Build the query, and get ALL states.
      // Note: self::states[] is populated in respective constructors.
      $query = db_select('workflow_states', 'ws');
      $query->fields('ws');
      $query->orderBy('ws.weight');
      $query->orderBy('ws.wid');
      // Just for grins, add a tag that might result in modifications.
      $query->addTag('workflow_states');

      $query->execute()->fetchAll(PDO::FETCH_CLASS, 'WorkflowState');
    }

    if (!$sid && !$wid) {
      // All states are requested and cached: return them.
      return self::$states;
    }

    // If only 1 State is requested and cached: return this one.
    if ($sid) {
      // The sid may be deactivated/non-existent.
      $state = isset(self::$states[$sid]) ? self::$states[$sid] : self::$states[$sid] = NULL;
      return array($sid => $state);
    }

    if ($wid) {
      // All states of only 1 Workflow is requested: return this one.
      $result = array();
      foreach (self::$states as $state) {
        if ($state->wid == $wid) {
          $result[$state->sid] = $state;
        }
      }
      return $result;
    }
  }

  public static function getStatesByName($name, $wid) {
    foreach($states = WorkflowState::getStates(0, $wid) as $state) {
      if ($name != $state->getName()) {
        unset($states[$state->sid]);
      }
    }
    return $states;
  }

  /**
   * Save (update/insert) a Workflow State into table workflow_states.
   * @deprecated: workflow_update_workflow_states() --> WorkflowState->save()
   */
  function save() {
    // Convert all properties to an array, the previous ones, too.
    $data['sid'] = $this->sid;
    $data['wid'] = $this->wid;
    $data['weight'] = $this->weight;
    $data['sysid'] = $this->sysid;
    $data['state'] = $this->state;
    $data['status'] = $this->status;

    if (!empty($this->sid) && count(WorkflowState::load($this->sid)) > 0) {
      drupal_write_record('workflow_states', $data, 'sid');
    }
    else {
      drupal_write_record('workflow_states', $data);
    }
  }

  /**
   * Given data, delete from workflow_states.
   */
  function delete() {
    db_delete('workflow_states')
      ->condition('sid', $this->sid)
      ->execute();
  }

  /**
   * Deactivate a Workflow State, moving existing nodes to a given State.
   * @deprecated workflow_delete_workflow_states_by_sid() --> WorkflowState->deactivate() + delete()
   */
  function deactivate($new_sid) {
    $sid = $this->sid;
    // Notify interested modules. We notify first to allow access to data before we zap it.
    module_invoke_all('workflow', 'state delete', $sid, NULL, NULL, FALSE);

    // Node API: Re-parent any nodes that we don't want to orphan, whilst deactivating a State.
    // @todo Field API: Re-parent any nodes that we don't want to orphan, whilst deactivating a State.
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

    // Node API: Delete any lingering node to state values.
    workflow_delete_workflow_node_by_sid($sid);
    // @todo: Field API: Delete any lingering node to state values.
    //workflow_delete_workflow_field_by_sid($sid);

    // Delete the state. -- We don't actually delete, just deactivate.
    // This is a matter up for some debate, to delete or not to delete, since this
    // causes name conflicts for states. In the meantime, we just stick with what we know.
    // If you really want to delete the states, use workflow_cleanup module, or delete().
    $this->status = FALSE;
    $this->save();

    // Clear the cache.
    self::$states = array();
  }

  /**
   * Property functions.
   */

  /**
   * Returns the Workflow object of this State.
   *
   * @return
   *  Workflow object.
   */
  function getWorkflow() {
    return isset($this->workflow) ? $this->workflow : Workflow::load($this->wid);
  }

  /**
   * Returns the Workflow object of this State.
   *
   * @return
   *  boolean TRUE if state is active, else FALSE.
   */
  function isActive() {
    return (boolean) $this->status;
  }

  function isCreationState() {
    return $this->sysid == WORKFLOW_CREATION;
  }

  /**
   * Returns the allowed values for the current state.
   * @deprecated workflow_field_choices() --> WorkflowState->getOptions()
   */
  function getOptions($entity_type, $entity, $force = FALSE) {
  global $user;
  static $cache = array(); // Entity-specific cache per page load.

  $choices = array();

  if (!$entity) {
    // If no entity is given, no result (e.g., on a Field settings page)
    $choices = array();
    return $choices;
  }

  $entity_id = _workflow_get_entity_id($entity_type, $entity);
  $sid = $this->sid;
  $workflow = Workflow::load($this->wid);

  // Get options from page cache.
  if (isset($cache[$entity_type][$entity_id][$force][$sid])) {
    $choices = $cache[$entity_type][$entity_id][$force][$sid];
    return $choices;
  }

  if ($workflow) {
    $roles = array_keys($user->roles);

    // If user is node author or this is a new page, give the authorship role.
    if (($user->uid == $entity->uid && $entity->uid > 0) || empty($entity_id)) {
      $roles += array('author' => 'author');
    }
    if ($user->uid == 1 || $force) {
      // Superuser is special. And Force allows Rules to cause transition.
      $roles = 'ALL';
    }

    // Workflow_allowable_transitions() does not return the entire transition row. Would like it to, but doesn't.
    // Instead it returns just the allowable data as:
    // [tid] => 1 [state_id] => 1 [state_name] => (creation) [state_weight] => -50
    $transitions = workflow_allowable_transitions($sid, 'to', $roles);

    // Include current state if it is not the (creation) state.
    foreach ($transitions as $transition) {
      if ($transition->sysid != WORKFLOW_CREATION && !$force) {
        // Invoke a callback indicating that we are collecting state choices. Modules
        // may veto a choice by returning FALSE. In this case, the choice is
        // never presented to the user.
        // @todo: for better performance, call a hook only once: can we find a way to pass all transitions at once
        $result = module_invoke_all('workflow', 'transition permitted', $sid, $transition->state_id, $entity, $field_name = '');
        // Did anybody veto this choice?
        if (!in_array(FALSE, $result)) {
          // If not vetoed, add to list.
          $choices[$transition->state_id] = check_plain(t($transition->state_name));
        }
      }
    }

    // Save to entity-specific cache.
    $cache[$entity_type][$entity_id][$force][$sid] = $choices;
  }

  return $choices;
  }

  /**
   * Mimics Entity API functions.
   */
  function label($langcode = NULL) {
    return t($this->state, $args = array(), $options = array('langcode' => $langcode));
  }
  function getName() {
    return $this->state;
  }
  function setName($name) {
    return $this->state = $name;
  }
  function value() {
    return $this->sid;
  }

}

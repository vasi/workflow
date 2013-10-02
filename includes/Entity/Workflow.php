<?php

/**
 * @file
 * Contains workflow\includes\Entity\Workflow.
 */

class Workflow {
  // Since workflows do not change, it is implemented as a singleton.
  private static $workflows = array();

  public $wid = 0;
  public $name = '';
  public $tab_roles = array();
  public $options = array();
  private $creation_sid = 0;
  private $creation_state = NULL;
  private $item = NULL; // helper for workflow_get_workflows_by_type() to get/set the Item of a particular Workflow.

  public function __construct($wid = 0) {
    if (!$wid) {
      // automatic constructor when casting an array or object.
      if (!is_array($this->options)) {
        $this->options = unserialize($this->options);
      }
      self::$workflows[$this->wid] = $this;
    }
    else {
      if (!isset(self::$workflows[$wid])) {
        self::$workflows[$wid] = self::getWorkflow($wid);
      }
      // @todo: this copy-thing should not be necessary.
      $this->wid = self::$workflows[$wid]->wid;
      $this->name = self::$workflows[$wid]->name;
      $this->tab_roles = self::$workflows[$wid]->tab_roles;
      $this->options = self::$workflows[$wid]->options;
      $this->creation_sid = self::$workflows[$wid]->creation_sid;
    }
  }

/* 
 * A Factory function to get Workflow data from the database, and return objects.
 * The execution of the query instantiates objects and saves them in a static array.
 */ 
  public static function load($wid) {
    $workflows = self::getWorkflows($wid, $reset = FALSE);
    return $workflows[$wid];
  }
  public static function getWorkflow($wid, $reset = FALSE) {
    $workflows = self::getWorkflows($wid, $reset);
    return $workflows[$wid];
  }

/* 
 * A Factory function to get Workflow data from the database, and return objects.
 * This is only called by CRUD functions in workflow.features.inc
 * More than likely in prep for an import / export action.
 * Therefore we don't want to fiddle with the response.
 * @deprecated: workflow_get_workflows_by_name() --> Workflow::getWorkflowByName($name)
 */ 
  public static function getWorkflowByName($name, $unserialize_options = FALSE) {
    foreach($workflows = self::getWorkflows() as $workflow) {
      if ($name == $workflow->getName()) {
        if (!$unserialize_options) {
          $workflow->options = serialize($workflow->options);
        }
        return $workflow;
      }
    }
    return FALSE;
  }

  public static function getWorkflows($wid = 0, $reset = FALSE) {
    if ($reset) {
      self::$workflows = array();
    }

    if ($wid && isset(self::$workflows[$wid])) {
      // Only 1 is requested and cached: return this one.
      return array($wid => self::$workflows[$wid]);
    }

    // Build the query.
    // If all are requested: read from db ($todo: cache this, but only used on Admin UI.)
    // If requested one is not cached: read from db
    $query = db_select('workflows', 'w');
    $query->leftJoin('workflow_states', 'ws', 'w.wid = ws.wid');
    $query->fields('w');
    $query->addField('ws', 'sid', 'creation_sid');
    // Initially, only get the creation_state of the Workflow.
    $query->condition('ws.sysid' , WORKFLOW_CREATION);

    // return array of objects, even if only 1 is requested.
    // note: self::workflows[] is populated in respective constructors.
    if ($wid) {
      // return 1 object.
      $query->condition('w.wid', $wid);
      $query->execute()->fetchAll(PDO::FETCH_CLASS, 'Workflow');
      return array($wid => self::$workflows[$wid]);
    }
    else {
      $query->execute()->fetchAll(PDO::FETCH_CLASS, 'Workflow');
      return self::$workflows;
    }
  }

  function getCreationState() {
    if (!isset($this->creation_state)) {
      $this->creation_state = new WorkflowState($this->creation_sid);
    }
    return $this->creation_state;
  }

  function getCreationSid() {
    return $this->creation_sid;
  }

  /* Get the first valid state ID, after the creation state.
   * Use getOptions(), because this does a access check.
   */
  function getFirstSid($node) {
    $creation_state = self::getCreationState();
    $options = $creation_state->getOptions($node);
    if ($options) {
      $keys = array_keys($options);
      $sid = $keys[0];
    }
    else {
      // This should never happen, but it did during testing.
      drupal_set_message(t('There are no workflow states available. Please notify your site administrator.'), 'error');
      $sid = 0;
    }
    return $sid;
  }

  /* 
   * @Return
   *   An array of WorflowState objects.
   */
  function getStates() {
    return WorkflowState::getStates(0, $this->wid);
  }

  /* 
   * @Return
   *   All states in a Workflow, as an array of $key => $label.
   */
  function getOptions() {
    $options = array();
    foreach($this->getStates() as $state) {
      $options[$state->value()] = $state->label();
    }
    return $options;
  }

  public function getSetting($key, array $field = array()) {
    switch ($key) {
      case 'watchdog_log':
        if (isset($workflow->options['watchdog_log'])) {
          // This is set via Node API.
          return $workflow->options['watchdog_log'];
        }
        elseif ($field) {
          if (isset($field['settings']['watchdog_log'])) {
          // This is set via Field API.
            return $field['settings']['watchdog_log'];
          }
        }
        drupal_set_message( 'Setting Workflow::getSetting(' . $key . ') does not exist', 'error');
        break;

      default:
        drupal_set_message( 'Setting Workflow::getSetting(' . $key . ') does not exist', 'error');
    }
  }

  /*
   * Helper function for workflow_get_workflows_by_type() to get/set the Item of a particular Workflow.
   * It loads the Workflow object with the particular Field Instance data.
   * @todo: this is not robust: 1 Item has 1 Workflow; 1 Workflow may have N Items (fields)
   */
  public function getWorkflowItem(WorkflowItem $item = NULL) {
    if ($item) {
      $this->item = $item;
    }
    return $this->item;
  }

  /*
   * Mimics Entity API functions.
   *
   */
  function label($langcode = NULL) {
    return t($this->name, $args = array(), $options = array('langcode' => $langcode));
  }
  function getName() {
    return $this->name;
  }
  function value() {
    return $this->wid;
  }

  /**
   * Given a wid, delete the workflow and its data.
   *
   * @deprecated: workflow_delete_workflows_by_wid() --> Workflow::delete().
   * @todo: This function does NOT delete WorkflowStates.
   */
  function delete() {
    $wid = $this->wid;

    // Notify any interested modules before we delete, in case there's data needed.
    module_invoke_all('workflow', 'workflow delete', $wid, NULL, NULL, FALSE);

    // Delete associated state (also deletes any associated transitions).
    foreach ($this->getStates() as $state) {
//    @todo:  $state->delete();
      workflow_delete_workflow_states_by_sid($state->sid);
    }

    // Delete type map.
    workflow_delete_workflow_type_map_by_wid($wid);

    // Delete the workflow.
    db_delete('workflows')->condition('wid', $wid)->execute();
  }

  /**
   * Given information, update or insert a new workflow.
   *
   * @deprecated: workflow_update_workflows() --> Workflow->save()
   * @todo: implement Workflow->save()
   */
  function save($create_creation_state = TRUE) {
//    if (isset($this->tab_roles) && is_array($this->tab_roles)) {
//      $this->tab_roles = implode(',', $this->tab_roles);
//    }
//
//    if (isset($this->wid) && count(Workflow::getWorkflow($data->wid)) > 0) {
//      drupal_write_record('workflows', $data, 'wid');
//    }
//    else {
//      drupal_write_record('workflows', $data);
//      if ($create_creation_state) {
//        $state_data = array(
//          'wid' => $data->wid,
//          'state' => t('(creation)'),
//          'sysid' => WORKFLOW_CREATION,
//          'weight' => WORKFLOW_CREATION_DEFAULT_WEIGHT,
//        );
//
//        workflow_update_workflow_states($state_data);
//      // @TODO consider adding state data to return here as part of workflow data structure.
//      // That way we could past structs and transitions around as a data object as a whole.
//      // Might make clone easier, but it might be a little hefty for our needs?
//    }
  }
  
}

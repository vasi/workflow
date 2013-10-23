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

  /**
   * CRUD functions.
   */

  public function __construct($wid = 0) {
    if (!$wid) {
      // automatic constructor when casting an array or object.
      if (!is_array($this->options)) {
        $this->options = unserialize($this->options);
      }
      if ($this->wid) {
        self::$workflows[$this->wid] = $this;
      }
    }
    else {
      if (!isset(self::$workflows[$wid])) {
        self::$workflows[$wid] = Workflow::load($wid);
      }
      // Workflow may not exist.
      if (self::$workflows[$wid]) {
        // @todo: this copy-thing should not be necessary.
        $this->wid = self::$workflows[$wid]->wid;
        $this->name = self::$workflows[$wid]->name;
        $this->tab_roles = self::$workflows[$wid]->tab_roles;
        $this->options = self::$workflows[$wid]->options;
        $this->creation_sid = self::$workflows[$wid]->creation_sid;
      }
    }
  }

  /**
   * Creates and returns a new Workflow object.
   *
   * $param string $name 
   *  The name of the new Workflow
   *
   * $return Workflow $workflow 
   *  A new Workflow object
   *
   * "New considered harmful".
   */
  public static function create($name) {
    $workflow = new Workflow();
    $workflow->name = $name;
    return $workflow;
  }

  /**
   * Loads a Workflow object from table {workflows}
   * Implements a 'Factory' pattern to get Workflow data from the database, and return objects.
   * The execution of the query instantiates objects and saves them in a static array.
   *
   * $param string $wid 
   *  The ID of the new Workflow
   *
   * $return Workflow $workflow 
   *  A new Workflow object
   */
  public static function load($wid, $reset = FALSE) {
    $workflows = self::getWorkflows($wid, $reset);
    $workflow = isset($workflows[$wid]) ? $workflows[$wid] : NULL;
    return $workflow;
  }

  /**
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
    return NULL;
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

    $query->execute()->fetchAll(PDO::FETCH_CLASS, 'Workflow');

    // return array of objects, even if only 1 is requested.
    // note: self::workflows[] is populated in respective constructors.
    if ($wid > 0) {
      // return 1 object.
      $workflow = isset(self::$workflows[$wid]) ? self::$workflows[$wid] : NULL;
      return array($wid => $workflow);
    }
    else {
      return self::$workflows;
    }
  }

  /**
   * Given information, update or insert a new workflow.
   *
   * @deprecated: workflow_update_workflows() --> Workflow->save()
   * @todo: implement Workflow->save()
   */
  function save($create_creation_state = TRUE) {
    if (isset($this->tab_roles) && is_array($this->tab_roles)) {
      $this->tab_roles = implode(',', $this->tab_roles);
    }
    if (is_array($this->options)) {
        $this->options = serialize($this->options);
    }

    if (($this->wid > 0) && Workflow::load($this->wid)) {
      drupal_write_record('workflows',  $this, 'wid');
    }
    else {
      drupal_write_record('workflows', $this);
      if ($create_creation_state) {
        $state_data = array(
          'wid' => $this->wid,
          'state' => t('(creation)'),
          'sysid' => WORKFLOW_CREATION,
          'weight' => WORKFLOW_CREATION_DEFAULT_WEIGHT,
        );

        workflow_update_workflow_states($state_data);
//      // @TODO consider adding state data to return here as part of workflow data structure.
//      // That way we could past structs and transitions around as a data object as a whole.
//      // Might make clone easier, but it might be a little hefty for our needs?
      }
    }
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
    foreach ($this->getStates($all = TRUE) as $state) {
      $state->deactivate($new_sid = 0);
      $state->delete();
    }

    // Delete type map. @todo: move this to hook_workflow of workflownode.module.
    if (TRUE || module_exists('workflownode')) {
      workflow_delete_workflow_type_map_by_wid($wid);
    }

    // Delete the workflow.
    db_delete('workflows')->condition('wid', $wid)->execute();
  }

  /**
   * Validate the workflow. Generate a message if not correct.
   * This function is used on the settings page of 
   * - Workflow node: workflow_admin_ui_type_map_form()
   * - Workflow field: WorkflowItem->settingsForm()
   *
   * @return
   *  boolean $isValid
   */
  function validate() {
    $isValid = TRUE;

    // Don't allow workflows with no states. (There should always be a creation state.)
    $states = $this->getStates();
    if (count($states) < 2) {
      // That's all, so let's remind them to create some states.
      $message = t('%workflow has no states defined, so it cannot be assigned to content yet.',
        array('%workflow' => ucwords($this->getName())));
      drupal_set_message($message, 'warning');

      // Skip allowing this workflow.
      $isValid = FALSE;
    }

    // Also check for transitions at least out of the creation state.
    // This always gets at least the "from" state.
    $transitions = workflow_allowable_transitions($this->getCreationSid(), 'to');
    if (count($transitions) < 2) {
      // That's all, so let's remind them to create some transitions.
      $message = t('%workflow has no transitions defined, so it cannot be assigned to content yet.',
        array('%workflow' => ucwords($this->getName())));
      drupal_set_message($message, 'warning');

      // Skip allowing this workflow.
      $isValid = FALSE;
    }

    return $isValid;
  }

  /**
   * Property functions.
   */

  function getCreationState() {
    if (!isset($this->creation_state)) {
      $this->creation_state = WorkflowState::load($this->creation_sid);
    }
    return $this->creation_state;
  }

  function getCreationSid() {
    return $this->creation_sid;
  }

  /**
   * Get the first valid state ID, after the creation state.
   * Use WorkflowState::getOptions(), because this does a access check.
   */
  function getFirstSid($entity_type, $entity) {
    $creation_state = $this->getCreationState();
    $options = $creation_state->getOptions($entity_type, $entity);
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

  /**
   * @param bool $all
   *   Indicates to return all (TRUE) or only active (FALSE) states of a workflow.
   * @return
   *   An array of WorkflowState objects.
   */
  function getStates($all = FALSE) {
    $states = WorkflowState::getStates(0, $this->wid);
    if (!$all) {
      foreach($states as $state) {
        if (!$state->isActive() && !$state->isCreationState()) {
          unset($states[$state->sid]);
        }
      }
    }
    return $states;
  }

  /**
   * @return
   *   A WorkflowState object.
   */
  function getState($sid) {
    return WorkflowState::load($sid);
  }

  /**
   * @param bool $grouped
   *   Indicates if the value must be grouped per workflow.
   *   This influence the rendering of the select_list options.
   *
   * @return
   *   All states in a Workflow, as an array of $key => $label.
   */
  function getOptions($grouped = FALSE) {
    $options = array();
    foreach($this->getStates() as $state) {
      $options[$state->value()] = check_plain($state->label());
    }
    if ($grouped) {
      // make a group for each Workflow.
      $label = check_plain($this->label());
      $grouped_options[$label] = $options;
      return $grouped_options;
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

  /**
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

  /**
   * Mimics Entity API functions.
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

}

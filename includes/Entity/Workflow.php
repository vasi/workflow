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
  public static function getWorkflow($wid, $reset = FALSE) {
    $workflows = self::getWorkflows($wid, $reset);
    return $workflows[$wid];
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

  /* Get the first valid state state, after the creation state.
   * Use getOptions(), because this does a access check.
   */
  function getFirstState($node) {
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
    return new WorkflowState($sid);
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
   * Mimics Entity API functions.
   *
   */
  function label() {
    return $this->name;
  }
  function value() {
    return $this->wid;
  }

}

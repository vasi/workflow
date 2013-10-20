<?php

/**
 * @file
 * Contains workflow\includes\Entity\WorkflowScheduledTransition.
 */

/*
 * Implements a scheduled transition, as shown on Workflow form.
 */
class WorkflowScheduledTransition extends WorkflowTransition {
  public $scheduled; // @todo: replace by $stamp;

  /*
   * Constructor
   * @todo: use parent::__construct ?
   */
  public function __construct($entity_type = '', $entity = NULL, $field_name = '', $old_sid = 0, $new_sid = 0, $uid = 0, $stamp = 0, $comment = '') {
    $this->entity_type = (!$entity_type) ? $this->entity_type : $entity_type;
    $this->field_name = (!$field_name) ? $this->field_name : $field_name;
    $this->language = ($this->language) ? $this->language : 'und';
    $this->entity = $entity;

    // If constructor is called with new() and arguments.
    // Load the supplied entity.
    if ($entity && !$entity_type) {
      // Not all paramaters are passed programmatically.
      drupal_set_message('Wrong call to new WorkflowScheduledTransition()', 'error');
    }
    elseif ($entity) {
      // When supplying the $entity, the $entity_type musst be known, too.
      $this->entity_type = $entity_type;
      $this->entity_id = ($entity_type == 'node') ? $entity->nid : entity_id($entity_type, $entity);
      $this->nid = $this->entity_id;
    }

    // If constructor is called with new() and arguments.
    if ($entity && $old_sid && $new_sid && $stamp) {
      $this->old_sid = $old_sid;
      $this->sid = $new_sid;

      $this->uid = $uid;
      $this->scheduled = $stamp;
      $this->comment = $comment;
    }
    elseif ($old_sid || $new_sid || $stamp) {
      // Not all paramaters are passed programmatically.
      drupal_set_message('Wrong call to new WorkflowScheduledTransition()', 'error');
    }

    // fill the 'new' fields correctly. @todo: rename these fields in db table.
    $this->entity_id = $this->nid;
    $this->new_sid = $this->sid;
  }

  /**
   * Given a node, get all scheduled transitions for it.
   * @param $nid
   *    The node ID.
   * @return
   *    An array of WorkflowScheduledTransitions
   *
   * @deprecated: workflow_get_workflow_scheduled_transition_by_nid() --> WorkflowScheduledTransition::load()
   */
  public static function load($entity_type, $entity_id, $field_name = '') {
    $results = db_query('SELECT * ' .
                        'FROM {workflow_scheduled_transition} ' .
                        'WHERE entity_type = :entity_type ' .
                        'AND   nid = :nid ' .
                        'ORDER BY scheduled ASC ',
                        array(':nid' => $entity_id, ':entity_type' => $entity_type));
    $result = $results->fetchAll(PDO::FETCH_CLASS, 'WorkflowScheduledTransition');
    return $result;
  }

  /**
   * Given a timeframe, get all scheduled transitions.
   * @deprecated: workflow_get_workflow_scheduled_transition_by_between() --> WorkflowScheduledTransition::loadBetween()
   */
  public static function loadBetween($start = 0, $end = REQUEST_TIME) {
    $results = db_query('SELECT * ' .
                        'FROM {workflow_scheduled_transition} ' .
                        'WHERE scheduled > :start AND scheduled < :end '.
                        'ORDER BY scheduled ASC',
                        array(':start' => $start, ':end' => $end));
    $result = $results->fetchAll(PDO::FETCH_CLASS, 'WorkflowScheduledTransition');
    return $result;
  }

  public function save() {
    // Avoid duplicate entries.
    $this->delete();
    // Save (insert or update) a record to the database based upon the schema.
    drupal_write_record('workflow_scheduled_transition', $this);

    // Get name of state.
    if ($state = WorkflowState::load($this->new_sid)) {
      $message = '@entity_title scheduled for state change to %state_name on %scheduled_date';
      $args = array(
          '@entity_type' => $this->entity_type,
          '@entity_title' => $this->entity->title,
          '%state_name' => t($state->label()),
          '%scheduled_date' => format_date($this->scheduled),
      );
      $uri = entity_uri($this->entity_type, $this->entity);
      watchdog('workflow', $message, $args, WATCHDOG_NOTICE, l('view', $uri['path'] . '/workflow'));

      drupal_set_message(t($message, $args));
    }
  }

  /**
   * Given a node, delete transitions for it.
   * @deprecated: workflow_delete_workflow_scheduled_transition_by_nid() --> WorkflowScheduledTransition::delete()
   */
  public function delete() {
    return $this->deleteByNid($this->entity_type, $this->entity_id);
  }

  /**
   * Given a node, delete transitions for it.
   * Better use delete(), instead of this static function.
   */
  public static function deleteByNid($entity_type, $nid) {
    return db_delete('workflow_scheduled_transition')
           ->condition('nid', $nid)
           ->execute();
  }

  /*
   * If a scheduled transition has no comment, a default comment is added before executing it.
   */
  public function addDefaultComment() {
    $this->comment = t('Scheduled by user @uid.',
                       array('@uid' => $this->uid));
  }

  /*
   * Get the Transition's $field_info.
   * This is called in hook_cron, to get the $field_info.
   * @todo: read $field_name directly from table.
   */
  public function getWorkflowItem() {
    $workflowItem = NULL;

    if (!empty($this->field_name)) {
      // @todo: read $field_name directly from table.
    }

    $entity_type = $this->entity_type;
    $entity = $this->getEntity();
    $entity_bundle = $this->getEntity()->type;

    foreach (field_info_instances($entity_type, $entity_bundle) as $field_name => $field_instance) {
      $field_info = field_info_field($field_instance['field_name']);
      $field_type = $field_info['type'];
      if ($field_type == 'workflow') {
        // Set cache.
        $this->field_name = $field_name;
        // Prepare return value.
        $workflowItem = new WorkflowItem($field_info, $field_instance, $entity_type, $this->getEntity());
      }
    }
    return $workflowItem;
  }

  /*
   * Functions, common to the WorkflowTransitions.
   */

  public function isScheduled() {
    return TRUE;
  }
  public function isExecuted() {
    return FALSE;
  }

}

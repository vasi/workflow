<?php

/**
 * @file
 * Contains workflow\includes\Entity\WorkflowScheduledTransition.
 */

class WorkflowScheduledTransition {
  public $nid; // @todo: make private. Use getEntity() instead.
  private $field_name = ''; // @todo: add support for Fields in ScheduledTransition.

  public $old_sid;
  public $new_sid;
  private $entity;
  public $sid; // @todo: remove $sid in D8: replaced by $new_sid.

  /**
   * Constructor
   * No arguments passed, when loading from DB.
   * All arguments must be passed, when creating an object programmatically.
   * One argument $entity may be passed, only to directly call delete() afterwards.
   */
  public function __construct($entity = NULL, $old_sid = 0, $new_sid = 0, $uid = 0, $stamp = 0, $comment = '') {
    if ($entity && $old_sid && $new_sid && $stamp) {
      $this->entity = $entity;

      $this->old_sid = $old_sid;
      $this->new_sid = $new_sid;
      $this->sid = $new_sid;
      $this->uid = $uid;
      $this->scheduled = $stamp;
      $this->comment = $comment;
    }
    elseif ($old_sid || $new_sid || $stamp) {
      // Not all paramaters are passed programmatically.
      drupal_set_message('Wrong call to new WorkflowScheduledTransition()', 'error');
    }

    $this->nid = isset($entity->nid) ? $entity->nid : $this->nid; // @todo: support other entity types.
    $this->new_sid = $this->sid; //@todo: deprecate $sid
  }

  /**
   * Given a node, get all scheduled transitions for it.
   * @param $nid
   *    The node ID.
   * @return
   *    An array of WorkflowScheduledTransitions
   *
   * @todo: deprecate: workflow_get_workflow_scheduled_transition_by_nid() --> WorkflowScheduledTransition::load()
   */
  public static function load($nid) {
    $results = db_query('SELECT nid, old_sid, sid, uid, scheduled, comment ' .
                        'FROM {workflow_scheduled_transition} ' . 
                        'WHERE nid = :nid ' .
                        'ORDER BY scheduled ASC ',
                        array(':nid' => $nid));
    return $results->fetchAll(PDO::FETCH_CLASS, 'WorkflowScheduledTransition');
  }

/**
 * Given a timeframe, get all scheduled transitions.
 * @todo: deprecate: workflow_get_workflow_scheduled_transition_by_between() --> WorkflowScheduledTransition::loadBetween()
 */
  public static function LoadBetween($start = 0, $end = REQUEST_TIME) {
    $results = db_query('SELECT nid, old_sid, sid, uid, scheduled, comment ' .
                        'FROM {workflow_scheduled_transition} ' .
                        'WHERE scheduled > :start AND scheduled < :end '.
                        'ORDER BY scheduled ASC',
                        array(':start' => $start, ':end' => $end));
    return $results->fetchAll(PDO::FETCH_CLASS, 'WorkflowScheduledTransition');
  }

  public function save() {
    // Avoid duplicate entries.
    $this->delete();
    drupal_write_record('workflow_scheduled_transition', $this);
  }

  /**
   * Given a node, delete transitions for it.
   * @todo: deprecate: workflow_delete_workflow_scheduled_transition_by_nid() --> WorkflowScheduledTransition::delete()
   */
  public function delete() {
    return $this->deleteByNid($this->nid); 
  }

  /**
   * Given a node, delete transitions for it.
   * Better use delete(), instead of this static function.
   */
  public static function deleteByNid($nid) {
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
//    @todo: read $field_name directly from table.
    }

    $entity_type = 'node';
    $entity_bundle = $this->getEntity()->type;

    foreach( field_info_instances($entity_type, $entity_bundle) as $field_name => $field_instance) {
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
   * Get the Transitions $entity.
   * @todo: support other Entity types then only 'node'.
   */
  public function getEntity() {
    return node_load($this->nid);
  }

}

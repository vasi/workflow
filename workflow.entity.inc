<?php

/**
 * @file
 * Integrates workflow with entity API.
 */

/**
 * Implements hook_entity_info().
 *
 *  @todo: implement hook_property_info, metadata.
 */
function workflow_entity_info() {

  $entities['Workflow'] = array(
    'label' => t('Workflow'),
    'plural label' => t('Workflows'),
    'entity class' => 'Workflow',
    'controller class' => 'WorkflowController',
    'features controller class' => 'WorkflowFeaturesController',
    'module' => 'workflow',
    'base table' => 'workflows',
    'fieldable' => FALSE,
    'exportable' => TRUE,
    'entity keys' => array(
      'id' => 'wid',
      'name' => 'name',
      // 'label' => 'label',
    ),
    'label callback' => 'entity_class_label',
    'uri callback' => 'entity_class_uri',
    // The following is added in workflow_admin_ui.module.
    // 'access callback' => 'workflow_access',
    // 'admin ui' => array(
    //   'path' => 'admin/config/workflow/workflow',
    //   'file' => 'workflow_admin_ui/workflow_admin_ui.pages.inc',
    //   'controller class' => 'EntityWorkflowUIController',
    //   'menu wildcard' => '%workflow',
    // ),
  );

  $entities['WorkflowState'] = array(
    'label' => t('Workflow state'),
    'entity class' => 'WorkflowState',
    'controller class' => 'WorkflowStateController',
//    'features controller class' => FALSE, //@todo: implement this.
    'module' => 'workflow',
    'base table' => 'workflow_states',
    'fieldable' => FALSE,
    'exportable' => FALSE,
    'entity keys' => array(
      'id' => 'sid',
    ),
    'label callback' => 'entity_class_label',
    'uri callback' => 'entity_class_uri',
  );

  $entities['WorkflowConfigTransition'] = array(
    'label' => t('Workflow config transition'),
    'entity class' => 'WorkflowConfigTransition',
    // Add controller Class. 'Workflow' class is the de-facto controller.
    'controller class' => 'WorkflowConfigTransitionController',
    'base table' => 'workflow_transitions',
    'exportable' => FALSE,
    'entity keys' => array(
      'id' => 'tid',
//      'name' => 'tid',
      'status' => 'status',
//      'bundle' => 'type',
    ),
    'label callback' => 'entity_class_label',
    // 'uri callback' => 'entity_class_uri',
//    'view modes' => array(
//      'full' => array(
//        'label' => t('Full'),
//        'custom settings' => TRUE,
//      ),
//    ),
//    'views controller class' => 'EntityDefaultViewsController',
    'module' => 'workflow',
//    'access callback' => 'workflow_tab_access', // @todo: use to-be workflow_access here. Access to Tab <> access to workflow.
  );

  // The Controller class of Transitions and ScheduledTransition is shared.
  $entities['WorkflowTransition'] = array(
    'label' => t('Workflow executed transition'),
    'entity class' => 'WorkflowTransition',
    'controller class' => 'WorkflowTransitionController',
    'views controller class' => 'EntityDefaultViewsController',
    'rules controller class' => 'EntityDefaultRulesController',
    'features controller class' => FALSE,
    'base table' => 'workflow_node_history',
    'entity keys' => array(
      'id' => 'hid',
    ),
    'label callback' => 'entity_class_label',
    'module' => 'workflow',
  );

  $entities['WorkflowScheduledTransition'] = array(
    'label' => t('Workflow scheduled transition'),
    'entity class' => 'WorkflowScheduledTransition',
    'controller class' => 'WorkflowTransitionController',
    'views controller class' => 'EntityDefaultViewsController',
    'rules controller class' => 'EntityDefaultRulesController',
    'features controller class' => FALSE,
    'base table' => 'workflow_scheduled_transition',
    'entity keys' => array(
      'id' => '',
    ),
    'label callback' => 'entity_class_label',
    'module' => 'workflow',
  );

  return $entities;
}

/**
 * Implements hook_entity_property_info_alter().
 *
 * P.S. When adding a new property here, you might delete code from workflow_tokens().
 */
function workflow_entity_property_info_alter(&$info) {
  $info['Workflow']['properties']['wid']['label'] = 'Workflow ID';
  $info['Workflow']['properties']['options']['getter callback'] = '_workflow_metadata_workflow_get_properties';
  $info['Workflow']['properties']['states'] = array(
    'type' => 'list<WorkflowState>',
    'label' => 'States of this Workflow',
    'getter callback' => '_workflow_metadata_workflow_get_properties',
    // 'options list' => 'entity_metadata_user_roles',
    // 'required' => TRUE/FALSE,
    // 'entity views field' => TRUE/FALSE,
    // 'entity token' => TRUE/FALSE,
  );
  $info['Workflow']['properties']['transitions'] = array(
    'type' => 'list<WorkflowConfigTransition>',
    'label' => 'Transitions of this Workflow',
    'getter callback' => '_workflow_metadata_workflow_get_properties',
  );

  unset($info['Workflow']['properties']['tab_roles']['description']);
  $info['Workflow']['properties']['tab_roles'] += array(
    'name' => t('roles'),
    'description' => t("The roles that can access the Workflow History tab."),
    'type' => 'list<integer>',
    'getter callback' => '_workflow_metadata_workflow_get_properties',
  );

  // Properties for WorkflowState.
  $info['WorkflowState']['properties']['label'] = array(
    'label' => t("Label"),
    'description' => t("The label of the state."),
    'schema field' => 'state',
    'required' => TRUE,
  ) + $info['WorkflowState']['properties']['state'];
  unset($info['WorkflowState']['properties']['state']);
	
  // Properties for WorkflowConfigTransition.
  $info['WorkflowConfigTransition']['properties']['workflow'] = array(
    'label' => t("Workflow"),
    'type' => 'Workflow',
    'description' => t("The workflow of the transition."),
    'getter callback' => '_workflow_metadata_workflow_get_properties',
    'entity token' => FALSE,
  );
  // Unify to old-state, new-state. Do not use underscores, or workflow_tokens will not work!
  unset($info['WorkflowTransition']['properties']['sid']);
  unset($info['WorkflowTransition']['properties']['target_sid']);
  $info['WorkflowConfigTransition']['properties']['old-state'] = array(
    'label' => t('Old state'),
    'description' => t("The old state."),
    'type' => 'WorkflowState',
    'getter callback' => '_workflow_metadata_workflow_get_properties',
  );
  $info['WorkflowConfigTransition']['properties']['new-state'] = array(
    'label' => t('New state'),
    'description' => t("The new state."),
    'type' => 'WorkflowState',
    'getter callback' => '_workflow_metadata_workflow_get_properties',
  );

  // Properties for WorkflowTransition.
  $info['WorkflowTransition']['properties']['label'] = $info['WorkflowConfigTransition']['properties']['label'];
  $info['WorkflowTransition']['properties']['workflow'] = $info['WorkflowConfigTransition']['properties']['workflow'];
  $info['WorkflowTransition']['properties']['hid']['label'] = 'Transition ID';
  // Unify to old-state, new-state. Do not use underscores, or workflow_tokens will not work!
  unset($info['WorkflowTransition']['properties']['old_sid']);
  unset($info['WorkflowTransition']['properties']['sid']);
  $info['WorkflowTransition']['properties']['old-state'] = $info['WorkflowConfigTransition']['properties']['old-state'];
  $info['WorkflowTransition']['properties']['new-state'] = $info['WorkflowConfigTransition']['properties']['new-state'];
  $info['WorkflowTransition']['properties']['user'] = array(
    'label' => t('User'),
    'description' => t('The user that executed the transition.'),
    'type' => 'user',
  );
  $info['WorkflowTransition']['properties']['created'] = array(
    'label' => t('Created'),
    'description' => t('The date the transition was created.'),
    'type' => 'date',
  );

}

/**
 * Getter callback for Workflow defined in hook_entity_property_info_alter.
 */
function _workflow_metadata_workflow_get_properties($entity, array $options, $name, $entity_type, $property) {
  switch ($name) {
    case 'workflow':
      return $entity->getWorkflow();
    case 'states':
      return $entity->getStates();
    case 'transitions':
// @todo: for some reason, getTransitions gives no result.
      return $entity->getTransitions();
    case 'tab_roles':
// @todo: for some reason, Tab_roles gives no result.
      // Taken from user.
      return isset($entity->tab_roles) ? array_keys($entity->tab_roles) : array();

    // The following properties meed more love. Also test their tokens!
    case 'options':
      return 'n/a';
  }
}

/**
 * Entity loader for Workflow.
 *
 * Also used as Menu wild card loader {wildcard_name}_load for '%workflow'.
 * @see http://www.phpgainers.com/content/creating-menu-wildcard-loader-function-drupal-7
 * @todo D8: deprecated in favour of workflow_load_single(), not needed for menu.
 */
function workflow_load($wid) {
  // Some Admin UI menu page loaders pass the $wid as string, not int.
  // @see workflow_admin_ui_edit_form_validate().
  // The EntityController must be Exportable, or else a string won't work. #1741956
  // $workflow = ctype_digit($wid) ? entity_load_single('Workflow', $wid) : workflow_load_by_name($wid);
  $workflow = entity_load_single('Workflow', $wid);
  return $workflow;
}

/*
 * Caveat: this only works for entities with EntityAPIControllerExportable.
 */
function workflow_load_by_name($name) {
  $workflows = entity_load_multiple_by_name('Workflow', array($name));
  return reset($workflows);
}

function workflow_load_single($wid) {
  return entity_load_single('Workflow', $wid);
}

function workflow_load_multiple($wids = FALSE, $reset = FALSE) {
  return entity_load('Workflow', $wids, array(), $reset);
}

function workflow_create($name) {
  // @todo: avoid double names in db-table, to get rid of this line of code.
  $workflow = workflow_load_by_name($name);
  if (!$workflow) {
    $workflow = entity_create('Workflow', array('name' => $name));
  }
  return $workflow;
}

/**
 * Reset the Workflow when States, Transitions have been changed.
 */
function workflow_reset_cache($wid) {
  $ids = array($wid);
  entity_get_controller('Workflow')->resetCache($ids);
}

/**
 * CRUD for WorkflowState.
 */
function workflow_state_load($sid) {
  return WorkflowState::load($sid);
}

function workflow_state_load_single($sid) {
  return WorkflowState::load($sid);
}

function workflow_state_load_multiple($wid = 0, $reset = FALSE) {
  return WorkflowState::getStates($wid, $reset);
}

function workflow_state_load_by_name($name, $wid) {
  return WorkflowState::loadByName($name, $wid);
}

/**
 * Load WorkflowTransitions, most recent first.
 *
 * @param $field_name
 *   Optional. Can be NULL, if you want to load any field.
 *
 * @deprecated: workflow_get_workflow_node_history_by_nid() --> workflow_transition_load_single()
 * @deprecated: workflow_get_recent_node_history() --> workflow_transition_load_single()
 */

function workflow_transition_load_multiple($entity_type, array $entity_ids, $field_name = '', $limit = NULL) {
  return WorkflowTransition::loadMultiple($entity_type, $entity_ids, $field_name, $limit);
}

function workflow_transition_load_single($entity_type, $entity_id, $field_name = '') {
  $limit = 1;
  if ($transitions = workflow_transition_load_multiple($entity_type, array($entity_id), $field_name, $limit)) {
    return reset($transitions);
  }
  return NULL;
}

/**
 * Load function belonging to the menu option 'workflow-comment/%'.
 * Maps to this function just like 'node/%node' maps to node_load().
 *
 * @param $hid
 *   The ID of the workflow state transition record to load.
 * @return
 *   object representing one row from the {workflow_node_history} table
 */
function workflow_transition_load($hid) {
  return entity_load_single('WorkflowTransition', $hid);
}

<?php

/**
 * @file
 * Defines a Workflow field, widget and formatter. (copied from list field).
 */

/**
 * Implements hook_field_info().
 */
function workflowfield_field_info() {
  return WorkflowItem::getInfo();
}

/**
 * Implements hook_form_FORM_ID_alter().
 *
 * Changes the hook_field_settings_form.
 * Fixes some Field settings and Field Instance settings, and makes sure users cannot change it.
 * @todo: perhaps this is core functionality, but these values are only saved 
 *        when the site builder explicitely save the instance settings. :-(
 */
function workflowfield_form_field_ui_field_edit_form_alter(&$form, &$form_state, $form_id) {
  if ($form['#field']['type'] == 'workflow') {
    // The Workflow field must have a value, so set to required.
    $form['instance']['required']['#type'] = 'hidden';
    $form['instance']['required']['#default_value'] = 1;
    $form['instance']['required']['#disabled'] = TRUE;

    // User may not set a default value, this is done by the Workflow module, @see workflow_field_choices().
    $form['instance']['default_value_widget']['#type'] = 'hidden';
    $form['instance']['default_value_widget']['#disabled'] = TRUE;
    unset($form['instance']['default_value_widget']);

    // Make sure only 1 value can be entered in the Workflow field.
    $form['field']['cardinality']['#type'] = 'hidden';
    $form['field']['cardinality']['#default_value'] = 1;
    $form['field']['cardinality']['#disabled'] = TRUE;
  }
}

/**
 * Implements hook_field_settings_form().
 * 
 * @todo: 'allowed_values_string' should not be saved in settings upon submit.
 */
function workflowfield_field_settings_form($field, $instance, $has_data) {
  $form = array();
  $form_state = array();

  $workflow_field = new WorkflowItem($field, $instance);
  return $workflow_field->settingsForm($form, $form_state, $has_data);
}

/**
 * We will be using some default formatters and widgets from the List and Options modules.
 */

/**
 * Implements hook_field_formatter_info_alter().
 *
 * The module reuses the formatters defined in list.module.
 */
function workflowfield_field_formatter_info_alter(&$info) {
  $info['list_default']['field types'][] = 'workflow';
}

/**
 * Implements hook_field_widget_info_alter().
 *
 * The module does not implement widgets of its own, but reuses the
 * widgets defined in options.module.
 *
 * @see workflowfield_options_list()
 */
function workflowfield_field_widget_info_alter(&$info) {
  $info['options_select']['field types'][] = 'workflow';
  $info['options_buttons']['field types'][] = 'workflow';
}

/**
 * Do not implement hook_field_presave(), 
 * since $nid is needed, but not yet known at this moment.
 */
//function workflowfield_field_presave($entity_type, $entity, $field, $instance, $langcode, &$items) {
//}

/**
 * Implements hook_field_insert().
 */
function workflowfield_field_insert($entity_type, $entity, $field, $instance, $langcode, &$items) {
  return workflowfield_field_update($entity_type, $entity, $field, $instance, $langcode, $items);
}

/**
 * Implements hook_field_update().
 * It is called also from hook_field_insert, since we need $nid to store workflow_node_history.
 * We cannot use hook_field_presave, since $nid is not yet known at that moment.
 */
function workflowfield_field_update($entity_type, $entity, $field, $instance, $langcode, &$items) {
  $workflow_field = new WorkflowItem($field, $instance, $entity_type, $entity);
  $workflow_field->update($items);
}

/**
 * Implements hook_field_delete().
 *
 * @todo: implement
 */
//function workflowfield_field_delete($entity_type, $entity, $field, $instance, $langcode, &$items) {
//  $workflow_field = new WorkflowItem($field, $instance, $entity_type, $entity);
//  $workflow_field->delete($items);
//}

/**
 * Implements hook_field_is_empty().
 *
 * The Workflow field is never empty.
 */
function workflowfield_field_is_empty($item, $field) {
//  $workflow_field = new WorkflowItem($field, $instance, $entity_type, $entity);
//  $workflow_field->isEmpty($item);
 
  return FALSE;
}

/**
 * Implements hook_field_delete_field().
 *
 * @todo: implement functionality from workflow_node_delete().
 */
//function workflowfield_field_delete_field($entity_type, $entity, $field, $instance, $langcode, &$items) {
//}

/*
 * Callback function for list.module formatter.
 *
 * Returns the array of allowed values for a list field.
 * @see list_allowed_values()
 */
function workflowfield_allowed_values($field, $instance, $entity_type, $entity) {
  $workflow_field = new WorkflowItem($field, $instance, $entity_type, $entity);
  return $workflow_field->getAllowedValues();
}

/**
 * Callback function for the default Options widgets.
 * Implements hook_options_list().
 * @todo: move to a class.
 */
function workflowfield_options_list($field, $instance, $entity_type, $entity) {
  $workflow_field = new WorkflowItem($field, $instance, $entity_type, $entity);
  return $workflow_field->getOptions();
}

/*
 * Start a simpletest.
 */
function __workflowfield_test() {
  module_enable(array('simpletest'), $enable_dependencies = TRUE);
  $wf_webtest = new WorkflowWebTest();
  $wf_webtest->testWorkflow();
  $wf_webtest->testWorkflowState();
}
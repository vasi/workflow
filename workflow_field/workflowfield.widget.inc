<?php

/**
 * @file
 * Defines a Workflow field, widget (sfrom options.module) and formatter (from list field).
 * All hooks are wrapper functions for a D8-style WorkflowDefaultWidget object.
 */

/**
 * Implements hook_field_widget_info().
 */
function workflowfield_field_widget_info() {
  return WorkflowDefaultWidget::settings();
}

/**
 * Implements hook_field_widget_settings_form().
 */
function workflowfield_field_widget_settings_form($field, $instance) {
  $form = array();
  $form_state = array();

  // The form element is created by a D8-like object.
  $widget = new WorkflowDefaultWidget($field, $instance);
  return $widget->settingsForm($form, $form_state, $has_data = 0);
}

/**
 * Implements hook_field_widget_form().
 *
 * This is a wrapper function for the 'workflow form' Widget. $form is modified by reference.
 */
function workflowfield_field_widget_form(&$form, &$form_state, $field, $instance, $langcode, $items, $delta, $element) {
  if (isset($form_state['node'])) {
    $entity = $form_state['node'];
    $entity_type = 'node'; // comments are only supported on nodes.
  }
  elseif (isset($element['#entity_type'])) {
    // We are on an Entity add/edit page.
    $entity = $element['#entity'];
    $entity_type = $element['#entity_type']; // @todo: add support for other entity types.
  }

  if (!$entity) {
    // We are now on the Field settings page, so do nothing.
    return $element;
  }

  if ($entity_type == 'comment') {
    // On a comment form, get the referenced entity.
    $entity_type = 'node'; // Comments are only possible on nodes
    $entity = $form['#node'];
  }

  $widget = new WorkflowDefaultWidget($field, $instance, $entity_type, $entity);
  $element += $widget->formElement($items, $delta, $element, $langcode, $form, $form_state);

  return $element;
}

/**
 * Implements hook_field_widget_form_alter().
 * 
 * Sets the default value for a workflow_field options widget.
 * This is normally OK for 'node edit', but not for 'node add' and 'comment form'.
 */
function workflowfield_field_widget_form_alter(&$element, &$form_state, $context) {
  if ($context['field']['type'] == 'workflow') {
    if (isset($form_state['node'])) {
      $entity = $form_state['node'];
      $entity_type = 'node'; // comments are only supported on nodes.
    }
    else {
      $entity = $element['#entity'];
      $entity_type = $element['#entity_type'];
    }

    if (!$entity) {
      // We are now on the Field settings page, so do nothing.
      return;
    }

    $field = $context['field'];
    $field_name = $context['field']['field_name'];
    if ($entity_type == 'comment') {
      $entity_type = 'node'; // @todo: Comments are only possible on nodes?
      $entity = $context['form']['#node'];
    }
    $current_sid = workflow_node_current_state($entity, $entity_type, $field_name);

    $element['#default_value'] = $current_sid;
  }
}

function workflowfield_field_widget_form_submit($form, &$form_state) {
  $field = $form_state['values']['workflow_field'];
  $instance = $form_state['values']['workflow_instance'];
  $entity_type = $form['#entity_type'];
  $field_name = $field['field_name'];
  $entity = $form['#entity'];

  // Remember: we are on an Entity view page, so we need to save the entity ourselves.
  $old_sid = workflow_node_current_state($entity, $entity_type, $field_name);
  $items[0]['workflow'] = $form_state['values'];
  $new_sid = _workflow_get_sid_by_items($items);
  $comment = $form_state['values']['workflow_comment'];
  if ($comment || $old_sid != $new_sid) {
    $entity->{$field_name}['und'][0]['workflow'] = $form_state['values'];
    workflow_entity_save($entity_type, $entity, $field_name, $new_sid);
  }
}

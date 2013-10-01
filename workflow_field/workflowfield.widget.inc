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
  else {
    $entity = $element['#entity']; // this is de comment on a comment form
    $entity_type = $element['#entity_type'];
  }

  if (!$entity) {
    // We are now on the Field settings page, so do nothing.
    return $element;
  }

  if ($entity_type == 'comment') {
    $entity_type = 'node'; // @todo: Comments are only possible on nodes?
    $entity = $form['#node'];
  }
  elseif (isset($form['#entity'])) {
    // We are on an Entity add/edit page.
    $entity = $form['#entity'];
    $entity_type = 'node'; // @todo: add support for other entity types.
  }
  elseif (isset($form_state['node'])) {
    // We are on a node edit page. We should never come here, since #entity is already filled.
    $entity = $form_state['node'];
    $entity_type = 'node'; // comments are only supported on nodes.
  }
  else {
    // We are on a Node View page, with self-added form.
    $entity = $element['#entity']; // this is comment on a comment form
    $entity_type = $element['#entity_type'];
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

    $field_name = $context['field']['field_name'];
    $wid = $context['field']['settings']['wid'];
    if ($entity_type == 'comment') {
      $entity_type = 'node'; // @todo: Comments are only possible on nodes?
      $entity = $context['form']['#node'];
      if ($current_sid = _workflow_get_sid_by_items($context['items'])) {
        // This is a preview: OK
      }
      else {
        // A comment form. Get data from Entity.
        $items = field_get_items($entity_type, $entity, $field_name);
        $current_sid = _workflow_get_sid_by_items($items);
      }
    }
    elseif ($items = $context['items']) {
      // A node edit page.
      $current_sid = _workflow_get_sid_by_items($items);
    }
    else {
      // A node add page.
      $workflow = Workflow::getWorkflow($wid);
      $current_sid = $workflow->getFirstState($entity)->sid;
    }

    $element['#default_value'] = $current_sid;
  }
}

function workflowfield_field_widget_form_submit($form, &$form_state) {
  // Emulate the D7 arrays.
  $field = $form_state['values']['workflow_field'];
  $instance = $form_state['values']['workflow_instance'];
  $entity_type = $form['#entity_type'];
  $entity = $form['#entity'];
  $items[0]['workflow'] = $form_state['values'];

  // Submit the data. $items is reset by reference to normal value, and is magically saved by the field itself.
  $widget = new WorkflowDefaultWidget($field, $instance, $entity_type, $entity);
  $widget->submit($form, $form_state, $items); // $items is a proprietary D7 parameter.

  // Remember: we are on a Node view, so the Widget->submit saves the scheduled transition, 
  // but not the new state of the node itself.
  // Widget::submit() returns the new value in a 'sane' state.
  // Save the referenced entity, but only is transition succeeded, and is not scheduled.
  $field_name = $field['field_name'];
  $old_sid = _workflow_get_sid_by_items($entity->{$field_name}['und']);
  $new_sid = _workflow_get_sid_by_items($items);
  if ($old_sid != $new_sid) {
      $entity->{$field_name}['und'][0]['value'] = $new_sid;
      $widget->entitySave($entity_type, $entity);
  }
}

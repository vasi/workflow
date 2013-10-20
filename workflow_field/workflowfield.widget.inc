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

    $field_name = $context['field']['field_name'];
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
      $wid = $context['field']['settings']['wid'];
      $workflow = Workflow::load($wid);
      $current_sid = $workflow->getFirstSid($entity_type, $entity);
    }

    $element['#default_value'] = $current_sid;
  }
}

function workflowfield_field_widget_form_submit($form, &$form_state) {
  // Emulate the D7 arrays.
  $field = $form_state['values']['workflow_field'];
  $instance = $form_state['values']['workflow_instance'];
  $entity_type = $form['#entity_type'];
  $field_name = $field['field_name'];
  $entity = $form['#entity'];
  $items[0]['workflow'] = $form_state['values'];

  $old_sid = _workflow_get_sid_by_items($entity->{$field_name}['und']); // @todo: replace by currentState().

  // Submit the data. $items is reset by reference to normal value, and is magically saved by the field itself.
  $widget = new WorkflowDefaultWidget($field, $instance, $entity_type, $entity);
//  $widget->submit($form, $form_state, $items); // $items is a proprietary D7 parameter.

  // Remember: we are on an Entity view page, so the Widget->submit saves the scheduled transition, 
  // but not the new state of the entity itself.
  // Widget::submit() returns the new value in a 'sane' state.
  // Save the referenced entity, but only is transition succeeded, and is not scheduled.
  $new_sid = _workflow_get_sid_by_items($items);
  if ($old_sid != $new_sid) {
//      $entity->{$field_name}['und'][0]['value'] = $new_sid;
      $entity->{$field_name}['und'][0]['workflow'] =$form_state['values'];
      $widget->entitySave($entity_type, $entity);
  }
}

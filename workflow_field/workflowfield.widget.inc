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

  if (isset($form['#entity'])) {
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

  if ($entity_type == 'comment') {
    $entity = $element['entity']['#value']; // this is de comment on a comment form
    $entity_type = $element['entity_type']['#value'];

    $entity_type = 'node'; // @todo: Comments are only possible on nodes?
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

/**
 * Form element validation handler for workflow element.
 */
function workflowfield_field_widget_validate($element, &$form_state) {
//  if ($element['#required'] && $element['#value'] == '_none') {
//    form_error($element, t('!name field is required.', array('!name' => $element['#title'])));
//  }
//  // Transpose selections from field => delta to delta => field, turning
//  // multiple selected options into multiple parent elements.
//  $items = _workflowfield_form_to_storage($element);
//  form_set_value($element, $items, $form_state);
}
